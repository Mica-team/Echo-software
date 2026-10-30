#include "echo_bluetooth.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
#include "echo_servo.h"
#include "echo_oled.h"
#include "echo_wifi.h"

static const char *TAG = "echo_bt";
static uint32_t s_spp_handle;
static char s_command[160];
static size_t s_command_len;

static void reply(const char *message)
{
    if (s_spp_handle) esp_spp_write(s_spp_handle, (int)strlen(message), (uint8_t *)message);
    ESP_LOGI(TAG, "%s", message);
}

static void execute_command(void)
{
    s_command[s_command_len] = '\0';
    if (strcmp(s_command, "IDENTIFY") == 0) reply("ECHO_ID:Echo-IDF\r\n");
    else if (strcmp(s_command, "VERSION") == 0) reply("VERSION:2.0.0-idf\r\n");
    else if (strcmp(s_command, "LEFT") == 0) { echo_servo_set_angle(40); reply("OK\r\n"); }
    else if (strcmp(s_command, "RIGHT") == 0) { echo_servo_set_angle(140); reply("OK\r\n"); }
    else if (strcmp(s_command, "CENTER") == 0) { echo_servo_set_angle(90); reply("OK\r\n"); }
    else if (strcmp(s_command, "HAPPY") == 0) { echo_oled_set_face(ECHO_FACE_HAPPY); reply("OK\r\n"); }
    else if (strcmp(s_command, "IDLE") == 0) { echo_oled_set_face(ECHO_FACE_IDLE); reply("OK\r\n"); }
    else if (strcmp(s_command, "SLEEP") == 0) { echo_oled_set_face(ECHO_FACE_SLEEP); reply("OK\r\n"); }
    else if (strcmp(s_command, "WIFI_DISCONNECT") == 0) { echo_wifi_disconnect(); reply("WIFI:DISCONNECTED\r\n"); }
    else if (strncmp(s_command, "WIFI_CONNECT=", 13) == 0) {
        char *separator = strchr(s_command + 13, '|');
        if (!separator) reply("ERR:FORMAT WIFI_CONNECT=ssid|password\r\n");
        else {
            *separator = '\0';
            esp_err_t err = echo_wifi_connect(s_command + 13, separator + 1);
            reply(err == ESP_OK ? "WIFI:CONNECTING\r\n" : "ERR:WIFI\r\n");
        }
    } else reply("ERR:UNKNOWN_COMMAND\r\n");
    s_command_len = 0;
}

static void spp_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event) {
    case ESP_SPP_INIT_EVT:
        if (param->init.status == ESP_SPP_SUCCESS) {
            esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, "Echo-SPP");
        }
        break;
    case ESP_SPP_START_EVT:
        ESP_LOGI(TAG, "Bluetooth SPP server started");
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        s_spp_handle = param->srv_open.handle;
        reply("ECHO_READY:ESP-IDF\r\n");
        break;
    case ESP_SPP_CLOSE_EVT:
        s_spp_handle = 0;
        s_command_len = 0;
        break;
    case ESP_SPP_DATA_IND_EVT:
        for (int i = 0; i < param->data_ind.len; ++i) {
            char c = (char)param->data_ind.data[i];
            if (c == '\n' || c == '\r') {
                if (s_command_len) execute_command();
            } else if (s_command_len < sizeof(s_command) - 1) {
                s_command[s_command_len++] = c;
            } else {
                s_command_len = 0;
                reply("ERR:COMMAND_TOO_LONG\r\n");
            }
        }
        break;
    default:
        break;
    }
}

esp_err_t echo_bluetooth_init(void)
{
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_bt_controller_init(&bt_cfg);
    if (err != ESP_OK) return err;
    err = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (err != ESP_OK) return err;
    err = esp_bluedroid_init();
    if (err != ESP_OK) return err;
    err = esp_bluedroid_enable();
    if (err != ESP_OK) return err;
    err = esp_bt_gap_set_device_name("Echo-IDF");
    if (err != ESP_OK) return err;
    err = esp_spp_register_callback(spp_callback);
    if (err != ESP_OK) return err;
    return esp_spp_init(ESP_SPP_MODE_CB);
}
