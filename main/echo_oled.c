#include "echo_oled.h"
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

#define OLED_SCLK 18
#define OLED_MOSI 23
#define OLED_DC    2
#define OLED_RST   4
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_BYTES (OLED_WIDTH * OLED_HEIGHT / 8)

static spi_device_handle_t s_spi;
static uint8_t s_fb[OLED_BYTES];

static esp_err_t oled_transfer(bool data, const uint8_t *bytes, size_t len)
{
    gpio_set_level(OLED_DC, data ? 1 : 0);
    spi_transaction_t t = { .length = len * 8, .tx_buffer = bytes };
    return spi_device_transmit(s_spi, &t);
}

static void oled_cmd(uint8_t cmd) { (void)oled_transfer(false, &cmd, 1); }

static void pixel(int x, int y, bool on)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    const unsigned index = (unsigned)x + ((unsigned)y / 8U) * OLED_WIDTH;
    const uint8_t mask = (uint8_t)(1U << (y & 7));
    if (on) s_fb[index] |= mask;
    else s_fb[index] &= (uint8_t)~mask;
}

static void line(int x0, int y0, int x1, int y1)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = -(y1 > y0 ? y1 - y0 : y0 - y1);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        pixel(x0, y0, true);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void circle(int cx, int cy, int r)
{
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        pixel(cx+x,cy+y,true); pixel(cx+y,cy+x,true);
        pixel(cx-y,cy+x,true); pixel(cx-x,cy+y,true);
        pixel(cx-x,cy-y,true); pixel(cx-y,cy-x,true);
        pixel(cx+y,cy-x,true); pixel(cx+x,cy-y,true);
        ++y;
        if (err < 0) err += 2*y + 1;
        else { --x; err += 2*(y-x) + 1; }
    }
}

static void flush(void)
{
    const uint8_t address[] = {0x21, 0, 127, 0x22, 0, 7};
    (void)oled_transfer(false, address, sizeof(address));
    (void)oled_transfer(true, s_fb, sizeof(s_fb));
}

esp_err_t echo_oled_init(void)
{
    gpio_config_t outputs = {
        .pin_bit_mask = (1ULL << OLED_DC) | (1ULL << OLED_RST),
        .mode = GPIO_MODE_OUTPUT
    };
    ESP_RETURN_ON_ERROR(gpio_config(&outputs), "echo_oled", "GPIO config failed");

    spi_bus_config_t bus = {
        .mosi_io_num = OLED_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = OLED_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = OLED_BYTES
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO),
                        "echo_oled", "SPI bus init failed");

    spi_device_interface_config_t dev = {
        .clock_speed_hz = 4 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1
    };
    ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI2_HOST, &dev, &s_spi),
                        "echo_oled", "SPI device init failed");

    gpio_set_level(OLED_RST, 0);
    esp_rom_delay_us(10000);
    gpio_set_level(OLED_RST, 1);
    esp_rom_delay_us(10000);

    const uint8_t init[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0x7F, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6,
        0xAF
    };
    for (size_t i = 0; i < sizeof(init); ++i) oled_cmd(init[i]);
    memset(s_fb, 0, sizeof(s_fb));
    flush();
    return ESP_OK;
}

void echo_oled_set_face(echo_face_t face)
{
    memset(s_fb, 0, sizeof(s_fb));
    if (face == ECHO_FACE_SLEEP) {
        line(32, 25, 46, 25);
        line(82, 25, 96, 25);
    } else {
        circle(40, 25, 6);
        circle(88, 25, 6);
        if (face == ECHO_FACE_HAPPY) {
            /* Simple smiling curve. */
            line(49, 42, 56, 48);
            line(56, 48, 72, 48);
            line(72, 48, 79, 42);
        }
    }
    flush();
}
