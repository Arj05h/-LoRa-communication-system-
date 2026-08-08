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
int soil = 0;
int rssi = 0, snr = 0;
unsigned long packetCount = 0;
unsigned long lastDisplay = 0;
bool hasPerson = false;

// Hàm vẽ cột sóng RSSI
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

// Hàm vẽ icon người 16x16, đặt tại góc (x,y) (không bị cắt)
void drawPersonIcon(int x, int y) {
  // Đầu (hình tròn đường kính 8 pixel)
  display.fillCircle(x + 7, y + 4, 4, WHITE);
  // Thân (hình chữ nhật 8x10)
  display.fillRect(x + 4, y + 9, 8, 10, WHITE);
  // Tay (hai bên)
  display.fillRect(x + 2, y + 11, 3, 3, WHITE);
  display.fillRect(x + 11, y + 11, 3, 3, WHITE);
  // Chân (hai chân)
  display.fillRect(x + 4, y + 19, 3, 4, WHITE);
  display.fillRect(x + 9, y + 19, 3, 4, WHITE);
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
  Serial.println("Node2 Ready");
}

void loop() {
  if (lora.available()) {
    String msg = lora.readStringUntil('\n');
    msg.trim();

    if (msg.startsWith("+RCV=")) {
      Serial.println(msg);
      packetCount++;

      // Parse RSSI và SNR (hai giá trị cuối)
      int lastComma = msg.lastIndexOf(',');
      int secondLastComma = msg.lastIndexOf(',', lastComma - 1);
      if (secondLastComma > 0) {
        rssi = msg.substring(secondLastComma + 1, lastComma).toInt();
        snr  = msg.substring(lastComma + 1).toInt();
      }

      // Parse T, H, S
      int tPos = msg.indexOf("T:");
      if (tPos >= 0) {
        int tEnd = msg.indexOf(',', tPos);
        if (tEnd > tPos) temperature = msg.substring(tPos + 2, tEnd).toFloat();
      }
      int hPos = msg.indexOf(",H:");
      if (hPos >= 0) {
        int hEnd = msg.indexOf(',', hPos + 1);
        if (hEnd > hPos) humidity = msg.substring(hPos + 3, hEnd).toFloat();
      }
      int sPos = msg.indexOf(",S:");
      if (sPos >= 0) {
        int sEnd = msg.indexOf(',', sPos + 1);
        if (sEnd > sPos) soil = msg.substring(sPos + 3, sEnd).toInt();
      }

      // Kiểm tra ALARM (có người)
      hasPerson = (msg.indexOf("ALARM") >= 0);
    }
  }

  if (millis() - lastDisplay >= 500) {
    lastDisplay = millis();
    drawOLED();
  }
}

void drawOLED() {
  display.clearDisplay();

  // Dòng tiêu đề
  display.setTextSize(1);
  display.setCursor(0, 2);
  display.print("NODE2");

  // 1. HIỂN THỊ SỐ RSSI TRƯỚC (đặt tại x=45)
  display.setCursor(60, 2);
  display.print(rssi);

  // 2. SAU ĐÓ MỚI VẼ CỘT SÓNG (bắt đầu từ x=75)
  drawRSSIBars(80, 2, rssi);

  // 3. HÌNH NGƯỜI (đặt tại x=105, y=8 để không bị cắt lên trên)
  if (hasPerson) {
    drawPersonIcon(105, 8);
  }

  // Các dòng dữ liệu
  display.setCursor(0, 18);
  display.print("TEMP: ");
  display.print(temperature, 1);
  display.print(" C");

  display.setCursor(0, 28);
  display.print("HUM: ");
  display.print(humidity, 1);
  display.print(" %");

  display.setCursor(0, 38);
  display.print("SOIL: ");
  display.print(soil);

  display.setCursor(0, 50);
  display.print("PK: ");
  display.print(packetCount);
  display.setCursor(70, 50);
  display.print("SNR: ");
  display.print(snr);

  display.display();
}
