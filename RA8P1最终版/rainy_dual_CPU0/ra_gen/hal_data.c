/* generated HAL source file - do not edit */
#include "hal_data.h"

ipc_instance_ctrl_t g_ipc_m85_ctrl;

/** IPC configuration */
const ipc_cfg_t g_ipc_m85_cfg = { .channel = 0, .p_callback =
		platform_services_ipc_callback,
#if defined(NULL)
                .p_context = NULL,
#else
		.p_context = (void*) &NULL,
#endif
		.ipl = (5),
#if defined(VECTOR_NUMBER_IPC_IRQ0)
                .irq = VECTOR_NUMBER_IPC_IRQ0,
#else
		.irq = FSP_INVALID_VECTOR,
#endif
		};

/* Instance structure to use this module. */
const ipc_instance_t g_ipc_m85 = { .p_ctrl = &g_ipc_m85_ctrl, .p_cfg =
		&g_ipc_m85_cfg, .p_api = &g_ipc_on_ipc };
#define RA_NOT_DEFINED (UINT32_MAX)
#if (RA_NOT_DEFINED) != (RA_NOT_DEFINED)

/* If the transfer module is DMAC, define a DMAC transfer callback. */
#include "r_dmac.h"
extern void spi_b_tx_dmac_callback(spi_b_instance_ctrl_t const * const p_ctrl);

void g_spi1_tx_transfer_callback (dmac_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    spi_b_tx_dmac_callback(&g_spi1_ctrl);
}
#endif

#if (RA_NOT_DEFINED) != (RA_NOT_DEFINED)

/* If the transfer module is DMAC, define a DMAC transfer callback. */
#include "r_dmac.h"
extern void spi_b_rx_dmac_callback(spi_b_instance_ctrl_t const * const p_ctrl);

void g_spi1_rx_transfer_callback (dmac_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    spi_b_rx_dmac_callback(&g_spi1_ctrl);
}
#endif
#undef RA_NOT_DEFINED

spi_b_instance_ctrl_t g_spi1_ctrl;

/** SPI extended configuration for SPI HAL driver */
const spi_b_extended_cfg_t g_spi1_ext_cfg = { .spi_clksyn =
		SPI_B_SSL_MODE_CLK_SYN, .spi_comm = SPI_B_COMMUNICATION_TRANSMIT_ONLY,
		.ssl_polarity = SPI_B_SSLP_LOW, .ssl_select = SPI_B_SSL_SELECT_SSL0,
		.mosi_idle = SPI_B_MOSI_IDLE_VALUE_FIXING_DISABLE, .parity =
				SPI_B_PARITY_MODE_DISABLE, .byte_swap = SPI_B_BYTE_SWAP_DISABLE,
		.clock_source = SPI_B_CLOCK_SOURCE_PCLK, .spck_div = {
		/* Actual calculated bitrate: 31250000. */.spbr = 1, .brdv = 0 },
		.spck_delay = SPI_B_DELAY_COUNT_1, .ssl_negation_delay =
				SPI_B_DELAY_COUNT_1, .next_access_delay = SPI_B_DELAY_COUNT_1,
		.burst_interframe_delay = SPI_B_BURST_TRANSFER_WITH_DELAY

};

/** SPI configuration for SPI HAL driver */
const spi_cfg_t g_spi1_cfg = { .channel = 1,

#if defined(VECTOR_NUMBER_SPI1_RXI)
    .rxi_irq             = VECTOR_NUMBER_SPI1_RXI,
#else
		.rxi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SPI1_TXI)
    .txi_irq             = VECTOR_NUMBER_SPI1_TXI,
#else
		.txi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SPI1_TEI)
    .tei_irq             = VECTOR_NUMBER_SPI1_TEI,
#else
		.tei_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SPI1_ERI)
    .eri_irq             = VECTOR_NUMBER_SPI1_ERI,
#else
		.eri_irq = FSP_INVALID_VECTOR,
#endif

		.rxi_ipl = (12), .txi_ipl = (12), .tei_ipl = (12), .eri_ipl = (12),

		.operating_mode = SPI_MODE_MASTER,

		.clk_phase = SPI_CLK_PHASE_EDGE_ODD, .clk_polarity =
				SPI_CLK_POLARITY_LOW,

		.mode_fault = SPI_MODE_FAULT_ERROR_DISABLE, .bit_order =
				SPI_BIT_ORDER_MSB_FIRST, .p_transfer_tx = g_spi1_P_TRANSFER_TX,
		.p_transfer_rx = g_spi1_P_TRANSFER_RX, .p_callback = spi_callback,

		.p_context = NULL, .p_extend = (void*) &g_spi1_ext_cfg, };

