#include "OTAManager.h"

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>

// OTA is intentionally isolated in this file. Other Echo managers do not need
// to know about Wi-Fi, firmware downloads, or update storage.
#define CURRENT_VERSION "1.0.0"
#define CURRENT_BUILD 1

static const char* VERSION_URL =
    "https://raw.githubusercontent.com/Mica-team/Echo-software/main/echo-update/version.json";

static const char* FIRMWARE_BASE_URL =
    "https://raw.githubusercontent.com/Mica-team/Echo-software/main/echo-update/";

static const unsigned long WIFI_RETRY_INTERVAL = 10000UL;
static const unsigned long OTA_CHECK_INTERVAL = 300000UL; // 5 minutes

static unsigned long lastWiFiAttempt = 0;
static unsigned long lastOTACheck = 0;
static bool updateInProgress = false;
static bool updateFound = false;

static Preferences preferences;

// BluetoothManager.cpp owns this command buffer. OTA only reads it so we can
// provision Wi-Fi without changing the existing Bluetooth manager.
extern String command;

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
            WiFi.disconnect(true, true);
            lastWiFiAttempt = 0;
        }

        command = "";
    }
    else if (command.startsWith("WIFI_PASS="))
    {
        String password = command.substring(10);
        password.trim();

        saveWiFiCredentials(wifiSSID, password);
        Serial.println("OTA: Wi-Fi password saved");
        WiFi.disconnect(true, true);
        lastWiFiAttempt = 0;
        command = "";
    }
    else if (command == "WIFI_CLEAR")
    {
        preferences.begin("echo-ota", false);
        preferences.clear();
        preferences.end();

        wifiSSID = "";
        wifiPassword = "";
        WiFi.disconnect(true, true);
        Serial.println("OTA: Wi-Fi credentials cleared");
        command = "";
    }
}

static void connectWiFi()
{
    if (wifiSSID.length() == 0)
        return;

    if (WiFi.status() == WL_CONNECTED)
        return;

    if (millis() - lastWiFiAttempt < WIFI_RETRY_INTERVAL)
        return;

    lastWiFiAttempt = millis();

    Serial.println("OTA: Connecting Wi-Fi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
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

static void checkForUpdate()
{
    if (updateInProgress || WiFi.status() != WL_CONNECTED)
        return;

    if (millis() - lastOTACheck < OTA_CHECK_INTERVAL)
        return;

    lastOTACheck = millis();

    HTTPClient http;

    if (!http.begin(VERSION_URL))
    {
        Serial.println("OTA: Version URL failed");
        return;
    }

    http.setTimeout(10000);
    int code = http.GET();

    if (code != HTTP_CODE_OK)
    {
        Serial.print("OTA: Version check failed: ");
        Serial.println(code);
        http.end();
        return;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (err)
    {
        Serial.println("OTA: Bad version.json");
        return;
    }

    const char* latestVersion = doc["version"] | "";
    int latestBuild = doc["build"] | 0;
    const char* firmwareFile = doc["firmware"] | "firmware.bin";
    const char* expectedSHA = doc["sha256"] | "";
    const char* channel = doc["channel"] | "stable";

    Serial.printf("OTA: Current %s (%d), Latest %s (%d), channel %s\n",
                  CURRENT_VERSION,
                  CURRENT_BUILD,
                  latestVersion,
                  latestBuild,
                  channel);

    if (!isNewerBuild(latestBuild))
    {
        Serial.println("OTA: Already latest");
        return;
    }

    if (latestVersion[0] == '\0' || expectedSHA[0] == '\0')
    {
        Serial.println("OTA: Missing update metadata");
        return;
    }

    Serial.printf("OTA: New firmware %s available\n", latestVersion);

    updateInProgress = true;
    updateFound = true;

    if (downloadAndFlash(String(firmwareFile), String(expectedSHA)))
    {
        Serial.println("OTA: Update complete. Restarting...");
        delay(1000);
        ESP.restart();
    }
    else
    {
        Serial.println("OTA: Update failed. Keeping current firmware.");
    }

    updateInProgress = false;
}

void otaSetup()
{
    loadWiFiCredentials();

    if (wifiSSID.length() == 0)
    {
        Serial.println("OTA Ready - configure Wi-Fi over Bluetooth");
        Serial.println("Commands: WIFI_SSID=... / WIFI_PASS=...");
    }
    else
    {
        Serial.println("OTA Ready");
    }

    connectWiFi();
}

void otaLoop()
{
    handleBluetoothWiFiCommand();
    connectWiFi();
    checkForUpdate();
}
