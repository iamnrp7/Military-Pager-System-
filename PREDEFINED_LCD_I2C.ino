#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>  // I2C LCD library

// === LoRa Pin Definitions (ESP32) ===
#define SCK     18
#define MISO    19
#define MOSI    13
#define SS      26  // Changed
#define RST     23
#define DIO0    25  // Changed

// === Hardware ===
#define LED     4
#define BUTTON  5

// === LCD Setup ===
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Use scanner if this address doesn't work

// === Node Settings ===
String tnID = "B";  // Transmit Node Prefix
int nID = 1;        // This node's ID
int nrID = 2;       // Target receiver node ID

// === Message Selection ===
String messages[] = {
  "Send Backup",
  "Need Ammo",
  "Enemy Spotted",
  "All Clear",
  "Regroup",
  "Send Medic"
};
int msgCount = sizeof(messages) / sizeof(messages[0]);
int selectedMsgIndex = 0;
unsigned long buttonPressTime = 0;
bool buttonPressed = false;

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  Serial.begin(115200);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
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

  Serial.println("Node " + String(nID) + " ready.");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Node ");
  lcd.print(nID);
  lcd.setCursor(0, 1);
  lcd.print("Ready.");

  delay(1000);
  updateLCDWithMessage();  // Show first message
}

void blinkOnce() {
  digitalWrite(LED, HIGH);
  delay(200);
  digitalWrite(LED, LOW);
}

void sendMessage() {
  String msg = tnID + ":" + String(nID) + ":" + String(nrID) + ":" + messages[selectedMsgIndex];

  blinkOnce();  // Blink when sending
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();

  Serial.println("Sent: " + msg);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sent to ID " + String(nrID));
  lcd.setCursor(0, 1);
  lcd.print(messages[selectedMsgIndex].substring(0, 16));  // LCD max 16 chars
}

void updateLCDWithMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select Msg:");
  lcd.setCursor(0, 1);
  lcd.print(messages[selectedMsgIndex].substring(0, 16));
}

void loop() {
  // === Handle Button Press (Short = Select, Long = Send) ===
  int buttonState = digitalRead(BUTTON);

  if (buttonState == LOW && !buttonPressed) {
    buttonPressed = true;
    buttonPressTime = millis();
  }

  if (buttonState == HIGH && buttonPressed) {
    unsigned long pressDuration = millis() - buttonPressTime;
    buttonPressed = false;

    if (pressDuration < 600) {
      // Short press: Cycle to next message
      selectedMsgIndex = (selectedMsgIndex + 1) % msgCount;
      updateLCDWithMessage();
      Serial.println("Selected: " + messages[selectedMsgIndex]);
    } else {
      // Long press: Send selected message
      sendMessage();
    }

    delay(300);  // Debounce
  }

  // === Receiving Messages ===
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

    if (firstColon == -1 || secondColon == -1 || thirdColon == -1) return;

    String tbID = incoming.substring(0, firstColon);
    int bsID = incoming.substring(firstColon + 1, secondColon).toInt();
    int brID = incoming.substring(secondColon + 1, thirdColon).toInt();
    String message = incoming.substring(thirdColon + 1);

    // Accept only if this node is receiver
    if (tbID == "N" && nID == brID) {
      blinkOnce();  // Blink on reception

      Serial.print("From Node ");
      Serial.print(bsID);
      Serial.print(": ");
      Serial.println(message);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("From Node: " + String(bsID));
      lcd.setCursor(0, 1);
      lcd.print(message.substring(0, 16));  // Trim to fit LCD
    }
  }
}
