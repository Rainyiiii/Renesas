/*
 * OV5640 摄像头驱动 - SCCB（软件 I2C）+ DVP
 * 适用于搭配 EK-RA8P1 CEU 的正点原子（ALIENTEK）OV5640 模块
 */

#ifndef OV5640_H
#define OV5640_H

#include <stdint.h>

/* 分辨率模式 */
#define OV5640_MODE_QVGA    0   /* 320x240 */
#define OV5640_MODE_VGA     1   /* 640x480 */
#define OV5640_MODE_720P    2   /* 1280x720 */
#define OV5640_MODE_1080P   3   /* 1920x1080 */

/* 初始化摄像头：复位、配置寄存器、启动图像输出 */
int ov5640_init(void);

/* 设置输出分辨率 */
int ov5640_set_resolution(uint8_t mode);

/* 测试图案：0 = 关闭，1 = 彩条，2 = 彩色方块。 */
int ov5640_set_test_pattern(uint8_t pattern);

#endif
