# DualSense over Bluetooth: Full Feature Support

This page documents how chiaki-ng drives every DualSense feature when the controller is connected over **Bluetooth**, how it was verified, and how to re-test it on real hardware.

Scope: only the controller integration in the GUI changed. Registration, authentication and the Remote Play streaming protocol are untouched.

## Feature Matrix

| Feature | USB | Bluetooth (before) | Bluetooth (now) | Path |
|---|---|---|---|---|
| Haptics (PS5 games) | yes, via the controller's USB audio device | no, only emulated rumble | **yes** | HID output report `0x32` (this page) |
| Adaptive triggers | yes | yes | yes | SDL `SDL_GameControllerSendEffect` (report `0x31`) |
| Gyro / accelerometer | yes | yes | yes | SDL sensors |
| Touchpad | yes | yes | yes, with fixes below | SDL touchpad events |
| Lightbar / player LEDs / mic LED | yes | yes | yes | SDL |
| Controller speaker / headset jack / controller mic | yes | no | no | not implemented (different, more complex reports) |

## Haptics over Bluetooth

### Why it did not work before

The console streams PS5 haptics as a 3 kHz stereo audio signal. chiaki-ng played it to the DualSense's USB audio device (4 channels, haptics on channels 3 and 4, resampled to 48 kHz). Over Bluetooth that audio device does not exist, so chiaki-ng fell back to approximating the haptics with rumble.

### How it works now

`gui/src/dualsensebthaptics.cpp` (`DualSenseBtHaptics`) writes the haptics signal directly to the controller as HID output report `0x32`:

| Bytes | Content |
|---|---|
| `0` | report id `0x32` |
| `1` | tag / sequence nibbles, `0x00` |
| `2..10` | packet `0x11` (sized, 7 bytes): `0x91 0x07 0xFE 0x00 0x00 0x00 0x00 0xFF <counter>` |
| `11..76` | packet `0x12` (sized, 64 bytes): `0x92 0x40` + 32 stereo frames, signed 8-bit, interleaved L/R, 3 kHz |
| `77..137` | zero |
| `138..141` | CRC32 (little endian) of `0xA2` followed by bytes `0..137` |

The console already sends 3 kHz stereo (30 frames every 10 ms), so samples are only converted from 16 to 8 bit (scaled by the haptics intensity setting), with no resampling.

