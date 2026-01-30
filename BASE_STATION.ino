#include <SPI.h>
#include <LoRa.h>

// LoRa Pins for ESP32
#define SCK     18
#define MISO    19
#define MOSI    13
#define SS      22
#define RST     23
#define DIO0    21

#define LED_PIN 4 // LED for status

unsigned long lastPacketTime = 0;
int packetCount = 0;
int lastPacketID = -1;

void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}

String getLinkQuality(int rssi) {
  if (rssi > -80) return "Excellent ✅";
  if (rssi > -100) return "Good 👍";
  if (rssi > -115) return "Weak ⚠";
  return "Very Weak ❌";
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);

  // LoRa setup
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) { 
    Serial.println("Starting LoRa failed!"); 
    while (1);
  }
  Serial.println("📡 Base Station Ready");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = LoRa.readString();
    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();
    
    lastPacketTime = millis();
    packetCount++;

    Serial.println("----- Packet Received -----");
    Serial.println("Message: " + incoming);
    Serial.println("RSSI: " + String(rssi) + " dBm");
    Serial.println("SNR: " + String(snr) + " dB");
    Serial.println("Link Quality: " + getLinkQuality(rssi));
    Serial.println("Total Packets: " + String(packetCount));
    Serial.println("---------------------------");

    blinkLED(2, 200); // Blink twice on receive

    // Parse message (format: TransID:SendID:RevID:Message)
    int firstColon = incoming.indexOf(':');
    int secondColon = incoming.indexOf(':', firstColon + 1);
    if (firstColon == -1 || secondColon == -1) {
      Serial.println("Invalid format - ignored");
      return;
    }

    String transID = incoming.substring(0, firstColon);
    String sendID = incoming.substring(firstColon + 1, secondColon);
    int msgStart = incoming.indexOf(':', secondColon + 1);
    if (msgStart == -1) {
      Serial.println("Invalid format - ignored");
      return;
    }
    String revID = incoming.substring(secondColon + 1, msgStart);
    String message = incoming.substring(msgStart + 1);

    if (transID == "N") {
      Serial.println("Ignored - TransID is N");
      return;
    }

    // Forward message with TransID as N
    String forwardMsg = "N:" + sendID + ":" + revID + ":" + message;
    delay(1000);
    LoRa.beginPacket();
    LoRa.print(forwardMsg);
    LoRa.endPacket();
    Serial.println("📤 Forwarded: " + forwardMsg);
    blinkLED(2, 200); // Blink twice on transmit
  }

  // Out-of-range alert if no packet in 10 seconds
  if (millis() - lastPacketTime > 10000 && lastPacketTime != 0) {
    Serial.println("🚨 No packets for 10s - Possible Out of Range");
    blinkLED(5, 100); // Blink fast to indicate warning
    lastPacketTime = millis(); // prevent spamming
  }
}
