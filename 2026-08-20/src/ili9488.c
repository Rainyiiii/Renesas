/*
 * ILI9488 320x480 IPS SPI 显示屏驱动 - 硬件 SPI (g_spi1)
 * 适用于 EK-RA8P1 + ILI9488 4.0" TFT (320x480 竖屏)
 *
 * 初始化序列来自 ILI9488_IPS_Code.txt (厂商提供，已在本屏上验证可用)。
 */

#include "hal_data.h"
#include "ili9488.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  480
#define DASHBOARD_HEIGHT (112U)
#define DASHBOARD_TEXT_SCALE (2U)
#define DASHBOARD_CHAR_ADVANCE (6U * DASHBOARD_TEXT_SCALE)
#define SPI_WRITE_TIMEOUT_LOOPS  (5000000UL)

/* 控制引脚分配 (SPI SCK/MOSI 为硬件 SPI 引脚)。 */
#define PIN_CS_PORT    4
#define PIN_CS_PIN     4
#define PIN_DC_PORT    4
#define PIN_DC_PIN     10
#define PIN_RST_PORT   4
#define PIN_RST_PIN    9
#define PIN_BLK_PORT   4
#define PIN_BLK_PIN    15

#define PODR_REG(port_num) \
    (*(volatile uint16_t *)((uint32_t)R_PORT0_BASE + (port_num) * 0x20UL + 0x02UL))

#define PFS_PmnPFS(port_num, pin_num) \
    (R_PFS->PORT[(port_num)].PIN[(pin_num)].PmnPFS)

#define PWPR_REG  (R_PMISC->PWPR)

/* 用于显示写入的单行暂存缓冲区 (320 像素 × 2 字节 = 640 字节)。 */
/* ILI9488 4 线 SPI 以 18 位 RGB 形式接收像素数据 (每像素 3 字节)。 */
static uint8_t g_line_buf[DISPLAY_WIDTH * 3];
#define CAMERA_CHUNK_ROWS  (8)
static uint8_t g_camera_chunk[DISPLAY_WIDTH * 3 * CAMERA_CHUNK_ROWS];

/* 由 FSP 回调驱动的 SPI 状态。 */
static volatile spi_event_t g_spi_last_event = SPI_EVENT_TRANSFER_ABORTED;
static volatile bool g_spi_transfer_done = false;
static bool g_spi_opened = false;

void spi_callback(spi_callback_args_t *p_args)
{
    if (0 == p_args) {
        return;
    }

    g_spi_last_event = p_args->event;
    g_spi_transfer_done = true;
}

static void delay_ms_approx(uint32_t ms)
{
    for (volatile uint32_t i = 0; i < ms * 12000UL; i++) {
    }
}

static void pfs_unlock(void)
{
    PWPR_REG = 0x00;
    PWPR_REG = 0x40;
}

static void pfs_lock(void)
{
    PWPR_REG = 0x00;
    PWPR_REG = 0x80;
}

static void gpio_init_output(uint8_t port, uint8_t pin)
{
    pfs_unlock();
    PFS_PmnPFS(port, pin) = 0x00000004UL;
    pfs_lock();
    PODR_REG(port) &= ~(1U << pin);
}

static void gpio_write(uint8_t port, uint8_t pin, uint8_t level)
{
    if (level) {
        PODR_REG(port) |= (1U << pin);
    } else {
        PODR_REG(port) &= ~(1U << pin);
    }
}

static fsp_err_t spi_init_once(void)
{
    if (g_spi_opened) {
        return FSP_SUCCESS;
    }

    fsp_err_t err = g_spi1.p_api->open(g_spi1.p_ctrl, g_spi1.p_cfg);
    if (FSP_SUCCESS == err) {
        g_spi_opened = true;
    }

    return err;
}

static fsp_err_t spi_recover(void)
{
    if (g_spi_opened) {
        (void) g_spi1.p_api->close(g_spi1.p_ctrl);
        g_spi_opened = false;
    }

    return spi_init_once();
}

static fsp_err_t spi_write_bytes(const uint8_t *p_data, uint32_t length)
{
    if ((0 == p_data) || (0U == length)) {
        return FSP_SUCCESS;
    }

    fsp_err_t err = spi_init_once();
    if (FSP_SUCCESS != err) {
        return err;
    }

    g_spi_transfer_done = false;
    g_spi_last_event = SPI_EVENT_TRANSFER_ABORTED;

    err = g_spi1.p_api->write(g_spi1.p_ctrl, p_data, length, SPI_BIT_WIDTH_8_BITS);
    if (FSP_SUCCESS != err) {
        return err;
    }

    uint32_t timeout = SPI_WRITE_TIMEOUT_LOOPS;
    while ((!g_spi_transfer_done) && (timeout-- > 0U)) {
    }

    if (!g_spi_transfer_done) {
        (void) spi_recover();
        return FSP_ERR_TIMEOUT;
    }

    if (SPI_EVENT_TRANSFER_COMPLETE != g_spi_last_event) {
        (void) spi_recover();
        return FSP_ERR_TRANSFER_ABORTED;
    }

    return FSP_SUCCESS;
}

static void wr_cmd(uint8_t cmd)
{
    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 0);
    gpio_write(PIN_DC_PORT, PIN_DC_PIN, 0);
    (void) spi_write_bytes(&cmd, 1);
    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 1);
}

