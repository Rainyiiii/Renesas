/*
 * OV5640 摄像头驱动 - SCCB（软件 I2C）+ DVP 控制
 * 初始化寄存器表来自 ALIENTEK ATK-MC5640（已验证可用）
 * 输出：RGB565，QVGA 320x240
 *
 * 引脚映射：
 *   SCCB SCL: P602    OV5640 RST:  P000
 *   SCCB SDA: P603    OV5640 PWDN: P001
 */

#include "hal_data.h"
#include "ov5640.h"

/* SCCB 软件 I2C 引脚 */
#define SCCB_SCL_PORT   6
#define SCCB_SCL_PIN    2
#define SCCB_SDA_PORT   6
#define SCCB_SDA_PIN    3

/* 控制引脚 */
#define RST_PORT        0
#define RST_PIN         0
#define PWDN_PORT       0
#define PWDN_PIN        1

/* OV5640 I2C 地址（7 位：0x3C，8 位写地址：0x78） */
#define OV5640_ADDR_W  0x78
#define OV5640_ADDR_R  0x79

/* 早期的飞线原型板能够承受更快的 0x21/0x46 PLL 时钟。
 * 在 PCB 上优先使用模组厂商的 15 FPS RGB565 时钟；这样能为
 * PCLK 和八条 DVP 数据线提供更充裕的建立/保持时间裕量。 */
#define OV5640_PLL_CTRL1_PCB_SAFE  0x41U
#define OV5640_PLL_CTRL2_PCB_SAFE  0x69U

/* GPIO 宏 — RA8 PORT 寄存器偏移：
 * 0x00 = PDR（方向），0x02 = PODR（输出），0x04 = PIDR（输入） */
#define PODR(port)  (*(volatile uint16_t *)((uint32_t)R_PORT0_BASE + (port) * 0x20UL + 0x02UL))
#define PIDR(port)  (*(volatile uint16_t *)((uint32_t)R_PORT0_BASE + (port) * 0x20UL + 0x04UL))

#define PFS_PmnPFS(port, pin)  (R_PFS->PORT[(port)].PIN[(pin)].PmnPFS)
#define PWPR_REG               (R_PMISC->PWPR)

static void gpio_set(uint8_t port, uint8_t pin)   { PODR(port) |=  (1U << pin); }
static void gpio_clr(uint8_t port, uint8_t pin)   { PODR(port) &= ~(1U << pin); }

/* 将 RST (P000) 和 PWDN (P001) 配置为 GPIO 输出。
 * 必须显式配置 — 不能依赖 R_IOPORT_Open + pin_data.c，
 * 因为 FSP Generate 可能会删除/覆盖引脚配置。 */
static void control_pins_init(void)
{
    PWPR_REG = 0x00;  PWPR_REG = 0x40;
    PFS_PmnPFS(RST_PORT, RST_PIN)  = 0x00000004UL;  /* GPIO 输出 */
    PFS_PmnPFS(PWDN_PORT, PWDN_PIN) = 0x00000004UL;  /* GPIO 输出 */
    PWPR_REG = 0x00;  PWPR_REG = 0x80;
    /* 默认状态：RST=0（复位有效），PWDN=1（掉电） */
    gpio_clr(RST_PORT, RST_PIN);
    gpio_set(PWDN_PORT, PWDN_PIN);
}

static void delay_ms(uint32_t ms)
{
    R_BSP_SoftwareDelay(ms, BSP_DELAY_UNITS_MILLISECONDS);
}

/* ================================================================
 * SCCB（软件 I2C）实现
 * ================================================================ */

static void sccb_gpio_init(void)
{
    PWPR_REG = 0x00;  PWPR_REG = 0x40;
    PFS_PmnPFS(SCCB_SDA_PORT, SCCB_SDA_PIN) = 0x00000054UL;  /* GPIO 输出 + 上拉 + 开漏 */
    PFS_PmnPFS(SCCB_SCL_PORT, SCCB_SCL_PIN) = 0x00000054UL;
    PWPR_REG = 0x00;  PWPR_REG = 0x80;
    gpio_set(SCCB_SDA_PORT, SCCB_SDA_PIN);
    gpio_set(SCCB_SCL_PORT, SCCB_SCL_PIN);
}

