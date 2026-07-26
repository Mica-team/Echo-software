#include "FaceManager.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_DC   2
#define OLED_RST  4
#define OLED_CS   -1

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &SPI,
  OLED_DC,
  OLED_RST,
  OLED_CS
);

void FaceManager::begin() {

  Serial.begin(115200);

  SPI.begin(18, -1, 23, -1);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("OLED FAILED");
    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();

  // Left eye
  display.fillCircle(40, 24, 6, SSD1306_WHITE);

  // Right eye
  display.fillCircle(88, 24, 6, SSD1306_WHITE);

  // Smile
  display.drawCircle(64, 40, 16, SSD1306_WHITE);
  display.fillRect(48, 24, 32, 16, SSD1306_BLACK);

  display.display();
}


void FaceManager::update() {
  // Future animations go here
}
