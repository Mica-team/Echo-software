#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// ESP32 Pins
#define OLED_SCK   18
#define OLED_MOSI  23
#define OLED_DC    2
#define OLED_RST   4

// No CS pin on your module
Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &SPI,
  OLED_DC,
  OLED_RST,
  -1
);

void drawHappyFace() {
  display.clearDisplay();

  // Eyes
  display.fillCircle(38, 24, 5, SSD1306_WHITE);
  display.fillCircle(90, 24, 5, SSD1306_WHITE);

  // Smile
  display.drawCircle(64, 40, 16, SSD1306_WHITE);
  display.fillRect(48, 24, 32, 16, SSD1306_BLACK);

  display.display();
}

void drawIdleFace() {
  display.clearDisplay();

  display.fillCircle(38, 24, 5, SSD1306_WHITE);
  display.fillCircle(90, 24, 5, SSD1306_WHITE);

  display.drawLine(52, 48, 76, 48, SSD1306_WHITE);

  display.display();
}

void drawSleepFace() {
  display.clearDisplay();

  // Closed eyes
  display.drawLine(30, 24, 46, 24, SSD1306_WHITE);
  display.drawLine(82, 24, 98, 24, SSD1306_WHITE);

  // Mouth
  display.fillRect(60, 46, 8, 3, SSD1306_WHITE);

  display.setCursor(105, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.print("Zz");

  display.display();
}

void setup() {
  SPI.begin(OLED_SCK, -1, OLED_MOSI, -1);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    while (1);
  }

  drawHappyFace();
}

void loop() {
  delay(3000);
  drawIdleFace();

  delay(3000);
  drawSleepFace();

  delay(3000);
  drawHappyFace();
}