static void wr_data(uint8_t data)
{
    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 0);
    gpio_write(PIN_DC_PORT, PIN_DC_PIN, 1);
    (void) spi_write_bytes(&data, 1);
    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 1);
}

static void rgb565_to_rgb666(uint16_t color, uint8_t *p_dst)
{
    uint8_t red   = (uint8_t)((color >> 11) & 0x1FU);
    uint8_t green = (uint8_t)((color >> 5) & 0x3FU);
    uint8_t blue  = (uint8_t)(color & 0x1FU);

    p_dst[0] = (uint8_t)((red << 3) | (red >> 2));
    p_dst[1] = (uint8_t)(green << 2);
    p_dst[2] = (uint8_t)((blue << 3) | (blue >> 2));
}

void ili9488_backlight(uint8_t on)
{
    gpio_write(PIN_BLK_PORT, PIN_BLK_PIN, on ? 1U : 0U);
}

void ili9488_init(void)
{
    gpio_init_output(PIN_CS_PORT, PIN_CS_PIN);
    gpio_init_output(PIN_DC_PORT, PIN_DC_PIN);
    gpio_init_output(PIN_RST_PORT, PIN_RST_PIN);
    gpio_init_output(PIN_BLK_PORT, PIN_BLK_PIN);

    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 1);
    gpio_write(PIN_DC_PORT, PIN_DC_PIN, 0);
    /* 40002 转接板在 LED 为高电平之前会一直关闭背光。 */
    ili9488_backlight(1);

    (void) spi_init_once();

    /* 硬件复位 */
    gpio_write(PIN_RST_PORT, PIN_RST_PIN, 1);
    delay_ms_approx(10);
    gpio_write(PIN_RST_PORT, PIN_RST_PIN, 0);
    delay_ms_approx(20);
    gpio_write(PIN_RST_PORT, PIN_RST_PIN, 1);
    delay_ms_approx(120);

    /* ============ ILI9488 IPS 初始化序列 ============
     * 适用于 CL40BC299-40A 4.0" IPS 面板，4 线 SPI 模式。
     * 来自 STC32_ILI9488_IPS_4SPI_CTP Demo_V1 (厂商提供)。 */

    /* Adjust Control 3 (调整控制 3) */
    wr_cmd(0xF7);
    wr_data(0xA9); wr_data(0x51); wr_data(0x2C); wr_data(0x82);

    /* 泵比控制 */
    wr_cmd(0xEC);
    wr_data(0x00); wr_data(0x02); wr_data(0x03); wr_data(0x7A);

    /* 电源控制 1 */
    wr_cmd(0xC0);
    wr_data(0x13); wr_data(0x13);

    /* 电源控制 2 */
    wr_cmd(0xC1);
    wr_data(0x41);

    /* VCOM 控制 */
    wr_cmd(0xC5);
    wr_data(0x00); wr_data(0x28); wr_data(0x80);

    wr_cmd(0xB0);
    wr_data(0x00);

    /* 帧率控制 ~70Hz */
    wr_cmd(0xB1);
    wr_data(0xB0); wr_data(0x11);

    /* 显示反转: 2 点反转 */
    wr_cmd(0xB4);
    wr_data(0x02);

    /* RGB/MCU 接口控制 */
    wr_cmd(0xB6);
    wr_data(0x02); wr_data(0x22);

    /* 栅极控制 */
    wr_cmd(0xB7);
    wr_data(0xC6);

    /* 电源/背光控制 */
    wr_cmd(0xBE);
    wr_data(0x00); wr_data(0x04);

    wr_cmd(0xE9);
    wr_data(0x00);

    /* 面板时序 */
    wr_cmd(0xF4);
    wr_data(0x00); wr_data(0x00); wr_data(0x0F);

    /* 正极性 gamma 校正 */
    wr_cmd(0xE0);
    wr_data(0x00); wr_data(0x04); wr_data(0x0E); wr_data(0x08);
    wr_data(0x17); wr_data(0x0A); wr_data(0x40); wr_data(0x79);
    wr_data(0x4D); wr_data(0x07); wr_data(0x0E); wr_data(0x0A);
    wr_data(0x1A); wr_data(0x1D); wr_data(0x0F);

    /* 负极性 gamma 校正 */
    wr_cmd(0xE1);
    wr_data(0x00); wr_data(0x1B); wr_data(0x1F); wr_data(0x02);
    wr_data(0x10); wr_data(0x05); wr_data(0x32); wr_data(0x34);
    wr_data(0x43); wr_data(0x02); wr_data(0x0A); wr_data(0x09);
    wr_data(0x33); wr_data(0x37); wr_data(0x0F);

    /* 面板时序 (按厂商序列重复设置) */
    wr_cmd(0xF4);
    wr_data(0x00); wr_data(0x00); wr_data(0x0F);

    /* 存储器访问控制 (MADCTL): 竖屏, BGR */
    wr_cmd(0x36);
    wr_data(0x08);

    /* 像素格式: 18 位 RGB666, 厂商 4SPI 示例要求使用此格式。 */
    wr_cmd(0x3A);
    wr_data(0x66);

    /* 开启显示反转 */
    wr_cmd(0x21);

    /* 退出睡眠 */
    wr_cmd(0x11);
    delay_ms_approx(120);

    /* 开启显示 */
    wr_cmd(0x29);
    delay_ms_approx(50);

    /* 让居中的 QVGA 摄像头图像周围未使用的行保持黑色。 */
    ili9488_fill_color(0x0000);

}

