#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "echo_bluetooth.h"
#include "echo_servo.h"
#include "echo_oled.h"
#include "echo_wifi.h"

static const char *TAG = "echo";

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(err);
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(echo_oled_init());
    echo_oled_set_face(ECHO_FACE_IDLE);
    ESP_ERROR_CHECK(echo_servo_init());
    ESP_ERROR_CHECK(echo_wifi_init());
    ESP_ERROR_CHECK(echo_bluetooth_init());

    ESP_LOGI(TAG, "Echo native ESP-IDF firmware started");
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());

    for (;;) {
        ESP_LOGI(TAG, "Diagnostics: free_heap=%lu bytes",
                 (unsigned long)esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