The haptics reach the client in network-paced bursts, but the controller plays them at a fixed rate. `PushSamples()` therefore only queues samples, and a sender thread drains the queue on a steady clock. It keeps a 32 ms jitter buffer before playback, drops the oldest data beyond 96 ms, and sends two 32-frame blocks per report every 21.33 ms, using DS5Dongle's layout with a 48-sample controller-side buffer. While haptics play, the console's duplicate rumble is not forwarded to the Bluetooth controller. This pacing, buffering and rumble de-duplication were contributed by @mikedinicola (fork PR #1, tested on macOS). The table below describes the original single-block layout; the paced layout replaces packet `0x11` with a 4-byte audio-control packet (flags `0x66`, buffer length, counter) and sets the double-block bit on packet `0x12`.

Flow:

1. `StreamSession::UpdateGamepads()` sees a DualSense whose SDL GUID bus type is Bluetooth (`Controller::IsBluetooth()`), and gets its HID path from SDL (`Controller::GetHidPath()`, e.g. `/dev/hidraw1`).
2. `StreamSession::ConnectBtHaptics()` starts `DualSenseBtHaptics` on its own `QThread` and opens a second handle on the device with `SDL_hid_open_path()`. SDL keeps its own handle for input, triggers and LEDs; both coexist.
3. `StreamSession::PushHapticsFrame()` converts each haptics frame and emits `BtHapticsPushed`, delivered to the worker thread, which writes the reports (HID writes may block, so they never run on the network thread).
4. If the device stops accepting writes (50 consecutive failures), the worker closes and chiaki-ng falls back to rumble haptics. Disconnecting the controller stops the worker.

USB DualSense controllers keep using the audio device path unchanged.

### Sources for the report format

* [SAxense](https://github.com/egormanga/SAxense): Linux proof of concept for DualSense haptics over Bluetooth.
* [AnyPS5 issue #2056](https://github.com/boykopovar/AnyPS5/issues/2056): same layout measured on macOS and Windows.

Both describe the same layout byte for byte. The CRC seed was cross-checked against both, and `DualSenseBtHaptics` output was compared byte for byte with an independent reference implementation of that layout.

### What Sony's own app does

Sony's PS Remote Play app (macOS 9.5.0) was analysed with Ghidra to see whether it supports this (static analysis only, nothing from it is reused):

* `DSDevice::isSupportedAudioFunction` returns "not supported" when the controller's bus type is Bluetooth. **The official app disables haptics over Bluetooth.**
* Its haptics are upsampled (`RpUpsampleHaptics`) and played through CoreAudio, i.e. the USB audio device.
* Over Bluetooth it only sends output report `0x31` (78 bytes, header `0x31, seq << 4, 0x10`) with a CRC32 seeded with `0xA2`, which is the same CRC chiaki-ng uses for `0x32`.
* Its rumble requests set only bit `0x01` of the first flag byte (plus bit `0x04` of byte 38 on newer firmware), never `0x02`. chiaki-ng's `Controller::SetDualSenseRumble()` also sets `0x02`. This has not caused problems so far, but it is the first thing to align with Sony if haptics ever stop after a rumble command.
* Haptics volume is flag bit `0x40` of the second flag byte plus byte 36, which matches what chiaki-ng already sends.

So with this change, chiaki-ng supports more over Bluetooth than the official app does.

## Touchpad Fixes

In `Controller::HandleTouchpadEvent()` (`gui/src/controllermanager.cpp`):

* **Per-console scaling.** Coordinates were always scaled to 1920 × 1079, even when streaming from a PS4 whose touchpad is 1920 × 942. Each controller now gets the console's range from `StreamSession` (`Controller::SetTouchpadMax()`). The D-pad-as-touchpad feature uses the same range.
* **Clamping.** SDL coordinates are clamped to [0, 1] and mapped to 0..1919 / 0..1079 (the old code could send 1920).
* **No phantom fingers.** `chiaki_controller_state_start_touch()` returns -1 when no slot is free. That value used to be stored as id 255, so the matching release never found the touch. Failed starts are now ignored, and a duplicate "finger down" is treated as a move.

## Stop the Desktop from Using the Touchpad as a Mouse (Linux)

The Linux `hid-playstation` driver exposes the DualSense touchpad as a regular PC touchpad, so while streaming the same swipe both reaches the game (through chiaki-ng) and moves the desktop cursor, which can leave the stream window or click outside it.

`doc/71-dualsense-touchpad-no-mouse.rules` tells libinput (GNOME, KDE, wlroots...) to ignore the touchpad of DualSense, DualSense Edge and DualShock 4 controllers. chiaki-ng is not affected, because it reads the touchpad through hidraw/SDL.

```bash
sudo install -m 644 doc/71-dualsense-touchpad-no-mouse.rules /etc/udev/rules.d/
sudo udevadm control --reload
sudo udevadm trigger --subsystem-match=input --action=change
```

Then turn the controller off and on again: compositors only read this property when the device is added. Check it with:

```bash
udevadm info /dev/input/eventX | grep LIBINPUT_IGNORE_DEVICE   # eventX = "Wireless Controller Touchpad"
```

`Handlers=mouse2` in `/proc/bus/input/devices` stays listed, which is expected: that is the legacy kernel mouse interface, not what Wayland compositors use.

Trade-off: the controller touchpad can no longer be used as a desktop mouse. A possible future alternative is to grab the touchpad's evdev node (`EVIOCGRAB`) only while a stream is running; that needs access to the event node (see `doc/71-chiaki4deck.rules`).

## Hardware Test Tools

`scripts/dualsense-tests/` contains small programs that exercise each feature on a real controller, independently of a console. Build them after building chiaki-ng:

```bash
scripts/dualsense-tests/build.sh build
```

| Tool | What it does | Expected result |
|---|---|---|
| `hwtest /dev/hidrawN` | plays patterns through the real `DualSenseBtHaptics` class: 3 s pause, 2 s 180 Hz on both sides, pause, 2 s 60 Hz left only, pause, 2 s 60 Hz right only, pause, 2 s pulses | each pattern felt on the right side(s), no write errors |
| `trigtest` | sends trigger effects through SDL exactly like `Controller::SetTriggerEffects()`: R2 full resistance, L2 resistance from mid-travel, both vibrating, off | `SendEffect -> 0` for each step and the effect felt when pressing the trigger |
| `padtest` | records 24 s in three phases (letter, swipes, two fingers + click) through the same conversion as chiaki-ng and the real libchiaki touch tracking, then draws each phase | drawing matches what was traced, sequential ids, no rejected touches |
| `edgetest` | records 15 s while tracing the touchpad's edges | top/bottom reach 0 and 1079, left/right ~98-99 % (finger size), no out-of-range samples |

Find the hidraw node with:

```bash
grep -l 054C /sys/class/hidraw/*/device/uevent   # bus 0005 = Bluetooth
```

### Verification Log

Real DualSense (`054C:0CE6`) over Bluetooth, Fedora 44, GNOME Wayland, SDL 2.32 (sdl2-compat over SDL 3.4):

| Test | Result |
|---|---|
| `hwtest` | all patterns felt, left/right separated, all reports accepted |
| `trigtest` | all four steps felt, all `SendEffect` calls returned 0 |
| `padtest` | letter "Z" reproduced in the right orientation, swipes detected, 2 simultaneous fingers, clicks detected, 13 strokes with sequential ids, 0 rejected |
| `edgetest` | x 34..1899, y 0..1079, 0 out-of-range samples |
| udev rule | `LIBINPUT_IGNORE_DEVICE=1` applied to the touchpad node |
| In-game streaming session | not yet tested |