void ili9488_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    /* 列地址设置 */
    wr_cmd(0x2A);
    wr_data((uint8_t)(x1 >> 8)); wr_data((uint8_t)x1);
    wr_data((uint8_t)(x2 >> 8)); wr_data((uint8_t)x2);

    /* 行地址设置 */
    wr_cmd(0x2B);
    wr_data((uint8_t)(y1 >> 8)); wr_data((uint8_t)y1);
    wr_data((uint8_t)(y2 >> 8)); wr_data((uint8_t)y2);

    /* 存储器写入 */
    wr_cmd(0x2C);
}

void ili9488_fill_color(uint16_t color)
{
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        rgb565_to_rgb666(color, &g_line_buf[3 * x]);
    }

    ili9488_set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 0);
    gpio_write(PIN_DC_PORT, PIN_DC_PIN, 1);

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        if (FSP_SUCCESS != spi_write_bytes(g_line_buf, sizeof(g_line_buf))) {
            break;
        }
    }

    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 1);
}

void ili9488_draw_colorbar(void)
{
    static const uint16_t colors[] =
    {
        0xF800, 0x07E0, 0x001F, 0xFFFF, 0xFFE0, 0x07FF, 0xF81F, 0x0000
    };

    uint8_t n = (uint8_t)(sizeof(colors) / sizeof(colors[0]));
    uint16_t h = DISPLAY_HEIGHT / n;

    for (uint8_t i = 0; i < n; i++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            rgb565_to_rgb666(colors[i], &g_line_buf[3 * x]);
        }

        ili9488_set_window(0, (uint16_t)(i * h), DISPLAY_WIDTH - 1,
                           (uint16_t)((i + 1) * h - 1));
        gpio_write(PIN_CS_PORT, PIN_CS_PIN, 0);
        gpio_write(PIN_DC_PORT, PIN_DC_PIN, 1);

        for (uint16_t y = 0; y < h; y++) {
            if (FSP_SUCCESS != spi_write_bytes(g_line_buf, sizeof(g_line_buf))) {
                break;
            }
        }

        gpio_write(PIN_CS_PORT, PIN_CS_PIN, 1);
    }
}

/*
 * 显示由 QVGA (320x240) 缩放到 320x480 的摄像头帧。
 * 每一源行在垂直方向复制 2 倍 (像素倍增)。
 * 源: OV5640 320x240 QVGA, RGB565, 字节交换 (低字节在前)。
 * 字节交换: buf[pixel*2+1]=高字节, buf[pixel*2+0]=低字节。
 */
void ili9488_display_qvga_2x(const uint8_t *buf)
{
    ili9488_set_window(0, 120, DISPLAY_WIDTH - 1, 359);
    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 0);
    gpio_write(PIN_DC_PORT, PIN_DC_PIN, 1);

    for (int sy = 0; sy < 240; sy += CAMERA_CHUNK_ROWS) {
        for (int row = 0; row < CAMERA_CHUNK_ROWS; row++) {
            const uint8_t *src = buf + (sy + row) * 320 * 2;
            uint8_t *dst = g_camera_chunk + row * DISPLAY_WIDTH * 3;

            for (int x = 0; x < 320; x++) {
                const uint8_t *pixel = src + x * 2;
                uint8_t low = pixel[0];
                uint8_t high = pixel[1];
                uint8_t *out = &dst[3 * x];
                out[0] = (uint8_t)((high & 0xF8U) | (high >> 5));
                out[1] = (uint8_t)(((high & 0x07U) << 5) | ((low & 0xE0U) >> 3));
                out[2] = (uint8_t)(((uint32_t) low << 3) | ((low & 0x1FU) >> 2));
            }
        }

        if (FSP_SUCCESS != spi_write_bytes(g_camera_chunk, sizeof(g_camera_chunk))) {
            gpio_write(PIN_CS_PORT, PIN_CS_PIN, 1);
            return;
        }
    }

    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 1);
}

void ili9488_draw_touch_marker(uint16_t x, uint16_t y, uint16_t color)
{
    const uint16_t radius = 5U;
    uint16_t x0 = (x > radius) ? (uint16_t) (x - radius) : 0U;
    uint16_t y0 = (y > radius) ? (uint16_t) (y - radius) : 0U;
    uint16_t x1 = (x + radius < DISPLAY_WIDTH) ? (uint16_t) (x + radius) : (DISPLAY_WIDTH - 1U);
    uint16_t y1 = (y + radius < DISPLAY_HEIGHT) ? (uint16_t) (y + radius) : (DISPLAY_HEIGHT - 1U);
    uint8_t pixel[3];

    rgb565_to_rgb666(color, pixel);
    for (uint16_t row = y0; row <= y1; row++) {
        uint32_t pixels = (uint32_t) x1 - x0 + 1U;
        for (uint32_t column = 0U; column < pixels; column++) {
            uint8_t *p_dst = &g_line_buf[column * 3U];
            if ((row == y) || ((uint16_t) (x0 + column) == x)) {
                p_dst[0] = pixel[0];
                p_dst[1] = pixel[1];
                p_dst[2] = pixel[2];
            } else {
                p_dst[0] = 0U;
                p_dst[1] = 0U;
                p_dst[2] = 0U;
            }
        }
        ili9488_set_window(x0, row, x1, row);
        gpio_write(PIN_CS_PORT, PIN_CS_PIN, 0U);
        gpio_write(PIN_DC_PORT, PIN_DC_PIN, 1U);
        (void) spi_write_bytes(g_line_buf, pixels * 3U);
        gpio_write(PIN_CS_PORT, PIN_CS_PIN, 1U);
    }
}