/* Instance structure to use this module. */
const spi_instance_t g_spi1 = { .p_ctrl = &g_spi1_ctrl, .p_cfg = &g_spi1_cfg,
		.p_api = &g_spi_on_spi_b };
gpt_instance_ctrl_t g_timer0_ctrl;
#if 0
const gpt_extended_pwm_cfg_t g_timer0_pwm_extend =
{
    .trough_ipl             = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_GPT0_COUNTER_UNDERFLOW)
    .trough_irq             = VECTOR_NUMBER_GPT0_COUNTER_UNDERFLOW,
#else
    .trough_irq             = FSP_INVALID_VECTOR,
#endif
    .poeg_link              = GPT_POEG_LINK_POEG0,
    .output_disable         = (gpt_output_disable_t) ( GPT_OUTPUT_DISABLE_NONE),
    .adc_trigger            = (gpt_adc_trigger_t) ( GPT_ADC_TRIGGER_NONE),
    .dead_time_count_up     = 0,
    .dead_time_count_down   = 0,
    .adc_a_compare_match    = 0,
    .adc_b_compare_match    = 0,
    .interrupt_skip_source  = GPT_INTERRUPT_SKIP_SOURCE_NONE,
    .interrupt_skip_count   = GPT_INTERRUPT_SKIP_COUNT_0,
    .interrupt_skip_adc     = GPT_INTERRUPT_SKIP_ADC_NONE,
    .gtioca_disable_setting = GPT_GTIOC_DISABLE_PROHIBITED,
    .gtiocb_disable_setting = GPT_GTIOC_DISABLE_PROHIBITED,
};
#endif
const gpt_extended_cfg_t g_timer0_extend =
		{ .gtioca = { .output_enabled = true, .stop_level = GPT_PIN_LEVEL_LOW },
				.gtiocb = { .output_enabled = false, .stop_level =
						GPT_PIN_LEVEL_LOW }, .start_source = (gpt_source_t)(
						GPT_SOURCE_NONE), .stop_source = (gpt_source_t)(
						GPT_SOURCE_NONE), .clear_source = (gpt_source_t)(
						GPT_SOURCE_NONE), .count_up_source = (gpt_source_t)(
						GPT_SOURCE_NONE), .count_down_source = (gpt_source_t)(
						GPT_SOURCE_NONE), .capture_a_source = (gpt_source_t)(
						GPT_SOURCE_NONE), .capture_b_source = (gpt_source_t)(
						GPT_SOURCE_NONE), .capture_a_ipl = (BSP_IRQ_DISABLED),
				.capture_b_ipl = (BSP_IRQ_DISABLED), .compare_match_c_ipl =
						(BSP_IRQ_DISABLED), .compare_match_d_ipl =
						(BSP_IRQ_DISABLED), .compare_match_e_ipl =
						(BSP_IRQ_DISABLED), .compare_match_f_ipl =
						(BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_GPT0_CAPTURE_COMPARE_A)
    .capture_a_irq         = VECTOR_NUMBER_GPT0_CAPTURE_COMPARE_A,
#else
				.capture_a_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT0_CAPTURE_COMPARE_B)
    .capture_b_irq         = VECTOR_NUMBER_GPT0_CAPTURE_COMPARE_B,
#else
				.capture_b_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT0_COMPARE_C)
    .compare_match_c_irq   = VECTOR_NUMBER_GPT0_COMPARE_C,
#else
				.compare_match_c_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT0_COMPARE_D)
    .compare_match_d_irq   = VECTOR_NUMBER_GPT0_COMPARE_D,
#else
				.compare_match_d_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT0_COMPARE_E)
    .compare_match_e_irq   = VECTOR_NUMBER_GPT0_COMPARE_E,
#else
				.compare_match_e_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT0_COMPARE_F)
    .compare_match_f_irq   = VECTOR_NUMBER_GPT0_COMPARE_F,
#else
				.compare_match_f_irq = FSP_INVALID_VECTOR,
#endif
				.compare_match_value = { (uint32_t) 0x0, /* CMP_A */
						(uint32_t) 0x0, /* CMP_B */(uint32_t) 0x0, /* CMP_C */
						(uint32_t) 0x0, /* CMP_D */(uint32_t) 0x0, /* CMP_E */
						(uint32_t) 0x0, /* CMP_F */}, .compare_match_status =
						((0U << 5U) | (0U << 4U) | (0U << 3U) | (0U << 2U)
								| (0U << 1U) | 0U), .capture_filter_gtioca =
						GPT_CAPTURE_FILTER_NONE, .capture_filter_gtiocb =
						GPT_CAPTURE_FILTER_NONE,
#if 0
    .p_pwm_cfg             = &g_timer0_pwm_extend,
#else
				.p_pwm_cfg = NULL,
