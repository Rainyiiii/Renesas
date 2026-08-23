#include "machine_control.h"
#include "r_gpt.h"
#include <string.h>

/* 最终扩展板引脚映射。电机驱动器的使能由板外单独处理。 */
#define PIN_STEPPER_PRIMARY_DIR  BSP_IO_PORT_06_PIN_00
#define PIN_STEPPER_BACKUP_DIR   BSP_IO_PORT_01_PIN_06

#define SERVO_PERIOD_COUNTS      (5000000UL)  /* 在 250 MHz PCLKD 下对应 20 ms。 */
#define SERVO_MIN_COUNTS         (125000UL)   /* 0.5 ms。 */
#define SERVO_MAX_COUNTS         (625000UL)   /* 2.5 ms。 */
#define SERVO_MAX_ANGLE          (270U)

/* ==================== 360° 连续舵机参数（现场标定区） ====================
 * j_servo1/j_servo2 为 360° 连续旋转舵机：脉宽只决定【转向和转速】，
 * 不决定角度，舵机自己不知道转到哪了。因此：
 *   推料 = 朝目标方向定速旋转 SERVO_PUSH_MS
 *   回位 = 反向旋转 SERVO_RETURN_MS（显式转回来，不能只发停转脉宽）
 *   结束 = 输出停转脉宽（*_NEUTRAL_ANGLE）
 *
 * 方向约定（已按实物确认）：
 *   - j_servo1：加大角度值（+）= 向【右】推
 *   - j_servo2：加大角度值（+）= 向【左】推
 *
 * 标定方法（按重要性排序）：
 *   1. 停止状态拨杆仍在慢转 → 微调该舵机 *_NEUTRAL_ANGLE（一次 1~3，
 *      直到完全停住。这是回位准不准的根基，最优先调！）
 *   2. 推料行程不够/过头 → 调 SERVO_PUSH_MS（时长）或 SERVO_PUSH_SPEED（转速）
 *   3. 回位后停的位置偏 → 单独微调 SERVO_RETURN_MS */
#define SERVO1_NEUTRAL_ANGLE       (135U) /* j_servo1 停转点 */
#define SERVO2_NEUTRAL_ANGLE       (135U) /* j_servo2 停转点 */
#define SERVO_BACKUP_NEUTRAL_ANGLE (135U) /* 备用舵机停转点 */
#define SERVO_PUSH_SPEED           (60U)  /* 推料转速：偏离停转点的量（1~135），越大越快 */

#define GPT_CLOCK_HZ             (250000000UL)
#define STEPPER_DEFAULT_HZ       (900U)
#define STEPPER_REVERSE_HZ       (900U)  /* 倒转输送速度，当前与正转一致。
                                          * 若倒转带载出现堵转（电机嗡嗡响不走），再调低换扭矩。 */
#define DETECTION_MIN_SCORE      (0.45f)
#define DETECTION_STABLE_TICKS   (3U)
#define CLEAR_DETECTION_TICKS    (4U)
#define SERVO_PUSH_MS            (1400U) /* 推料旋转时长（原 600ms 按现场实测加大 3 倍）。 */
#define SERVO_RETURN_MS          (1400U) /* 回位反转时长。理论上与推料时长相同；
                                          * 若回位后停偏，单独微调这个值。 */
#define SERVO_RETURN_WAIT_MS     (250U)

/* ==================== 分拣路由配置（现场标定区） ====================
 * 机械布局：摄像头位于传送带中间，两侧各有一个推料舵机。
 *   - j_servo1（P915，MACHINE_SERVO_LEFT）装在摄像头前方 = 传送带前进方向的下游
 *   - j_servo2（P914，MACHINE_SERVO_RIGHT）装在摄像头后方 = 上游
 * 送料方式：
 *   - 去前舵机的格子：识别确认后传送带继续【正转】
 *   - 去后舵机的格子：识别确认后传送带改为【倒转】
 * 到位后停带，舵机朝 push_left 指定一侧旋转把物料推进左格或右格，
 * 再反向转回原位停住，然后传送带恢复正转等待下一件。
 *
 * ==================== 四个类别输送时间修改区 ====================
 * 下面四个数就是每个类别从【黄色判定线】运动到对应舵机位置的时间。
 * 单位为毫秒：1000U = 1 秒，数值增大则物料运动得更远，数值减小则更近。
 * 这里只改变步进电机的运行时间，不会改变类别名称、舵机方向或识别结果。
 * 调试时建议每次增加或减少 200U~500U，再观察物料最终停靠位置。 */
