#ifndef FT6336_H
#define FT6336_H

#include <stdint.h>
#include "r_i2c_master_api.h"

typedef struct st_ft6336_touch
{
    uint16_t x;
    uint16_t y;
    uint8_t  points;
} ft6336_touch_t;

#define FT6336_STATUS_OK         0U
#define FT6336_STATUS_BUS_STUCK  1U
#define FT6336_STATUS_NO_ACK     2U
#define FT6336_STATUS_BAD_ID     3U

/* 本工程使用的 CTP 接线：
 * SDA=P511, SCL=P512, RST=P105。因为读取采用轮询方式，所以 INT 可选。 */
uint8_t ft6336_init(void);
uint8_t ft6336_read(ft6336_touch_t *p_touch);
uint8_t ft6336_get_status(void);
uint8_t ft6336_get_chip_id(void);
uint8_t ft6336_get_bus_levels(void);
uint8_t ft6336_get_address(void);
void ft6336_i2c_callback(i2c_master_callback_args_t *p_args);

#endif
