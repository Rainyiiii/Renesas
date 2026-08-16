/*
 * 通过 CEU 进行 OV5640 摄像头测试
 *
 * 彩条        = 显示正常
 * 黄色        = OV5640 SCCB 初始化正常（传感器有响应）
 * 红色        = 未检测到传感器（SCCB 失败）
 *
 * CEU 采集循环：对所有错误自动恢复（永不停机）。
 * 在超时/溢出/无信号时：静默执行 Close→Open→CaptureStart。
 * IGHS/IGVS（HD/VD 周期不匹配）——忽略，帧数据仍然有效。
 * restart_count（调试用）记录 CEU 被重启了多少次。
 */

#include "hal_data.h"
#include "ov5640.h"
#include "ili9488.h"
#include "ft6336.h"
#include "yolo_detector.h"
#include "machine_control.h"
#include "weight_sensor.h"
#include "dataset_storage.h"
#include <stdbool.h>
#include <stdint.h>

#define CAMERA_WIRING_TEST_ENABLE (0)
#define CAMERA_HOLD_TEST_PATTERN  (0)
#define TOUCH_UI_ENABLE           (1)
#define TOUCH_DIAGNOSTIC_ENABLE   (0)
/* 设为 1 可单独启动称重模块：仅启动显示和 SCI4 UART，屏幕上显示原始的
 * 字节/帧计数器以及标定后的克数。跳过摄像头和 NPU，这样即使摄像头未插入
 * 也能验证秤是否正常。恢复到 0 即为正常运行。 */
#define WEIGHT_DIAGNOSTIC_ENABLE  (0)
#define YOLO_INFERENCE_STRIDE (1U)
#define DASHBOARD_REFRESH_STRIDE (4U)
#define CEU_WAIT_TIMEOUT_LOOPS (8000000UL)
#define CEU_SOFT_RECOVER_MAX   (8U)
#define CAMERA_FRAME_BYTES     (320U * 240U * 2U)
#define CAMERA_WIRING_TEST_MS  (8000U)
#define DATASET_AUTO_INTERVAL_FRAMES (1U)

typedef enum e_app_state
{
    APP_STATE_RUNNING = 0,
    APP_STATE_PAUSED  = 1,
    APP_STATE_STOPPED = 2,
    APP_STATE_DATASET = 3
} app_state_t;

#if TOUCH_UI_ENABLE
static int32_t touch_button_at(uint16_t x, uint16_t y)
{
    if (y < 376U) {
        /* 同时兼容相对 LCD 旋转 180 度安装的面板。 */
        if (y > 103U) {
            return -1;
        }
        x = (uint16_t) (319U - x);
        y = (uint16_t) (479U - y);
    }
    if (y < 376U) {
        return -1;
    }
    return (int32_t) (x / 80U);
}
#endif

static volatile uint32_t g_ceu_callback_events = 0U;
static volatile uint8_t g_ceu_frame_done = 0U;

void dataset_storage_progress(uint16_t stage)
{
    ili9488_dashboard_t dashboard = {0};
    dashboard.dataset_mode = 1U;
    dashboard.dataset_status = (uint8_t) DATASET_STORAGE_NOT_INITIALIZED;
    dashboard.dataset_busy = 1U;
    dashboard.dataset_error_detail = stage;
    ili9488_draw_dashboard(&dashboard);
}

void g_ceu_callback(capture_callback_args_t *p_args)
{
    g_ceu_callback_events |= p_args->event;
    if (p_args->event & CEU_EVENT_FRAME_END) {
        g_ceu_frame_done = 1U;
    }
}

/* 双缓冲：CEU 采集到其中一个缓冲区，同时我们显示另一个。 */
static uint8_t ceu_buf_a[CAMERA_FRAME_BYTES] __attribute__((aligned(32)));
static uint8_t ceu_buf_b[CAMERA_FRAME_BYTES] __attribute__((aligned(32)));

static void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 12000UL; i++);
}

