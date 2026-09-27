#include <Arduino.h>

#include "BluetoothManager.h"
#include "ServoManager.h"
#include "FaceManager.h"
#include "OTAManager.h"
#include "ThermalManager.h"

void setup()
{
    Serial.begin(115200);

    bluetoothSetup();
    servoSetup();
    faceSetup();
    otaSetup();
    thermalSetup();
}

void loop()
{
    // Thermal protection runs first so communication and hardware commands
    // cannot keep operating normally while the ESP32 is overheating.
    if (thermalLoop())
        return;

    if (thermalEmergencyActive())
        return;

    bluetoothLoop();
    otaLoop();

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
        command = "";
        delay(1);
        return;
    }

    command = "";
    delay(1);
}
