#pragma once
#include "esp_err.h"

esp_err_t echo_servo_init(void);
void echo_servo_set_angle(unsigned angle);
