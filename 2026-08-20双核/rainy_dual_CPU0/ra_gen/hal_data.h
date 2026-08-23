/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_ipc.h"
#include "r_spi_b.h"
#include "r_gpt.h"
#include "r_timer_api.h"
#include "r_capture_api.h"
#include "r_ceu.h"
#include "r_dmac.h"
#include "r_transfer_api.h"
#include "r_sdhi.h"
#include "r_sdmmc_api.h"
FSP_HEADER
/** IPC Instance. */
extern const ipc_instance_t g_ipc_m85;

/** Access the IPC instance using these structures when calling API functions directly
 (::p_api is not used). */
extern ipc_instance_ctrl_t g_ipc_m85_ctrl;
extern const ipc_cfg_t g_ipc_m85_cfg;

#ifndef platform_services_ipc_callback
void platform_services_ipc_callback(ipc_callback_args_t *p_args);
#endif
/** SPI on SPI Instance. */
extern const spi_instance_t g_spi1;

/** Access the SPI instance using these structures when calling API functions directly (::p_api is not used). */
extern spi_b_instance_ctrl_t g_spi1_ctrl;
extern const spi_cfg_t g_spi1_cfg;

/** Callback used by SPI Instance. */
#ifndef spi_callback
void spi_callback(spi_callback_args_t *p_args);
#endif

#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
#define g_spi1_P_TRANSFER_TX (NULL)
#else
    #define g_spi1_P_TRANSFER_TX (&RA_NOT_DEFINED)
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
#define g_spi1_P_TRANSFER_RX (NULL)
#else
    #define g_spi1_P_TRANSFER_RX (&RA_NOT_DEFINED)
#endif
#undef RA_NOT_DEFINED
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer0;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_timer0_ctrl;
extern const timer_cfg_t g_timer0_cfg;

#ifndef NULL
void NULL(timer_callback_args_t *p_args);
#endif
/* CEU on CAPTURE instance */
extern const capture_instance_t g_ceu_qvga;
/* Access the CEU instance using these structures when calling API functions directly (::p_api is not used). */
extern ceu_instance_ctrl_t g_ceu_qvga_ctrl;
extern const capture_cfg_t g_ceu_qvga_cfg;
#ifndef g_ceu_callback
void g_ceu_callback(capture_callback_args_t *p_args);
#endif
/* Transfer on DMAC Instance. */
extern const transfer_instance_t g_sdmmc0_transfer;

/** Access the DMAC instance using these structures when calling API functions directly (::p_api is not used). */
extern dmac_instance_ctrl_t g_sdmmc0_transfer_ctrl;
extern const transfer_cfg_t g_sdmmc0_transfer_cfg;

#ifndef g_sdmmc0_dmac_callback
void g_sdmmc0_dmac_callback(transfer_callback_args_t *p_args);
#endif
/** SDMMC on SDMMC Instance. */
extern const sdmmc_instance_t g_sdmmc0;

/** Access the SDMMC instance using these structures when calling API functions directly (::p_api is not used). */
extern sdhi_instance_ctrl_t g_sdmmc0_ctrl;
extern sdmmc_cfg_t g_sdmmc0_cfg;

#ifndef dataset_storage_sdhi_callback
void dataset_storage_sdhi_callback(sdmmc_callback_args_t *p_args);
#endif
void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
