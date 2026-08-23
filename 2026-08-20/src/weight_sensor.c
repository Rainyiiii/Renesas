#include "weight_sensor.h"
#include <limits.h>

/* 保留原 STM32F4 应用所使用的校准系数。 */
#define WEIGHT_SCALE_NUMERATOR   (4L)
#define WEIGHT_SCALE_DENOMINATOR (1L)

static volatile uint8_t g_rx_byte;
static volatile uint8_t g_frame[4];
static volatile uint8_t g_frame_index;
static volatile int16_t g_weight_grams;
static volatile uint8_t g_has_value;

/* 调试诊断信息（参见 weight_sensor.h）。 */
static volatile uint32_t g_rx_bytes;
static volatile uint32_t g_frame_count;
static volatile int16_t  g_last_raw;

static void receive_next(void)
{
    (void) g_weight_uart.p_api->read(g_weight_uart.p_ctrl, (uint8_t *) &g_rx_byte, 1U);
}

static void parse_byte(uint8_t byte)
{
    /* 统计 UART 交给我们的每一个字节，包括重新同步期间被丢弃的字节。
     * rx_bytes 持续增长即可证明物理链路和 baud 率均正确。 */
    g_rx_bytes++;

    if ((0U == g_frame_index) && (0xFFU != byte))
    {
        return;
    }
    g_frame[g_frame_index++] = byte;
    if (g_frame_index < 4U)
    {
        return;
    }

    if ((0xFFU == g_frame[0]) && (0xFEU == g_frame[3]))
    {
        int16_t raw = (int16_t) (((uint16_t) g_frame[1] << 8U) | g_frame[2]);
        int32_t grams = ((int32_t) raw * WEIGHT_SCALE_NUMERATOR) / WEIGHT_SCALE_DENOMINATOR;
        if (grams > INT16_MAX)
        {
            grams = INT16_MAX;
        }
        else if (grams < INT16_MIN)
        {
            grams = INT16_MIN;
        }
        g_weight_grams = (int16_t) grams;
        g_last_raw = raw;
        g_frame_count++;
        g_has_value = 1U;
    }
    g_frame_index = (0xFFU == byte) ? 1U : 0U;
    if (1U == g_frame_index)
    {
        g_frame[0] = byte;
    }
}

void weight_uart_callback(uart_callback_args_t *p_args)
{
    if (UART_EVENT_RX_COMPLETE == p_args->event)
    {
        parse_byte(g_rx_byte);
        receive_next();
    }
    else if (p_args->event & (UART_EVENT_ERR_PARITY |
                              UART_EVENT_ERR_FRAMING |
                              UART_EVENT_ERR_OVERFLOW |
                              UART_EVENT_BREAK_DETECT))
    {
        (void) g_weight_uart.p_api->communicationAbort(g_weight_uart.p_ctrl, UART_DIR_RX);
        g_frame_index = 0U;
        receive_next();
    }
}

fsp_err_t weight_sensor_init(void)
{
    fsp_err_t err = g_weight_uart.p_api->open(g_weight_uart.p_ctrl, g_weight_uart.p_cfg);
    if (FSP_SUCCESS == err)
    {
        receive_next();
    }
    return err;
}

bool weight_sensor_has_value(void)
{
    return (bool) g_has_value;
}

int16_t weight_sensor_get_grams(void)
{
    return g_weight_grams;
}

uint32_t weight_sensor_get_rx_bytes(void)
{
    return g_rx_bytes;
}

uint32_t weight_sensor_get_frame_count(void)
{
    return g_frame_count;
}

int16_t weight_sensor_get_last_raw(void)
{
    return g_last_raw;
}
