#ifndef WEIGHT_SENSOR_H
#define WEIGHT_SENSOR_H

#include "hal_data.h"
#include <stdbool.h>
#include <stdint.h>

fsp_err_t weight_sensor_init(void);
bool weight_sensor_has_value(void);
int16_t weight_sensor_get_grams(void);

/* 上电调试诊断。这些接口让显示端无需调试器即可确认数据通路：
 * rx_bytes 统计接收到的每一个 UART 字节（验证接线/baud 正确），
 * frame_count 统计有效的 FF..FE frame 数量（验证 framing/协议正确），
 * last_raw 则暴露未经缩放的 16 位传感器原始值。 */
uint32_t weight_sensor_get_rx_bytes(void);
uint32_t weight_sensor_get_frame_count(void);
int16_t  weight_sensor_get_last_raw(void);

#endif