#endif
#if 0
    .gtior_setting.gtior_b.gtioa  = (0U << 4U) | (0U << 2U) | (0U << 0U),
    .gtior_setting.gtior_b.oadflt = (uint32_t) GPT_PIN_LEVEL_LOW,
    .gtior_setting.gtior_b.oahld  = 0U,
    .gtior_setting.gtior_b.oae    = (uint32_t) true,
    .gtior_setting.gtior_b.oadf   = (uint32_t) GPT_GTIOC_DISABLE_PROHIBITED,
    .gtior_setting.gtior_b.nfaen  = ((uint32_t) GPT_CAPTURE_FILTER_NONE & 1U),
    .gtior_setting.gtior_b.nfcsa  = ((uint32_t) GPT_CAPTURE_FILTER_NONE >> 1U),
    .gtior_setting.gtior_b.gtiob  = (0U << 4U) | (0U << 2U) | (0U << 0U),
    .gtior_setting.gtior_b.obdflt = (uint32_t) GPT_PIN_LEVEL_LOW,
    .gtior_setting.gtior_b.obhld  = 0U,
    .gtior_setting.gtior_b.obe    = (uint32_t) false,
    .gtior_setting.gtior_b.obdf   = (uint32_t) GPT_GTIOC_DISABLE_PROHIBITED,
    .gtior_setting.gtior_b.nfben  = ((uint32_t) GPT_CAPTURE_FILTER_NONE & 1U),
    .gtior_setting.gtior_b.nfcsb  = ((uint32_t) GPT_CAPTURE_FILTER_NONE >> 1U),
#else
				.gtior_setting.gtior = 0U,
#endif

				.gtioca_polarity = GPT_GTIOC_POLARITY_NORMAL, .gtiocb_polarity =
						GPT_GTIOC_POLARITY_NORMAL, };

