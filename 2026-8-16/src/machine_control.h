#ifndef MACHINE_CONTROL_H
#define MACHINE_CONTROL_H

#include "hal_data.h"
#include <stdbool.h>
#include <stdint.h>

/* 视觉对齐判定线：摄像头画面 Y 坐标（0~239）。屏幕上显示为黄色虚线。
 * 物体正转时从画面底部（Y 大）往顶部（Y 小）走，识别框中心一越过这条线
 * 就开始定时输送。数值越【大】线越靠【下】（物体来的方向）、越早触发；
 * 数值越小线越靠上、物体走得越远才触发。定义放在头文件里，
 * machine_control.c 的判定和 hal_entry.c 的画线共用，改这一处即可。 */
#define MACHINE_ALIGN_TARGET_Y (160U)

typedef enum e_machine_mode
{
    MACHINE_MODE_RUNNING = 0,
    MACHINE_MODE_PAUSED  = 1,
    MACHINE_MODE_STOPPED = 2
} machine_mode_t;

typedef enum e_machine_sort_state
{
    MACHINE_SORT_STOPPED = 0,
    MACHINE_SORT_WAITING,
    MACHINE_SORT_POSITIONING,
    MACHINE_SORT_EJECTING,
    MACHINE_SORT_CLOSING,
    MACHINE_SORT_RETURNING,
    MACHINE_SORT_CLEARING,
    MACHINE_SORT_FAULT,
    MACHINE_SORT_ALIGNING   /* 视觉对齐：低速把目标送到画面参考线再开始定时输送 */
} machine_sort_state_t;

typedef enum e_machine_stepper_output
{
    MACHINE_STEPPER_PRIMARY = 0,
    MACHINE_STEPPER_BACKUP  = 1
} machine_stepper_output_t;

typedef enum e_machine_stepper_direction
{
    MACHINE_STEPPER_FORWARD = 0,
    MACHINE_STEPPER_REVERSE = 1
} machine_stepper_direction_t;

typedef enum e_machine_servo_output
{
    MACHINE_SERVO_LEFT   = 0,
    MACHINE_SERVO_RIGHT  = 1,
    MACHINE_SERVO_BACKUP = 2
} machine_servo_output_t;

typedef struct st_machine_status
{
    machine_mode_t          mode;
    machine_sort_state_t    sort_state;
    uint8_t                 active_class;
    uint8_t                 weight_valid;
    machine_stepper_output_t stepper_output;
    machine_servo_output_t  left_servo_output;
    machine_servo_output_t  right_servo_output;
    uint32_t                state_elapsed_ms;
    uint32_t                sorted_count[4];
    int16_t                 weight_grams;
} machine_status_t;

fsp_err_t machine_control_init(void);
void machine_control_set_mode(machine_mode_t mode);
/* center_x/center_y 为识别框中心在摄像头画面中的坐标（320×240），
 * 供视觉对齐使用；detection_valid 为 false 时传 0 即可。 */
void machine_control_process(bool detection_valid, uint8_t class_id, float confidence,
                             uint16_t center_x, uint16_t center_y);
void machine_control_get_status(machine_status_t *status);

void machine_stepper_select(machine_stepper_output_t output);
void machine_stepper_run(machine_stepper_direction_t direction, uint32_t step_hz);
void machine_stepper_stop(void);
void machine_servo_set(machine_servo_output_t servo, uint16_t angle_degrees);
bool machine_servo_route_to_backup(machine_servo_output_t failed_primary);
void machine_servo_restore_routes(void);

#endif