void ili9488_fill_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    if ((x0 > x1) || (y0 > y1) || (x1 >= DISPLAY_WIDTH) || (y1 >= DISPLAY_HEIGHT)) {
        return;
    }

    uint32_t width = (uint32_t) x1 - x0 + 1U;
    for (uint32_t x = 0U; x < width; x++) {
        rgb565_to_rgb666(color, &g_line_buf[x * 3U]);
    }

    ili9488_set_window(x0, y0, x1, y1);
    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 0U);
    gpio_write(PIN_DC_PORT, PIN_DC_PIN, 1U);
    for (uint16_t y = y0; y <= y1; y++) {
        if (FSP_SUCCESS != spi_write_bytes(g_line_buf, width * 3U)) {
            break;
        }
    }
    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 1U);
}

static const uint8_t *dashboard_glyph_for(char c)
{
    static const uint8_t blank[5] = {0U, 0U, 0U, 0U, 0U};
    static const uint8_t minus[5] = {0x08U, 0x08U, 0x08U, 0x08U, 0x08U};
    static const uint8_t percent[5] = {0x62U, 0x64U, 0x08U, 0x13U, 0x23U};
    static const uint8_t digits[10][5] =
    {
        {0x3EU,0x51U,0x49U,0x45U,0x3EU}, {0x00U,0x42U,0x7FU,0x40U,0x00U},
        {0x42U,0x61U,0x51U,0x49U,0x46U}, {0x21U,0x41U,0x45U,0x4BU,0x31U},
        {0x18U,0x14U,0x12U,0x7FU,0x10U}, {0x27U,0x45U,0x45U,0x45U,0x39U},
        {0x3CU,0x4AU,0x49U,0x49U,0x30U}, {0x01U,0x71U,0x09U,0x05U,0x03U},
        {0x36U,0x49U,0x49U,0x49U,0x36U}, {0x06U,0x49U,0x49U,0x29U,0x1EU}
    };
    static const uint8_t letters[26][5] =
    {
        {0x7EU,0x11U,0x11U,0x11U,0x7EU}, {0x7FU,0x49U,0x49U,0x49U,0x36U},
        {0x3EU,0x41U,0x41U,0x41U,0x22U}, {0x7FU,0x41U,0x41U,0x22U,0x1CU},
        {0x7FU,0x49U,0x49U,0x49U,0x41U}, {0x7FU,0x09U,0x09U,0x09U,0x01U},
        {0x3EU,0x41U,0x49U,0x49U,0x7AU}, {0x7FU,0x08U,0x08U,0x08U,0x7FU},
        {0x00U,0x41U,0x7FU,0x41U,0x00U}, {0x20U,0x40U,0x41U,0x3FU,0x01U},
        {0x7FU,0x08U,0x14U,0x22U,0x41U}, {0x7FU,0x40U,0x40U,0x40U,0x40U},
        {0x7FU,0x02U,0x0CU,0x02U,0x7FU}, {0x7FU,0x04U,0x08U,0x10U,0x7FU},
        {0x3EU,0x41U,0x41U,0x41U,0x3EU}, {0x7FU,0x09U,0x09U,0x09U,0x06U},
        {0x3EU,0x41U,0x51U,0x21U,0x5EU}, {0x7FU,0x09U,0x19U,0x29U,0x46U},
        {0x26U,0x49U,0x49U,0x49U,0x32U}, {0x01U,0x01U,0x7FU,0x01U,0x01U},
        {0x3FU,0x40U,0x40U,0x40U,0x3FU}, {0x1FU,0x20U,0x40U,0x20U,0x1FU},
        {0x7FU,0x20U,0x18U,0x20U,0x7FU}, {0x63U,0x14U,0x08U,0x14U,0x63U},
        {0x03U,0x04U,0x78U,0x04U,0x03U}, {0x61U,0x51U,0x49U,0x45U,0x43U}
    };

    if ((c >= '0') && (c <= '9'))
    {
        return digits[(uint32_t) (c - '0')];
    }
    if ((c >= 'A') && (c <= 'Z'))
    {
        return letters[(uint32_t) (c - 'A')];
    }
    if ('-' == c)
    {
        return minus;
    }
    if ('%' == c)
    {
        return percent;
    }
    return blank;
}

static void dashboard_append_char(char *text, uint32_t capacity, uint32_t *length, char value)
{
    if ((*length + 1U) < capacity)
    {
        text[(*length)++] = value;
        text[*length] = '\0';
    }
}

static void dashboard_append_text(char *text, uint32_t capacity, uint32_t *length, const char *value)
{
    while ('\0' != *value)
    {
        dashboard_append_char(text, capacity, length, *value++);
    }
}

static void dashboard_append_u32(char *text, uint32_t capacity, uint32_t *length,
                                 uint32_t value, uint32_t maximum)
{
    char digits[10];
    uint32_t digit_count = 0U;
    if (value > maximum)
    {
        value = maximum;
    }
    do
    {
        digits[digit_count++] = (char) ('0' + (value % 10U));
        value /= 10U;
    } while ((value > 0U) && (digit_count < sizeof(digits)));

    while (digit_count > 0U)
    {
        dashboard_append_char(text, capacity, length, digits[--digit_count]);
    }
}

