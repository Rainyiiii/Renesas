/*
 * ILI9488 320x480 IPS SPI 显示屏驱动
 */
#ifndef ILI9488_H
#define ILI9488_H

#include <stdint.h>

#define ILI9488_WIDTH   320
#define ILI9488_HEIGHT  480

typedef struct st_ili9488_dashboard
{
    uint8_t  touch_ready;
    uint8_t  mode;
    uint8_t  sort_state;
    uint8_t  active_class;
    uint8_t  detection_valid;
    uint8_t  detection_class;
    uint8_t  confidence_percent;
    uint8_t  weight_valid;
    uint32_t state_elapsed_ms;
    uint16_t detection_x;
    uint16_t detection_y;
    int16_t  weight_grams;
    uint32_t sorted_count[4];
    uint8_t  dataset_mode;
    uint8_t  dataset_status;
    uint8_t  dataset_class;
    uint8_t  dataset_auto;
    uint8_t  dataset_busy;
    uint16_t dataset_error_detail;
    uint32_t dataset_saved_count[4];
    /* 当 weight_debug 被置位时，整个仪表盘显示重量调试
     * 视图（原始值、校准后的克数、字节/帧计数器），而不是
     * 正常的机器状态。 */
    uint8_t  weight_debug;
    uint32_t weight_rx_bytes;
    uint32_t weight_frame_count;
    int16_t  weight_raw;
} ili9488_dashboard_t;

void ili9488_init(void);
void ili9488_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void ili9488_fill_color(uint16_t color);
void ili9488_draw_colorbar(void);
void ili9488_backlight(uint8_t on);
void ili9488_display_qvga_2x(const uint8_t *buf);
void ili9488_draw_touch_marker(uint16_t x, uint16_t y, uint16_t color);
void ili9488_fill_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void ili9488_draw_controls(uint8_t active_button, uint8_t touch_ready);
void ili9488_draw_dataset_controls(uint8_t selected_class, uint8_t auto_capture, uint8_t touch_ready);
void ili9488_draw_dashboard(const ili9488_dashboard_t *dashboard);

#endif
