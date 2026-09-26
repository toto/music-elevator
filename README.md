# Music Elevator

Source code for the ESP32-based music elevator project. Play a sound on every level.

## Layout

```text
audio/
└── test-floor-announcements/
    └── mp3/               # eight spoken floor numbers for the SD card
calibration/
└── 2026-09-26/           # raw samples and pressure summary
firmware/
├── dfplayer_test/
│   └── dfplayer_test.ino
└── elevator_controller/
    ├── elevator_controller.ino
    ├── secrets.example.h
    └── secrets.h          # local only; ignored by Git
server/
├── package.json
├── server.js
└── server.test.js
```

## Current prototype

- ESP32-WROOM-32 DevKit, selected as `ESP32 Dev Module` in Arduino IDE
- BME680 pressure sensor with soldered headers
- Relative altitude calibrated to `0.00 m` at startup
- Serial pressure and height samples for offline floor calibration
- Optional Wi-Fi reporting to a Node.js server every 500 ms
- DFPlayer Mini playback on detected arrivals

The earlier BMP390 and Mini D1 ideas were replaced by the hardware above. The BME680 produced stable readings around 1016.98–1016.99 hPa after its header was soldered.

### Wiring

| BME680 | ESP32 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |
| CS | Disconnected |
| SDO | Disconnected |

### DFPlayer Mini connection

Wire the standard DFPlayer Mini as follows, with power disconnected:

| DFPlayer Mini | ESP32 DevKit |
| --- | --- |
| VCC | Board pin that supplies 5 V from USB (`5V`, `VIN`, or `VCC` depending on the DevKit; verify its label and voltage) |
| GND | GND |
| RX | GPIO17 through a ~1 kΩ resistor |
| TX | GPIO16 |
| SPK1 | Speaker `+` |
| SPK2 | Speaker `−` |

Use a common ground with the ESP32. Connect the speaker only between SPK1 and SPK2; neither speaker wire goes to GND. The DFPlayer TX connects directly to the ESP32's GPIO16 input; the series resistor goes on the ESP32 GPIO17 → DFPlayer RX wire.

### Test the MP3 player

1. Format a microSD card of up to 32 GB as FAT32. Put one known-good MP3, such as `001.mp3`, at the card root. The test plays the first file on the card once. Insert the card while power is off. On macOS, run `dot_clean /Volumes/<card-name>` after copying to remove hidden `._` files that may affect the DFPlayer's file order.
2. In Arduino IDE, install **DFRobotDFPlayerMini** by DFRobot from Library Manager. Open `firmware/dfplayer_test/dfplayer_test.ino`, select **ESP32 Dev Module**, and upload. This temporarily replaces the controller sketch on the ESP32.
3. Open Serial Monitor at **115200 baud**. After startup, expect `DFPlayer online` and hear the file once at volume 10/30. Send `p` to replay or `s` to stop. Reupload `firmware/elevator_controller/elevator_controller.ino` for the integrated controller when finished.

