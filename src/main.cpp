#include <Arduino.h>

#include "BluetoothManager.h"
#include "ServoManager.h"
#include "FaceManager.h"

void setup()
{
    Serial.begin(115200);

    bluetoothSetup();
    servoSetup();
    faceSetup();
}

void loop()
{
    bluetoothLoop();

    if(command=="LEFT")
        servoLeft();

    if(command=="RIGHT")
        servoRight();

    if(command=="CENTER")
        servoCenter();

    if(command=="HAPPY")
        happyFace();

    if(command=="IDLE")
        idleFace();

    if(command=="SLEEP")
        sleepFace();
}
