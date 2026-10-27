// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <dualsensebthaptics.h>

#include <cstring>

// Layout of output report 0x32 (142 bytes, Bluetooth only):
//   [0]       report id 0x32
//   [1]       tag/sequence nibbles (0)
//   [2..10]   packet 0x11 (sized, 7 bytes): 0xFE 0 0 0 0 0xFF counter
//   [11..76]  packet 0x12 (sized, 64 bytes): 32 stereo frames, signed 8-bit, 3 kHz
//   [138..141] CRC32 (little endian) over the 0xA2 BT output header + bytes [0..137]
#define DS_BT_HAPTICS_REPORT_ID 0x32
#define DS_BT_HAPTICS_REPORT_SIZE 142
#define DS_BT_HAPTICS_COUNTER_OFFSET 10
#define DS_BT_HAPTICS_SAMPLES_OFFSET 13
#define DS_BT_HAPTICS_SAMPLES_SIZE 64
#define DS_BT_HAPTICS_CRC_OFFSET (DS_BT_HAPTICS_REPORT_SIZE - 4)
#define DS_BT_OUTPUT_CRC_SEED 0xA2
#define DS_BT_HAPTICS_MAX_FAILURES 50

static_assert(DS_BT_HAPTICS_SAMPLES_OFFSET + DS_BT_HAPTICS_SAMPLES_SIZE <= DS_BT_HAPTICS_CRC_OFFSET, "haptics samples overlap crc");

static uint32_t Crc32Update(uint32_t crc, const uint8_t *data, size_t size)
{
	while(size--)
	{
		crc ^= *data++;
		for(int i = 0; i < 8; i++)
			crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
	}
	return crc;
}

DualSenseBtHaptics::DualSenseBtHaptics(ChiakiLog *log, QObject *parent)
	: QObject(parent), log(log)
{
	static_assert(sizeof(report) == DS_BT_HAPTICS_REPORT_SIZE, "unexpected haptics report size");
	memset(report, 0, sizeof(report));
	report[0] = DS_BT_HAPTICS_REPORT_ID;
	// packet 0x11: id 0x11 | sized flag (0x80), length 7
	report[2] = 0x91;
	report[3] = 0x07;
	report[4] = 0xFE;
	report[9] = 0xFF;
	// packet 0x12: id 0x12 | sized flag (0x80), length 64
	report[11] = 0x92;
	report[12] = DS_BT_HAPTICS_SAMPLES_SIZE;
}

DualSenseBtHaptics::~DualSenseBtHaptics()
{
	Close();
}

bool DualSenseBtHaptics::Open(const QString &hid_path)
{
	Close();
	if(hid_path.isEmpty())
		return false;
	if(SDL_hid_init() != 0)
	{
		CHIAKI_LOGE(log, "DualSense BT haptics: SDL_hid_init failed: %s", SDL_GetError());
		return false;
	}
	dev = SDL_hid_open_path(hid_path.toUtf8().constData(), 0);
	if(!dev)
	{
		CHIAKI_LOGE(log, "DualSense BT haptics: could not open HID device %s: %s", hid_path.toUtf8().constData(), SDL_GetError());
		SDL_hid_exit();
		return false;
	}
	path = hid_path;
	fill = 0;
	counter = 0;
	consecutive_failures = 0;
	CHIAKI_LOGI(log, "DualSense BT haptics: streaming haptics to %s via output report 0x32", hid_path.toUtf8().constData());
	return true;
}

void DualSenseBtHaptics::Close()
{
	if(!dev)
		return;
	SDL_hid_close(dev);
	dev = nullptr;
	SDL_hid_exit();
	CHIAKI_LOGI(log, "DualSense BT haptics: closed %s", path.toUtf8().constData());
	path.clear();
}

void DualSenseBtHaptics::PushSamples(const QByteArray &samples)
{
	if(!dev)
		return;
	const char *data = samples.constData();
	size_t remaining = (size_t)samples.size();
	while(remaining > 0)
	{
		size_t chunk = DS_BT_HAPTICS_SAMPLES_SIZE - fill;
		if(chunk > remaining)
			chunk = remaining;
		memcpy(report + DS_BT_HAPTICS_SAMPLES_OFFSET + fill, data, chunk);
		fill += chunk;
		data += chunk;
		remaining -= chunk;
		if(fill == DS_BT_HAPTICS_SAMPLES_SIZE)
		{
			SendReport();
			fill = 0;
			if(!dev)
				return;
		}
	}
}

void DualSenseBtHaptics::SendReport()
{
	report[DS_BT_HAPTICS_COUNTER_OFFSET] = counter++;
	const uint8_t seed = DS_BT_OUTPUT_CRC_SEED;
	uint32_t crc = Crc32Update(0xFFFFFFFFu, &seed, 1);
	crc = ~Crc32Update(crc, report, DS_BT_HAPTICS_CRC_OFFSET);
	report[DS_BT_HAPTICS_CRC_OFFSET + 0] = (uint8_t)(crc & 0xFF);
	report[DS_BT_HAPTICS_CRC_OFFSET + 1] = (uint8_t)((crc >> 8) & 0xFF);
	report[DS_BT_HAPTICS_CRC_OFFSET + 2] = (uint8_t)((crc >> 16) & 0xFF);
	report[DS_BT_HAPTICS_CRC_OFFSET + 3] = (uint8_t)((crc >> 24) & 0xFF);

	if(SDL_hid_write(dev, report, sizeof(report)) < 0)
	{
		if(consecutive_failures++ == 0)
			CHIAKI_LOGW(log, "DualSense BT haptics: HID write failed: %s", SDL_GetError());
		if(consecutive_failures >= DS_BT_HAPTICS_MAX_FAILURES)
		{
			CHIAKI_LOGE(log, "DualSense BT haptics: %d consecutive write failures, giving up", consecutive_failures);
			Close();
		}
		return;
	}
	consecutive_failures = 0;
}