static void dashboard_append_weight(char *text, uint32_t capacity, uint32_t *length,
                                    uint8_t valid, int16_t grams)
{
    if (!valid)
    {
        dashboard_append_text(text, capacity, length, "--");
        return;
    }

    int32_t value = grams;
    if (value < 0)
    {
        dashboard_append_char(text, capacity, length, '-');
        value = -value;
    }
    dashboard_append_u32(text, capacity, length, (uint32_t) value, 32768U);
}

static const char *dashboard_mode_name(uint8_t mode)
{
    static const char * const names[3] = {"RUN", "PAUSE", "STOP"};
    return (mode < 3U) ? names[mode] : "STOP";
}

static const char *dashboard_state_name(uint8_t state)
{
    static const char * const names[9] =
    {
        "IDLE", "WAIT", "POSITION", "EJECT", "CLOSE", "RETURN", "CLEAR", "FAULT",
        "ALIGN"
    };
    return (state < 9U) ? names[state] : "FAULT";
}

static const char *dashboard_class_name(uint8_t class_id)
{
    static const char * const names[4] = {"HARM", "KITCHEN", "OTHER", "RECOVER"};
    return (class_id < 4U) ? names[class_id] : "NONE";
}

static uint16_t dashboard_class_color(uint8_t class_id)
{
    static const uint16_t colors[4] = {0xF800U, 0xFD20U, 0x001FU, 0x07E0U};
    return (class_id < 4U) ? colors[class_id] : 0x8410U;
}

static void dashboard_append_i32(char *text, uint32_t capacity, uint32_t *length,
                                 int32_t value, uint32_t maximum)
{
    if (value < 0)
    {
        dashboard_append_char(text, capacity, length, '-');
        value = -value;
    }
    dashboard_append_u32(text, capacity, length, (uint32_t) value, maximum);
}

