#include <Arduino.h>

#include "BluetoothManager.h"
#include "ServoManager.h"
#include "FaceManager.h"
#include "OTAManager.h"

void setup()
{
    Serial.begin(115200);

    delay(500);

    Serial.println();
    Serial.println("====================");
    Serial.println("      ECHO BOOT     ");
    Serial.println("====================");

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
