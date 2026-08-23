/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_sci_b_uart.h"
#include "r_uart_api.h"
#include "r_iic_master.h"
#include "r_i2c_master_api.h"
#include "r_ipc.h"
FSP_HEADER
/** UART on SCI Instance. */
extern const uart_instance_t g_weight_uart;

/** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
extern sci_b_uart_instance_ctrl_t g_weight_uart_ctrl;
extern const uart_cfg_t g_weight_uart_cfg;
extern const sci_b_uart_extended_cfg_t g_weight_uart_cfg_extend;

#ifndef weight_uart_callback
void weight_uart_callback(uart_callback_args_t *p_args);
#endif
/* I2C Master on IIC Instance. */
extern const i2c_master_instance_t g_i2c_master0;

/** Access the I2C Master instance using these structures when calling API functions directly (::p_api is not used). */
extern iic_master_instance_ctrl_t g_i2c_master0_ctrl;
extern const i2c_master_cfg_t g_i2c_master0_cfg;

#ifndef ft6336_i2c_callback
void ft6336_i2c_callback(i2c_master_callback_args_t *p_args);
#endif
/** IPC Instance. */
extern const ipc_instance_t g_ipc_m33;

/** Access the IPC instance using these structures when calling API functions directly
 (::p_api is not used). */
extern ipc_instance_ctrl_t g_ipc_m33_ctrl;
extern const ipc_cfg_t g_ipc_m33_cfg;

#ifndef m33_services_ipc_callback
void m33_services_ipc_callback(ipc_callback_args_t *p_args);
#endif
void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