#define TRAVEL_HARM_MS           (12000U) /* 有害垃圾 HARM：到前舵机 j_servo1 的正转时间 */
#define TRAVEL_KITCHEN_MS        (4000U)  /* 厨余垃圾 KITCHEN：到后舵机 j_servo2 的倒转时间 */
#define TRAVEL_OTHER_MS          (4000U)  /* 其他垃圾 OTHER：到后舵机 j_servo2 的倒转时间 */
#define TRAVEL_RECOVER_MS        (12000U) /* 可回收垃圾 RECOVER：到前舵机 j_servo1 的正转时间 */

/* ===== 视觉对齐（提高输送精度的关键） =====
 * 识别确认后不再立刻定时输送，而是先低速正转，把识别框中心送到画面
 * 参考线上，再从参考线开始计时。这样每次输送的起点固定，固定时间
 * 才能对应固定距离。
 * 画面方向（已按实物确认）：传送带正转时识别框【从下往上】移动，
 * 即 Y 坐标减小；对齐判定为 center_y <= MACHINE_ALIGN_TARGET_Y（屏幕上有黄色虚线）。 */
#define ALIGN_STEP_HZ            (450U)  /* 对齐阶段低速逼近，速度越低越准 */
/* 判定线坐标 MACHINE_ALIGN_TARGET_Y 在 machine_control.h 中定义（屏幕上有黄色虚线显示）。 */
#define ALIGN_TOLERANCE_PX       (12U)   /* 双向收敛容差：中心落在判定线±此范围内才算对齐完成。
                                          * 冲过头会自动倒回来，保证物体最终一定停在线上。 */
#define ALIGN_TIMEOUT_MS         (5000U) /* 对齐超时兜底：到时按当前位置继续输送 */
#define ALIGN_LOST_TICKS_MAX     (6U)    /* 对齐中连续丢失目标这么多帧则放弃，回到等待 */
#define TRANSPORT_SETTLE_MS      (200U)  /* 输送起步前先停稳的时间：对齐结束后皮带可能
                                          * 刚在正转或倒转，统一停稳再按目标方向起步，防丢步 */

typedef struct st_sort_route
{
    uint8_t rear;      /* 0 = 前舵机 j_servo1；1 = 后舵机 j_servo2（需倒带送料） */
    uint8_t push_left; /* 1 = 推向该舵机的【左】格；0 = 推向【右】格。
                        * 左右按面向传送带前进方向定义；
                        * 类别进错格子时改这个标志位即可对调。 */
    uint32_t travel_ms;/* 从黄色判定线到该类别舵机位置的步进运行时间，单位毫秒。 */
} sort_route_t;

/* 类别顺序：0=HARM 有害, 1=KITCHEN 厨余, 2=OTHER 其他, 3=RECOVER 可回收。
 * 默认按现场接线指南分组：j_servo1 分拣 HARM/RECOVER，j_servo2 分拣 KITCHEN/OTHER。
 * 料斗位置定下来后，直接改下面 8 个数字即可完成任意"类别 → 格子"映射。 */
static const sort_route_t g_sort_routes[4] =
{
    [0] = { .rear = 0U, .push_left = 1U, .travel_ms = TRAVEL_HARM_MS    }, /* HARM    → 前舵机，左格 */
    [1] = { .rear = 1U, .push_left = 1U, .travel_ms = TRAVEL_KITCHEN_MS }, /* KITCHEN → 后舵机，左格 */
    [2] = { .rear = 1U, .push_left = 0U, .travel_ms = TRAVEL_OTHER_MS   }, /* OTHER   → 后舵机，右格 */
    [3] = { .rear = 0U, .push_left = 0U, .travel_ms = TRAVEL_RECOVER_MS }, /* RECOVER → 前舵机，右格 */
};

