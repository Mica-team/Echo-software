#include "FaceManager.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_DC 2
#define OLED_RST 4
#define OLED_CS -1

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &SPI,
    OLED_DC,
    OLED_RST,
    OLED_CS
);

enum class Face : uint8_t
{
    None,
    Happy,
    Idle,
    Sleep
};

static Face currentFace = Face::None;

static void beginFrame()
{
    display.clearDisplay();
}

static void finishFrame()
{
    display.display();
}

void faceSetup()
{
    SPI.begin(18, -1, 23, -1);
    SPI.setFrequency(8000000);

    if (!display.begin(SSD1306_SWITCHCAPVCC))
    {
        Serial.println("OLED FAILED");
        return;
    }

    currentFace = Face::None;
    idleFace();
}

void happyFace()
{
    if (currentFace == Face::Happy)
        return;

    beginFrame();

    display.fillCircle(40, 24, 6, SSD1306_WHITE);
    display.fillCircle(88, 24, 6, SSD1306_WHITE);

    display.drawCircle(64, 40, 16, SSD1306_WHITE);
    display.fillRect(48, 24, 32, 16, SSD1306_BLACK);

    finishFrame();
    currentFace = Face::Happy;
}

void idleFace()
{
    if (currentFace == Face::Idle)
        return;

    beginFrame();

    display.fillCircle(40, 24, 6, SSD1306_WHITE);
    display.fillCircle(88, 24, 6, SSD1306_WHITE);

    finishFrame();
    currentFace = Face::Idle;
}

void sleepFace()
{
    if (currentFace == Face::Sleep)
        return;

    beginFrame();

    display.drawLine(35, 24, 45, 24, SSD1306_WHITE);
    display.drawLine(83, 24, 93, 24, SSD1306_WHITE);

    finishFrame();
    currentFace = Face::Sleep;
}
