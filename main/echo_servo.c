#include "echo_servo.h"
#include "driver/ledc.h"
#include "esp_check.h"

#define SERVO_GPIO       13
#define SERVO_TIMER      LEDC_TIMER_0
#define SERVO_CHANNEL    LEDC_CHANNEL_0
#define SERVO_FREQ_HZ    50
#define SERVO_RESOLUTION LEDC_TIMER_16_BIT

esp_err_t echo_servo_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = SERVO_RESOLUTION,
        .timer_num = SERVO_TIMER,
        .freq_hz = SERVO_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer), "echo_servo", "LEDC timer init failed");

    ledc_channel_config_t channel = {
        .gpio_num = SERVO_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = SERVO_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = SERVO_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel), "echo_servo", "LEDC channel init failed");
    echo_servo_set_angle(90);
    return ESP_OK;
}

void echo_servo_set_angle(unsigned angle)
{
    if (angle > 180) angle = 180;

    /* Typical servo pulse range: 500–2500 us at 20 ms period. */
    const uint32_t pulse_us = 500U + (2000U * angle) / 180U;
    const uint32_t max_duty = (1U << 16) - 1U;
    const uint32_t duty = (pulse_us * max_duty) / 20000U;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL);
}