static gpt_instance_ctrl_t g_servo_pair_ctrl;
static gpt_instance_ctrl_t g_servo_backup_ctrl;
static gpt_instance_ctrl_t g_stepper_primary_ctrl;
static gpt_instance_ctrl_t g_stepper_backup_ctrl;

static const gpt_extended_cfg_t g_servo_pair_extend =
{
    .gtioca = {.output_enabled = true, .stop_level = GPT_PIN_LEVEL_LOW},
    .gtiocb = {.output_enabled = true, .stop_level = GPT_PIN_LEVEL_LOW},
    .capture_a_ipl = BSP_IRQ_DISABLED,
    .capture_b_ipl = BSP_IRQ_DISABLED,
    .compare_match_c_ipl = BSP_IRQ_DISABLED,
    .compare_match_d_ipl = BSP_IRQ_DISABLED,
    .compare_match_e_ipl = BSP_IRQ_DISABLED,
    .compare_match_f_ipl = BSP_IRQ_DISABLED,
    .capture_a_irq = FSP_INVALID_VECTOR,
    .capture_b_irq = FSP_INVALID_VECTOR,
    .compare_match_c_irq = FSP_INVALID_VECTOR,
    .compare_match_d_irq = FSP_INVALID_VECTOR,
    .compare_match_e_irq = FSP_INVALID_VECTOR,
    .compare_match_f_irq = FSP_INVALID_VECTOR,
    .capture_filter_gtioca = GPT_CAPTURE_FILTER_NONE,
    .capture_filter_gtiocb = GPT_CAPTURE_FILTER_NONE,
    .gtioca_polarity = GPT_GTIOC_POLARITY_NORMAL,
    .gtiocb_polarity = GPT_GTIOC_POLARITY_NORMAL,
};

static const gpt_extended_cfg_t g_single_output_extend =
{
    .gtioca = {.output_enabled = true, .stop_level = GPT_PIN_LEVEL_LOW},
    .gtiocb = {.output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW},
    .capture_a_ipl = BSP_IRQ_DISABLED,
    .capture_b_ipl = BSP_IRQ_DISABLED,
    .compare_match_c_ipl = BSP_IRQ_DISABLED,
    .compare_match_d_ipl = BSP_IRQ_DISABLED,
    .compare_match_e_ipl = BSP_IRQ_DISABLED,
    .compare_match_f_ipl = BSP_IRQ_DISABLED,
    .capture_a_irq = FSP_INVALID_VECTOR,
    .capture_b_irq = FSP_INVALID_VECTOR,
    .compare_match_c_irq = FSP_INVALID_VECTOR,
    .compare_match_d_irq = FSP_INVALID_VECTOR,
    .compare_match_e_irq = FSP_INVALID_VECTOR,
    .compare_match_f_irq = FSP_INVALID_VECTOR,
    .capture_filter_gtioca = GPT_CAPTURE_FILTER_NONE,
    .capture_filter_gtiocb = GPT_CAPTURE_FILTER_NONE,
    .gtioca_polarity = GPT_GTIOC_POLARITY_NORMAL,
    .gtiocb_polarity = GPT_GTIOC_POLARITY_NORMAL,
};

static const timer_cfg_t g_servo_pair_cfg =
{
    .mode = TIMER_MODE_PWM,
    .period_counts = SERVO_PERIOD_COUNTS,
    .duty_cycle_counts = SERVO_MIN_COUNTS,
    .source_div = TIMER_SOURCE_DIV_1,
    .channel = 5U,
    .p_extend = &g_servo_pair_extend,
    .cycle_end_ipl = BSP_IRQ_DISABLED,
    .cycle_end_irq = FSP_INVALID_VECTOR,
};

static const timer_cfg_t g_servo_backup_cfg =
{
    .mode = TIMER_MODE_PWM,
    .period_counts = SERVO_PERIOD_COUNTS,
    .duty_cycle_counts = SERVO_MIN_COUNTS,
    .source_div = TIMER_SOURCE_DIV_1,
    .channel = 13U,
    .p_extend = &g_single_output_extend,
    .cycle_end_ipl = BSP_IRQ_DISABLED,
    .cycle_end_irq = FSP_INVALID_VECTOR,
};

