#pragma once 

#include "common.h"
#include <driver/i2c.h>

#define OLED_I2C_PORT I2C_NUM_0

typedef enum
{
    SSD1306_COLOR_TRANSPARENT = -1, //!< Transparent (not drawing)
    SSD1306_COLOR_BLACK = 0,        //!< Black (pixel off)
    SSD1306_COLOR_WHITE = 1,        //!< White (or blue, yellow, pixel on)
    SSD1306_COLOR_INVERT = 2,       //!< Invert pixel (XOR)
} ssd1306_color_t;

void ssd1306_init();
void ssd1306_term();
void ssd1306_clear();
void ssd1306_refresh(bool force);
void ssd1306_draw_pixel(int8_t x, int8_t y, ssd1306_color_t color);
void ssd1306_draw_hline(int8_t x, int8_t y, uint8_t w, ssd1306_color_t color);
void ssd1306_draw_vline(int8_t x, int8_t y, uint8_t h, ssd1306_color_t color);
void ssd1306_draw_rectangle(int8_t x, int8_t y, uint8_t w, uint8_t h, ssd1306_color_t color);
void ssd1306_fill_rectangle(int8_t x, int8_t y, uint8_t w, uint8_t h, ssd1306_color_t color);
void ssd1306_draw_circle(int8_t x0, int8_t y0, uint8_t r, ssd1306_color_t color);
void ssd1306_fill_circle(int8_t x0, int8_t y0, uint8_t r, ssd1306_color_t color);
void ssd1306_select_font(uint8_t idx);
uint8_t ssd1306_draw_char(uint8_t x, uint8_t y, char c, ssd1306_color_t foreground, ssd1306_color_t background, bool reversed);
uint8_t ssd1306_draw_string(uint8_t x, uint8_t y, char *str, ssd1306_color_t foreground, ssd1306_color_t background, bool reversed);
uint8_t ssd1306_measure_string(char *str);
uint8_t ssd1306_get_font_height();
uint8_t ssd1306_get_font_c();
void ssd1306_invert_display(bool invert);
void ssd1306_update_buffer(uint8_t* data, uint16_t length);
