#include <Arduino.h>

#include "BluetoothManager.h"
#include "ServoManager.h"
#include "FaceManager.h"
#include "OTAManager.h"

void setup()
{
    Serial.begin(115200);

    bluetoothSetup();
    delay(500);

    servoSetup();
    faceSetup();
    otaSetup();
}

void loop()
{
    bluetoothLoop();
    otaLoop();

    // -----------------------------
    // Servo commands
    // -----------------------------

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

    // -----------------------------
    // Face commands
    // -----------------------------

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

    delay(20);
}