#if TOUCH_DIAGNOSTIC_ENABLE
static void draw_touch_diagnostic(uint8_t status, uint8_t chip_id)
{
    static const uint16_t status_colors[4] =
    {
        0x07E0U, /* ID 有效 */
        0xF800U, /* 总线被拉低卡死 */
        0xF81FU, /* 无 ACK */
        0xFFE0U  /* 有 ACK 但 ID 无效 */
    };
    uint16_t color = (status < 4U) ? status_colors[status] : 0xFFFFU;
    uint8_t bus_levels = ft6336_get_bus_levels();
    ili9488_fill_color(0x0000U);
    /* 左=SDA，中=SCL，右=探测结果。绿色表示线路空闲为高电平。 */
    ili9488_fill_rect(8U, 8U, 71U, 71U, (bus_levels & 1U) ? 0x07E0U : 0xF800U);
    ili9488_fill_rect(88U, 8U, 151U, 71U, (bus_levels & 2U) ? 0x07E0U : 0xF800U);
    ili9488_fill_rect(168U, 8U, 231U, 71U, color);

    for (uint8_t bit = 0U; bit < 8U; bit++) {
        uint16_t x0 = (uint16_t) (8U + bit * 38U);
        uint16_t bit_color = (chip_id & (uint8_t) (0x80U >> bit)) ? 0x07E0U : 0x2104U;
        ili9488_fill_rect(x0, 88U, (uint16_t) (x0 + 25U), 113U, bit_color);
    }
    uint8_t address = ft6336_get_address();
    for (uint8_t bit = 0U; bit < 7U; bit++) {
        uint16_t x0 = (uint16_t) (8U + bit * 44U);
        uint16_t bit_color = (address & (uint8_t) (0x40U >> bit)) ? 0x07FFU : 0x2104U;
        ili9488_fill_rect(x0, 128U, (uint16_t) (x0 + 29U), 157U, bit_color);
    }
}

static void run_touch_diagnostic(void)
{
    uint8_t ready = ft6336_init();
    draw_touch_diagnostic(ft6336_get_status(), ft6336_get_chip_id());

    uint8_t was_down = 0U;
    while (1) {
        if (!ready) {
            delay_ms(1000U);
            ready = ft6336_init();
            draw_touch_diagnostic(ft6336_get_status(), ft6336_get_chip_id());
            continue;
        }

        ft6336_touch_t touch;
        uint8_t down = ft6336_read(&touch);
        if (down) {
            ili9488_fill_rect(248U, 8U, 311U, 71U, 0x07FFU);
            ili9488_draw_touch_marker(touch.x, touch.y, 0xFFFFU);
        } else if (was_down) {
            ili9488_fill_rect(248U, 8U, 311U, 71U, 0x0000U);
        }
        was_down = down;
        delay_ms(20U);
    }
}
#endif

static fsp_err_t ceu_start_frame(uint8_t *buffer)
{
    g_ceu_callback_events = 0U;
    g_ceu_frame_done = 0U;
    __DMB();
    return R_CEU_CaptureStart(&g_ceu_qvga_ctrl, buffer);
}

static int ceu_wait_for_frame(uint32_t *events_out)
{
    uint32_t timeout = 0U;
    while (!g_ceu_frame_done) {
        if (++timeout > CEU_WAIT_TIMEOUT_LOOPS) {
            return -1;
        }
    }

    __DMB();
    uint32_t events = g_ceu_callback_events;
    if (events_out) {
        *events_out = events;
    }

    const uint32_t invalid_frame_events = CEU_EVENT_CRAM_OVERFLOW |
                                          CEU_EVENT_FIREWALL;
    return (events & invalid_frame_events) ? -2 : 0;
}

#if CAMERA_WIRING_TEST_ENABLE
static int ceu_capture_frame_blocking(uint8_t *buffer, uint32_t *events_out)
{
    if (FSP_SUCCESS != ceu_start_frame(buffer)) {
        return -1;
    }
    return ceu_wait_for_frame(events_out);
}

static void frame_fill_rect(uint8_t *frame,
                            uint16_t x0,
                            uint16_t y0,
                            uint16_t x1,
                            uint16_t y1,
                            uint16_t color)
{
    for (uint16_t y = y0; y < y1; y++) {
        for (uint16_t x = x0; x < x1; x++) {
            uint32_t offset = ((uint32_t) y * 320U + x) * 2U;
            frame[offset]     = (uint8_t) color;
            frame[offset + 1] = (uint8_t) (color >> 8);
        }
    }
}