static const timer_cfg_t g_stepper_primary_cfg =
{
    .mode = TIMER_MODE_PWM,
    .period_counts = GPT_CLOCK_HZ / STEPPER_DEFAULT_HZ,
    .duty_cycle_counts = GPT_CLOCK_HZ / STEPPER_DEFAULT_HZ / 2U,
    .source_div = TIMER_SOURCE_DIV_1,
    .channel = 7U,
    .p_extend = &g_single_output_extend,
    .cycle_end_ipl = BSP_IRQ_DISABLED,
    .cycle_end_irq = FSP_INVALID_VECTOR,
};

static const timer_cfg_t g_stepper_backup_cfg =
{
    .mode = TIMER_MODE_PWM,
    .period_counts = GPT_CLOCK_HZ / STEPPER_DEFAULT_HZ,
    .duty_cycle_counts = GPT_CLOCK_HZ / STEPPER_DEFAULT_HZ / 2U,
    .source_div = TIMER_SOURCE_DIV_1,
    .channel = 12U,
    .p_extend = &g_single_output_extend,
    .cycle_end_ipl = BSP_IRQ_DISABLED,
    .cycle_end_irq = FSP_INVALID_VECTOR,
};

static machine_status_t g_status;
static machine_stepper_output_t g_stepper_output = MACHINE_STEPPER_PRIMARY;
static machine_servo_output_t g_left_servo_output = MACHINE_SERVO_LEFT;
static machine_servo_output_t g_right_servo_output = MACHINE_SERVO_RIGHT;
static uint8_t g_candidate_class = 0xFFU;
static uint8_t g_candidate_ticks;
static uint8_t g_clear_ticks;
static uint8_t g_align_lost_ticks;
static uint8_t g_align_dir;          /* 对齐中当前皮带指令：0=停 1=正转 2=倒转（避免每帧重发） */
static uint8_t g_transport_settling; /* 1 = 输送起步前的停稳阶段 */
static uint8_t g_initialized;
static uint32_t g_state_start_cycles;
static uint32_t g_cycles_per_ms;

static void output_write(bsp_io_port_pin_t pin, bsp_io_level_t level)
{
    (void) R_IOPORT_PinWrite(&g_ioport_ctrl, pin, level);
}

static uint32_t servo_counts(uint16_t angle)
{
    if (angle > SERVO_MAX_ANGLE)
    {
        angle = SERVO_MAX_ANGLE;
    }

    return SERVO_MIN_COUNTS +
           (((SERVO_MAX_COUNTS - SERVO_MIN_COUNTS) * (uint32_t) angle) / SERVO_MAX_ANGLE);
}

void machine_servo_set(machine_servo_output_t servo, uint16_t angle_degrees)
{
    uint32_t counts = servo_counts(angle_degrees);

    if (MACHINE_SERVO_LEFT == servo)
    {
        (void) R_GPT_DutyCycleSet(&g_servo_pair_ctrl, counts, GPT_IO_PIN_GTIOCA);
    }
    else if (MACHINE_SERVO_RIGHT == servo)
    {
        (void) R_GPT_DutyCycleSet(&g_servo_pair_ctrl, counts, GPT_IO_PIN_GTIOCB);
    }
    else if (MACHINE_SERVO_BACKUP == servo)
    {
        (void) R_GPT_DutyCycleSet(&g_servo_backup_ctrl, counts, GPT_IO_PIN_GTIOCA);
    }
}

/* 全部舵机输出各自的停转脉宽（360° 舵机在停转点上不转动）。 */
static void all_servos_stop(void)
{
    machine_servo_set(MACHINE_SERVO_LEFT, SERVO1_NEUTRAL_ANGLE);
    machine_servo_set(MACHINE_SERVO_RIGHT, SERVO2_NEUTRAL_ANGLE);
    machine_servo_set(MACHINE_SERVO_BACKUP, SERVO_BACKUP_NEUTRAL_ANGLE);
}

