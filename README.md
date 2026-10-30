# Echo Firmware — ESP-IDF

Native ESP-IDF firmware for the original ESP32-WROOM-32 / ESP32 Dev Module (4 MB flash).

## Build

Install Espressif ESP-IDF (5.4 or newer), activate its environment, then:

```sh
idf.py set-target esp32
idf.py build
idf.py -p YOUR_SERIAL_PORT flash monitor
```

This is a native ESP-IDF/CMake project. Arduino, PlatformIO, and Arduino libraries are not used.

## Current hardware mapping

| Function | GPIO / interface |
|---|---|
| Servo signal | GPIO 13 (LEDC, 50 Hz) |
| SSD1306 OLED | SPI2, SCLK 18, MOSI 23, DC 2, RESET 4, no CS |
| Bluetooth | Bluetooth Classic SPP |
| Wi-Fi | Station mode, credentials supplied at runtime |

Confirm the OLED wiring and servo power supply before connecting hardware. Power the servo from a suitable external supply and share ground with the ESP32; do not power a servo from a GPIO pin.

## Bluetooth command interface

Commands are newline-terminated UTF-8 text:

- `IDENTIFY`
- `VERSION`
- `LEFT`, `RIGHT`, `CENTER`
- `HAPPY`, `IDLE`, `SLEEP`
- `WIFI_SSID=<ssid>`, `WIFI_PASS=<password>`, `WIFI_CONNECT`, `WIFI_DISCONNECT`

The protocol is intentionally being redesigned; the companion app is not treated as a compatibility constraint.

## Migration status

This is the first native ESP-IDF foundation. Bluetooth SPP, servo PWM, and basic SSD1306 face rendering are implemented as ESP-IDF components. HTTPS OTA and verified thermal protection are not enabled yet.

**Thermal safety:** the original ESP32-WROOM-32 does not provide a universally supported calibrated board-temperature measurement through the ESP-IDF temperature-sensor driver. Do not claim an 80 °C throttle / 85 °C shutdown until a supported external temperature sensor and its wiring/calibration are added. The firmware reports heap and connection diagnostics in the meantime.

## Target

- SoC: ESP32 (classic, dual-core)
- Board: ESP32 Dev Module / ESP32-WROOM-32
- Framework: ESP-IDF
- Language: C
- RTOS: FreeRTOS (included with ESP-IDF)
