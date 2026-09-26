# Project instructions

- This is a work-in-progress ESP32 Halloween elevator project.
- Current hardware is an ESP32-WROOM-32 DevKit using the Arduino `ESP32 Dev Module` board profile and a BME680 pressure sensor.
- BME680 wiring is VCC → 3V3, GND → GND, SDA → GPIO21 and SCL → GPIO22; CS and SDO are disconnected. Its header pins must be soldered.
- Keep changes small and extend the structure only when a real component needs it.
- Store Arduino sketches in a directory with the same name as the `.ino` file.
- Never commit Wi-Fi credentials or other secrets. Keep them in `secrets.h`; update `secrets.example.h` when adding configuration keys.
- Preserve hardware calibration and tuning values unless the user explicitly asks to change them.
- Offline serial sampling is the default (`ENABLE_WIFI_REPORTING = false`). When Wi-Fi reporting is enabled, the ESP32 sends pressure in hPa and relative height in metres to `POST /data`; keep the server contract aligned with the firmware.
- DFPlayer Mini playback and provisional arrival detection are implemented. Eight floors (-1 through 6) have pressure samples; floor -2 shares floor -1's height. An optional accelerometer remains planned.
- Before changing shared firmware logic, search for every caller and fix the common path.
- Verify firmware changes with `arduino-cli compile` when the CLI and board configuration are available. Otherwise state that compilation was not run.
- Verify server changes with `npm test` from `server/`.