static void sccb_delay(void)
{
    /* 在无源扩展 PCB 上刻意让 SCCB 保持较慢的速度。GPIO 内部
     * 上拉较弱，因此更长的高电平时间能让 SDA/SCL 即便在存在
     * 连接器和走线电容的情况下也有足够的时间拉高。 */
    R_BSP_SoftwareDelay(10U, BSP_DELAY_UNITS_MICROSECONDS);
}

static void sccb_start(void) {
    gpio_set(SCCB_SDA_PORT, SCCB_SDA_PIN);
    gpio_set(SCCB_SCL_PORT, SCCB_SCL_PIN);
    sccb_delay();
    gpio_clr(SCCB_SDA_PORT, SCCB_SDA_PIN);
    sccb_delay();
    gpio_clr(SCCB_SCL_PORT, SCCB_SCL_PIN);
}

static void sccb_stop(void) {
    gpio_clr(SCCB_SDA_PORT, SCCB_SDA_PIN);
    gpio_set(SCCB_SCL_PORT, SCCB_SCL_PIN);
    sccb_delay();
    gpio_set(SCCB_SDA_PORT, SCCB_SDA_PIN);
    sccb_delay();
}

static void sccb_bus_recover(void)
{
    /* 释放 SDA 并发出时钟脉冲，以恢复上电期间可能在传输某字节
     * 中途被打断的从机。 */
    gpio_set(SCCB_SDA_PORT, SCCB_SDA_PIN);
    gpio_set(SCCB_SCL_PORT, SCCB_SCL_PIN);
    sccb_delay();
    for (uint8_t pulse = 0U; pulse < 9U; pulse++) {
        gpio_clr(SCCB_SCL_PORT, SCCB_SCL_PIN);
        sccb_delay();
        gpio_set(SCCB_SCL_PORT, SCCB_SCL_PIN);
        sccb_delay();
    }
    sccb_stop();
}

static int sccb_send_byte(uint8_t data) {
    for (int i = 7; i >= 0; i--) {
        if (data & (1U << i))
            gpio_set(SCCB_SDA_PORT, SCCB_SDA_PIN);
        else
            gpio_clr(SCCB_SDA_PORT, SCCB_SDA_PIN);
        sccb_delay();
        gpio_set(SCCB_SCL_PORT, SCCB_SCL_PIN);
        sccb_delay();
        gpio_clr(SCCB_SCL_PORT, SCCB_SCL_PIN);
    }
    gpio_set(SCCB_SDA_PORT, SCCB_SDA_PIN);
    sccb_delay();
    gpio_set(SCCB_SCL_PORT, SCCB_SCL_PIN);
    sccb_delay();
    int ack = (PIDR(SCCB_SDA_PORT) >> SCCB_SDA_PIN) & 1U;
    gpio_clr(SCCB_SCL_PORT, SCCB_SCL_PIN);
    return ack;
}

static uint8_t sccb_read_byte(int ack) {
    uint8_t data = 0;
    gpio_set(SCCB_SDA_PORT, SCCB_SDA_PIN);
    for (int i = 7; i >= 0; i--) {
        gpio_set(SCCB_SCL_PORT, SCCB_SCL_PIN);
        sccb_delay();
        if (PIDR(SCCB_SDA_PORT) & (1U << SCCB_SDA_PIN))
            data |= (1U << i);
        gpio_clr(SCCB_SCL_PORT, SCCB_SCL_PIN);
        sccb_delay();
    }
    if (ack) gpio_clr(SCCB_SDA_PORT, SCCB_SDA_PIN);
    else     gpio_set(SCCB_SDA_PORT, SCCB_SDA_PIN);
    gpio_set(SCCB_SCL_PORT, SCCB_SCL_PIN);
    sccb_delay();
    gpio_clr(SCCB_SCL_PORT, SCCB_SCL_PIN);
    return data;
}

static int ov5640_write_reg(uint16_t reg, uint8_t val) {
    int ret;
    sccb_start();
    ret = sccb_send_byte(OV5640_ADDR_W);  if (ret) goto err;
    ret = sccb_send_byte((uint8_t)(reg >> 8));  if (ret) goto err;
    ret = sccb_send_byte((uint8_t)(reg & 0xFF)); if (ret) goto err;
    ret = sccb_send_byte(val);
err:
    sccb_stop();
    return ret;
}