bool machine_servo_route_to_backup(machine_servo_output_t failed_primary)
{
    if ((MACHINE_MODE_RUNNING == g_status.mode) ||
        ((MACHINE_SERVO_LEFT != failed_primary) && (MACHINE_SERVO_RIGHT != failed_primary)))
    {
        return false;
    }

    all_servos_stop();
    g_left_servo_output = (MACHINE_SERVO_LEFT == failed_primary) ?
                          MACHINE_SERVO_BACKUP : MACHINE_SERVO_LEFT;
    g_right_servo_output = (MACHINE_SERVO_RIGHT == failed_primary) ?
                           MACHINE_SERVO_BACKUP : MACHINE_SERVO_RIGHT;
    return true;
}

void machine_servo_restore_routes(void)
{
    if (MACHINE_MODE_RUNNING != g_status.mode)
    {
        all_servos_stop();
        g_left_servo_output = MACHINE_SERVO_LEFT;
        g_right_servo_output = MACHINE_SERVO_RIGHT;
    }
}

static void stepper_stop_ctrl(gpt_instance_ctrl_t *ctrl)
{
    (void) R_GPT_Stop(ctrl);
    (void) R_GPT_Reset(ctrl);
}

void machine_stepper_stop(void)
{
    stepper_stop_ctrl(&g_stepper_primary_ctrl);
    stepper_stop_ctrl(&g_stepper_backup_ctrl);
}

void machine_stepper_select(machine_stepper_output_t output)
{
    machine_stepper_stop();
    g_stepper_output = (MACHINE_STEPPER_BACKUP == output) ?
                       MACHINE_STEPPER_BACKUP : MACHINE_STEPPER_PRIMARY;
}

static void stepper_frequency_set(gpt_instance_ctrl_t *ctrl, uint32_t frequency_hz)
{
    if (frequency_hz < 20U)
    {
        frequency_hz = 20U;
    }
    if (frequency_hz > 20000U)
    {
        frequency_hz = 20000U;
    }

    uint32_t period = GPT_CLOCK_HZ / frequency_hz;
    (void) R_GPT_PeriodSet(ctrl, period);
    (void) R_GPT_DutyCycleSet(ctrl, period / 2U, GPT_IO_PIN_GTIOCA);
}

void machine_stepper_run(machine_stepper_direction_t direction, uint32_t step_hz)
{
    machine_stepper_stop();
    bsp_io_level_t direction_level = (MACHINE_STEPPER_REVERSE == direction) ?
                                     BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW;

    if (MACHINE_STEPPER_BACKUP == g_stepper_output)
    {
        output_write(PIN_STEPPER_BACKUP_DIR, direction_level);
        stepper_frequency_set(&g_stepper_backup_ctrl, step_hz);
        (void) R_GPT_Start(&g_stepper_backup_ctrl);
    }
    else
    {
        output_write(PIN_STEPPER_PRIMARY_DIR, direction_level);
        stepper_frequency_set(&g_stepper_primary_ctrl, step_hz);
        (void) R_GPT_Start(&g_stepper_primary_ctrl);
    }
}

static void all_actuators_safe(void)
{
    machine_stepper_stop();
    all_servos_stop();
}

/* 状态计时（毫秒累加器实现）。
 * 旧实现用 "毫秒 × 每毫秒周期数" 与 CYCCNT 差值直接比较，32 位乘积在
 * 时长超过约 4 秒时溢出回绕，表现为：把时间参数改大，实际时长却不变。
 * 现改为每次调用把新增周期折算成毫秒累加，任意时长都正确。 */
static uint32_t g_state_elapsed_ms;

static void sort_state_set(machine_sort_state_t state)
{
    g_status.sort_state = state;
    g_state_start_cycles = DWT->CYCCNT;
    g_state_elapsed_ms = 0U;
}

static uint8_t state_elapsed(uint32_t milliseconds)
{
    uint32_t now = DWT->CYCCNT;
    uint32_t delta = now - g_state_start_cycles; /* 无符号减法天然处理回绕 */
    uint32_t whole_ms = delta / g_cycles_per_ms;
    if (whole_ms > 0U)
    {
        g_state_elapsed_ms += whole_ms;
        g_state_start_cycles += whole_ms * g_cycles_per_ms;
    }
    return (uint8_t) (g_state_elapsed_ms >= milliseconds);
}