const timer_cfg_t g_timer0_cfg = { .mode = TIMER_MODE_PWM,
/* Actual period: 4e-8 seconds. Actual duty: 50%. */.period_counts =
		(uint32_t) 0xa, .duty_cycle_counts = 0x5, .source_div =
		(timer_source_div_t) 0, .channel = 0, .p_callback = NULL,
/** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
		.p_context = (void*) &NULL,
#endif
		.p_extend = &g_timer0_extend, .cycle_end_ipl = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_GPT0_COUNTER_OVERFLOW)
    .cycle_end_irq       = VECTOR_NUMBER_GPT0_COUNTER_OVERFLOW,
#else
		.cycle_end_irq = FSP_INVALID_VECTOR,
#endif
		};
/* Instance structure to use this module. */
const timer_instance_t g_timer0 = { .p_ctrl = &g_timer0_ctrl, .p_cfg =
		&g_timer0_cfg, .p_api = &g_timer_on_gpt };
ceu_instance_ctrl_t g_ceu_qvga_ctrl;
const ceu_extended_cfg_t g_ceu_qvga_extended_cfg = { .capture_format =
		CEU_CAPTURE_FORMAT_DATA_SYNCHRONOUS, .input_order =
		CEU_INPUT_ORDER_CB0Y0CR0Y1, .output_format = CEU_OUTPUT_FORMAT_YCBCR422,
		.data_bus_width = CEU_DATA_BUS_SIZE_8_BIT, .edge_info.dsel = 0,
		.edge_info.hdsel = 0, .edge_info.vdsel = 0, .hsync_polarity =
				CEU_HSYNC_POLARITY_HIGH, .vsync_polarity =
				CEU_VSYNC_POLARITY_HIGH, .byte_swapping = { .swap_8bit_units =
				(0x1 | 0x2 | 0x4 | 0x0) >> 0x00 & 0x01, .swap_16bit_units = (0x1
				| 0x2 | 0x4 | 0x0) >> 0x01 & 0x01, .swap_32bit_units = (0x1
				| 0x2 | 0x4 | 0x0) >> 0x02 & 0x01, }, .burst_mode =
				CEU_BURST_TRANSFER_MODE_X8, .scale_down_factor = 0x0U,
		.h_output_size = 0, .v_output_size = 0,
		.image_area_size = 320 * 240 * 2, .interrupts_enabled = 0
				| R_CEU_CEIER_CPEIE_Msk | 0 | R_CEU_CEIER_VDIE_Msk
				| R_CEU_CEIER_CDTOFIE_Msk | 0 | 0 | R_CEU_CEIER_VBPIE_Msk
				| R_CEU_CEIER_NHDIE_Msk | R_CEU_CEIER_NVDIE_Msk,
		.ceu_ipl = (10), .ceu_irq = VECTOR_NUMBER_CEU_CEUI, };

const capture_cfg_t g_ceu_qvga_cfg = { .x_capture_pixels = 320,
		.y_capture_pixels = 240, .x_capture_start_pixel = 0,
		.y_capture_start_pixel = 0, .bytes_per_pixel = 2, .p_callback =
				g_ceu_callback, .p_context = (void*) NULL, .p_extend =
				&g_ceu_qvga_extended_cfg, };

const capture_instance_t g_ceu_qvga = { .p_ctrl = &g_ceu_qvga_ctrl, .p_cfg =
		&g_ceu_qvga_cfg, .p_api = &g_ceu_on_capture, };

dmac_instance_ctrl_t g_sdmmc0_transfer_ctrl;
transfer_info_t g_sdmmc0_transfer_info =
		{ .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
				.transfer_settings_word_b.repeat_area =
						TRANSFER_REPEAT_AREA_SOURCE,
				.transfer_settings_word_b.irq = TRANSFER_IRQ_END,
				.transfer_settings_word_b.chain_mode =
						TRANSFER_CHAIN_MODE_DISABLED,
				.transfer_settings_word_b.src_addr_mode =
						TRANSFER_ADDR_MODE_INCREMENTED,
				.transfer_settings_word_b.size = TRANSFER_SIZE_4_BYTE,
				.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL, .p_dest =
						(void*) NULL, .p_src = (void const*) NULL, .num_blocks =
						0, .length = 128, };
const dmac_extended_cfg_t g_sdmmc0_transfer_extend = { .offset = 1,
		.src_buffer_size = 1,
#if defined(VECTOR_NUMBER_DMAC0_INT)
    .irq                 = VECTOR_NUMBER_DMAC0_INT,
#else
		.irq = FSP_INVALID_VECTOR,
#endif
		.ipl = (11), .channel = 0, .p_callback = g_sdmmc0_dmac_callback,
		.p_context = &g_sdmmc0_ctrl, .activation_source =
				ELC_EVENT_SDHIMMC0_DMA_REQ, };
const transfer_cfg_t g_sdmmc0_transfer_cfg =
		{ .p_info = &g_sdmmc0_transfer_info, .p_extend =
				&g_sdmmc0_transfer_extend, };
/* Instance structure to use this module. */
const transfer_instance_t g_sdmmc0_transfer = { .p_ctrl =
		&g_sdmmc0_transfer_ctrl, .p_cfg = &g_sdmmc0_transfer_cfg, .p_api =
		&g_transfer_on_dmac };
#define RA_NOT_DEFINED (UINT32_MAX)
#if (RA_NOT_DEFINED) != (1)

/* If the transfer module is DMAC, define a DMAC transfer callback. */
#include "r_dmac.h"
extern void r_sdhi_transfer_callback(sdhi_instance_ctrl_t *p_ctrl);

void g_sdmmc0_dmac_callback(dmac_callback_args_t *p_args) {
	r_sdhi_transfer_callback((sdhi_instance_ctrl_t*) p_args->p_context);
}
#endif
#undef RA_NOT_DEFINED

sdhi_instance_ctrl_t g_sdmmc0_ctrl;
sdmmc_cfg_t g_sdmmc0_cfg = { .bus_width = SDMMC_BUS_WIDTH_1_BIT, .channel = 0,
		.p_callback = dataset_storage_sdhi_callback, .p_context = NULL,
		.block_size = 512, .card_detect = SDMMC_CARD_DETECT_CD, .write_protect =
				SDMMC_WRITE_PROTECT_NONE,

		.p_extend = NULL, .p_lower_lvl_transfer = &g_sdmmc0_transfer,

		.access_ipl = (11), .sdio_ipl = BSP_IRQ_DISABLED, .card_ipl = (11),
		.dma_req_ipl = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_SDHIMMC0_ACCS)
    .access_irq             = VECTOR_NUMBER_SDHIMMC0_ACCS,
#else
		.access_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SDHIMMC0_CARD)
    .card_irq               = VECTOR_NUMBER_SDHIMMC0_CARD,
#else
		.card_irq = FSP_INVALID_VECTOR,
#endif
		.sdio_irq = FSP_INVALID_VECTOR,
#if defined(VECTOR_NUMBER_SDHIMMC0_DMA_REQ)
    .dma_req_irq            = VECTOR_NUMBER_SDHIMMC0_DMA_REQ,
#else
		.dma_req_irq = FSP_INVALID_VECTOR,
#endif
		};
/* Instance structure to use this module. */
const sdmmc_instance_t g_sdmmc0 = { .p_ctrl = &g_sdmmc0_ctrl, .p_cfg =
		&g_sdmmc0_cfg, .p_api = &g_sdmmc_on_sdhi };
void g_hal_init(void) {
	g_common_init();
}
