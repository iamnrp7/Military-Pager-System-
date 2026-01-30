#include <SPI.h>
#include <LoRa.h>
#include <LiquidCrystal.h>  // For parallel LCD

// === LoRa Pin Definitions (ESP32) ===
#define SCK     18
#define MISO    19
#define MOSI    13
#define SS      22
#define RST     23
#define DIO0    21

// === Hardware ===
#define LED     4
#define BUTTON  5

// === LCD Pin Mapping ===
// RS, EN, D4, D5, D6, D7
LiquidCrystal lcd(14, 27, 32, 33, 25, 26);

// === Node Settings ===
String tnID = "B";   // Transmit Node Prefix
int nID = 2;         // This node's ID
int nrID = 1;        // Target receiver node ID when sending

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  Serial.begin(115200);

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Node " + String(nID));
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  // Initialize LoRa
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check wiring.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LoRa Failed!");
    while (1);
  }

  Serial.print("Node ");
  Serial.print(nID);
  Serial.println(" ready.");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Node ");
  lcd.print(nID);
  lcd.setCursor(0, 1);
  lcd.print("Ready.");
}

void blinkOnce() {
  digitalWrite(LED, HIGH);
  delay(200);
  digitalWrite(LED, LOW);
}

void sendMessage() {
  String msg = tnID + ":" + String(nID) + ":" + String(nrID) + ":Send Backup";

  blinkOnce(); // Blink when sending
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();

  Serial.println("Sent: " + msg);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sent to ID " + String(nrID));
  lcd.setCursor(0, 1);
  lcd.print("Msg: Backup");
}

void loop() {
  // ===== Sending =====
  if (digitalRead(BUTTON) == LOW) {
    sendMessage();
    delay(500); // Debounce delay
  }

  // ===== Receiving =====
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }

    // Expected format: N:2:1:Message Text
    int firstColon = incoming.indexOf(':');
    int secondColon = incoming.indexOf(':', firstColon + 1);
    int thirdColon = incoming.indexOf(':', secondColon + 1);

    if (firstColon == -1 || secondColon == -1 || thirdColon == -1) {
      return; // Ignore invalid packets
    }

    String tbID = incoming.substring(0, firstColon);
    int bsID = incoming.substring(firstColon + 1, secondColon).toInt();
    int brID = incoming.substring(secondColon + 1, thirdColon).toInt();
    String message = incoming.substring(thirdColon + 1);

    if (tbID == "N" && nID == brID) {
      blinkOnce(); // Blink on valid reception

      Serial.print("From Base Station (ID ");
      Serial.print(bsID);
      Serial.print("): ");
      Serial.println(message);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("From Node : " + String(bsID));
      lcd.setCursor(0, 1);
      lcd.print(message.substring(0, 16)); // Trim if longer than 16 chars
    }
  }
}