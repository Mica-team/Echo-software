#include <Arduino.h>

#include "BluetoothManager.h"
#include "ServoManager.h"
#include "FaceManager.h"

void setup()
{
    Serial.begin(115200);

    bluetoothSetup();
    delay(500);
    
    servoSetup();
    faceSetup();
}

void loop()
{
    bluetoothLoop();

    if(command=="LEFT")
    {
        servoLeft();
        command="";
    }

    else if(command=="RIGHT")
    {
        servoRight();
        command="";
    }

    else if(command=="CENTER")
    {
        servoCenter();
        command="";
    }

    else if(command=="HAPPY")
    {
        happyFace();
        command="";
    }

    else if(command=="IDLE")
    {
        idleFace();
        command="";
    }

    else if(command=="SLEEP")
    {
        sleepFace();
        command="";
    }

    delay(20);
}