/* 根据路由表选择实际输出的舵机（保留备用舵机的改道能力）。 */
static machine_servo_output_t sorting_servo_output(uint8_t class_id)
{
    return g_sort_routes[class_id & 3U].rear ?
           g_right_servo_output : g_left_servo_output;
}

/* 该类别所用舵机的停转脉宽。 */
static uint16_t sorting_neutral_angle(uint8_t class_id)
{
    return g_sort_routes[class_id & 3U].rear ?
           SERVO2_NEUTRAL_ANGLE : SERVO1_NEUTRAL_ANGLE;
}

static uint16_t servo_angle_clamp(int32_t angle)
{
    if (angle < 0)
    {
        angle = 0;
    }
    if (angle > (int32_t) SERVO_MAX_ANGLE)
    {
        angle = (int32_t) SERVO_MAX_ANGLE;
    }
    return (uint16_t) angle;
}

/* 推料转速值 = 停转点 ± SERVO_PUSH_SPEED。
 * 方向换算：j_servo1 加角度为向右推，j_servo2 加角度为向左推，
 * 所以“推向左格”对 servo1 是减、对 servo2 是加。 */
static uint16_t sorting_push_angle(uint8_t class_id)
{
    const sort_route_t *route = &g_sort_routes[class_id & 3U];
    int32_t angle = (int32_t) sorting_neutral_angle(class_id);

    if (route->push_left == (route->rear ? 1U : 0U))
    {
        angle += (int32_t) SERVO_PUSH_SPEED;
    }
    else
    {
        angle -= (int32_t) SERVO_PUSH_SPEED;
    }
    return servo_angle_clamp(angle);
}

/* 回位转速值 = 推料值关于停转点的镜像（等速反向旋转）。 */
static uint16_t sorting_return_angle(uint8_t class_id)
{
    int32_t neutral = (int32_t) sorting_neutral_angle(class_id);
    int32_t push = (int32_t) sorting_push_angle(class_id);
    return servo_angle_clamp((2 * neutral) - push);
}

static void sorting_servo_start(void)
{
    uint8_t class_id = g_status.active_class;
    machine_servo_set(sorting_servo_output(class_id), sorting_push_angle(class_id));
    sort_state_set(MACHINE_SORT_EJECTING);
}

fsp_err_t machine_control_init(void)
{
    fsp_err_t err;
    memset(&g_status, 0, sizeof(g_status));
    g_status.mode = MACHINE_MODE_STOPPED;
    g_status.sort_state = MACHINE_SORT_STOPPED;
    g_status.active_class = 0xFFU;

    err = R_GPT_Open(&g_servo_pair_ctrl, &g_servo_pair_cfg);
    if (FSP_SUCCESS != err)
    {
        return err;
    }
    err = R_GPT_Open(&g_servo_backup_ctrl, &g_servo_backup_cfg);
    if (FSP_SUCCESS != err)
    {
        return err;
    }
    err = R_GPT_Open(&g_stepper_primary_ctrl, &g_stepper_primary_cfg);
    if (FSP_SUCCESS != err)
    {
        return err;
    }
    err = R_GPT_Open(&g_stepper_backup_ctrl, &g_stepper_backup_cfg);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    g_cycles_per_ms = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_CPUCLK) / 1000U;
    if (0U == g_cycles_per_ms)
    {
        g_cycles_per_ms = 500000U;
    }
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    all_servos_stop();
    (void) R_GPT_Start(&g_servo_pair_ctrl);
    (void) R_GPT_Start(&g_servo_backup_ctrl);
    machine_stepper_stop();
    output_write(PIN_STEPPER_PRIMARY_DIR, BSP_IO_LEVEL_LOW);
    output_write(PIN_STEPPER_BACKUP_DIR, BSP_IO_LEVEL_LOW);
    g_initialized = 1U;
    return FSP_SUCCESS;
}

