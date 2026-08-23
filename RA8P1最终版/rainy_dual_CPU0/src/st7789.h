/*
 * ST7789V2 SPI Display Driver
 */
#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>

#define ST7789_WIDTH   128
#define ST7789_HEIGHT  160

void st7789_init(void);
void st7789_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void st7789_fill_color(uint16_t color);
void st7789_draw_colorbar(void);
void st7789_backlight(uint8_t on);
void st7789_display_rgb565(const uint8_t *buf, int src_w, int src_h);
void st7789_display_rgb565_qvga_center(const uint8_t *buf);
void st7789_display_yuyv(const uint8_t *buf, int src_w, int src_h);

#endif
