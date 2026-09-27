#include "ThermalManager.h"

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp_sleep.h>

#include "FaceManager.h"
#include "ServoManager.h"
#include "OTAManager.h"

namespace
{
constexpr float THERMAL_WARNING_C = 76.0f;
constexpr float THERMAL_THROTTLE_C = 80.0f;
constexpr float THERMAL_CRITICAL_C = 85.0f;
constexpr float THERMAL_RECOVERY_C = 65.0f;

constexpr uint32_t TEMP_CHECK_INTERVAL_MS = 1000;
constexpr uint32_t CRITICAL_SLEEP_US = 60ULL * 1000000ULL;

constexpr uint8_t POWER_CYCLE_OVERRIDE_COUNT = 3;
constexpr char THERMAL_NVS_NAMESPACE[] = "thermal";
constexpr char THERMAL_LATCH_KEY[] = "shutdown";
constexpr char THERMAL_CYCLES_KEY[] = "cycles";

unsigned long lastThermalCheck = 0;
bool throttled = false;
bool emergency = false;
bool overrideBoot = false;

Preferences thermalPrefs;

void prepareThermalPowerCycleState()
{
    thermalPrefs.begin(THERMAL_NVS_NAMESPACE, false);

    const bool shutdownLatched =
        thermalPrefs.getBool(THERMAL_LATCH_KEY, false);

    if (!shutdownLatched)
        return;

    uint8_t cycles = thermalPrefs.getUChar(THERMAL_CYCLES_KEY, 0);

    if (cycles < POWER_CYCLE_OVERRIDE_COUNT)
        ++cycles;

    if (cycles >= POWER_CYCLE_OVERRIDE_COUNT)
    {
        thermalPrefs.putBool(THERMAL_LATCH_KEY, false);
        thermalPrefs.putUChar(THERMAL_CYCLES_KEY, 0);
        overrideBoot = true;

        Serial.println(
            "THERMAL: 3 power cycles detected - shutdown override cleared"
        );
    }
    else
    {
        thermalPrefs.putUChar(THERMAL_CYCLES_KEY, cycles);

        Serial.printf(
            "THERMAL: shutdown latch active | power cycle %u/%u\n",
            cycles,
            POWER_CYCLE_OVERRIDE_COUNT
        );
    }
}

void enterThermalThrottle(float temperature)
{
    if (throttled)
        return;

    throttled = true;

    setCpuFrequencyMhz(80);

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

    thermalPrefs.putBool(THERMAL_LATCH_KEY, true);
    thermalPrefs.putUChar(THERMAL_CYCLES_KEY, 0);

    Serial.printf(
        "THERMAL: CRITICAL %.2f C - shutting down for 60 seconds\n",
        temperature
    );

    thermalFace();
    servoStop();
    otaThermalShutdown();

    // NOTE: microphone/speaker drivers are not present in this firmware yet.
    // Their thermal-safe stop/siren hooks should be connected when audio
    // hardware support is added.

    thermalPrefs.end();

    delay(50);
    esp_sleep_enable_timer_wakeup(CRITICAL_SLEEP_US);
    esp_deep_sleep_start();
}

} // namespace

void thermalSetup()
{
    prepareThermalPowerCycleState();

    lastThermalCheck = millis();
    Serial.printf(
        "THERMAL: protection ready | warning=%.1f C | throttle=%.1f C | critical=%.1f C | power-cycle override=%s\n",
        THERMAL_WARNING_C,
        THERMAL_THROTTLE_C,
        THERMAL_CRITICAL_C,
        overrideBoot ? "ACTIVE" : "READY"
    );
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
        Serial.printf(
            "THERMAL: recovered %.2f C | CPU=%u MHz\n",
            temperature,
            getCpuFrequencyMhz()
        );
    }

    return false;
}

bool thermalEmergencyActive()
{
    return emergency;
}