If the player does not respond, first check the shared ground, 5 V at the player, crossed UART connections, and seated card. If it responds but is silent, check the card contents, MP3 encoding, speaker wiring, and volume. The [DFRobot setup guide](https://wiki.dfrobot.com/dfr0299/docs/20906) explains that `play(1)` selects the first file copied to the card; the [module reference](https://wiki.dfrobot.com/dfr0299) covers power, speaker output, and the 1 kΩ resistor.

### Audio on floor arrival

The main controller uses named files in the card's `mp3` folder. Floors -2 and -1 are two sides at the same height, so both use floor -1. The eight distinct heights use European numbering:

Ready-made English test clips are in `audio/test-floor-announcements/mp3/`. They were made with macOS speech synthesis (Daniel voice) and encoded as mono 44.1 kHz MP3s. Copy the eight files into the SD card's `mp3` folder. The existing filenames are retained, so `0001.mp3` is no longer used.

| Floor | Measured pressure (hPa) | MP3 file |
| ---: | ---: | --- |
| -1 | 1019.07 | `/mp3/0002.mp3` |
| 0 | 1018.79 | `/mp3/0003.mp3` |
| 1 | 1018.27 | `/mp3/0004.mp3` |
| 2 | 1017.92 | `/mp3/0005.mp3` |
| 3 | 1017.59 | `/mp3/0006.mp3` |
| 4 | 1017.21 | `/mp3/0007.mp3` |
| 5 | 1016.83 | `/mp3/0008.mp3` |
| 6 | 1016.49 | `/mp3/0009.mp3` |

Put a fallback sound at `/mp3/0099.mp3`; you can copy the successfully tested `001.mp3` there. Power off before moving the card, then eject it safely from the computer. Run `dot_clean` on macOS after copying.

Start the controller while the elevator is stationary at any floor so its relative height can calibrate. It detects an arrival after at least 1.5 m of travel within 10 seconds, followed by a height stable within 0.35 m for 1.5 seconds. It plays once per arrival. These are provisional detection settings based on the expected 3–4 m between floors. Audio detection continues if Wi-Fi is disconnected.

`FLOOR_PRESSURE_HPA` in the controller sketch contains the measured values above, in -1 through 6 order. The nearest pressure within `FLOOR_PRESSURE_TOLERANCE_HPA` selects that floor's track; an unmatched pressure plays `/mp3/0099.mp3`. Set a `FLOOR_TRACK` entry to zero to use the fallback for a floor without an assigned sound. The DFPlayer does not reliably report a missing MP3 before playback, so copy every assigned file to the card. These absolute pressures came from one trip and can shift with weather; the raw captures and selection method are in `calibration/2026-09-26/`.

Serial Monitor at 115200 baud prints `SAMPLE,millis,pressure_hpa,height_m` rows and each arrival decision. Record the stable pressure at each floor for the next calibration step. Absolute pressure changes with weather, so the floor matching tolerance and reference method will need checking against real readings.

### Collect floor pressures without Wi-Fi

`ENABLE_WIFI_REPORTING` is `false` in the controller sketch, so it samples and plays audio without attempting a Wi-Fi connection. The HTTP code and its `POST /data` contract remain available when this setting is changed to `true` later.

1. Upload the integrated controller and start it while the elevator is stationary at any floor. Wait for `Calibration complete` before moving.
2. Find the current USB port with `arduino-cli board list`, then run `arduino-cli monitor --port /dev/cu.usbserial-23110 --config baudrate=115200,dtr=off,rts=off`, replacing the port if needed. Turning DTR and RTS off avoids resetting the ESP32 when attaching the monitor.
3. Visit floors -1 through 6. At each floor, let the reading settle and capture about 10 seconds of `SAMPLE` rows. Keep a note of the floor label and the corresponding `millis` range. Keep the board powered throughout the trip.

Send the labelled samples back for the pressure table. The board reports hPa to three decimal places in the monitor; its `height_m` column remains relative to where it booted.

## Firmware setup

1. Install `esp32 by Espressif Systems` board support.
2. Install `Adafruit BME680 Library` and its `Adafruit Unified Sensor` and `Adafruit BusIO` dependencies.
3. Copy `secrets.example.h` to `secrets.h`. Wi-Fi values are only used when `ENABLE_WIFI_REPORTING` is `true`.
4. Open `firmware/elevator_controller/elevator_controller.ino`, select `ESP32 Dev Module` and upload it.

To install the same board support and libraries with Arduino CLI:

```sh
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib update-index
arduino-cli lib install "Adafruit BME680 Library" "DFRobotDFPlayerMini"
```

Arduino CLI installs the BME680 library's dependencies automatically. `DFRobotDFPlayerMini` is used by both sketches. The `ESP32 Dev Module` board profile is `esp32:esp32:esp32` in Arduino CLI.

On Apple Silicon, the Arduino CLI bundled `ctags` helper may fail with `bad CPU type in executable`. The VS Code build task uses a project-local helper because these sketches define functions before use. For a terminal build, add `--build-property runtime.tools.ctags.path="$PWD/tools/arduino-ctags"` to `arduino-cli compile` when running it from the repository root.

`Project: Check` builds the main controller. To compile the speaker test instead, run from the repository root:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 --build-property runtime.tools.ctags.path="$PWD/tools/arduino-ctags" firmware/dfplayer_test
```

The BME680 tries I²C addresses `0x76` and `0x77`. The controller averages 30 pressure readings at startup. With Wi-Fi reporting enabled, it posts this JSON to the server:

```json
{
  "pressure": 1016.82,
  "height": 1.43
}
```

### Status LED

The onboard LED uses GPIO2 by default. Change `STATUS_LED_PIN` or `STATUS_LED_ACTIVE_HIGH` near the top of the sketch if your board's LED differs.

| Pattern | Meaning |
| --- | --- |
| Slow blink | Connecting or reconnecting to Wi-Fi |
| Solid | Offline sampling ready, or Wi-Fi connected |
| One brief dark blink | Sending an HTTP request |
| Three brief dark blinks | Sensor or HTTP error |

## Server setup

```sh
cd server
npm install
npm start
```

Open <http://localhost:5000>. `GET /data` returns the latest reading and `POST /data` accepts readings from the ESP32. The server listens on the local network, so set `SERVER_URL` to the computer's LAN address, for example `http://192.168.1.42:5000/data`.

## VS Code

Open this repository in VS Code, then use **Terminal → Run Task**:

- `Project: Check` builds the firmware and tests the server. It is also the default `⌘⇧B` build task.
- `Arduino: Upload` builds and uploads to the configured USB serial port at a conservative 115200 baud.
- `Arduino: Monitor` opens the serial monitor at 115200 baud without resetting the board.
- `Server: Start` serves the dashboard at <http://localhost:5000>.

The Arduino tasks reuse the CLI, ESP32 core and Adafruit libraries already installed with Arduino IDE. If the ESP32 appears under a different serial port later, update the two port values in `.vscode/tasks.json`.

## Planned

- Validate floor matching across later trips and weather changes, then adjust the pressure reference method and arrival settings if needed.
- Power the prototype from a roughly 5,000 mAh USB power bank.
- Add an accelerometer only if pressure-based movement detection is not reliable enough.