/* 为每根 DVP 数据线返回一个两位的结果：
 * 0 = 卡死/缺失，1 = 不稳定，2 = 稳定且有跳变。 */
static uint16_t analyze_dvp_data_wires(const uint8_t *frame_a, const uint8_t *frame_b)
{
    uint8_t bits_or = 0U;
    uint8_t bits_and = 0xFFU;
    uint8_t bits_changed = 0U;

    for (uint32_t i = 0; i < CAMERA_FRAME_BYTES; i++) {
        bits_or |= (uint8_t) (frame_a[i] | frame_b[i]);
        bits_and &= (uint8_t) (frame_a[i] & frame_b[i]);
        bits_changed |= (uint8_t) (frame_a[i] ^ frame_b[i]);
    }

    uint16_t result = 0U;
    for (uint32_t bit = 0; bit < 8U; bit++) {
        uint8_t mask = (uint8_t) (1U << bit);
        uint16_t state = 0U;
        if ((bits_or & mask) && !(bits_and & mask)) {
            state = (bits_changed & mask) ? 1U : 2U;
        }
        result |= (uint16_t) (state << (bit * 2U));
    }
    return result;
}

static void show_camera_wiring_report(uint8_t *frame, uint16_t wire_result, uint8_t sync_ok)
{
    const uint16_t red = 0xF800U;
    const uint16_t yellow = 0xFFE0U;
    const uint16_t green = 0x07E0U;
    uint8_t all_ok = sync_ok;

    frame_fill_rect(frame, 0, 0, 320, 240, 0x0000U);
    for (uint16_t bit = 0; bit < 8U; bit++) {
        uint16_t state = (wire_result >> (bit * 2U)) & 0x03U;
        uint16_t color = (2U == state) ? green : ((1U == state) ? yellow : red);
        if (2U != state) {
            all_ok = 0U;
        }
        uint16_t y0 = (uint16_t) (40U + bit * 20U);
        frame_fill_rect(frame, 0, y0, 320, (uint16_t) (y0 + 18U), color);
    }

    frame_fill_rect(frame, 0, 0, 320, 32, all_ok ? green : (sync_ok ? yellow : red));
    frame_fill_rect(frame, 0, 208, 320, 240, sync_ok ? green : red);
    ili9488_display_qvga_2x(frame);
}

static void run_camera_wiring_test(void)
{
    uint32_t events_a = 0U;
    uint32_t events_b = 0U;
    uint8_t sync_ok = 0U;
    uint16_t wire_result = 0U;

    if (0 == ov5640_set_test_pattern(2U)) {
        delay_ms(100);
        int capture_a = ceu_capture_frame_blocking(ceu_buf_a, &events_a);
        int capture_b = ceu_capture_frame_blocking(ceu_buf_b, &events_b);
        sync_ok = (uint8_t) ((0 == capture_a) && (0 == capture_b) &&
                            (events_a & 0x01U) && (events_b & 0x01U));
        if (sync_ok) {
            wire_result = analyze_dvp_data_wires(ceu_buf_a, ceu_buf_b);
        }
    }

    show_camera_wiring_report(ceu_buf_a, wire_result, sync_ok);
    delay_ms(CAMERA_WIRING_TEST_MS);
#if !CAMERA_HOLD_TEST_PATTERN
    (void) ov5640_set_test_pattern(0U);
    delay_ms(300);
#endif
}
#endif

/* 把视觉对齐判定线画进摄像头帧（黄色虚线，粗 2 像素，上屏后 2x 倍频为 4 行）。
 * 识别框中心一越过这条线就开始定时输送；线的位置改 machine_control.h
 * 里的 MACHINE_ALIGN_TARGET_Y。 */
