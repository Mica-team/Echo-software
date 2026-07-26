#include <Arduino.h>
#include <BluetoothSerial.h>
#include "BluetoothManager.h"

BluetoothSerial SerialBT;

String command="";

void bluetoothSetup()
{
    SerialBT.begin("Echo");
    Serial.println("Bluetooth Ready");
}

void bluetoothLoop()
{
    if(SerialBT.available())
    {
        command=SerialBT.readStringUntil('\n');
        command.trim();
        Serial.println(command);
    }
}
