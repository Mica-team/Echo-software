#include <Arduino.h>
#include <BluetoothSerial.h>
#include "BluetoothManager.h"

BluetoothSerial SerialBT;

String command = "";

static unsigned long lastTemperatureReport = 0;
static const unsigned long TEMPERATURE_INTERVAL = 30000; // 30 seconds

void bluetoothSetup()
{
    delay(2000);

    if (!SerialBT.begin("Echo"))
    {
        Serial.println("Bluetooth FAILED!");

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("Bluetooth READY");

    lastTemperatureReport = millis();
}

void bluetoothLoop()
{
    // -------------------------------------------------
    // Receive commands from Android / Bluetooth Serial
    // -------------------------------------------------

    if (SerialBT.available())
    {
        command = SerialBT.readStringUntil('\n');
        command.trim();

        if (command.length() > 0)
        {
            Serial.print("Bluetooth command: ");
            Serial.println(command);
        }
    }

    // -------------------------------------------------
    // Report ESP32 internal temperature every 30 sec
    // -------------------------------------------------

    unsigned long now = millis();

    if (now - lastTemperatureReport >= TEMPERATURE_INTERVAL)
    {
        lastTemperatureReport = now;

        float temperature = temperatureRead();

        // USB Serial debug output
        Serial.print("ESP32 CPU temperature: ");
        Serial.print(temperature);
        Serial.println(" C");

        // Bluetooth report
        if (SerialBT.hasClient())
        {
            SerialBT.print("TEMP:");
            SerialBT.println(temperature, 2);
        }
    }
}
