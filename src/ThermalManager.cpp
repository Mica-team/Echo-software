#include "ThermalManager.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>

#include "FaceManager.h"
#include "ServoManager.h"
#include "OTAManager.h"

namespace
{
constexpr float THERMAL_WARNING_C = 70.0f;
constexpr float THERMAL_THROTTLE_C = 80.0f;
constexpr float THERMAL_CRITICAL_C = 85.0f;
constexpr float THERMAL_RECOVERY_C = 65.0f;

constexpr uint32_t TEMP_CHECK_INTERVAL_MS = 1000;
constexpr uint32_t CRITICAL_SLEEP_US = 60ULL * 1000000ULL;

unsigned long lastThermalCheck = 0;
bool throttled = false;
bool emergency = false;

void enterThermalThrottle(float temperature)
{
    if (throttled)
        return;

    throttled = true;

    // Reduce CPU frequency to the lowest normal ESP32 setting.
    setCpuFrequencyMhz(80);

    // Wi-Fi is a major heat/power source and OTA is not safe while hot.
    otaThermalShutdown();
    WiFi.mode(WIFI_OFF);

    servoStop();

    Serial.printf(
        "THERMAL: throttle active at %.2f C | CPU=%u MHz | Wi-Fi OFF\n",
        temperature,
        getCpuFrequencyMhz()
    );
}

void enterCriticalShutdown(float temperature)
{
    if (emergency)
        return;

    emergency = true;

    Serial.printf(
        "THERMAL: CRITICAL %.2f C - shutting down for 60 seconds\n",
        temperature
    );

    // Put the OLED into its emergency state before sleeping.
    thermalFace();

    // Stop hardware that can add heat.
    servoStop();

    // Disable Wi-Fi and prevent OTA from reconnecting.
    otaThermalShutdown();

    // NOTE: microphone/speaker drivers are not present in this firmware yet.
    // Their thermal-safe stop/siren hooks should be connected when audio
    // hardware support is added.

    delay(50);
    esp_sleep_enable_timer_wakeup(CRITICAL_SLEEP_US);
    esp_deep_sleep_start();
}

} // namespace

void thermalSetup()
{
    lastThermalCheck = millis();
    Serial.printf("THERMAL: protection ready | critical=%.1f C\n",
                  THERMAL_CRITICAL_C);
}

bool thermalLoop()
{
    const unsigned long now = millis();

    if (now - lastThermalCheck < TEMP_CHECK_INTERVAL_MS)
        return false;

    lastThermalCheck = now;

    const float temperature = temperatureRead();

    if (temperature >= THERMAL_CRITICAL_C)
    {
        enterCriticalShutdown(temperature);
        return true;
    }

    if (temperature >= THERMAL_THROTTLE_C)
    {
        enterThermalThrottle(temperature);
        return false;
    }

    if (temperature >= THERMAL_WARNING_C)
    {
        Serial.printf("THERMAL: warning %.2f C\n", temperature);
    }

    if (throttled && temperature <= THERMAL_RECOVERY_C)
    {
        setCpuFrequencyMhz(240);
        throttled = false;
        Serial.printf("THERMAL: recovered %.2f C | CPU=%u MHz\n",
                      temperature,
                      getCpuFrequencyMhz());
    }

    return false;
}

bool thermalEmergencyActive()
{
    return emergency;
}
