#include "OTAManager.h"

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>

#include "BluetoothManager.h"
#include "ServoManager.h"
#include "FaceManager.h"

// Keep these numbers synchronized with echo-update/version.json.
#define CURRENT_VERSION "1.0.1"
#define CURRENT_BUILD 2

static const char* VERSION_URL =
    "https://raw.githubusercontent.com/Mica-team/Echo-software/main/echo-update/version.json";

static const char* FIRMWARE_BASE_URL =
    "https://raw.githubusercontent.com/Mica-team/Echo-software/main/echo-update/";

static const unsigned long OTA_WIFI_TIMEOUT = 20000UL;

static bool updateInProgress = false;
static bool wifiConnectionRequested = false;
static bool wifiStatusReported = false;

static Preferences preferences;

static String wifiSSID;
static String wifiPassword;

static void loadWiFiCredentials()
{
    preferences.begin("echo-ota", true);
    wifiSSID = preferences.getString("ssid", "");
    wifiPassword = preferences.getString("pass", "");
    preferences.end();
}

static void saveWiFiCredentials(const String& ssid, const String& password)
{
    preferences.begin("echo-ota", false);
    preferences.putString("ssid", ssid);
    preferences.putString("pass", password);
    preferences.end();

    wifiSSID = ssid;
    wifiPassword = password;
}

static void requestWiFiConnection()
{
    if (wifiSSID.length() == 0)
        return;

    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_STA);
    wifiConnectionRequested = true;
    wifiStatusReported = false;

    Serial.println("OTA: Connecting to configured Wi-Fi once");
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
}

static void handleBluetoothWiFiCommand()
{
    if (command.startsWith("WIFI_SSID="))
    {
        String ssid = command.substring(10);
        ssid.trim();

        if (ssid.length() > 0)
        {
            saveWiFiCredentials(ssid, wifiPassword);
            Serial.println("OTA: Wi-Fi SSID saved");
        }

        command = "";
    }
    else if (command.startsWith("WIFI_PASS="))
    {
        String password = command.substring(10);
        password.trim();

        if (wifiSSID.length() > 0)
        {
            saveWiFiCredentials(wifiSSID, password);
            Serial.println("OTA: Wi-Fi password saved");
            requestWiFiConnection();
        }
        else
        {
            Serial.println("OTA: Cannot save password before SSID");
        }

        command = "";
    }
    else if (command == "WIFI_CLEAR")
    {
        preferences.begin("echo-ota", false);
        preferences.clear();
        preferences.end();

        wifiSSID = "";
        wifiPassword = "";
        wifiConnectionRequested = false;
        wifiStatusReported = false;
        WiFi.disconnect(true, true);
        Serial.println("OTA: Wi-Fi credentials cleared");
        command = "";
    }
}

static void reportWiFiStatus()
{
    if (!wifiConnectionRequested || wifiStatusReported)
        return;

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("OTA: Wi-Fi connected: ");
        Serial.println(WiFi.SSID());
        SerialBT.print("WIFI_CONNECTED\n");
        wifiStatusReported = true;
        return;
    }

    // Do not repeatedly call WiFi.begin(). The Wi-Fi stack handles the
    // connection attempt itself; retry storms were contributing to heat.
}

static bool waitForWiFi()
{
    if (wifiSSID.length() == 0)
    {
        Serial.println("OTA: No Wi-Fi credentials configured");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.disconnect(true, true);
        WiFi.mode(WIFI_STA);
        wifiConnectionRequested = true;
        wifiStatusReported = false;
        Serial.println("OTA: Starting Wi-Fi connection for manual update");
        WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    }

    const unsigned long started = millis();

    while (millis() - started < OTA_WIFI_TIMEOUT)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("OTA: Wi-Fi connected for manual update");
            return true;
        }

        delay(100);
    }

    Serial.println("OTA: Wi-Fi connection timeout");
    return false;
}

static bool isNewerBuild(int latestBuild)
{
    return latestBuild > CURRENT_BUILD;
}