static int ov5640_read_reg(uint16_t reg, uint8_t *val) {
    int ret;
    sccb_start();
    ret = sccb_send_byte(OV5640_ADDR_W);  if (ret) goto err;
    ret = sccb_send_byte((uint8_t)(reg >> 8));  if (ret) goto err;
    ret = sccb_send_byte((uint8_t)(reg & 0xFF)); if (ret) goto err;
    sccb_stop();
    sccb_start();
    ret = sccb_send_byte(OV5640_ADDR_R);  if (ret) goto err;
    *val = sccb_read_byte(0);
err:
    sccb_stop();
    return ret;
}

/* ================================================================
 * 硬件复位 — 依据官方指南第 3.1.1 节
 * ================================================================ */
static void ov5640_hw_reset(void) {
    gpio_clr(RST_PORT, RST_PIN);
    gpio_set(PWDN_PORT, PWDN_PIN);
    delay_ms(10);
    gpio_clr(PWDN_PORT, PWDN_PIN);
    delay_ms(1);
    gpio_set(RST_PORT, RST_PIN);
    delay_ms(20);
}

/* ================================================================
 * OV5640 初始化寄存器表
 * 来自 ALIENTEK ATK-MC5640（已验证可用）
 * ================================================================ */
static const struct { uint16_t reg; uint8_t val; } ov5640_init_regs[] = {
    /* ---- 软件掉电 ---- */
    {0x3008, 0x42},
    {0x3103, 0x03},
    {0x3017, 0xFF},
    {0x3018, 0xFF},
    {0x3034, 0x1A},
    {0x3037, 0x13},
    {0x3108, 0x01},

    /* ---- 模拟部分初始化 ---- */
    {0x3630, 0x36},
    {0x3631, 0x0E},
    {0x3632, 0xE2},
    {0x3633, 0x12},
    {0x3621, 0xE0},
    {0x3704, 0xA0},
    {0x3703, 0x5A},
    {0x3715, 0x78},
    {0x3717, 0x01},
    {0x370B, 0x60},
    {0x3705, 0x1A},
    {0x3905, 0x02},
    {0x3906, 0x10},
    {0x3901, 0x0A},
    {0x3731, 0x12},
    {0x3600, 0x08},
    {0x3601, 0x33},
    {0x302D, 0x60},
    {0x3620, 0x52},
    {0x371B, 0x20},
    {0x471C, 0x50},
    {0x3A13, 0x43},
    {0x3A18, 0x00},
    {0x3A19, 0xF8},
    {0x3635, 0x13},
    {0x3636, 0x03},
    {0x3634, 0x40},
    {0x3622, 0x01},

    /* ---- 50/60Hz 检测 ---- */
    {0x3C01, 0x34},
    {0x3C04, 0x28},
    {0x3C05, 0x98},
    {0x3C06, 0x00},
    {0x3C07, 0x08},
    {0x3C08, 0x00},
    {0x3C09, 0x1C},
    {0x3C0A, 0x9C},
    {0x3C0B, 0x40},

    /* ---- 时序偏移 ---- */
    {0x3810, 0x00},
    {0x3811, 0x10},
    {0x3812, 0x00},
    {0x3708, 0x64},

    /* ---- BLC（黑电平校正） ---- */
    {0x4001, 0x02},
    {0x4005, 0x1A},

    /* ---- 时钟使能 ---- */
    {0x3000, 0x00},
    {0x3004, 0xFF},

    /* ---- 使能 DVP（禁用 MIPI） ---- */
    {0x300E, 0x58},
    {0x302E, 0x00},

    /* ---- 输出格式：RGB565 ---- */
    {0x4300, 0x6F},   /* RGB565 输出 */
    {0x501F, 0x01},   /* RGB565 模式 */
    {0x440E, 0x00},
    {0x5000, 0xA7},

    /* ---- AEC 目标值 ---- */
    {0x3A0F, 0x30},
    {0x3A10, 0x28},
    {0x3A1B, 0x30},
    {0x3A1E, 0x26},
    {0x3A11, 0x60},
    {0x3A1F, 0x14},

    /* ---- 镜头校正 ---- */
    {0x5800, 0x23}, {0x5801, 0x14}, {0x5802, 0x0F}, {0x5803, 0x0F},
    {0x5804, 0x12}, {0x5805, 0x26}, {0x5806, 0x0C}, {0x5807, 0x08},
    {0x5808, 0x05}, {0x5809, 0x05}, {0x580A, 0x08}, {0x580B, 0x0D},
    {0x580C, 0x08}, {0x580D, 0x03}, {0x580E, 0x00}, {0x580F, 0x00},
    {0x5810, 0x03}, {0x5811, 0x09}, {0x5812, 0x07}, {0x5813, 0x03},
    {0x5814, 0x00}, {0x5815, 0x01}, {0x5816, 0x03}, {0x5817, 0x08},
    {0x5818, 0x0D}, {0x5819, 0x08}, {0x581A, 0x05}, {0x581B, 0x06},
    {0x581C, 0x08}, {0x581D, 0x0E}, {0x581E, 0x29}, {0x581F, 0x17},
    {0x5820, 0x11}, {0x5821, 0x11}, {0x5822, 0x15}, {0x5823, 0x28},
    {0x5824, 0x46}, {0x5825, 0x26}, {0x5826, 0x08}, {0x5827, 0x26},
    {0x5828, 0x64}, {0x5829, 0x26}, {0x582A, 0x24}, {0x582B, 0x22},
    {0x582C, 0x24}, {0x582D, 0x24}, {0x582E, 0x06}, {0x582F, 0x22},
    {0x5830, 0x40}, {0x5831, 0x42}, {0x5832, 0x24}, {0x5833, 0x26},
    {0x5834, 0x24}, {0x5835, 0x22}, {0x5836, 0x22}, {0x5837, 0x26},
    {0x5838, 0x44}, {0x5839, 0x24}, {0x583A, 0x26}, {0x583B, 0x28},
    {0x583C, 0x42}, {0x583D, 0xCE},

    /* ---- AWB ---- */
    {0x5180, 0xFF}, {0x5181, 0xF2}, {0x5182, 0x00}, {0x5183, 0x14},
    {0x5184, 0x25}, {0x5185, 0x24}, {0x5186, 0x09}, {0x5187, 0x09},
    {0x5188, 0x09}, {0x5189, 0x75}, {0x518A, 0x54}, {0x518B, 0xE0},
    {0x518C, 0xB2}, {0x518D, 0x42}, {0x518E, 0x3D}, {0x518F, 0x56},
    {0x5190, 0x46}, {0x5191, 0xF8}, {0x5192, 0x04}, {0x5193, 0x70},
    {0x5194, 0xF0}, {0x5195, 0xF0}, {0x5196, 0x03}, {0x5197, 0x01},
    {0x5198, 0x04}, {0x5199, 0x12}, {0x519A, 0x04}, {0x519B, 0x00},
    {0x519C, 0x06}, {0x519D, 0x82}, {0x519E, 0x38},

    /* ---- Gamma（伽马校正） ---- */
    {0x5480, 0x01}, {0x5481, 0x08}, {0x5482, 0x14}, {0x5483, 0x28},
    {0x5484, 0x51}, {0x5485, 0x65}, {0x5486, 0x71}, {0x5487, 0x7D},
    {0x5488, 0x87}, {0x5489, 0x91}, {0x548A, 0x9A}, {0x548B, 0xAA},
    {0x548C, 0xB8}, {0x548D, 0xCD}, {0x548E, 0xDD}, {0x548F, 0xEA},
    {0x5490, 0x1D},

    /* ---- 色彩矩阵 ---- */
    {0x5381, 0x1E}, {0x5382, 0x5B}, {0x5383, 0x08}, {0x5384, 0x0A},
    {0x5385, 0x7E}, {0x5386, 0x88}, {0x5387, 0x7C}, {0x5388, 0x6C},
    {0x5389, 0x10}, {0x538A, 0x01}, {0x538B, 0x98},

    /* ---- UV 调整 ---- */
    {0x5580, 0x06}, {0x5583, 0x40}, {0x5584, 0x10},
    {0x5589, 0x10}, {0x558A, 0x00}, {0x558B, 0xF8},

    /* ---- 对比度 ---- */
    {0x501D, 0x40},

    /* ---- CIP（锐化 + 降噪） ---- */
    {0x5300, 0x08}, {0x5301, 0x30}, {0x5302, 0x10}, {0x5303, 0x00},
    {0x5304, 0x08}, {0x5305, 0x30}, {0x5306, 0x08}, {0x5307, 0x16},
    {0x5309, 0x08}, {0x530A, 0x30}, {0x530B, 0x04}, {0x530C, 0x06},
    {0x5025, 0x00},

    /* ============================================================
     * QVGA 320x240 + RGB565 分辨率设置
     * ============================================================ */

    /* ---- QVGA 的 PLL 配置（厂商 15 FPS 时序，为 PCB 信号裕量考虑） ---- */
    {0x3035, OV5640_PLL_CTRL1_PCB_SAFE},
    {0x3036, OV5640_PLL_CTRL2_PCB_SAFE},

    {0x3820, 0x46},
    {0x3821, 0x00},

    /* ---- X/Y 方向增量（INC） ---- */
    {0x3814, 0x31},
    {0x3815, 0x31},

    /* ---- 窗口：HS=0, VS=4 ---- */
    {0x3800, 0x00}, {0x3801, 0x00},
    {0x3802, 0x00}, {0x3803, 0x04},
    {0x3804, 0x0A}, {0x3805, 0x3F},
    {0x3806, 0x07}, {0x3807, 0x9B},

    /* ---- 输出尺寸：320x240 ---- */
    {0x3808, 0x01}, {0x3809, 0x40},
    {0x380A, 0x00}, {0x380B, 0xF0},

    /* ---- 总尺寸 ---- */
    {0x380C, 0x07}, {0x380D, 0x68},
    {0x380E, 0x03}, {0x380F, 0xE8},

    /* ---- 垂直方向偏移 ---- */
    {0x3813, 0x06},

    /* ---- 分辨率相关的模拟配置 ---- */
    {0x3618, 0x00},
    {0x3612, 0x29},
    {0x3709, 0x52},
    {0x370C, 0x03},

    /* ---- 最大曝光 ---- */
    {0x3A02, 0x0B}, {0x3A03, 0x88},
    {0x3A14, 0x0B}, {0x3A15, 0x88},

    /* ---- BLC（黑电平校正） ---- */
    {0x4004, 0x02},

    /* ---- 禁用 JPEG ---- */
    {0x3002, 0x1C},
    {0x3006, 0xC3},
    {0x4713, 0x03},
    {0x4407, 0x04},
    {0x460B, 0x37},
    {0x460C, 0x20},

    /* ---- DVP 时钟 ---- */
    {0x4837, 0x16},
    {0x3824, 0x04},

    /* ---- ISP 控制 ---- */
    {0x5001, 0xA3},
    {0x3503, 0x00},

    /* ---- 测光 ---- */
    {0x3C07, 0x08},

    /* ---- 从待机唤醒 ---- */
    {0x3008, 0x02},
    {0x4740, 0x21},
};

