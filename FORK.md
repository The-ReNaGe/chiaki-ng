# Fork Notes: DualSense Bluetooth Integration

This fork of [chiaki-ng](https://github.com/chiaki-ng/chiaki-ng) improves the DualSense integration so that a controller connected over **Bluetooth** gets every feature it gets over USB when streaming from a PS5.

Constraints kept on purpose:

* No change to registration, authentication or the Remote Play streaming protocol.
* Only behaviour that was verified on real hardware is enabled.

Based on upstream commit `c8bea264` (Merge pull request #840).

## Changes

| Area | Change | Files |
|---|---|---|
| Haptics over Bluetooth | Console haptics are written to the controller as HID output report `0x32` instead of being approximated with rumble | `gui/include/dualsensebthaptics.h`, `gui/src/dualsensebthaptics.cpp`, `gui/src/streamsession.cpp`, `gui/include/streamsession.h` |
| Bluetooth detection | `Controller::IsBluetooth()` (SDL GUID bus type), `Controller::GetHidPath()` | `gui/src/controllermanager.cpp`, `gui/include/controllermanager.h` |
| Touchpad | Per-console coordinate range (PS4 vs PS5), clamping, no phantom touch ids | `gui/src/controllermanager.cpp`, `gui/src/streamsession.cpp` |
| Linux desktop | udev rule so the desktop stops using the controller touchpad as a mouse | `doc/71-dualsense-touchpad-no-mouse.rules` |
| Test tools | Standalone hardware tests for haptics, triggers and touchpad | `scripts/dualsense-tests/` |
| UI design | PS5-inspired visual refresh (night-blue gradient, translucent cards, PlayStation blue accent). Visual only: no change to behaviour, key/controller navigation or backend | `gui/src/qml/AppTheme.qml`, `gui/src/qml/qtquickcontrols2.conf`, `gui/src/qml/Main.qml`, `gui/src/qml/MainView.qml`, `gui/src/qml/DialogView.qml` |
| Docs | Technical page and user docs update | `docs/diy/dualsense-bluetooth.md`, `docs/setup/controlling.md`, `mkdocs.yml` |

Full details, report layout, research notes and the verification log: [docs/diy/dualsense-bluetooth.md](docs/diy/dualsense-bluetooth.md).

## Status

| Item | Status |
|---|---|
| Haptics over Bluetooth (standalone test) | verified on hardware |
| Adaptive triggers over Bluetooth | verified on hardware |
| Touchpad (drawing, swipes, 2 fingers, click, edges) | verified on hardware |
| Touchpad ignored by the desktop (udev rule) | rule applied, cursor check pending |
| Full in-game session (haptics + triggers + gyro + touchpad) | works over Bluetooth on wired LAN (user feedback, 2026-10-27); session log confirms the 0x32 haptics path is active |
| UI redesign: home screen and dialog headers | done, visual review pending |
| Builds on Linux (Fedora 44) | yes |
| Builds on Windows / macOS / Flatpak | not tested; the new code only uses SDL and Qt APIs already used by chiaki-ng |

## Ideas Not Done Yet

* Align `Controller::SetDualSenseRumble()` flags with Sony's app (bit `0x01` only) if a rumble command is found to interrupt Bluetooth haptics.
* Grab the touchpad evdev node only during a stream (`EVIOCGRAB`) instead of the global udev rule.
* Controller speaker, headset jack and controller microphone over Bluetooth.
