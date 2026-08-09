#include <Arduino.h>
#include <Preferences.h>

#include "BluetoothManager.h"
#include "ServoManager.h"
#include "FaceManager.h"
#include "OTAManager.h"

// ESP32 DevKit onboard blue LED is normally connected to GPIO 2.
#ifndef ECHO_BLUE_LED_PIN
#define ECHO_BLUE_LED_PIN 2
#endif

// OTAManager stores this value before rebooting after a successful update.
// false = LED OFF, true = LED ON. Every successful OTA flips the state.
static Preferences ledPreferences;

void setup()
{
    Serial.begin(115200);

    pinMode(ECHO_BLUE_LED_PIN, OUTPUT);

    // Remember the LED state selected by the previous successful OTA.
    // First boot defaults to OFF.
    bool ledOn = false;
    ledPreferences.begin("echo-led", true);
    ledOn = ledPreferences.getBool("ota_led", false);
    ledPreferences.end();

    digitalWrite(ECHO_BLUE_LED_PIN, ledOn ? HIGH : LOW);

    delay(500);

    Serial.println();
    Serial.println("====================");
    Serial.println("      ECHO BOOT     ");
    Serial.println("====================");
    Serial.printf("OTA LED state: %s\n", ledOn ? "ON" : "OFF");

    bluetoothSetup();
    delay(500);

    servoSetup();
    delay(100);

    faceSetup();
    delay(100);

    otaSetup();

    Serial.println("Echo ready.");
}

void loop()
{
    bluetoothLoop();
    otaLoop();

    if (command == "LEFT")
    {
        servoLeft();
        command = "";
    }
    else if (command == "RIGHT")
    {
        servoRight();
        command = "";
    }
    else if (command == "CENTER")
    {
        servoCenter();
        command = "";
    }
    else if (command == "HAPPY")
    {
        happyFace();
        command = "";
    }
    else if (command == "IDLE")
    {
        idleFace();
        command = "";
    }
    else if (command == "SLEEP")
    {
        sleepFace();
        command = "";
    }
    else if (command.length() > 0)
    {
        Serial.print("Unknown command: ");
        Serial.println(command);
        command = "";
    }

    delay(5);
}