/* ================================================================
 * 公共 API
 * ================================================================ */

int ov5640_init(void) {
    control_pins_init();
    sccb_gpio_init();
    /* 保持传感器处于非工作状态，直到 3.3 V 电源轨和模组 LDO 稳定。 */
    delay_ms(50U);
    ov5640_hw_reset();
    sccb_bus_recover();

    /* 校验 chip id（0x300A=0x56，0x300B=0x40） */
    uint8_t id_high = 0U;
    uint8_t id_low = 0U;
    int id_read_error = -1;
    for (uint8_t attempt = 0U; attempt < 3U; attempt++) {
        id_read_error = ov5640_read_reg(0x300A, &id_high) ? -1 : 0;
        if (0 == id_read_error) {
            id_read_error = ov5640_read_reg(0x300B, &id_low) ? -2 : 0;
        }
        if (0 == id_read_error) {
            break;
        }
        sccb_bus_recover();
        delay_ms(10U);
    }
    if (0 != id_read_error) return id_read_error;
    if (id_high != 0x56 || id_low != 0x40) return -3;

    /* 写入初始化寄存器表 */
    delay_ms(5);
    int n = (int)(sizeof(ov5640_init_regs) / sizeof(ov5640_init_regs[0]));
    for (int i = 0; i < n; i++) {
        if (ov5640_write_reg(ov5640_init_regs[i].reg, ov5640_init_regs[i].val))
            return -4 - i;
    }

    delay_ms(100);
    return 0;
}

int ov5640_set_resolution(uint8_t mode) {
    (void)mode;
    return 0;
}

int ov5640_set_test_pattern(uint8_t pattern) {
    uint8_t value = 0x00U;

    if (1U == pattern) {
        value = 0x80U;
    } else if (2U == pattern) {
        value = 0x82U;
    } else if (0U != pattern) {
        return -1;
    }

    if (ov5640_write_reg(0x503D, value)) {
        return -2;
    }
    return ov5640_write_reg(0x4741, 0x00U);
}
