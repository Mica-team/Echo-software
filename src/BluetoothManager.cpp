#include <Arduino.h>
#include <BluetoothSerial.h>
#include "BluetoothManager.h"

BluetoothSerial SerialBT;
String command = "";

static unsigned long lastTemperatureReport = 0;
static const unsigned long TEMPERATURE_INTERVAL = 30000UL;

void bluetoothSetup()
{
    delay(2000);

    if (!SerialBT.begin("Echo"))
    {
        Serial.println("Bluetooth FAILED!");
        while (true) delay(1000);
    }

    Serial.println("Bluetooth READY");
    lastTemperatureReport = millis();
}

static void sendTemperatureReport()
{
    const float temperature = temperatureRead();
    const unsigned long freeHeap = ESP.getFreeHeap();
    const unsigned long cpuMHz = getCpuFrequencyMhz();
    const char* btStatus = SerialBT.hasClient() ? "CONNECTED" : "WAITING";

    // Existing Android telemetry format.
    SerialBT.printf("TEMP:%.2f\n", temperature);

    // Diagnostic data for investigating Bluetooth-related heating/load.
    SerialBT.printf(
        "INFO:TEMP=%.2f;CPU=%lu;HEAP=%lu;BT=%s\n",
        temperature,
        cpuMHz,
        freeHeap,
        btStatus
    );

    Serial.printf(
        "TEMP: %.2f C | CPU: %lu MHz | HEAP: %lu | BT: %s\n",
        temperature,
        cpuMHz,
        freeHeap,
        btStatus
    );
}

void bluetoothLoop()
{
    // Non-blocking command reader. Avoids readStringUntil() timeout waits.
    while (SerialBT.available())
    {
        const char c = static_cast<char>(SerialBT.read());

        if (c == '\n' || c == '\r')
        {
            if (command.length() > 0)
            {
                command.trim();
                Serial.print("Bluetooth command: ");
                Serial.println(command);
            }
        }
        else
        {
            command += c;

            // Prevent a malformed client from filling RAM indefinitely.
            if (command.length() > 128)
            {
                command = "";
            }
        }
    }

    const unsigned long now = millis();

    // Report every 30 seconds.
    if (now - lastTemperatureReport >= TEMPERATURE_INTERVAL)
    {
        lastTemperatureReport = now;

        if (SerialBT.hasClient())
        {
            sendTemperatureReport();
        }
        else
        {
            Serial.printf(
                "Bluetooth not connected | CPU: %lu MHz | HEAP: %lu\n",
                getCpuFrequencyMhz(),
                ESP.getFreeHeap()
            );
        }
    }
}