static void draw_align_line(uint8_t *frame)
{
    const uint16_t color = 0xFFE0U; /* 黄色 */
    for (uint16_t y = MACHINE_ALIGN_TARGET_Y;
         (y < (uint16_t) (MACHINE_ALIGN_TARGET_Y + 2U)) && (y < 240U); y++) {
        for (uint16_t x = 0U; x < 320U; x++) {
            if ((x / 10U) & 1U) {
                continue; /* 10 像素间隔虚线，与识别框区分 */
            }
            uint32_t offset = ((uint32_t) y * 320U + x) * 2U;
            frame[offset] = (uint8_t) color;
            frame[offset + 1U] = (uint8_t) (color >> 8);
        }
    }
}

static void draw_dataset_dashboard_now(uint8_t touch_ready,
                                       uint8_t dataset_class,
                                       uint8_t dataset_auto,
                                       uint8_t dataset_busy)
{
    ili9488_dashboard_t dashboard = {0};
    dashboard.touch_ready = touch_ready;
    dashboard.dataset_mode = 1U;
    dashboard.dataset_status = (uint8_t) dataset_storage_get_status();
    dashboard.dataset_class = dataset_class;
    dashboard.dataset_auto = dataset_auto;
    dashboard.dataset_busy = dataset_busy;
    dashboard.dataset_error_detail = dataset_storage_get_error_detail();
    for (uint8_t class_id = 0U; class_id < DATASET_CLASS_COUNT; class_id++)
    {
        dashboard.dataset_saved_count[class_id] = dataset_storage_get_saved_count(class_id);
    }
    ili9488_draw_dashboard(&dashboard);
}

#if WEIGHT_DIAGNOSTIC_ENABLE
static void run_weight_diagnostic(void)
{
    ili9488_fill_color(0x0000U);
    fsp_err_t open_err = weight_sensor_init();

    while (1) {
        ili9488_dashboard_t dashboard = {0};
        dashboard.weight_debug = 1U;
        dashboard.weight_valid = (uint8_t) weight_sensor_has_value();
        dashboard.weight_grams = weight_sensor_get_grams();
        dashboard.weight_raw = weight_sensor_get_last_raw();
        dashboard.weight_rx_bytes = weight_sensor_get_rx_bytes();
        dashboard.weight_frame_count = weight_sensor_get_frame_count();
        /* 此处绿色的触摸点同时也表示 "UART 已成功打开"。 */
        dashboard.touch_ready = (uint8_t) (FSP_SUCCESS == open_err);
        ili9488_draw_dashboard(&dashboard);
        delay_ms(100U);
    }
}
#endif