void machine_control_set_mode(machine_mode_t mode)
{
    if (mode > MACHINE_MODE_STOPPED)
    {
        mode = MACHINE_MODE_STOPPED;
    }

    g_status.mode = mode;
    g_candidate_ticks = 0U;
    g_candidate_class = 0xFFU;
    g_clear_ticks = 0U;
    g_align_lost_ticks = 0U;
    g_align_dir = 0U;
    g_transport_settling = 0U;

    if (MACHINE_MODE_RUNNING != mode)
    {
        all_actuators_safe();
        g_status.sort_state = MACHINE_SORT_STOPPED;
        g_status.active_class = 0xFFU;
        return;
    }

    g_status.sort_state = MACHINE_SORT_WAITING;
    g_status.active_class = 0xFFU;
    machine_stepper_run(MACHINE_STEPPER_FORWARD, STEPPER_DEFAULT_HZ);
}

void machine_control_set_weight(bool valid, int16_t grams)
{
    g_status.weight_valid = (uint8_t) valid;
    g_status.weight_grams = grams;
}

/* 识别确认后进入视觉对齐：低速把目标收敛到判定线上。 */
static void begin_align(uint8_t class_id)
{
    g_status.active_class = class_id;
    g_align_lost_ticks = 0U;
    machine_stepper_run(MACHINE_STEPPER_FORWARD, ALIGN_STEP_HZ);
    g_align_dir = 1U;
    sort_state_set(MACHINE_SORT_ALIGNING);
}

/* 目标已停在判定线上（或对齐超时）：从固定起点开始定时输送。
 * 统一流程：先停稳 TRANSPORT_SETTLE_MS，再按目标方向（前=正转/后=倒转）
 * 起步并开始计时——对齐结束时皮带方向不定，停稳起步可防换向丢步。 */
static void begin_transport(void)
{
    machine_stepper_stop();
    g_align_dir = 0U;
    g_transport_settling = 1U;
    sort_state_set(MACHINE_SORT_POSITIONING);
}