static void dashboard_build_lines(const ili9488_dashboard_t *dashboard,
                                  char lines[4][28], uint16_t colors[4])
{
    uint32_t length = 0U;

    if (dashboard->weight_debug)
    {
        /* 全屏重量调试视图。先观察 RX BYTES 增长 (链路 + 波特率
         * 正常)，再观察 FRAMES 增长 (FF..FE 帧同步正常)，然后读取 GRAMS。 */
        dashboard_append_text(lines[0], 28U, &length, "WEIGHT DIAG");
        colors[0] = 0x07FFU;

        length = 0U;
        dashboard_append_text(lines[1], 28U, &length, "GRAMS ");
        if (dashboard->weight_valid)
        {
            dashboard_append_i32(lines[1], 28U, &length, dashboard->weight_grams, 32768U);
        }
        else
        {
            dashboard_append_text(lines[1], 28U, &length, "--");
        }
        colors[1] = dashboard->weight_valid ? 0x07E0U : 0x8410U;

        length = 0U;
        dashboard_append_text(lines[2], 28U, &length, "RAW ");
        dashboard_append_i32(lines[2], 28U, &length, dashboard->weight_raw, 32768U);
        dashboard_append_text(lines[2], 28U, &length, " FR ");
        dashboard_append_u32(lines[2], 28U, &length, dashboard->weight_frame_count, 99999U);
        colors[2] = 0xFFFFU;

        length = 0U;
        dashboard_append_text(lines[3], 28U, &length, "RX ");
        dashboard_append_u32(lines[3], 28U, &length, dashboard->weight_rx_bytes, 999999U);
        dashboard_append_text(lines[3], 28U, &length, " BYTES");
        colors[3] = (dashboard->weight_rx_bytes > 0U) ? 0xFFE0U : 0xF800U;
        return;
    }

    if (dashboard->dataset_mode)
    {
        static const char * const storage_names[6] =
        {
            "SD INIT", "SD READY", "SD NO CARD", "SD CARD ERR", "SD FAT ERR", "SD WRITE ERR"
        };
        const char *storage_name = (dashboard->dataset_status < 6U) ?
                                   storage_names[dashboard->dataset_status] : "SD ERROR";

        dashboard_append_text(lines[0], 28U, &length, "DATA ");
        dashboard_append_text(lines[0], 28U, &length, storage_name);
        colors[0] = (1U == dashboard->dataset_status) ? 0x07E0U :
                    ((0U == dashboard->dataset_status) ? 0xFFE0U : 0xF800U);

        length = 0U;
        dashboard_append_text(lines[1], 28U, &length, "CLASS ");
        dashboard_append_text(lines[1], 28U, &length, dashboard_class_name(dashboard->dataset_class));
        dashboard_append_text(lines[1], 28U, &length, " AUTO ");
        dashboard_append_text(lines[1], 28U, &length, dashboard->dataset_auto ? "ON" : "OFF");
        colors[1] = dashboard_class_color(dashboard->dataset_class);

        length = 0U;
        dashboard_append_text(lines[2], 28U, &length, "SAVED H");
        dashboard_append_u32(lines[2], 28U, &length, dashboard->dataset_saved_count[0], 999U);
        dashboard_append_text(lines[2], 28U, &length, " K");
        dashboard_append_u32(lines[2], 28U, &length, dashboard->dataset_saved_count[1], 999U);
        dashboard_append_text(lines[2], 28U, &length, " O");
        dashboard_append_u32(lines[2], 28U, &length, dashboard->dataset_saved_count[2], 999U);
        dashboard_append_text(lines[2], 28U, &length, " R");
        dashboard_append_u32(lines[2], 28U, &length, dashboard->dataset_saved_count[3], 999U);
        colors[2] = 0xFFFFU;

        uint32_t total = dashboard->dataset_saved_count[0] + dashboard->dataset_saved_count[1] +
                         dashboard->dataset_saved_count[2] + dashboard->dataset_saved_count[3];
        length = 0U;
        if (dashboard->dataset_error_detail)
        {
            dashboard_append_text(lines[3], 28U, &length,
                                  (0U == dashboard->dataset_status) ? "STEP " : "ERROR ");
            dashboard_append_u32(lines[3], 28U, &length, dashboard->dataset_error_detail, 9999U);
            colors[3] = 0xF800U;
        }
        else
        {
            dashboard_append_text(lines[3], 28U, &length, "TOTAL ");
            dashboard_append_u32(lines[3], 28U, &length, total, 9999U);
            dashboard_append_char(lines[3], 28U, &length, ' ');
            dashboard_append_text(lines[3], 28U, &length, dashboard->dataset_busy ? "SAVING" : "READY");
            colors[3] = dashboard->dataset_busy ? 0xFFE0U : 0x07FFU;
        }
        return;
    }

    dashboard_append_text(lines[0], 28U, &length, dashboard_mode_name(dashboard->mode));
    dashboard_append_char(lines[0], 28U, &length, ' ');
    dashboard_append_text(lines[0], 28U, &length, dashboard_state_name(dashboard->sort_state));
    dashboard_append_text(lines[0], 28U, &length, " W");
    dashboard_append_weight(lines[0], 28U, &length, dashboard->weight_valid, dashboard->weight_grams);
    dashboard_append_char(lines[0], 28U, &length, 'G');
    colors[0] = (0U == dashboard->mode) ? 0x07E0U :
                ((1U == dashboard->mode) ? 0xFFE0U : 0xF800U);

    length = 0U;
    if (dashboard->detection_valid && (dashboard->detection_class < 4U))
    {
        dashboard_append_text(lines[1], 28U, &length, dashboard_class_name(dashboard->detection_class));
        dashboard_append_char(lines[1], 28U, &length, ' ');
        dashboard_append_u32(lines[1], 28U, &length, dashboard->confidence_percent, 99U);
        dashboard_append_text(lines[1], 28U, &length, "% X");
        dashboard_append_u32(lines[1], 28U, &length, dashboard->detection_x, 319U);
        dashboard_append_text(lines[1], 28U, &length, " Y");
        dashboard_append_u32(lines[1], 28U, &length, dashboard->detection_y, 239U);
        colors[1] = dashboard_class_color(dashboard->detection_class);
    }
    else
    {
        dashboard_append_text(lines[1], 28U, &length, "DET NONE");
        colors[1] = 0x8410U;
    }

    length = 0U;
    dashboard_append_text(lines[2], 28U, &length, "CNT H");
    dashboard_append_u32(lines[2], 28U, &length, dashboard->sorted_count[0], 999U);
    dashboard_append_text(lines[2], 28U, &length, " K");
    dashboard_append_u32(lines[2], 28U, &length, dashboard->sorted_count[1], 999U);
    dashboard_append_text(lines[2], 28U, &length, " O");
    dashboard_append_u32(lines[2], 28U, &length, dashboard->sorted_count[2], 999U);
    dashboard_append_text(lines[2], 28U, &length, " R");
    dashboard_append_u32(lines[2], 28U, &length, dashboard->sorted_count[3], 999U);
    colors[2] = 0xFFFFU;

    uint32_t total = dashboard->sorted_count[0] + dashboard->sorted_count[1] +
                     dashboard->sorted_count[2] + dashboard->sorted_count[3];
    length = 0U;
    dashboard_append_text(lines[3], 28U, &length, "ACTIVE ");
    dashboard_append_text(lines[3], 28U, &length, dashboard_class_name(dashboard->active_class));
    if (2U == dashboard->sort_state)
    {
        dashboard_append_text(lines[3], 28U, &length, " T");
        dashboard_append_u32(lines[3], 28U, &length, dashboard->state_elapsed_ms, 99999U);
    }
    else
    {
        dashboard_append_text(lines[3], 28U, &length, " TOTAL ");
        dashboard_append_u32(lines[3], 28U, &length, total, 9999U);
    }
    colors[3] = dashboard_class_color(dashboard->active_class);
}

static void dashboard_draw_text_row(uint16_t screen_y, uint16_t text_y, uint16_t text_x,
                                    const char *text, uint16_t color)
{
    if ((screen_y < text_y) ||
        (screen_y >= (uint16_t) (text_y + 7U * DASHBOARD_TEXT_SCALE)))
    {
        return;
    }

    uint32_t glyph_row = ((uint32_t) screen_y - text_y) / DASHBOARD_TEXT_SCALE;
    uint8_t rgb[3];
    rgb565_to_rgb666(color, rgb);

    for (uint32_t character = 0U; '\0' != text[character]; character++)
    {
        uint32_t character_x = text_x + character * DASHBOARD_CHAR_ADVANCE;
        const uint8_t *glyph = dashboard_glyph_for(text[character]);
        for (uint32_t column = 0U; column < 5U; column++)
        {
            if (glyph[column] & (1U << glyph_row))
            {
                uint32_t pixel_x = character_x + column * DASHBOARD_TEXT_SCALE;
                for (uint32_t scale_x = 0U; scale_x < DASHBOARD_TEXT_SCALE; scale_x++)
                {
                    if ((pixel_x + scale_x) < DISPLAY_WIDTH)
                    {
                        uint8_t *pixel = &g_line_buf[(pixel_x + scale_x) * 3U];
                        pixel[0] = rgb[0];
                        pixel[1] = rgb[1];
                        pixel[2] = rgb[2];
                    }
                }
            }
        }
    }
}

