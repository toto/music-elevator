#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

#include "secrets.h"

Adafruit_BME680 bme;

// Generic ESP32 Dev Module onboard LED. Change these if your board differs.
constexpr uint8_t STATUS_LED_PIN = 2;
constexpr bool STATUS_LED_ACTIVE_HIGH = true;

float referencePressure;
float filteredAltitude = 0;

float relativeAltitude(float pressure, float reference) {
  return 44330.0 * (1.0 - pow(pressure / reference, 0.1903));
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

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    showConnecting();
    delay(50);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  setStatusLed(true);

  float total = 0;

  for (int i = 0; i < 30; i++) {
    bme.performReading();
    total += bme.pressure;
    delay(100);
  }

  referencePressure = total / 30.0;

  Serial.println("Calibration complete");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    showConnecting();
    delay(250);
    return;
  }

  setStatusLed(true);

  if (!bme.performReading()) {
    Serial.println("BME680 reading failed");
    showError();
    delay(500);
    return;
  }

  float altitude = relativeAltitude(bme.pressure, referencePressure);

  filteredAltitude = filteredAltitude * 0.8 + altitude * 0.2;

  float pressure = bme.pressure / 100.0;

  Serial.print("Height: ");
  Serial.println(filteredAltitude);

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
