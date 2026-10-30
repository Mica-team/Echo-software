#include "echo_wifi.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

static const char *TAG = "echo_wifi";
static bool s_initialized;

esp_err_t echo_wifi_init(void)
{
    if (s_initialized) return ESP_OK;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&cfg);
    if (err != ESP_OK) return err;
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) return err;
    err = esp_wifi_start();
    if (err != ESP_OK) return err;
    s_initialized = true;
    ESP_LOGI(TAG, "Wi-Fi station ready; credentials are supplied at runtime");
    return ESP_OK;
}

esp_err_t echo_wifi_connect(const char *ssid, const char *password)
{
    if (!s_initialized || !ssid || !password || strlen(ssid) == 0) return ESP_ERR_INVALID_ARG;
    wifi_config_t config = {0};
    strlcpy((char *)config.sta.ssid, ssid, sizeof(config.sta.ssid));
    strlcpy((char *)config.sta.password, password, sizeof(config.sta.password));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_config(WIFI_IF_STA, &config));
    return esp_wifi_connect();
}

void echo_wifi_disconnect(void)
{
    if (s_initialized) (void)esp_wifi_disconnect();
}
