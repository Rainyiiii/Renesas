#include "platform_services.h"
#include "multicore_protocol.h"
#include "ft6336.h"
#include "weight_sensor.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define PLATFORM_M33_TIMEOUT_MS (3000U)

static volatile platform_touch_t g_touch;
static volatile platform_weight_t g_weight;
static volatile platform_health_t g_health;
#if PLATFORM_SERVICES_MULTICORE
static volatile uint32_t g_touch_sequence;
static uint32_t g_cpu_cycles_per_ms;
static volatile uint32_t g_last_m33_health_cycles;

static void platform_services_receive(uint32_t message)
{
    uint32_t payload = mc_ipc_payload(message);

    switch (mc_ipc_type(message))
    {
        case MC_IPC_MSG_TOUCH:
            g_touch.x = (uint16_t) (payload & 0x1FFU);
            g_touch.y = (uint16_t) ((payload >> 9U) & 0x1FFU);
            g_touch.points = (uint8_t) ((payload >> 18U) & 0x03U);
            g_touch.down = (uint8_t) ((payload >> 20U) & 0x01U);
            __DMB();
            g_touch_sequence++;
            break;

        case MC_IPC_MSG_WEIGHT:
            g_weight.grams = (int16_t) (payload & 0xFFFFU);
            g_weight.valid = (uint8_t) ((payload >> 16U) & 0x01U);
            break;

        case MC_IPC_MSG_HEALTH:
            g_health.touch_ready = (uint8_t) (payload & 0x01U);
            g_health.touch_status = (uint8_t) ((payload >> 1U) & 0x03U);
            g_health.touch_bus_levels = (uint8_t) ((payload >> 3U) & 0x03U);
            g_health.m85_seen_by_m33 = (uint8_t) ((payload >> 5U) & 0x01U);
            g_health.weight_ready = (uint8_t) ((payload >> 6U) & 0x01U);
            g_health.m33_tx_drops = (uint16_t) ((payload >> 8U) & 0x0FFFU);
            g_health.m33_online = 1U;
            g_last_m33_health_cycles = DWT->CYCCNT;
            break;

        case MC_IPC_MSG_WEIGHT_DIAG:
            g_weight.raw = (int16_t) (payload & 0xFFFFU);
            g_weight.frame_count = (payload >> 16U) & 0x0FFFU;
            break;

        case MC_IPC_MSG_HEARTBEAT:
        default:
            break;
    }
}

void platform_services_ipc_callback(ipc_callback_args_t * p_args)
{
    if ((NULL != p_args) && (0U != (IPC_EVENT_MESSAGE_RECEIVED & p_args->event)))
    {
        platform_services_receive(p_args->message);
    }
}
#endif

fsp_err_t platform_services_init(void)
{
    memset((void *) &g_touch, 0, sizeof(g_touch));
    memset((void *) &g_weight, 0, sizeof(g_weight));
    memset((void *) &g_health, 0, sizeof(g_health));

#if PLATFORM_SERVICES_MULTICORE
    g_health.multicore_enabled = 1U;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    g_cpu_cycles_per_ms = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_CPUCLK) / 1000U;
    if (0U == g_cpu_cycles_per_ms)
    {
        g_cpu_cycles_per_ms = 1U;
    }

    fsp_err_t err = g_ipc_m85.p_api->open(g_ipc_m85.p_ctrl, g_ipc_m85.p_cfg);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    R_BSP_SecondaryCoreStart();
    return FSP_SUCCESS;
#else
    g_health.multicore_enabled = 0U;
    g_health.touch_ready = ft6336_init();
    g_health.touch_status = ft6336_get_status();
    g_health.touch_bus_levels = ft6336_get_bus_levels();
    fsp_err_t weight_err = weight_sensor_init();
    g_health.weight_ready = (uint8_t) (FSP_SUCCESS == weight_err);
    return FSP_SUCCESS;
#endif
}

void platform_services_poll(void)
{
#if PLATFORM_SERVICES_MULTICORE
    if (g_health.m33_online)
    {
        uint32_t elapsed = DWT->CYCCNT - g_last_m33_health_cycles;
        uint32_t timeout_cycles = g_cpu_cycles_per_ms * PLATFORM_M33_TIMEOUT_MS;
        if (elapsed > timeout_cycles)
        {
            g_health.m33_online = 0U;
            g_health.m85_seen_by_m33 = 0U;
            g_health.touch_ready = 0U;
            g_health.weight_ready = 0U;
            g_touch.down = 0U;
            g_weight.valid = 0U;
        }
    }
#else
    g_health.touch_ready = (uint8_t) (FT6336_STATUS_OK == ft6336_get_status());
    g_health.touch_status = ft6336_get_status();
    g_health.touch_bus_levels = ft6336_get_bus_levels();
#endif
}

uint8_t platform_services_touch_read(platform_touch_t * touch)
{
    if (NULL == touch)
    {
        return 0U;
    }

#if PLATFORM_SERVICES_MULTICORE
    uint32_t before;
    uint32_t after;
    do
    {
        before = g_touch_sequence;
        __DMB();
        touch->x = g_touch.x;
        touch->y = g_touch.y;
        touch->points = g_touch.points;
        touch->down = g_touch.down;
        __DMB();
        after = g_touch_sequence;
    } while (before != after);
    return (uint8_t) (g_health.m33_online && g_health.touch_ready && touch->down);
#else
    ft6336_touch_t local_touch = {0};
    uint8_t down = ft6336_read(&local_touch);
    touch->x = local_touch.x;
    touch->y = local_touch.y;
    touch->points = local_touch.points;
    touch->down = down;
    return down;
#endif
}

void platform_services_weight_get(platform_weight_t * weight)
{
    if (NULL == weight)
    {
        return;
    }

#if PLATFORM_SERVICES_MULTICORE
    __DMB();
    *weight = g_weight;
    if (!g_health.m33_online || !g_health.weight_ready)
    {
        weight->valid = 0U;
    }
#else
    weight->valid = (uint8_t) weight_sensor_has_value();
    weight->grams = weight_sensor_get_grams();
    weight->raw = weight_sensor_get_last_raw();
    weight->frame_count = weight_sensor_get_frame_count();
#endif
}

void platform_services_health_get(platform_health_t * health)
{
    if (NULL != health)
    {
        __DMB();
        *health = g_health;
    }
}

void platform_services_m85_heartbeat(uint32_t frame_count, uint32_t ceu_recoveries)
{
#if PLATFORM_SERVICES_MULTICORE
    if (0U == (frame_count & 31U))
    {
        uint32_t message = mc_ipc_message(MC_IPC_MSG_HEARTBEAT,
                                          mc_ipc_pack_heartbeat(frame_count, ceu_recoveries));
        if (FSP_SUCCESS != g_ipc_m85.p_api->messageSend(g_ipc_m85.p_ctrl, message))
        {
            if (g_health.m85_tx_drops < UINT16_MAX)
            {
                g_health.m85_tx_drops++;
            }
        }
    }
#else
    (void) frame_count;
    (void) ceu_recoveries;
#endif
}
