#ifndef YOLO_DETECTOR_H
#define YOLO_DETECTOR_H

#include "hal_data.h"
#include <stdbool.h>
#include <stdint.h>

#define YOLO_INPUT_W       (192U)
#define YOLO_INPUT_H       (192U)
#define YOLO_MAX_DETECTIONS (8U)

typedef struct st_yolo_detection_result
{
    uint8_t class_id;
    float confidence;
    float x0;
    float y0;
    float x1;
    float y1;
} yolo_detection_result_t;

fsp_err_t yolo_detector_open(void);
uint32_t yolo_detector_infer(const uint8_t *frame_rgb565);
void yolo_detector_draw_latest(uint8_t *frame_rgb565);
uint32_t yolo_detector_run_and_draw(uint8_t *frame_rgb565);
bool yolo_detector_get_best(yolo_detection_result_t *result);

#endif
