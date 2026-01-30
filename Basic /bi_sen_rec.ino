#include <SPI.h>
#include <LoRa.h>

// Pin definitions
#define SCK     18    // SPI Clock
#define MISO    19    // SPI MISO
#define MOSI    2     // SPI MOSI
#define SS      22    // LoRa CS (NSS)
#define RST     23    // LoRa Reset
#define DIO0    21    // LoRa IRQ (DIO0)
#define LED     4     // LED on GPIO4 (D4)
#define BUTTON  5     // Push button connected to GPIO5 (active LOW)

// Device ID: Change to 1 for Pager 1, 2 for Pager 2
#define DEVICE_ID 1   // <<< Change to 2 for the second device

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  pinMode(BUTTON, INPUT_PULLUP); // Button is active LOW

  // Initialize SPI
  SPI.begin(SCK, MISO, MOSI, SS);

  // Initialize LoRa
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check connections.");
    while (1);
  }
  Serial.println("✅ LoRa Pager System Ready!");
}

void loop() {
  // 1️⃣ ---- RECEIVE MESSAGE ----
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedMessage = "";
    while (LoRa.available()) {
      receivedMessage += (char)LoRa.read();
    }

    Serial.print("📩 Received: ");
    Serial.println(receivedMessage);

    blinkLED();  // Blink LED on receiving message

    // Auto acknowledgment for both devices
    if (DEVICE_ID == 1 && receivedMessage == "Hello from Pager 2") {
      delay(2000);
      sendMessage("Received by Pager 1");
    }
    else if (DEVICE_ID == 2 && receivedMessage == "Hello from Pager 1") {
      delay(2000);
      sendMessage("Received by Pager 2");
    }
  }

  // 2️⃣ ---- BUTTON PRESS TO SEND MESSAGE ----
  if (digitalRead(BUTTON) == LOW) {
    delay(500); // Debounce

    if (DEVICE_ID == 1) {
      sendMessage("Hello from Pager 1");
    } 
    else if (DEVICE_ID == 2) {
      sendMessage("Hello from Pager 2");
    }

    while (digitalRead(BUTTON) == LOW); // Wait until button released
  }
}

// Function to send message and blink LED
void sendMessage(String msg) {
  Serial.print("📤 Sending: ");
  Serial.println(msg);

  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();

  blinkLED(); // Blink LED on sending message
}

// Function to blink LED
void blinkLED() {
  digitalWrite(LED, HIGH);
  delay(300);
  digitalWrite(LED, LOW);
}
