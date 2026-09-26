#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <DFRobotDFPlayerMini.h>

#include "secrets.h"

Adafruit_BME680 bme;
DFRobotDFPlayerMini player;

// Generic ESP32 Dev Module onboard LED. Change these if your board differs.
constexpr uint8_t STATUS_LED_PIN = 2;
constexpr bool STATUS_LED_ACTIVE_HIGH = true;
constexpr bool ENABLE_WIFI_REPORTING = false;  // Use Serial Monitor for elevator sampling.

constexpr int DFPLAYER_RX_PIN = 16;  // ESP32 RX <- DFPlayer TX
constexpr int DFPLAYER_TX_PIN = 17;  // ESP32 TX -> 1 kOhm -> DFPlayer RX
constexpr uint8_t PLAYER_VOLUME = 10;  // 0-30
constexpr uint8_t FLOOR_COUNT = 8;
constexpr uint8_t FALLBACK_TRACK = 99;  // /mp3/0099.mp3

// Matching entries are ordered from floor -1 to floor 6.
constexpr int8_t FLOOR_LABEL[FLOOR_COUNT] = {-1, 0, 1, 2, 3, 4, 5, 6};
// Settled 15-second medians from calibration/2026-09-26/, in hPa.
constexpr float FLOOR_PRESSURE_HPA[FLOOR_COUNT] = {
    1019.07, 1018.79, 1018.27, 1017.92,
    1017.59, 1017.21, 1016.83, 1016.49};
// Keep the established card filenames: /mp3/0002.mp3 through /mp3/0009.mp3.
// Set an entry to zero for fallback.
constexpr uint8_t FLOOR_TRACK[FLOOR_COUNT] = {2, 3, 4, 5, 6, 7, 8, 9};
constexpr float FLOOR_PRESSURE_TOLERANCE_HPA = 0.15;

// Provisional arrival settings until playback is checked across trips.
constexpr float MIN_TRAVEL_METERS = 1.5;
constexpr float STABLE_BAND_METERS = 0.35;
constexpr unsigned long TRAVEL_WINDOW_MS = 10000;
constexpr unsigned long STABLE_TIME_MS = 1500;

float referencePressure;
float filteredAltitude = 0;
float lastStopAltitude = 0;
float travelWindowAltitude = 0;
float stableAnchorAltitude = 0;
unsigned long travelWindowStarted = 0;
unsigned long stableSince = 0;
unsigned long lastReconnectAttempt = 0;
bool travelled = false;
bool playerReady = false;

float relativeAltitude(float pressure, float reference) {
  return 44330.0 * (1.0 - pow(pressure / reference, 0.1903));
}

int8_t floorIndexForPressure(float pressureHpa) {
  int8_t nearestIndex = -1;
  float nearestDifference = FLOOR_PRESSURE_TOLERANCE_HPA;

  for (uint8_t i = 0; i < FLOOR_COUNT; i++) {
    if (FLOOR_PRESSURE_HPA[i] <= 0) continue;
    float difference = fabs(pressureHpa - FLOOR_PRESSURE_HPA[i]);
    if (difference <= nearestDifference) {
      nearestDifference = difference;
      nearestIndex = i;
    }
  }

  return nearestIndex;
}

void checkArrival(float altitude, float pressureHpa) {
  unsigned long now = millis();

  if (now - travelWindowStarted >= TRAVEL_WINDOW_MS) {
    travelWindowAltitude = altitude;
    travelWindowStarted = now;
  }
  if (fabs(altitude - travelWindowAltitude) >= MIN_TRAVEL_METERS) {
    travelled = true;
  }

  if (fabs(altitude - stableAnchorAltitude) > STABLE_BAND_METERS) {
    stableAnchorAltitude = altitude;
    stableSince = now;
  }

  if (!travelled || now - stableSince < STABLE_TIME_MS) return;

  travelled = false;
  travelWindowAltitude = altitude;
  travelWindowStarted = now;

  if (fabs(altitude - lastStopAltitude) < MIN_TRAVEL_METERS) return;
  lastStopAltitude = altitude;

  int8_t floorIndex = floorIndexForPressure(pressureHpa);
  uint8_t track = floorIndex < 0 || FLOOR_TRACK[floorIndex] == 0
                      ? FALLBACK_TRACK
                      : FLOOR_TRACK[floorIndex];
  Serial.print("Arrival at ");
  Serial.print(altitude, 2);
  Serial.print(" m, ");
  Serial.print(pressureHpa, 2);
  Serial.print(" hPa. Floor ");
  if (floorIndex < 0) {
    Serial.print("unknown");
  } else {
    Serial.print(static_cast<int>(FLOOR_LABEL[floorIndex]));
  }
  Serial.print(", track ");
  Serial.println(track);

  if (playerReady) player.playMp3Folder(track);
}

