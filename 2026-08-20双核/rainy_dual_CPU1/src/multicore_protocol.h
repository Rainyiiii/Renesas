#ifndef MULTICORE_PROTOCOL_H
#define MULTICORE_PROTOCOL_H

#include <stdint.h>

/* One IPC FIFO entry is 32 bits: type[31:28] + payload[27:0]. */
#define MC_IPC_TYPE_SHIFT       (28U)
#define MC_IPC_PAYLOAD_MASK     (0x0FFFFFFFUL)

typedef enum e_mc_ipc_message_type
{
    MC_IPC_MSG_TOUCH       = 1U,
    MC_IPC_MSG_WEIGHT      = 2U,
    MC_IPC_MSG_HEALTH      = 3U,
    MC_IPC_MSG_HEARTBEAT   = 4U,
    MC_IPC_MSG_WEIGHT_DIAG = 5U
} mc_ipc_message_type_t;

static inline uint32_t mc_ipc_message(mc_ipc_message_type_t type, uint32_t payload)
{
    return ((uint32_t) type << MC_IPC_TYPE_SHIFT) | (payload & MC_IPC_PAYLOAD_MASK);
}

static inline mc_ipc_message_type_t mc_ipc_type(uint32_t message)
{
    return (mc_ipc_message_type_t) ((message >> MC_IPC_TYPE_SHIFT) & 0x0FU);
}

static inline uint32_t mc_ipc_payload(uint32_t message)
{
    return message & MC_IPC_PAYLOAD_MASK;
}

/* TOUCH: x[8:0], y[17:9], points[19:18], down[20]. */
static inline uint32_t mc_ipc_pack_touch(uint16_t x, uint16_t y, uint8_t points, uint8_t down)
{
    return ((uint32_t) x & 0x1FFU) |
           (((uint32_t) y & 0x1FFU) << 9U) |
           (((uint32_t) points & 0x03U) << 18U) |
           (((uint32_t) down & 0x01U) << 20U);
}

/* WEIGHT: signed grams[15:0], valid[16]. */
static inline uint32_t mc_ipc_pack_weight(int16_t grams, uint8_t valid)
{
    return (uint16_t) grams | (((uint32_t) valid & 1U) << 16U);
}

/* HEALTH: touch ready/status/bus, M85 seen, weight ready, drops and sequence. */
static inline uint32_t mc_ipc_pack_health(uint8_t touch_ready,
                                          uint8_t touch_status,
                                          uint8_t bus_levels,
                                          uint8_t m85_alive,
                                          uint8_t weight_ready,
                                          uint16_t tx_drops,
                                          uint8_t sequence)
{
    return ((uint32_t) touch_ready & 1U) |
           (((uint32_t) touch_status & 0x03U) << 1U) |
           (((uint32_t) bus_levels & 0x03U) << 3U) |
           (((uint32_t) m85_alive & 1U) << 5U) |
           (((uint32_t) weight_ready & 1U) << 6U) |
           (((uint32_t) tx_drops & 0x0FFFU) << 8U) |
           ((uint32_t) sequence << 20U);
}

/* HEARTBEAT: M85 frame[23:0], CEU recovery count[27:24]. */
static inline uint32_t mc_ipc_pack_heartbeat(uint32_t frame, uint32_t ceu_recoveries)
{
    return (frame & 0x00FFFFFFU) | ((ceu_recoveries & 0x0FU) << 24U);
}

/* WEIGHT_DIAG: signed raw[15:0], valid frame count[27:16]. */
static inline uint32_t mc_ipc_pack_weight_diag(int16_t raw, uint32_t frame_count)
{
    return (uint16_t) raw | ((frame_count & 0x0FFFU) << 16U);
}

#endif
