#include <Arduino.h>
#include <BluetoothSerial.h>
#include "BluetoothManager.h"

// ============================================================
// Firmware version
// Keep these numbers synchronized with OTA version.json.
// ============================================================
#define FIRMWARE_VERSION "1.0.1"
#define FIRMWARE_BUILD 2

static constexpr size_t COMMAND_BUFFER_SIZE = 128;

BluetoothSerial SerialBT;
String command = "";

static unsigned long lastTemperatureReport = 0;
static constexpr unsigned long TEMPERATURE_INTERVAL = 30000UL;
static String echoBluetoothName;

static String buildEchoBluetoothName()
{
    const uint32_t uniqueId = static_cast<uint32_t>(ESP.getEfuseMac() & 0xFFFFFFULL);
    char suffix[7];
    snprintf(suffix, sizeof(suffix), "%06lX", static_cast<unsigned long>(uniqueId));
    return String("Echo-") + String(suffix);
}

void bluetoothSetup()
{
    echoBluetoothName = buildEchoBluetoothName();

    command.reserve(COMMAND_BUFFER_SIZE);
    echoBluetoothName.reserve(20);

    if (!SerialBT.begin(echoBluetoothName.c_str()))
    {
        Serial.println("Bluetooth FAILED!");
        return;
    }

    Serial.println("Bluetooth READY");
    Serial.print("Bluetooth name: ");
    Serial.println(echoBluetoothName);

    Serial.printf(
        "Firmware: %s | Build: %d\n",
        FIRMWARE_VERSION,
        FIRMWARE_BUILD
    );

    lastTemperatureReport = millis();
}

static void sendIdentityReport()
{
    const uint32_t uniqueId = static_cast<uint32_t>(ESP.getEfuseMac() & 0xFFFFFFULL);

    char identity[32];
    snprintf(
        identity,
        sizeof(identity),
        "ECHO_ID:Echo-%06lX\n",
        static_cast<unsigned long>(uniqueId)
    );

    SerialBT.print(identity);
    Serial.print("Identity: ");
    Serial.print(identity);
}

static void sendVersionReport()
{
    SerialBT.printf("VERSION:%s\n", FIRMWARE_VERSION);
    SerialBT.printf("BUILD:%d\n", FIRMWARE_BUILD);

    Serial.printf(
        "VERSION: %s | BUILD: %d\n",
        FIRMWARE_VERSION,
        FIRMWARE_BUILD
    );
}

static void sendTemperatureReport()
{
    const float temperature = temperatureRead();
    const unsigned long freeHeap = ESP.getFreeHeap();
    const unsigned long cpuMHz = getCpuFrequencyMhz();
    const char* btStatus =
        SerialBT.hasClient() ? "CONNECTED" : "WAITING";

    SerialBT.printf("TEMP:%.2f\n", temperature);
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
        else if (command.length() < COMMAND_BUFFER_SIZE - 1)
        {
            command += c;
        }
        else
        {
            // Drop an oversized command instead of repeatedly
            // allocating and copying the String buffer.
            command = "";
            command.reserve(COMMAND_BUFFER_SIZE);
        }
    }

    if (command == "IDENTIFY")
    {
        sendIdentityReport();
        command = "";
    }
    else if (command == "VERSION")
    {
        sendVersionReport();
        command = "";
    }

    const unsigned long now = millis();

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