void setStatusLed(bool on) {
  digitalWrite(STATUS_LED_PIN, on == STATUS_LED_ACTIVE_HIGH ? HIGH : LOW);
}

void showConnecting() {
  setStatusLed((millis() / 250) % 2 == 0);
}

void showSending() {
  setStatusLed(false);
  delay(60);
  setStatusLed(true);
}

void showError() {
  for (int i = 0; i < 3; i++) {
    setStatusLed(false);
    delay(120);
    setStatusLed(true);
    delay(120);
  }
}

void haltWithError() {
  while (true) {
    showError();
    delay(600);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(STATUS_LED_PIN, OUTPUT);
  setStatusLed(false);

  Wire.begin(21, 22);

  if (!bme.begin(0x76)) {
    if (!bme.begin(0x77)) {
      Serial.println("BME680 not found");
      haltWithError();
    }
  }

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setPressureOversampling(BME680_OS_16X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_7);

  Serial2.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
  delay(1500);
  playerReady = player.begin(Serial2, true, true);
  if (playerReady) {
    player.volume(PLAYER_VOLUME);
    Serial.println("DFPlayer online");
  } else {
    Serial.println("DFPlayer unavailable; pressure reporting will continue");
  }

  if (ENABLE_WIFI_REPORTING) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastReconnectAttempt = millis();
    Serial.println("Connecting to WiFi");
  }

  float total = 0;

  for (int i = 0; i < 30; i++) {
    bme.performReading();
    total += bme.pressure;
    delay(100);
  }

  referencePressure = total / 30.0;

  Serial.println("Calibration complete");
  Serial.println("SAMPLE,millis,pressure_hpa,height_m");
  if (!ENABLE_WIFI_REPORTING) setStatusLed(true);
}

void loop() {
  if (!bme.performReading()) {
    Serial.println("BME680 reading failed");
    showError();
    delay(500);
    return;
  }

  float altitude = relativeAltitude(bme.pressure, referencePressure);

  filteredAltitude = filteredAltitude * 0.8 + altitude * 0.2;

  float pressure = bme.pressure / 100.0;

  Serial.print("SAMPLE,");
  Serial.print(millis());
  Serial.print(',');
  Serial.print(pressure, 3);
  Serial.print(',');
  Serial.println(filteredAltitude, 2);

  checkArrival(filteredAltitude, pressure);

  if (playerReady && player.available()) {
    uint8_t event = player.readType();
    int value = player.read();
    if (event == DFPlayerError) {
      Serial.print("DFPlayer error: ");
      Serial.println(value);
    }
  }

  if (!ENABLE_WIFI_REPORTING) {
    delay(500);
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnectAttempt >= 5000) {
      WiFi.reconnect();
      lastReconnectAttempt = millis();
    }
    showConnecting();
    delay(250);
    return;
  }

  setStatusLed(true);

  HTTPClient http;

  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  String json = "{";
  json += "\"pressure\":" + String(pressure, 2) + ",";
  json += "\"height\":" + String(filteredAltitude, 2);
  json += "}";

  showSending();
  int responseCode = http.POST(json);

  Serial.print("HTTP response: ");
  Serial.println(responseCode);

  http.end();

  if (responseCode < 200 || responseCode >= 300) {
    showError();
  }

  delay(500);
}