void machine_control_process(bool detection_valid, uint8_t class_id, float confidence,
                             uint16_t center_x, uint16_t center_y)
{
    (void) center_x; /* 当前对齐轴为 Y；若摄像头改为横向安装则改用 X */

    if (!g_initialized || (MACHINE_MODE_RUNNING != g_status.mode))
    {
        return;
    }

    switch (g_status.sort_state)
    {
        case MACHINE_SORT_WAITING:
            if (detection_valid && (class_id < 4U) && (confidence >= DETECTION_MIN_SCORE))
            {
                if (g_candidate_class == class_id)
                {
                    if (g_candidate_ticks < 0xFFU)
                    {
                        g_candidate_ticks++;
                    }
                }
                else
                {
                    g_candidate_class = class_id;
                    g_candidate_ticks = 1U;
                }

                if (g_candidate_ticks >= DETECTION_STABLE_TICKS)
                {
                    begin_align(class_id);
                }
            }
            else
            {
                g_candidate_class = 0xFFU;
                g_candidate_ticks = 0U;
            }
            break;

        case MACHINE_SORT_ALIGNING:
            /* 兜底：对齐超时就按当前位置继续，不让机器卡死。 */
            if (state_elapsed(ALIGN_TIMEOUT_MS))
            {
                begin_transport();
                break;
            }
            /* 类别在进入 ALIGN 时已经锁定。这里只跟踪同一类别的最高置信度框，
             * 防止画面中另一个物体突然成为最高分后把传送带带向错误位置。 */
            if (detection_valid &&
                (class_id == g_status.active_class) &&
                (confidence >= DETECTION_MIN_SCORE))
            {
                g_align_lost_ticks = 0U;
                /* 双向收敛：正转时识别框从下往上移动（Y 减小）。
                 * 中心在线下方 → 正转靠近；冲过头到线上方 → 倒转退回；
                 * 落在判定线 ± 容差内 → 对齐完成。保证最终一定停在线上。 */
                int32_t offset_px = (int32_t) center_y - (int32_t) MACHINE_ALIGN_TARGET_Y;
                if (offset_px > (int32_t) ALIGN_TOLERANCE_PX)
                {
                    if (1U != g_align_dir)
                    {
                        machine_stepper_run(MACHINE_STEPPER_FORWARD, ALIGN_STEP_HZ);
                        g_align_dir = 1U;
                    }
                }
                else if (offset_px < -(int32_t) ALIGN_TOLERANCE_PX)
                {
                    if (2U != g_align_dir)
                    {
                        machine_stepper_run(MACHINE_STEPPER_REVERSE, ALIGN_STEP_HZ);
                        g_align_dir = 2U;
                    }
                }
                else
                {
                    begin_transport();
                }
            }
            else if (++g_align_lost_ticks >= ALIGN_LOST_TICKS_MAX)
            {
                /* 目标已稳定确认并锁定，回判定线途中短暂丢框时不再退回 WAITING，
                 * 否则会形成“重新识别→再次对齐→再次丢失”的循环。停止当前低速
                 * 对齐动作并从当前位置进入定时输送，后续仍按锁定类别完成分拣。 */
                begin_transport();
            }
            break;

        case MACHINE_SORT_POSITIONING:
            /* 起步前的停稳阶段：静置结束才按目标方向起步，然后重新计时输送段。 */
            if (g_transport_settling)
            {
                if (state_elapsed(TRANSPORT_SETTLE_MS))
                {
                    g_transport_settling = 0U;
                    if (g_sort_routes[g_status.active_class & 3U].rear)
                    {
                        /* 倒转用低速：带载反向更易堵转，降速换扭矩。 */
                        machine_stepper_run(MACHINE_STEPPER_REVERSE, STEPPER_REVERSE_HZ);
                    }
                    else
                    {
                        machine_stepper_run(MACHINE_STEPPER_FORWARD, STEPPER_DEFAULT_HZ);
                    }
                    sort_state_set(MACHINE_SORT_POSITIONING);
                }
                break;
            }
            /* 输送计时结束 = 物料已到达目标舵机正对位置：停带并开始推料。 */
            /* 每个类别使用上方独立配置的 travel_ms，方便逐类标定停靠位置。 */
            if (state_elapsed(g_sort_routes[g_status.active_class & 3U].travel_ms))
            {
                machine_stepper_stop();
                sorting_servo_start();
            }
            break;

        case MACHINE_SORT_EJECTING:
            /* 推料旋转结束：改发反向转速，把拨杆显式转回来
             * （360° 舵机只发停转脉宽是不会自己回位的）。 */
            if (state_elapsed(SERVO_PUSH_MS))
            {
                machine_servo_set(sorting_servo_output(g_status.active_class),
                                  sorting_return_angle(g_status.active_class));
                sort_state_set(MACHINE_SORT_CLOSING);
            }
            break;

        case MACHINE_SORT_CLOSING:
            /* 回位旋转结束：输出停转脉宽，拨杆停在原位附近。 */
            if (state_elapsed(SERVO_RETURN_MS))
            {
                machine_servo_set(sorting_servo_output(g_status.active_class),
                                  sorting_neutral_angle(g_status.active_class));
                g_status.sorted_count[g_status.active_class]++;
                sort_state_set(MACHINE_SORT_RETURNING);
            }
            break;

        case MACHINE_SORT_RETURNING:
            if (state_elapsed(SERVO_RETURN_WAIT_MS))
            {
                machine_stepper_run(MACHINE_STEPPER_FORWARD, STEPPER_DEFAULT_HZ);
                g_status.sort_state = MACHINE_SORT_CLEARING;
                g_clear_ticks = 0U;
            }
            break;

        case MACHINE_SORT_CLEARING:
            if (!detection_valid)
            {
                if (g_clear_ticks < 0xFFU)
                {
                    g_clear_ticks++;
                }
            }
            else
            {
                g_clear_ticks = 0U;
            }

            if (g_clear_ticks >= CLEAR_DETECTION_TICKS)
            {
                g_status.active_class = 0xFFU;
                g_status.sort_state = MACHINE_SORT_WAITING;
                g_candidate_class = 0xFFU;
                g_candidate_ticks = 0U;
            }
            break;

        case MACHINE_SORT_STOPPED:
        case MACHINE_SORT_FAULT:
        default:
            break;
    }
}

void machine_control_get_status(machine_status_t *status)
{
    if (NULL != status)
    {
        g_status.stepper_output = g_stepper_output;
        g_status.left_servo_output = g_left_servo_output;
        g_status.right_servo_output = g_right_servo_output;
        *status = g_status;
        status->state_elapsed_ms = g_state_elapsed_ms;
    }
}