void hal_entry(void)
{
    fsp_err_t err;

    /* ---- 步骤 1：显示屏初始化 ---- */
    ili9488_init();

#if TOUCH_DIAGNOSTIC_ENABLE
    run_touch_diagnostic();
    return;
#endif

#if WEIGHT_DIAGNOSTIC_ENABLE
    run_weight_diagnostic();
    return;
#endif

#if TOUCH_UI_ENABLE
    /* 触摸是可选的：缺少触摸控制器不应阻止摄像头/NPU 的启动。 */
    uint8_t touch_ready = ft6336_init();
    app_state_t app_state = APP_STATE_STOPPED;
    uint8_t touch_was_down = 0U;
    ili9488_draw_controls((uint8_t) app_state, touch_ready);
#else
    uint8_t touch_ready = 0U;
    app_state_t app_state = APP_STATE_STOPPED;
#endif

    /* ---- 步骤 2：OV5640 SCCB 初始化 ---- */
    int ret = ov5640_init();
    if (ret != 0) {
        uint16_t err_color;
        if (ret == -1 || ret == -2)      err_color = 0xF800;  /* 红色 = SCCB NACK */
        else if (ret == -3)              err_color = 0xF81F;  /* 品红 = ID 错误 */
        else                             err_color = 0xFD20;  /* 橙色 = 寄存器写入失败 */
        ili9488_fill_color(err_color);
        while (1);
    }
    /* ---- 步骤 3：打开 CEU ---- */
    err = R_CEU_Open(&g_ceu_qvga_ctrl, &g_ceu_qvga_cfg);
    if (err != FSP_SUCCESS) {
        ili9488_fill_color(0xF81F);  /* 紫色 = CEU 打开失败 */
        while (1);
    }

    /* ---- 步骤 4：可选的 DVP 接线自动测试 ---- */
#if CAMERA_WIRING_TEST_ENABLE
    run_camera_wiring_test();

    /* 在确定性测试帧之后复位 CEU，然后进入实时预览。 */
    (void) R_CEU_Close(&g_ceu_qvga_ctrl);
    err = R_CEU_Open(&g_ceu_qvga_ctrl, &g_ceu_qvga_cfg);
    if (err != FSP_SUCCESS) {
        ili9488_fill_color(0xF81F);
        while (1);
    }
#endif

    /* ---- 步骤 5：打开 RA8P1 Ethos-U55 NPU ---- */
    err = yolo_detector_open();
    if (err != FSP_SUCCESS) {
        ili9488_fill_color(0x001F);  /* 蓝色 = NPU 驱动打开失败 */
        while (1);
    }

    /* ---- 步骤 6：安全的运动输出和非阻塞分拣器 ---- */
    err = machine_control_init();
    if (err != FSP_SUCCESS) {
        ili9488_fill_color(0xFD20);  /* 橙色 = 执行器 GPT 配置失败 */
        while (1);
    }
    machine_control_set_mode(MACHINE_MODE_STOPPED);
    /* 允许称重模块未连接；摄像头分拣功能仍然可用。 */
    (void) weight_sensor_init();

    /* ---- 步骤 7：采集、推理、分拣、叠加显示 ----
     * 与已验证可用的工程保持一致：采集一整帧，在 CEU 空闲时显示该帧，
     * 然后启动下一次采集。
     * 流程：等待 CPE → CaptureStart(capture_buf) → Display(display_buf) → 交换
     * 这样可让 CEU 采集（约 33ms）与显示（约 16ms）重叠进行。 */
    uint8_t *capture_buf = ceu_buf_a;

    /* 启动第一次采集，采集到 display_buf */
    err = ceu_start_frame(capture_buf);
    if (err != FSP_SUCCESS) {
        ili9488_fill_color(0x07FF);  /* 青色 = 采集启动失败 */
        while (1);
    }

    uint32_t frame_count = 0;
    uint32_t ceu_soft_fail_count = 0;
    uint32_t dataset_last_capture_frame = 0U;
    uint8_t dataset_class = 0U;
    uint8_t dataset_auto = 0U;
    uint8_t dataset_capture_requested = 0U;
    uint8_t dataset_busy = 0U;
    uint8_t dashboard_refresh_requested = 1U;
    while (1) {
        /* 带超时地等待帧完成（CPE = 第 0 位）。
         * 使用较宽松的超时以避免误触发恢复流程。 */
        if (0 != ceu_wait_for_frame(NULL)) {
            goto ceu_soft_recover;
        }

        /* 读取所有事件标志并清除它们 */
        /* 事件标志由 g_ceu_callback() 收集。 */

        /* IGHS（bit17=HD 不匹配）和 IGVS（bit18=VD 不匹配）是时序警告——
         * 在 DATA_SYNCHRONOUS 模式下帧数据仍然有效。
         * 只把 CDTOF/NHD/NVD 视为致命错误。 */
        /* 选择另一个缓冲区，但在 CEU 空闲期间轮询触摸。这样可以把
         * 软件 I2C 的 GPIO 活动排除在摄像头采集时段之外。 */
        uint8_t *completed_buf = capture_buf;
        capture_buf = (capture_buf == ceu_buf_a) ? ceu_buf_b : ceu_buf_a;

        /* 以视频帧率的一半进行轮询。FT6336 不需要每帧读取，
         * 这样可以避免 GPIO 流量占用摄像头/显示的时序余量。 */
#if TOUCH_UI_ENABLE
        if (touch_ready && ((frame_count & 1U) == 0U)) {
            ft6336_touch_t touch;
            uint8_t touch_down = ft6336_read(&touch);
            if (touch_down != touch_was_down) {
                ili9488_fill_rect(8U, 8U, 23U, 23U, touch_down ? 0x07FFU : 0x07E0U);
            }
            if (touch_down && !touch_was_down) {
                int32_t button = touch_button_at(touch.x, touch.y);
                if ((button >= 0) && (button < 4)) {
                    if (APP_STATE_DATASET == app_state) {
                        if (0 == button) {
                            dataset_class = (uint8_t) ((dataset_class + 1U) % DATASET_CLASS_COUNT);
                        } else if (1 == button) {
                            dataset_capture_requested = 1U;
                        } else if (2 == button) {
                            dataset_auto ^= 1U;
                            dataset_last_capture_frame = frame_count;
                        } else {
                            dataset_auto = 0U;
                            dataset_capture_requested = 0U;
                            app_state = APP_STATE_STOPPED;
                            machine_control_set_mode(MACHINE_MODE_STOPPED);
                            ili9488_draw_controls((uint8_t) app_state, touch_ready);
                        }
                        if (APP_STATE_DATASET == app_state) {
                            ili9488_draw_dataset_controls(dataset_class, dataset_auto, touch_ready);
                        }
                        dashboard_refresh_requested = 1U;
                    } else if (button <= (int32_t) APP_STATE_STOPPED) {
                        if ((app_state_t) button != app_state) {
                            app_state = (app_state_t) button;
                            machine_control_set_mode((machine_mode_t) app_state);
                            ili9488_draw_controls((uint8_t) app_state, touch_ready);
                            dashboard_refresh_requested = 1U;
                        }
                    } else {
                        app_state = APP_STATE_DATASET;
                        dataset_auto = 0U;
                        dataset_capture_requested = 0U;
                        dataset_last_capture_frame = frame_count;
                        machine_control_set_mode(MACHINE_MODE_STOPPED);
                        ili9488_draw_dataset_controls(dataset_class, dataset_auto, touch_ready);
                        draw_dataset_dashboard_now(touch_ready, dataset_class, dataset_auto, 0U);
                        dashboard_refresh_requested = 1U;
                    }
                }
            }
            touch_was_down = touch_down;
        }
#endif

        /* 仅在 CEU 完成该帧之后再保存。把 SDHI 流量放在这段空闲窗口内，
         * 可防止卡写入期间损坏摄像头 DMA 数据。 */
        if (APP_STATE_DATASET == app_state) {
            bool auto_due = dataset_auto &&
                            ((frame_count - dataset_last_capture_frame) >= DATASET_AUTO_INTERVAL_FRAMES);
            if (dataset_capture_requested || auto_due) {
                dataset_busy = 1U;
                dashboard_refresh_requested = 1U;
                draw_dataset_dashboard_now(touch_ready, dataset_class, dataset_auto, dataset_busy);
                (void) dataset_storage_save_frame(completed_buf, dataset_class);
                dataset_busy = 0U;
                dataset_capture_requested = 0U;
                dataset_last_capture_frame = frame_count;
            }
        }

        err = ceu_start_frame(capture_buf);
        if (err != FSP_SUCCESS) {
            goto ceu_soft_recover;
        }

        bool detection_valid = false;
        yolo_detection_result_t best_detection;
        if (APP_STATE_RUNNING == app_state) {
            if ((frame_count % YOLO_INFERENCE_STRIDE) == 0U) {
                (void) yolo_detector_infer(completed_buf);
            }
            detection_valid = yolo_detector_get_best(&best_detection);
            /* 识别框中心坐标（画面 320×240），供分拣的视觉对齐使用。 */
            uint16_t detect_center_x = 0U;
            uint16_t detect_center_y = 0U;
            if (detection_valid) {
                float fx = (best_detection.x0 + best_detection.x1) * 0.5f;
                float fy = (best_detection.y0 + best_detection.y1) * 0.5f;
                if (fx < 0.0f) fx = 0.0f;
                if (fx > 319.0f) fx = 319.0f;
                if (fy < 0.0f) fy = 0.0f;
                if (fy > 239.0f) fy = 239.0f;
                detect_center_x = (uint16_t) fx;
                detect_center_y = (uint16_t) fy;
            }
            machine_control_process(detection_valid,
                                    detection_valid ? best_detection.class_id : 0U,
                                    detection_valid ? best_detection.confidence : 0.0f,
                                    detect_center_x,
                                    detect_center_y);
            yolo_detector_draw_latest(completed_buf);
        } else {
            machine_control_process(false, 0U, 0.0f, 0U, 0U);
        }
        machine_status_t machine_status;
        machine_control_get_status(&machine_status);
        /* 暂停/停止时预览仍保持实时；只有推理和运动停止。 */
        draw_align_line(completed_buf);
        ili9488_display_qvga_2x(completed_buf);

        /* 原先 STM32 屏幕上的数据现在放在顶部的仪表板中。限制刷新频率
         * 可避免把显示带宽浪费在抖动的坐标数值上。 */
        if (((frame_count % DASHBOARD_REFRESH_STRIDE) == 0U) || dashboard_refresh_requested) {
            ili9488_dashboard_t dashboard = {0};
            dashboard.touch_ready = touch_ready;
            dashboard.mode = (uint8_t) machine_status.mode;
            dashboard.sort_state = (uint8_t) machine_status.sort_state;
            dashboard.active_class = machine_status.active_class;
            dashboard.state_elapsed_ms = machine_status.state_elapsed_ms;
            dashboard.detection_valid = (uint8_t) detection_valid;
            dashboard.weight_valid = machine_status.weight_valid;
            dashboard.weight_grams = machine_status.weight_grams;
            for (uint32_t class_id = 0U; class_id < 4U; class_id++) {
                dashboard.sorted_count[class_id] = machine_status.sorted_count[class_id];
                dashboard.dataset_saved_count[class_id] = dataset_storage_get_saved_count((uint8_t) class_id);
            }
            dashboard.dataset_mode = (uint8_t) (APP_STATE_DATASET == app_state);
            dashboard.dataset_status = (uint8_t) dataset_storage_get_status();
            dashboard.dataset_class = dataset_class;
            dashboard.dataset_auto = dataset_auto;
            dashboard.dataset_busy = dataset_busy;
            dashboard.dataset_error_detail = dataset_storage_get_error_detail();
            if (detection_valid) {
                int32_t confidence = (int32_t) (best_detection.confidence * 100.0f + 0.5f);
                int32_t center_x = (int32_t) ((best_detection.x0 + best_detection.x1) * 0.5f + 0.5f);
                int32_t center_y = (int32_t) ((best_detection.y0 + best_detection.y1) * 0.5f + 0.5f);
                if (confidence < 0) confidence = 0;
                if (confidence > 99) confidence = 99;
                if (center_x < 0) center_x = 0;
                if (center_x > 319) center_x = 319;
                if (center_y < 0) center_y = 0;
                if (center_y > 239) center_y = 239;
                dashboard.detection_class = best_detection.class_id;
                dashboard.confidence_percent = (uint8_t) confidence;
                dashboard.detection_x = (uint16_t) center_x;
                dashboard.detection_y = (uint16_t) center_y;
            }
            ili9488_draw_dashboard(&dashboard);
            dashboard_refresh_requested = 0U;
        }

        frame_count++;
        ceu_soft_fail_count = 0;

        continue;

    ceu_soft_recover:
        ceu_soft_fail_count++;
        err = ceu_start_frame(capture_buf);
        if ((err == FSP_SUCCESS) && (ceu_soft_fail_count < CEU_SOFT_RECOVER_MAX)) {
            continue;
        }

        /* 完整重启 CEU。不要刷屏——保持上一帧可见。
         * 复位到已知状态：采集到 display_buf。 */
        R_CEU_Close(&g_ceu_qvga_ctrl);
        err = R_CEU_Open(&g_ceu_qvga_ctrl, &g_ceu_qvga_cfg);
        if (err != FSP_SUCCESS) {
            delay_ms(10);
            continue;
        }
        /* 将缓冲区指针复位到已知状态 */
        capture_buf = ceu_buf_a;
        ceu_soft_fail_count = 0;
        err = ceu_start_frame(capture_buf);
        if (err != FSP_SUCCESS) {
            delay_ms(10);
        }
    }
}
