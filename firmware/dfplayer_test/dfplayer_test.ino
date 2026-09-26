#include <DFRobotDFPlayerMini.h>

// UART2 leaves USB Serial and the BME680 I2C pins free.
constexpr int DFPLAYER_RX_PIN = 16;  // ESP32 RX <- DFPlayer TX
constexpr int DFPLAYER_TX_PIN = 17;  // ESP32 TX -> 1 kOhm -> DFPlayer RX
constexpr uint8_t TEST_VOLUME = 10; // DFPlayer range: 0-30

DFRobotDFPlayerMini player;
bool playerReady = false;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
  delay(1500);  // Let the player mount its microSD card.

  Serial.println("Starting DFPlayer Mini test");
  if (!player.begin(Serial2, true, true)) {
    Serial.println("DFPlayer did not respond. Check 5V, GND, RX/TX, and microSD card.");
    return;
  }
  playerReady = true;

  Serial.println("DFPlayer online. Playing first file on microSD card once.");
  Serial.println("Send p to replay, s to stop (115200 baud).");
  player.volume(TEST_VOLUME);
  player.play(1);
}

void loop() {
  if (!playerReady) {
    delay(1000);
    return;
  }

  if (Serial.available()) {
    char command = Serial.read();
    if (command == 'p') {
      player.play(1);
      Serial.println("Play requested");
    } else if (command == 's') {
      player.stop();
      Serial.println("Stop requested");
    }
  }

  if (player.available()) {
    uint8_t event = player.readType();
    int value = player.read();
    Serial.print("DFPlayer event ");
    Serial.print(event);
    Serial.print(" value ");
    Serial.println(value);
  }
}
