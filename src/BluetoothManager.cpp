#include <Arduino.h>
#include <BluetoothSerial.h>
#include "BluetoothManager.h"

BluetoothSerial SerialBT;

String command = "";

void bluetoothSetup()
{
    delay(2000);

    if (!SerialBT.begin("Echo")) {
        Serial.println("Bluetooth FAILED!");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("Bluetooth READY");
}

void bluetoothLoop()
{
    if (SerialBT.available()) {
        command = SerialBT.readStringUntil('\n');
        command.trim();
        Serial.println(command);
    }
}
