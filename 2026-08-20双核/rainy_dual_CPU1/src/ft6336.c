#include "hal_data.h"
#include "ft6336.h"
#include <stddef.h>

#define TOUCH_SDA_PORT       5U
#define TOUCH_SDA_PIN        11U
#define TOUCH_SCL_PORT       5U
#define TOUCH_SCL_PIN        12U
#define TOUCH_RST_PIN        BSP_IO_PORT_01_PIN_05

#define FT6336_ADDRESS       0x38U
#define FT6336_REG_TD_STATUS 0x02U
#define FT6336_REG_CHIP_ID   0xA3U
#define I2C_WAIT_LOOPS       4000000UL

#define PIDR(port) (*(volatile uint16_t *) ((uint32_t) R_PORT0_BASE + (port) * 0x20UL + 0x04UL))

static volatile i2c_master_event_t g_i2c_event = I2C_MASTER_EVENT_ABORTED;
static volatile uint8_t g_i2c_done = 0U;
static uint8_t g_i2c_open = 0U;
static uint8_t g_ft6336_status = FT6336_STATUS_NO_ACK;
static uint8_t g_ft6336_chip_id = 0U;
static uint8_t g_ft6336_bus_levels = 0U;

static void delay_ms(uint32_t ms)
{
    R_BSP_SoftwareDelay(ms, BSP_DELAY_UNITS_MILLISECONDS);
}

void ft6336_i2c_callback(i2c_master_callback_args_t *p_args)
{
    if (NULL != p_args) {
        g_i2c_event = p_args->event;
        if ((I2C_MASTER_EVENT_TX_COMPLETE == p_args->event) ||
            (I2C_MASTER_EVENT_RX_COMPLETE == p_args->event) ||
            (I2C_MASTER_EVENT_ABORTED == p_args->event)) {
            g_i2c_done = 1U;
        }
    }
}

static uint8_t i2c_wait(i2c_master_event_t expected)
{
    uint32_t timeout = I2C_WAIT_LOOPS;
    while ((!g_i2c_done) && (timeout > 0U)) {
        timeout--;
    }

    if ((0U == timeout) || (expected != g_i2c_event)) {
        (void) g_i2c_master0.p_api->abort(g_i2c_master0.p_ctrl);
        return 0U;
    }
    return 1U;
}

static uint8_t i2c_write(const uint8_t *p_data, uint32_t length, bool restart)
{
    g_i2c_done = 0U;
    g_i2c_event = I2C_MASTER_EVENT_ABORTED;
    if (FSP_SUCCESS != g_i2c_master0.p_api->write(g_i2c_master0.p_ctrl,
                                                  (uint8_t *) p_data,
                                                  length,
                                                  restart)) {
        return 0U;
    }
    return i2c_wait(I2C_MASTER_EVENT_TX_COMPLETE);
}

static uint8_t i2c_read(uint8_t *p_data, uint32_t length)
{
    g_i2c_done = 0U;
    g_i2c_event = I2C_MASTER_EVENT_ABORTED;
    if (FSP_SUCCESS != g_i2c_master0.p_api->read(g_i2c_master0.p_ctrl,
                                                 p_data,
                                                 length,
                                                 false)) {
        return 0U;
    }
    return i2c_wait(I2C_MASTER_EVENT_RX_COMPLETE);
}

static uint8_t ft6336_read_regs(uint8_t reg, uint8_t *p_data, uint8_t length)
{
    if ((NULL == p_data) || (0U == length) || (!g_i2c_open)) {
        return 0U;
    }

    if (!i2c_write(&reg, 1U, true)) {
        return 0U;
    }
    return i2c_read(p_data, length);
}

uint8_t ft6336_init(void)
{
    uint8_t chip_id = 0U;

    g_ft6336_status = FT6336_STATUS_NO_ACK;
    g_ft6336_chip_id = 0U;
    g_ft6336_bus_levels = 0U;

    if (g_i2c_open) {
        (void) g_i2c_master0.p_api->close(g_i2c_master0.p_ctrl);
        g_i2c_open = 0U;
    }

    (void) R_IOPORT_PinWrite(&g_ioport_ctrl, TOUCH_RST_PIN, BSP_IO_LEVEL_LOW);
    delay_ms(20U);
    (void) R_IOPORT_PinWrite(&g_ioport_ctrl, TOUCH_RST_PIN, BSP_IO_LEVEL_HIGH);
    delay_ms(300U);

    uint16_t sda_mask = (uint16_t) (1U << TOUCH_SDA_PIN);
    uint16_t scl_mask = (uint16_t) (1U << TOUCH_SCL_PIN);
    g_ft6336_bus_levels = (uint8_t) (((PIDR(TOUCH_SDA_PORT) & sda_mask) ? 1U : 0U) |
                                     ((PIDR(TOUCH_SCL_PORT) & scl_mask) ? 2U : 0U));
    if (3U != g_ft6336_bus_levels) {
        g_ft6336_status = FT6336_STATUS_BUS_STUCK;
        return 0U;
    }

    if (FSP_SUCCESS != g_i2c_master0.p_api->open(g_i2c_master0.p_ctrl, g_i2c_master0.p_cfg)) {
        return 0U;
    }
    g_i2c_open = 1U;

    if (!ft6336_read_regs(FT6336_REG_CHIP_ID, &chip_id, 1U)) {
        g_ft6336_status = FT6336_STATUS_NO_ACK;
        return 0U;
    }

    g_ft6336_chip_id = chip_id;
    if ((0x00U == chip_id) || (0xFFU == chip_id)) {
        g_ft6336_status = FT6336_STATUS_BAD_ID;
        return 0U;
    }

    g_ft6336_status = FT6336_STATUS_OK;
    return 1U;
}

uint8_t ft6336_get_status(void)
{
    return g_ft6336_status;
}

uint8_t ft6336_get_chip_id(void)
{
    return g_ft6336_chip_id;
}

uint8_t ft6336_get_bus_levels(void)
{
    return g_ft6336_bus_levels;
}

uint8_t ft6336_get_address(void)
{
    return FT6336_ADDRESS;
}

uint8_t ft6336_read(ft6336_touch_t *p_touch)
{
    uint8_t packet[5];

    if (NULL == p_touch) {
        return 0U;
    }
    p_touch->points = 0U;

    /* TD_STATUS 与第一个触摸点在地址上是连续的，因此每次轮询用一次事务
     * 读取即可。 */
    if (!ft6336_read_regs(FT6336_REG_TD_STATUS, packet, sizeof(packet))) {
        return 0U;
    }

    uint8_t points = (uint8_t) (packet[0] & 0x0FU);
    if ((0U == points) || (points > 2U)) {
        return 0U;
    }

    /* 事件 1 表示抬起。事件 0（按下）和事件 2（接触）为有效触摸。 */
    if (((packet[1] >> 6U) & 0x03U) == 1U) {
        return 0U;
    }

    uint16_t raw_x = (uint16_t) ((((uint16_t) packet[1] & 0x0FU) << 8U) | packet[2]);
    uint16_t raw_y = (uint16_t) ((((uint16_t) packet[3] & 0x0FU) << 8U) | packet[4]);
    if ((raw_x < 320U) && (raw_y < 480U)) {
        p_touch->x = raw_x;
        p_touch->y = raw_y;
    } else if ((raw_x < 480U) && (raw_y < 320U)) {
        p_touch->x = raw_y;
        p_touch->y = raw_x;
    } else {
        return 0U;
    }

    p_touch->points = points;
    return 1U;
}
