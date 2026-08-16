#ifndef DATASET_STORAGE_H
#define DATASET_STORAGE_H

#include <stdbool.h>
#include <stdint.h>
#include "r_sdmmc_api.h"

#define DATASET_CLASS_COUNT (4U)

typedef enum e_dataset_storage_status
{
    DATASET_STORAGE_NOT_INITIALIZED = 0,
    DATASET_STORAGE_READY,
    DATASET_STORAGE_NO_CARD,
    DATASET_STORAGE_CARD_ERROR,
    DATASET_STORAGE_FAT_ERROR,
    DATASET_STORAGE_WRITE_ERROR
} dataset_storage_status_t;

bool dataset_storage_mount(void);
bool dataset_storage_save_frame(const uint8_t *frame_rgb565, uint8_t class_id);
bool dataset_storage_is_ready(void);
dataset_storage_status_t dataset_storage_get_status(void);
uint32_t dataset_storage_get_saved_count(uint8_t class_id);
uint16_t dataset_storage_get_error_detail(void);
void dataset_storage_progress(uint16_t stage);
void dataset_storage_sdhi_callback(sdmmc_callback_args_t *p_args);

#endif
