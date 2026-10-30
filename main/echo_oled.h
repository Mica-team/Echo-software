#pragma once
#include "esp_err.h"

typedef enum {
    ECHO_FACE_IDLE,
    ECHO_FACE_HAPPY,
    ECHO_FACE_SLEEP
} echo_face_t;

esp_err_t echo_oled_init(void);
void echo_oled_set_face(echo_face_t face);
