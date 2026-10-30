#pragma once
#include "esp_err.h"

esp_err_t echo_wifi_init(void);
esp_err_t echo_wifi_connect(const char *ssid, const char *password);
void echo_wifi_disconnect(void);
