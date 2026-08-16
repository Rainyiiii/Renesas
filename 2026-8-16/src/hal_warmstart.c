/*
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "hal_data.h"

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);

FSP_CPP_FOOTER

/*******************************************************************************************************************//**
 * 本函数在启动过程的多个阶段被调用。本实现使用在 main() 之前紧接着触发的事件来配置引脚。
 *
 * @param[in]  event    代码当前处于启动过程中的哪个阶段
 **********************************************************************************************************************/
void R_BSP_WarmStart (bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_RESET == event)
    {
#if BSP_FEATURE_FLASH_LP_VERSION != 0

        /* 使能对 data flash 的读取。 */
        R_FACI_LP->DFLCTL = 1U;

        /* 通常需要等待 tDSTOP(6us) 以便 data flash 恢复。将使能操作放在这里，即在 clock 和
         * C runtime 初始化之前，应该可以省去延时，因为初始化通常会耗时超过 6us。 */
#endif
    }

#if BSP_CFG_OSPI_B_STARTUP_ENABLED && defined(BSP_CFG_OSPI_B_STARTUP_FN)
    if (BSP_WARM_START_POST_CLOCK == event)
    {
        /* 配置 OSPI_B SiP flash 并对其进行初始化。 */
        R_BSP_OspiBInit(BSP_CFG_OSPI_B_STARTUP_FN, true);
    }
#endif

    if (BSP_WARM_START_POST_C == event)
    {
        /* C runtime 环境和 system clock 已配置完成。 */

        /* 配置引脚。 */
        R_IOPORT_Open(&IOPORT_CFG_CTRL, &IOPORT_CFG_NAME);

#if BSP_CFG_SDRAM_ENABLED

        /* 配置 SDRAM 并对其进行初始化。必须先配置引脚。 */
        R_BSP_SdramInit(true);
#endif
    }
}
