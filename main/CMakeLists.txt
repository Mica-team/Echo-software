idf_component_register(
    SRCS
        "echo_main.c"
        "echo_bluetooth.c"
        "echo_servo.c"
        "echo_oled.c"
        "echo_wifi.c"
    INCLUDE_DIRS "."
    REQUIRES
        bt
        driver
        esp_event
        esp_netif
        esp_wifi
        nvs_flash
        esp_timer
)
