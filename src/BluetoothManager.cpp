#include <Arduino.h>
#include <BluetoothSerial.h>
#include "BluetoothManager.h"

// ============================================================
// Firmware version
// Keep these numbers synchronized with OTA version.json.
// ============================================================
#define FIRMWARE_VERSION "1.0.1"
#define FIRMWARE_BUILD 2

BluetoothSerial SerialBT;
String command = "";

static unsigned long lastTemperatureReport = 0;
static const unsigned long TEMPERATURE_INTERVAL = 30000UL;
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
    delay(2000);

    echoBluetoothName = buildEchoBluetoothName();

    if (!SerialBT.begin(echoBluetoothName.c_str()))
    {
        Serial.println("Bluetooth FAILED!");
        while (true)
            delay(1000);
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

// ============================================================
// Identify this device to the Echo Android app.
// The app must verify this response after connecting before it
// treats the Bluetooth connection as an Echo connection.
// ============================================================
static void sendIdentityReport()
{
    const uint32_t uniqueId = static_cast<uint32_t>(ESP.getEfuseMac() & 0xFFFFFFULL);

    char identity[64];
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

// ============================================================
// Send real firmware version/build
// ============================================================
static void sendVersionReport()
{
    SerialBT.printf(
        "VERSION:%s\n",
        FIRMWARE_VERSION
    );

    SerialBT.printf(
        "BUILD:%d\n",
        FIRMWARE_BUILD
    );

    Serial.printf(
        "VERSION: %s | BUILD: %d\n",
        FIRMWARE_VERSION,
        FIRMWARE_BUILD
    );
}

// ============================================================
// Temperature + diagnostic telemetry
// ============================================================
static void sendTemperatureReport()
{
    const float temperature = temperatureRead();
    const unsigned long freeHeap = ESP.getFreeHeap();
    const unsigned long cpuMHz = getCpuFrequencyMhz();
    const char* btStatus =
        SerialBT.hasClient() ? "CONNECTED" : "WAITING";

    SerialBT.printf(
        "TEMP:%.2f\n",
        temperature
    );

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
    // --------------------------------------------------------
    // Non-blocking Bluetooth command reader
    // --------------------------------------------------------
    while (SerialBT.available())
    {
        const char c =
            static_cast<char>(SerialBT.read());

        if (c == '\n' || c == '\r')
        {
            if (command.length() > 0)
            {
                command.trim();

                Serial.print(
                    "Bluetooth command: "
                );
                Serial.println(command);
            }
        }
        else
        {
            command += c;

            // Prevent malformed input from filling RAM.
            if (command.length() > 128)
            {
                command = "";
            }
        }
    }

    // --------------------------------------------------------
    // Handle identity handshake immediately
    // --------------------------------------------------------
    if (command == "IDENTIFY")
    {
        sendIdentityReport();
        command = "";
    }

    // --------------------------------------------------------
    // Handle VERSION command immediately
    // --------------------------------------------------------
    if (command == "VERSION")
    {
        sendVersionReport();
        command = "";
    }

    // --------------------------------------------------------
    // Temperature every 30 seconds
    // --------------------------------------------------------
    const unsigned long now = millis();

    if (
        now - lastTemperatureReport >=
        TEMPERATURE_INTERVAL
    )
    {
        lastTemperatureReport = now;

        if (SerialBT.hasClient())
        {
            sendTemperatureReport();
        }
        else
        {
            Serial.printf(
                "Bluetooth not connected | "
                "CPU: %lu MHz | HEAP: %lu\n",
                getCpuFrequencyMhz(),
                ESP.getFreeHeap()
            );
        }
    }
}
