#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LORA_RX 9
#define LORA_TX 8
SoftwareSerial lora(LORA_RX, LORA_TX);

float temperature = 0, humidity = 0;
int soil = 0;                           // Thêm biến độ ẩm đất
int rssi = 0, snr = 0;
unsigned long packetCount = 0;
unsigned long lastDisplay = 0;

// Hàm vẽ cột sóng dạng 4 vạch dọc
void drawRSSIBars(int x, int y, int rssi) {
  int bars = 0;
  if (rssi >= -30) bars = 4;
  else if (rssi >= -50) bars = 3;
  else if (rssi >= -65) bars = 2;
  else if (rssi >= -80) bars = 1;
  
  for (int i = 0; i < 4; i++) {
    int x0 = x + i * 5;
    int height = (i + 1) * 2;
    if (i < bars)
      display.fillRect(x0, y + 8 - height, 3, height, WHITE);
    else
      display.drawRect(x0, y + 8 - height, 3, height, WHITE);
  }
}

void setup() {
  Serial.begin(9600);
  lora.begin(9600);
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED NOT FOUND");
    while (1);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();
  delay(1000);
  Serial.println("Node2 Ready");
}

void loop() {
  if (lora.available()) {
    String msg = lora.readStringUntil('\n');
    msg.trim();
    if (msg.startsWith("+RCV=")) {
      Serial.println(msg);
      packetCount++;

      // Parse RSSI & SNR
      int lastComma = msg.lastIndexOf(',');
      int secondLastComma = msg.lastIndexOf(',', lastComma - 1);
      if (secondLastComma > 0) {
        rssi = msg.substring(secondLastComma + 1, lastComma).toInt();
        snr = msg.substring(lastComma + 1).toInt();
      }

      // Parse nhiệt độ (T:)
      int tPos = msg.indexOf("T:");
      if (tPos >= 0) {
        int tEnd = msg.indexOf(',', tPos);
        if (tEnd > tPos)
          temperature = msg.substring(tPos + 2, tEnd).toFloat();
      }

      // Parse độ ẩm không khí (,H:)
      int hPos = msg.indexOf(",H:");
      if (hPos >= 0) {
        int hEnd = msg.indexOf(',', hPos + 1);
        if (hEnd > hPos)
          humidity = msg.substring(hPos + 3, hEnd).toFloat();
      }

      // Parse độ ẩm đất (,S:)
      int sPos = msg.indexOf(",S:");
      if (sPos >= 0) {
        int sEnd = msg.indexOf(',', sPos + 1);
        if (sEnd > sPos)
          soil = msg.substring(sPos + 3, sEnd).toInt();
      }
    }
  }

  if (millis() - lastDisplay >= 500) {
    lastDisplay = millis();
    drawOLED();
  }
}

void drawOLED() {
  display.clearDisplay();

  // Dòng tiêu đề + cột sóng
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("NODE2 MONITOR");
  drawRSSIBars(100, 1, rssi);

  // Dòng 1: Nhiệt độ
  display.setCursor(0, 12);
  display.print("T:");
  display.print(temperature, 1);
  display.print(" C");

  // Dòng 2: Độ ẩm không khí
  display.setCursor(0, 22);
  display.print("H:");
  display.print(humidity, 1);
  display.print(" %");

  // Dòng 3: Độ ẩm đất
  display.setCursor(0, 32);
  display.print("S:");
  display.print(soil);
  display.print("   ");  // padding

  // Dòng 4: Packet count và SNR
  display.setCursor(0, 42);
  display.print("PK:");
  display.print(packetCount);
  display.setCursor(70, 42);
  display.print("SNR:");
  display.print(snr);

  display.display();
}