void ili9488_draw_dashboard(const ili9488_dashboard_t *dashboard)
{
    if (NULL == dashboard)
    {
        return;
    }

    char lines[4][28] = {{0}};
    uint16_t colors[4];
    static const uint16_t text_y[4] = {7U, 34U, 61U, 88U};
    dashboard_build_lines(dashboard, lines, colors);

    ili9488_set_window(0U, 0U, DISPLAY_WIDTH - 1U, DASHBOARD_HEIGHT - 1U);
    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 0U);
    gpio_write(PIN_DC_PORT, PIN_DC_PIN, 1U);

    for (uint16_t y = 0U; y < DASHBOARD_HEIGHT; y++)
    {
        uint16_t background = ((27U == y) || (54U == y) || (81U == y) || (108U == y)) ?
                              0x3186U : 0x0000U;
        for (uint32_t x = 0U; x < DISPLAY_WIDTH; x++)
        {
            rgb565_to_rgb666(background, &g_line_buf[x * 3U]);
        }

        for (uint32_t row = 0U; row < 4U; row++)
        {
            uint16_t text_x = (0U == row) ? 32U : 8U;
            dashboard_draw_text_row(y, text_y[row], text_x, lines[row], colors[row]);
        }

        if ((y >= 7U) && (y <= 22U))
        {
            uint8_t indicator[3];
            uint8_t touch_indicator[3];
            uint16_t service_color = dashboard->service_core_online ?
                                     (dashboard->service_peer_alive ? 0x07E0U : 0xFFE0U) :
                                     0xF800U;
            rgb565_to_rgb666(service_color, indicator);
            rgb565_to_rgb666(dashboard->touch_ready ? 0x07FFU : 0xF800U, touch_indicator);
            for (uint32_t x = 8U; x <= 23U; x++)
            {
                uint8_t *pixel = &g_line_buf[x * 3U];
                uint8_t *selected = indicator;
                if (!dashboard->service_multicore || (x >= 16U))
                {
                    selected = dashboard->service_multicore ? touch_indicator :
                               (dashboard->touch_ready ? touch_indicator : indicator);
                }
                pixel[0] = selected[0];
                pixel[1] = selected[1];
                pixel[2] = selected[2];
            }
        }

        if (FSP_SUCCESS != spi_write_bytes(g_line_buf, sizeof(g_line_buf)))
        {
            break;
        }
    }
    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 1U);
}

static uint8_t control_icon_pixel(uint8_t button, uint16_t x, uint16_t y)
{
    if (0U == button) {
        /* 播放三角形。 */
        uint16_t distance = (y > 35U) ? (uint16_t) (y - 35U) : (uint16_t) (35U - y);
        return (uint8_t) ((distance <= 18U) && (x >= 25U) &&
                          (x <= (uint16_t) (49U - distance / 2U)));
    }
    if (1U == button) {
        return (uint8_t) ((y >= 18U) && (y <= 52U) &&
                          (((x >= 20U) && (x <= 28U)) || ((x >= 43U) && (x <= 51U))));
    }
    if (2U == button) {
        return (uint8_t) ((x >= 21U) && (x <= 51U) && (y >= 20U) && (y <= 50U));
    }

    /* 摄像头: 机身、顶部握把和镜头环。 */
    int32_t dx = (int32_t) x - 36;
    int32_t dy = (int32_t) y - 37;
    uint8_t body = (uint8_t) ((x >= 15U) && (x <= 57U) && (y >= 25U) && (y <= 51U));
    uint8_t grip = (uint8_t) ((x >= 23U) && (x <= 34U) && (y >= 20U) && (y <= 25U));
    uint8_t lens = (uint8_t) (((dx * dx + dy * dy) >= 49) && ((dx * dx + dy * dy) <= 121));
    return (uint8_t) ((body || grip) && !((dx * dx + dy * dy) < 49)) | lens;
}

static void draw_control_button(uint8_t button, uint8_t active)
{
    static const uint16_t x0_table[4] = {5U, 84U, 163U, 242U};
    static const uint16_t colors[4] = {0x04E0U, 0xD500U, 0xA800U, 0x045FU};
    const uint16_t x0 = x0_table[button];
    const uint16_t x1 = (uint16_t) (x0 + 72U);
    const uint16_t y0 = 392U;
    const uint16_t y1 = 463U;

    ili9488_set_window(x0, y0, x1, y1);
    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 0U);
    gpio_write(PIN_DC_PORT, PIN_DC_PIN, 1U);

    for (uint16_t y = 0U; y <= y1 - y0; y++) {
        for (uint16_t x = 0U; x <= x1 - x0; x++) {
            uint8_t border = (uint8_t) ((x < 3U) || (x > 69U) || (y < 3U) || (y > 68U));
            uint16_t color = colors[button];
            if (border) {
                color = active ? 0xFFFFU : 0x4208U;
            } else if (control_icon_pixel(button, x, y)) {
                color = 0xFFFFU;
            }
            rgb565_to_rgb666(color, &g_line_buf[x * 3U]);
        }
        if (FSP_SUCCESS != spi_write_bytes(g_line_buf, (uint32_t) (x1 - x0 + 1U) * 3U)) {
            break;
        }
    }
    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 1U);
}

