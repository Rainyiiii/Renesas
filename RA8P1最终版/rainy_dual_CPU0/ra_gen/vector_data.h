/* generated vector header file - do not edit */
#ifndef VECTOR_DATA_H
#define VECTOR_DATA_H
#ifdef __cplusplus
        extern "C" {
        #endif
/* Number of interrupts allocated */
#ifndef VECTOR_DATA_IRQ_COUNT
#define VECTOR_DATA_IRQ_COUNT    (10)
#endif
/* ISR prototypes */
void sdhimmc_accs_isr(void);
void sdhimmc_card_isr(void);
void dmac_int_isr(void);
void ceu_isr(void);
void spi_b_rxi_isr(void);
void spi_b_txi_isr(void);
void spi_b_tei_isr(void);
void spi_b_eri_isr(void);
void rm_ethosu_isr(void);
void ipc_isr(void);

/* Vector table allocations */
#define VECTOR_NUMBER_SDHIMMC0_ACCS ((IRQn_Type) 0) /* SDHIMMC0 ACCS (Card access) */
#define SDHIMMC0_ACCS_IRQn          ((IRQn_Type) 0) /* SDHIMMC0 ACCS (Card access) */
#define VECTOR_NUMBER_SDHIMMC0_CARD ((IRQn_Type) 1) /* SDHIMMC0 CARD (Card detect) */
#define SDHIMMC0_CARD_IRQn          ((IRQn_Type) 1) /* SDHIMMC0 CARD (Card detect) */
#define VECTOR_NUMBER_DMAC0_INT ((IRQn_Type) 2) /* DMAC0 INT (DMAC0 transfer end) */
#define DMAC0_INT_IRQn          ((IRQn_Type) 2) /* DMAC0 INT (DMAC0 transfer end) */
#define VECTOR_NUMBER_CEU_CEUI ((IRQn_Type) 3) /* CEU CEUI (CEU interrupt) */
#define CEU_CEUI_IRQn          ((IRQn_Type) 3) /* CEU CEUI (CEU interrupt) */
#define VECTOR_NUMBER_SPI1_RXI ((IRQn_Type) 4) /* SPI1 RXI (Receive buffer full) */
#define SPI1_RXI_IRQn          ((IRQn_Type) 4) /* SPI1 RXI (Receive buffer full) */
#define VECTOR_NUMBER_SPI1_TXI ((IRQn_Type) 5) /* SPI1 TXI (Transmit buffer empty) */
#define SPI1_TXI_IRQn          ((IRQn_Type) 5) /* SPI1 TXI (Transmit buffer empty) */
#define VECTOR_NUMBER_SPI1_TEI ((IRQn_Type) 6) /* SPI1 TEI (Transmission complete event) */
#define SPI1_TEI_IRQn          ((IRQn_Type) 6) /* SPI1 TEI (Transmission complete event) */
#define VECTOR_NUMBER_SPI1_ERI ((IRQn_Type) 7) /* SPI1 ERI (Error) */
#define SPI1_ERI_IRQn          ((IRQn_Type) 7) /* SPI1 ERI (Error) */
#define VECTOR_NUMBER_NPU_IRQ ((IRQn_Type) 8) /* NPU IRQ (NPU IRQ) */
#define NPU_IRQ_IRQn          ((IRQn_Type) 8) /* NPU IRQ (NPU IRQ) */
#define VECTOR_NUMBER_IPC_IRQ0 ((IRQn_Type) 9) /* IPC IRQ0 (CPU Mutual Interrupt 0) */
#define IPC_IRQ0_IRQn          ((IRQn_Type) 9) /* IPC IRQ0 (CPU Mutual Interrupt 0) */
/* The number of entries required for the ICU vector table. */
#define BSP_ICU_VECTOR_NUM_ENTRIES (10)

#ifdef __cplusplus
        }
        #endif
#endif /* VECTOR_DATA_H */
