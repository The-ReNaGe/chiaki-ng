// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_DUALSENSEBTHAPTICS_H
#define CHIAKI_DUALSENSEBTHAPTICS_H

#include <chiaki/log.h>

#include <QByteArray>
#include <QObject>
#include <QString>

#include <SDL.h>

#if !SDL_VERSION_ATLEAST(2, 0, 18)
// SDL_hid_* is only available since SDL 2.0.18; Open() fails and rumble is used instead
typedef struct SDL_hid_device_ SDL_hid_device;
#endif

// Over Bluetooth the DualSense does not expose an audio device, so the console's
// 3 kHz haptics stream is written directly as HID output report 0x32.
// Lives on its own thread because HID writes may block.
class DualSenseBtHaptics : public QObject
{
	Q_OBJECT

	public:
		explicit DualSenseBtHaptics(ChiakiLog *log, QObject *parent = nullptr);
		~DualSenseBtHaptics() override;

		bool Open(const QString &hid_path);
		void Close();
		bool IsOpen() const { return dev != nullptr; }
		QString GetPath() const { return path; }

		// Interleaved stereo (L, R) signed 8-bit samples at 3 kHz
		void PushSamples(const QByteArray &samples);

	private:
		void SendReport();

		ChiakiLog *log;
		SDL_hid_device *dev = nullptr;
		QString path;
		uint8_t report[142];
		size_t fill = 0;
		uint8_t counter = 0;
		int consecutive_failures = 0;
};

#endif // CHIAKI_DUALSENSEBTHAPTICS_H
