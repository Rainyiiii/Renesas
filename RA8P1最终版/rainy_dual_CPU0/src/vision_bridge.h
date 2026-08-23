#ifndef VISION_BRIDGE_H
#define VISION_BRIDGE_H

#include <stdint.h>

#define YOLO_INPUT_W     (96)
#define YOLO_INPUT_H     (96)
#define YOLO_PREP_STRIDE (2)

extern uint8_t g_yolo_input_rgb888[YOLO_INPUT_W * YOLO_INPUT_H * 3];
extern volatile uint32_t g_yolo_frame_id;

#endif
