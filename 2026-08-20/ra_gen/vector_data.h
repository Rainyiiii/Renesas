/* generated vector header file - do not edit */
#ifndef VECTOR_DATA_H
#define VECTOR_DATA_H
#ifdef __cplusplus
        extern "C" {
        #endif
/* Number of interrupts allocated */
#ifndef VECTOR_DATA_IRQ_COUNT
#define VECTOR_DATA_IRQ_COUNT    (17)
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
void iic_master_rxi_isr(void);
void iic_master_txi_isr(void);
void iic_master_tei_isr(void);
void iic_master_eri_isr(void);
void sci_b_uart_rxi_isr(void);
void sci_b_uart_txi_isr(void);
void sci_b_uart_tei_isr(void);
void sci_b_uart_eri_isr(void);

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
#define VECTOR_NUMBER_IIC1_RXI ((IRQn_Type) 9) /* IIC1 RXI (Receive data full) */
#define IIC1_RXI_IRQn          ((IRQn_Type) 9) /* IIC1 RXI (Receive data full) */
#define VECTOR_NUMBER_IIC1_TXI ((IRQn_Type) 10) /* IIC1 TXI (Transmit data empty) */
#define IIC1_TXI_IRQn          ((IRQn_Type) 10) /* IIC1 TXI (Transmit data empty) */
#define VECTOR_NUMBER_IIC1_TEI ((IRQn_Type) 11) /* IIC1 TEI (Transmit end) */
#define IIC1_TEI_IRQn          ((IRQn_Type) 11) /* IIC1 TEI (Transmit end) */
#define VECTOR_NUMBER_IIC1_ERI ((IRQn_Type) 12) /* IIC1 ERI (Transfer error) */
#define IIC1_ERI_IRQn          ((IRQn_Type) 12) /* IIC1 ERI (Transfer error) */
#define VECTOR_NUMBER_SCI4_RXI ((IRQn_Type) 13) /* SCI4 RXI (Receive data full) */
#define SCI4_RXI_IRQn          ((IRQn_Type) 13) /* SCI4 RXI (Receive data full) */
#define VECTOR_NUMBER_SCI4_TXI ((IRQn_Type) 14) /* SCI4 TXI (Transmit data empty) */
#define SCI4_TXI_IRQn          ((IRQn_Type) 14) /* SCI4 TXI (Transmit data empty) */
#define VECTOR_NUMBER_SCI4_TEI ((IRQn_Type) 15) /* SCI4 TEI (Transmit end) */
#define SCI4_TEI_IRQn          ((IRQn_Type) 15) /* SCI4 TEI (Transmit end) */
#define VECTOR_NUMBER_SCI4_ERI ((IRQn_Type) 16) /* SCI4 ERI (Receive error) */
#define SCI4_ERI_IRQn          ((IRQn_Type) 16) /* SCI4 ERI (Receive error) */
/* The number of entries required for the ICU vector table. */
#define BSP_ICU_VECTOR_NUM_ENTRIES (17)

#ifdef __cplusplus
        }
        #endif
#endif /* VECTOR_DATA_H */
