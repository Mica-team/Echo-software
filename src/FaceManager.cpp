#include "FaceManager.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace
{
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;

constexpr int OLED_DC = 2;
constexpr int OLED_RST = 4;
constexpr int OLED_CS = -1;
constexpr uint32_t OLED_SPI_FREQUENCY = 4000000;
}

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &SPI,
    OLED_DC,
    OLED_RST,
    OLED_CS
);

void faceSetup()
{
    SPI.begin(18, -1, 23, -1);
    SPI.setFrequency(OLED_SPI_FREQUENCY);

    if (!display.begin(SSD1306_SWITCHCAPVCC))
    {
        Serial.println("OLED FAILED");
        while (true)
        {
            delay(1000);
        }
    }

    display.clearDisplay();
    display.display();

    idleFace();
}

void happyFace()
{
    display.clearDisplay();

    display.fillCircle(40, 24, 6, SSD1306_WHITE);
    display.fillCircle(88, 24, 6, SSD1306_WHITE);

    display.drawCircle(64, 40, 16, SSD1306_WHITE);
    display.fillRect(48, 24, 32, 16, SSD1306_BLACK);

    display.display();
}

void idleFace()
{
    display.clearDisplay();

    display.fillCircle(40, 24, 6, SSD1306_WHITE);
    display.fillCircle(88, 24, 6, SSD1306_WHITE);

    display.display();
}

void sleepFace()
{
    display.clearDisplay();

    display.drawLine(35, 24, 45, 24, SSD1306_WHITE);
    display.drawLine(83, 24, 93, 24, SSD1306_WHITE);

    display.display();
}
