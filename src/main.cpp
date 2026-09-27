#include <Arduino.h>

#include "BluetoothManager.h"
#include "ServoManager.h"
#include "FaceManager.h"
#include "OTAManager.h"

void setup()
{
    Serial.begin(115200);

    bluetoothSetup();
    servoSetup();
    faceSetup();
    otaSetup();
}

void loop()
{
    bluetoothLoop();
    otaLoop();

    // Execute hardware commands only after the communication
    // managers have had a chance to consume their commands.
    if (command == "LEFT")
    {
        servoLeft();
    }
    else if (command == "RIGHT")
    {
        servoRight();
    }
    else if (command == "CENTER")
    {
        servoCenter();
    }
    else if (command == "HAPPY")
    {
        happyFace();
    }
    else if (command == "IDLE")
    {
        idleFace();
    }
    else if (command == "SLEEP")
    {
        sleepFace();
    }
    else
    {
        // Commands handled by Bluetooth/OTA managers are already
        // cleared there. Unknown commands are discarded here.
        command = "";
        delay(1);
        return;
    }

    command = "";
    delay(1);
}
