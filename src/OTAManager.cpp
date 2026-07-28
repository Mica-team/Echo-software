#include "OTAManager.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define CURRENT_VERSION "1.0.0"

const char* VERSION_URL =
"https://raw.githubusercontent.com/YOUR_USERNAME/Echo/main/echo-update/version.json";

unsigned long lastCheck = 0;

void otaSetup()
{
    Serial.println("OTA Ready");
}

void otaLoop()
{
    if (millis() - lastCheck < 60000)
        return;

    lastCheck = millis();

    if (WiFi.status() != WL_CONNECTED)
        return;

    HTTPClient http;

    http.begin(VERSION_URL);

    int code = http.GET();

    if (code != HTTP_CODE_OK)
    {
        Serial.println("OTA Check Failed");
        http.end();
        return;
    }

    String payload = http.getString();

    http.end();

    JsonDocument doc;

    DeserializationError err = deserializeJson(doc, payload);

    if (err)
    {
        Serial.println("Bad version.json");
        return;
    }

    String latest = doc["version"];

    if (latest != CURRENT_VERSION)
    {
        Serial.println("New Version Available");
        Serial.println(latest);

        // Download firmware later
    }
    else
    {
        Serial.println("Already Latest");
    }
}
