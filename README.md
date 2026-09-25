# Halloween Elevator

Source code for the ESP32-based Halloween elevator project.

## Layout

```text
firmware/
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
- Wi-Fi reporting to a Node.js server every 500 ms

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

The audio player is not used by the firmware yet; these are the prepared connections for the standard DFPlayer Mini.

| DFPlayer Mini | ESP32 DevKit |
| --- | --- |
| VCC | `VCC` / 5 V pin near the USB-C connector |
| GND | GND |
| RX | GPIO17 through a ~1 kΩ resistor |
| TX | GPIO16 |
| SPK1 | Speaker `+` |
| SPK2 | Speaker `−` |

Format the microSD card as FAT32 and start with `001.mp3`. Connect the speaker only between SPK1 and SPK2; neither speaker wire goes to GND.

## Firmware setup

1. Install `esp32 by Espressif Systems` board support.
2. Install `Adafruit BME680 Library` and its `Adafruit Unified Sensor` and `Adafruit BusIO` dependencies.
3. Copy `secrets.example.h` to `secrets.h` and enter the Wi-Fi and server details.
4. Open `firmware/elevator_controller/elevator_controller.ino`, select `ESP32 Dev Module` and upload it.

The BME680 tries I²C addresses `0x76` and `0x77`. The controller averages 30 pressure readings at startup, then posts this JSON to the server:

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
| Solid | Wi-Fi connected |
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
- `Arduino: Upload` builds and uploads to `/dev/cu.usbserial-8310` at a conservative 115200 baud.
- `Arduino: Monitor` opens the serial monitor at 115200 baud.
- `Server: Start` serves the dashboard at <http://localhost:5000>.

The Arduino tasks reuse the CLI, ESP32 core and Adafruit libraries already installed with Arduino IDE. If the ESP32 appears under a different serial port later, update the two port values in `.vscode/tasks.json`.

## Planned

- Detect six floors, expected to be roughly 3–4 m apart, after measuring the real floor heights.
- Require a stable height for 1–2 seconds before declaring a floor and playing audio.
- Add a DFPlayer Mini, microSD card and 4 Ω / 3 W speaker.
- Power the prototype from a roughly 5,000 mAh USB power bank.
- Add an accelerometer only if pressure-based movement detection is not reliable enough.
