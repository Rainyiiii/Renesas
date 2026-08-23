#include "m33_services.h"
#include "multicore_protocol.h"
#include "ft6336.h"
#include "weight_sensor.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define M33_TOUCH_PERIOD_MS       (10U)
#define M33_TOUCH_REFRESH_MS      (100U)
#define M33_HEALTH_PERIOD_MS      (1000U)
#define M33_M85_TIMEOUT_MS        (3000U)

static volatile uint32_t g_last_m85_heartbeat_cycles;
static volatile uint8_t g_m85_heartbeat_seen;
static uint32_t g_cycles_per_ms;
static uint16_t g_tx_drops;
static uint8_t g_health_sequence;
static uint8_t g_touch_ready;
static uint8_t g_weight_ready;

static uint32_t m33_elapsed_ms(uint32_t start_cycles)
{
    return (DWT->CYCCNT - start_cycles) / g_cycles_per_ms;
}

static bool m33_send(uint32_t message)
{
    if (FSP_SUCCESS == g_ipc_m33.p_api->messageSend(g_ipc_m33.p_ctrl, message))
    {
        return true;
    }

    if (g_tx_drops < UINT16_MAX)
    {
        g_tx_drops++;
    }
    return false;
}

void m33_services_ipc_callback(ipc_callback_args_t * p_args)
{
    if ((NULL == p_args) || (0U == (IPC_EVENT_MESSAGE_RECEIVED & p_args->event)))
    {
        return;
    }

    if (MC_IPC_MSG_HEARTBEAT == mc_ipc_type(p_args->message))
    {
        g_last_m85_heartbeat_cycles = DWT->CYCCNT;
        g_m85_heartbeat_seen = 1U;
    }
}

fsp_err_t m33_services_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    g_cycles_per_ms = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_CPUCLK) / 1000U;
    if (0U == g_cycles_per_ms)
    {
        g_cycles_per_ms = 1U;
    }

    fsp_err_t err = g_ipc_m33.p_api->open(g_ipc_m33.p_ctrl, g_ipc_m33.p_cfg);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    g_touch_ready = ft6336_init();
    g_weight_ready = (uint8_t) (FSP_SUCCESS == weight_sensor_init());
    return FSP_SUCCESS;
}

void m33_services_run(void)
{
    ft6336_touch_t previous_touch = {0};
    uint8_t previous_touch_down = 0U;
    uint32_t previous_weight_frame = 0U;
    uint32_t last_touch_send = DWT->CYCCNT;
    uint32_t last_health_send = DWT->CYCCNT;

    while (1)
    {
        ft6336_touch_t touch = {0};
        uint8_t touch_down = g_touch_ready ? ft6336_read(&touch) : 0U;
        bool touch_changed = (touch_down != previous_touch_down) ||
                             (touch_down && ((touch.x != previous_touch.x) ||
                                             (touch.y != previous_touch.y) ||
                                             (touch.points != previous_touch.points)));
        if (touch_changed || (m33_elapsed_ms(last_touch_send) >= M33_TOUCH_REFRESH_MS))
        {
            uint32_t payload = mc_ipc_pack_touch(touch.x, touch.y, touch.points, touch_down);
            if (m33_send(mc_ipc_message(MC_IPC_MSG_TOUCH, payload)))
            {
                last_touch_send = DWT->CYCCNT;
            }
            previous_touch.x = touch.x;
            previous_touch.y = touch.y;
            previous_touch.points = touch.points;
            previous_touch_down = touch_down;
        }

        uint32_t weight_frame = weight_sensor_get_frame_count();
        if (g_weight_ready && (weight_frame != previous_weight_frame))
        {
            previous_weight_frame = weight_frame;
            (void) m33_send(mc_ipc_message(MC_IPC_MSG_WEIGHT,
                                            mc_ipc_pack_weight(weight_sensor_get_grams(),
                                                               weight_sensor_has_value())));
            (void) m33_send(mc_ipc_message(MC_IPC_MSG_WEIGHT_DIAG,
                                            mc_ipc_pack_weight_diag(weight_sensor_get_last_raw(),
                                                                    weight_frame)));
        }

        if (m33_elapsed_ms(last_health_send) >= M33_HEALTH_PERIOD_MS)
        {
            uint8_t m85_alive = (uint8_t) (g_m85_heartbeat_seen &&
                                           (m33_elapsed_ms(g_last_m85_heartbeat_cycles) <=
                                            M33_M85_TIMEOUT_MS));
            uint32_t payload = mc_ipc_pack_health(g_touch_ready,
                                                   ft6336_get_status(),
                                                   ft6336_get_bus_levels(),
                                                   m85_alive,
                                                   g_weight_ready,
                                                   g_tx_drops,
                                                   g_health_sequence++);
            (void) m33_send(mc_ipc_message(MC_IPC_MSG_HEALTH, payload));
            last_health_send = DWT->CYCCNT;
        }

        R_BSP_SoftwareDelay(M33_TOUCH_PERIOD_MS, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
