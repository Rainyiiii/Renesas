/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = sdhimmc_accs_isr, /* SDHIMMC0 ACCS (Card access) */
            [1] = sdhimmc_card_isr, /* SDHIMMC0 CARD (Card detect) */
            [2] = dmac_int_isr, /* DMAC0 INT (DMAC0 transfer end) */
            [3] = ceu_isr, /* CEU CEUI (CEU interrupt) */
            [4] = spi_b_rxi_isr, /* SPI1 RXI (Receive buffer full) */
            [5] = spi_b_txi_isr, /* SPI1 TXI (Transmit buffer empty) */
            [6] = spi_b_tei_isr, /* SPI1 TEI (Transmission complete event) */
            [7] = spi_b_eri_isr, /* SPI1 ERI (Error) */
            [8] = rm_ethosu_isr, /* NPU IRQ (NPU IRQ) */
            [9] = ipc_isr, /* IPC IRQ0 (CPU Mutual Interrupt 0) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_SDHIMMC0_ACCS,GROUP0), /* SDHIMMC0 ACCS (Card access) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_SDHIMMC0_CARD,GROUP1), /* SDHIMMC0 CARD (Card detect) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_DMAC0_INT,GROUP2), /* DMAC0 INT (DMAC0 transfer end) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_CEU_CEUI,GROUP3), /* CEU CEUI (CEU interrupt) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_SPI1_RXI,GROUP4), /* SPI1 RXI (Receive buffer full) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_SPI1_TXI,GROUP5), /* SPI1 TXI (Transmit buffer empty) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_SPI1_TEI,GROUP6), /* SPI1 TEI (Transmission complete event) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_SPI1_ERI,GROUP7), /* SPI1 ERI (Error) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_NPU_IRQ,GROUP0), /* NPU IRQ (NPU IRQ) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_IPC_IRQ0,GROUP1), /* IPC IRQ0 (CPU Mutual Interrupt 0) */
        };
        #endif
        #endif