static bool downloadAndFlash(const String& firmwareFile, const String& expectedSHA)
{
    if (firmwareFile.length() == 0 || expectedSHA.length() != 64)
    {
        Serial.println("OTA: Invalid firmware metadata");
        return false;
    }

    String url = String(FIRMWARE_BASE_URL) + firmwareFile;

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (!http.begin(url))
    {
        Serial.println("OTA: HTTP begin failed");
        return false;
    }

    http.setTimeout(15000);
    int code = http.GET();

    if (code != HTTP_CODE_OK)
    {
        Serial.print("OTA: Firmware download failed: ");
        Serial.println(code);
        http.end();
        return false;
    }

    int contentLength = http.getSize();

    if (contentLength <= 0)
    {
        Serial.println("OTA: Invalid firmware size");
        http.end();
        return false;
    }

    if (!Update.begin((size_t)contentLength))
    {
        Serial.print("OTA: Not enough space: ");
        Serial.println(Update.errorString());
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[1024];
    size_t written = 0;

    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts_ret(&sha, 0);

    unsigned long lastData = millis();

    while (http.connected() && written < (size_t)contentLength)
    {
        size_t available = stream->available();

        if (available > 0)
        {
            size_t toRead = available;
            if (toRead > sizeof(buffer))
                toRead = sizeof(buffer);

            int readBytes = stream->readBytes(buffer, toRead);

            if (readBytes <= 0)
                break;

            size_t updateWritten = Update.write(buffer, readBytes);

            if (updateWritten != (size_t)readBytes)
            {
                Serial.println("OTA: Flash write failed");
                Update.abort();
                mbedtls_sha256_free(&sha);
                http.end();
                return false;
            }

            mbedtls_sha256_update_ret(&sha, buffer, readBytes);
            written += readBytes;
            lastData = millis();

            int progress = (int)((written * 100ULL) / contentLength);
            Serial.printf("OTA: Download %d%%\n", progress);
        }
        else
        {
            if (millis() - lastData > 15000)
            {
                Serial.println("OTA: Download timeout");
                Update.abort();
                mbedtls_sha256_free(&sha);
                http.end();
                return false;
            }
            delay(10);
        }
    }

    uint8_t digest[32];
    mbedtls_sha256_finish_ret(&sha, digest);
    mbedtls_sha256_free(&sha);
    http.end();

    if (written != (size_t)contentLength)
    {
        Serial.println("OTA: Incomplete firmware");
        Update.abort();
        return false;
    }

    char actualSHA[65];
    for (int i = 0; i < 32; i++)
        sprintf(actualSHA + (i * 2), "%02x", digest[i]);
    actualSHA[64] = '\0';

    String expected = expectedSHA;
    expected.toLowerCase();

    if (expected != String(actualSHA))
    {
        Serial.println("OTA: SHA-256 verification FAILED");
        Update.abort();
        return false;
    }

    if (!Update.end(true))
    {
        Serial.print("OTA: Finalize failed: ");
        Serial.println(Update.errorString());
        return false;
    }

    if (!Update.isFinished())
    {
        Serial.println("OTA: Update not finished");
        return false;
    }

    Serial.println("OTA: Firmware verified and installed");
    return true;
}

// OTA is deliberately manual. The ESP32 never polls version.json by itself.
// The Android app must send OTA_UPDATE after the user taps Update Software.
static void handleManualUpdate()
{
    command = "";
    updateInProgress = true;

    Serial.println("OTA: Manual update requested");

    // Connect to Wi-Fi only because the user explicitly requested an update.
    if (!waitForWiFi())
    {
        Serial.println("OTA_WIFI_NOT_CONNECTED");
        updateInProgress = false;
        return;
    }

    // First read metadata while Bluetooth is still available so the app gets
    // a useful result even when there is no newer firmware.
    HTTPClient http;

    if (!http.begin(VERSION_URL))
    {
        Serial.println("OTA: Version URL failed");
        SerialBT.print("OTA_ERROR:VERSION_URL\n");
        updateInProgress = false;
        return;
    }

    http.setTimeout(10000);
    int code = http.GET();

    if (code != HTTP_CODE_OK)
    {
        Serial.print("OTA: Version check failed: ");
        Serial.println(code);
        SerialBT.printf("OTA_ERROR:VERSION_HTTP_%d\n", code);
        http.end();
        updateInProgress = false;
        return;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (err)
    {
        Serial.println("OTA: Bad version.json");
        SerialBT.print("OTA_ERROR:BAD_METADATA\n");
        updateInProgress = false;
        return;
    }

    const char* latestVersion = doc["version"] | "";
    int latestBuild = doc["build"] | 0;
    const char* firmwareFile = doc["firmware"] | "firmware.bin";
    const char* expectedSHA = doc["sha256"] | "";
    const char* channel = doc["channel"] | "stable";

    Serial.printf(
        "OTA: Current %s (%d), Latest %s (%d), channel %s\n",
        CURRENT_VERSION,
        CURRENT_BUILD,
        latestVersion,
        latestBuild,
        channel
    );

    if (!isNewerBuild(latestBuild))
    {
        SerialBT.printf("OTA_UP_TO_DATE:%s:%d\n", CURRENT_VERSION, CURRENT_BUILD);
        Serial.println("OTA: Already latest");
        updateInProgress = false;
        return;
    }

    if (latestVersion[0] == '\0' || expectedSHA[0] == '\0')
    {
        Serial.println("OTA: Missing update metadata");
        SerialBT.print("OTA_ERROR:MISSING_METADATA\n");
        updateInProgress = false;
        return;
    }

    // Enter the exclusive OTA state only when an update is actually needed.
    // Bluetooth, servo PWM, and normal command/face work are stopped so the
    // radio/CPU can focus on the firmware transfer.
    SerialBT.printf("OTA_AVAILABLE:%s:%d\n", latestVersion, latestBuild);
    delay(100);
    bluetoothStop();
    servoStop();
    sleepFace();

    Serial.println("OTA: Entering exclusive update mode");

    if (downloadAndFlash(String(firmwareFile), String(expectedSHA)))
    {
        Serial.println("OTA: Update complete. Restarting...");
        delay(1000);
        ESP.restart();
    }

    // If flashing failed, restore Bluetooth so the app can report/retry.
    Serial.println("OTA: Update failed. Restoring Bluetooth.");
    bluetoothSetup();
    updateInProgress = false;
}

void otaSetup()
{
    loadWiFiCredentials();

    wifiConnectionRequested = false;
    wifiStatusReported = false;

    if (wifiSSID.length() == 0)
    {
        Serial.println("OTA Ready - configure Wi-Fi over Bluetooth");
        Serial.println("Commands: WIFI_SSID=... / WIFI_PASS=...");
    }
    else
    {
        Serial.println("OTA Ready - automatic Wi-Fi/OTA polling disabled");
    }
}

void otaLoop()
{
    if (updateInProgress)
        return;

    handleBluetoothWiFiCommand();
    reportWiFiStatus();

    if (command == "OTA_UPDATE")
    {
        handleManualUpdate();
        return;
    }

    // No WiFi.begin() retry loop and no GitHub polling here.
    // This keeps the ESP32 cool while it is being used normally.
}

bool otaIsInProgress()
{
    return updateInProgress;
}