void ili9488_draw_controls(uint8_t active_button, uint8_t touch_ready)
{
    /* 绿色表示 FT6336 已响应; 红色表示需检查触摸接线。 */
    ili9488_fill_rect(8U, 8U, 23U, 23U, touch_ready ? 0x07E0U : 0xF800U);
    ili9488_fill_rect(0U, 376U, DISPLAY_WIDTH - 1U, DISPLAY_HEIGHT - 1U, 0x0000U);
    for (uint8_t button = 0U; button < 4U; button++) {
        draw_control_button(button, (uint8_t) (button == active_button));
    }
}

static uint8_t dataset_letter_pixel(uint8_t class_id, uint16_t x, uint16_t y)
{
    static const char class_letters[4] = {'H', 'K', 'O', 'R'};
    if ((class_id >= 4U) || (x < 24U) || (x >= 49U) || (y < 18U) || (y >= 53U))
    {
        return 0U;
    }
    const uint8_t *glyph = dashboard_glyph_for(class_letters[class_id]);
    uint32_t column = ((uint32_t) x - 24U) / 5U;
    uint32_t row = ((uint32_t) y - 18U) / 5U;
    return (uint8_t) ((glyph[column] & (1U << row)) != 0U);
}

static uint8_t dataset_icon_pixel(uint8_t button, uint8_t class_id, uint16_t x, uint16_t y)
{
    if (0U == button)
    {
        return dataset_letter_pixel(class_id, x, y);
    }
    if (1U == button)
    {
        int32_t dx = (int32_t) x - 36;
        int32_t dy = (int32_t) y - 37;
        uint8_t body = (uint8_t) ((x >= 15U) && (x <= 57U) && (y >= 25U) && (y <= 51U));
        uint8_t grip = (uint8_t) ((x >= 23U) && (x <= 34U) && (y >= 20U) && (y <= 25U));
        uint8_t lens_hole = (uint8_t) ((dx * dx + dy * dy) < 49);
        return (uint8_t) ((body || grip) && !lens_hole);
    }
    if (2U == button)
    {
        int32_t dx = (int32_t) x - 36;
        int32_t dy = (int32_t) y - 35;
        return (uint8_t) ((dx * dx + dy * dy) <= 225);
    }

    /* 返回箭头。 */
    if ((y >= 31U) && (y <= 39U) && (x >= 18U) && (x <= 55U))
    {
        return 1U;
    }
    int32_t distance = (int32_t) y - 35;
    if (distance < 0)
    {
        distance = -distance;
    }
    return (uint8_t) ((distance <= 18) && (x >= (uint16_t) (17 + distance)) &&
                      (x <= (uint16_t) (25 + distance)));
}

static void draw_dataset_button(uint8_t button, uint8_t selected_class, uint8_t auto_capture)
{
    static const uint16_t x0_table[4] = {5U, 84U, 163U, 242U};
    static const uint16_t class_colors[4] = {0xA800U, 0xD500U, 0x0018U, 0x04E0U};
    const uint16_t x0 = x0_table[button];
    const uint16_t x1 = (uint16_t) (x0 + 72U);
    const uint16_t y0 = 392U;
    const uint16_t y1 = 463U;
    uint16_t fill = (0U == button) ? class_colors[selected_class & 3U] :
                    ((1U == button) ? 0x045FU :
                     ((2U == button) ? (auto_capture ? 0x04E0U : 0x3186U) : 0x528AU));

    ili9488_set_window(x0, y0, x1, y1);
    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 0U);
    gpio_write(PIN_DC_PORT, PIN_DC_PIN, 1U);
    for (uint16_t y = 0U; y <= y1 - y0; y++)
    {
        for (uint16_t x = 0U; x <= x1 - x0; x++)
        {
            uint8_t border = (uint8_t) ((x < 3U) || (x > 69U) || (y < 3U) || (y > 68U));
            uint16_t color = border ? ((2U == button && auto_capture) ? 0xFFFFU : 0x8410U) : fill;
            if (!border && dataset_icon_pixel(button, selected_class, x, y))
            {
                color = 0xFFFFU;
            }
            rgb565_to_rgb666(color, &g_line_buf[x * 3U]);
        }
        if (FSP_SUCCESS != spi_write_bytes(g_line_buf, (uint32_t) (x1 - x0 + 1U) * 3U))
        {
            break;
        }
    }
    gpio_write(PIN_CS_PORT, PIN_CS_PIN, 1U);
}

void ili9488_draw_dataset_controls(uint8_t selected_class, uint8_t auto_capture, uint8_t touch_ready)
{
    ili9488_fill_rect(8U, 8U, 23U, 23U, touch_ready ? 0x07E0U : 0xF800U);
    ili9488_fill_rect(0U, 376U, DISPLAY_WIDTH - 1U, DISPLAY_HEIGHT - 1U, 0x0000U);
    for (uint8_t button = 0U; button < 4U; button++)
    {
        draw_dataset_button(button, selected_class, auto_capture);
    }
}
