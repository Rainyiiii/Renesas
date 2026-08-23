#include "dataset_storage.h"
#include "hal_data.h"
#include "fatfs/ff.h"
#include "fatfs/diskio.h"
#include <stddef.h>
#include <string.h>

#define CAMERA_WIDTH              (320U)
#define CAMERA_HEIGHT             (240U)
#define MODEL_WIDTH               (192U)
#define MODEL_HEIGHT              (192U)
#define SD_SECTOR_SIZE            (512U)
#define SD_TRANSFER_TIMEOUT_MS    (1500U)
#define SD_POWER_STABLE_MS        (500U)
#define SD_MEDIA_INIT_ATTEMPTS    (3U)
#define SD_MEDIA_RETRY_DELAY_MS   (300U)
#define BMP_HEADER_SIZE           (54U)
#define BMP_PIXEL_OFFSET          (512U)
#define BMP_CHUNK_ROWS            (16U)
#define BMP_CHUNK_BYTES           (CAMERA_WIDTH * 3U * BMP_CHUNK_ROWS)
#define DATASET_SAVE_RAW          (0U)

static const char * const g_class_directories[DATASET_CLASS_COUNT] =
{
    "HARM", "KITCHEN", "OTHER", "RECOVER"
};

static FATFS g_dataset_fs;
static sdmmc_device_t g_sd_device;
static uint32_t g_next_index[DATASET_CLASS_COUNT] = {1U, 1U, 1U, 1U};
static uint8_t g_bmp_chunk[BMP_CHUNK_BYTES] __attribute__((aligned(32)));
static uint8_t g_sd_test_block[SD_SECTOR_SIZE] __attribute__((aligned(32)));
static volatile uint8_t g_sd_transfer_done;
static volatile uint8_t g_sd_transfer_error;
static volatile uint8_t g_sd_card_inserted;
static uint8_t g_sd_driver_open;
static volatile uint8_t g_sd_media_ready;
static volatile uint8_t g_fs_mounted;
static volatile dataset_storage_status_t g_storage_status = DATASET_STORAGE_NOT_INITIALIZED;
static volatile uint16_t g_storage_error_detail;

extern volatile uint32_t g_sdhi_last_command;
extern volatile uint32_t g_sdhi_last_error_info2;

static uint16_t sd_response_error_kind(uint32_t info2)
{
    uint32_t error_bits = info2 & 0x807FU;
    if ((0U == error_bits) || (0U != (error_bits & (error_bits - 1U))))
    {
        return (0U == error_bits) ? 0U : 9U;
    }
    if (0U != (error_bits & 0x0001U)) { return 1U; }
    if (0U != (error_bits & 0x0002U)) { return 2U; }
    if (0U != (error_bits & 0x0004U)) { return 3U; }
    if (0U != (error_bits & 0x0008U)) { return 4U; }
    if (0U != (error_bits & 0x0010U)) { return 5U; }
    if (0U != (error_bits & 0x0020U)) { return 6U; }
    if (0U != (error_bits & 0x0040U)) { return 7U; }
    return 8U;
}

static uint16_t sd_media_error_detail(fsp_err_t error)
{
    uint16_t command = (uint16_t) (g_sdhi_last_command & 0x3FU);

    if (FSP_ERR_RESPONSE == error)
    {
        /* 3CCK：CC 表示命令编号，K 用于标识 SD_INFO2 中出错的位。 */
        return (uint16_t) (3000U + (command * 10U) + sd_response_error_kind(g_sdhi_last_error_info2));
    }
    if (FSP_ERR_TIMEOUT == error)
    {
        return (uint16_t) (3200U + command);
    }
    if (FSP_ERR_DEVICE_BUSY == error)
    {
        return (uint16_t) (3300U + command);
    }
    if (FSP_ERR_CARD_INIT_FAILED == error)
    {
        return (uint16_t) (3400U + command);
    }

    return (uint16_t) (3900U + ((uint16_t) error % 100U));
}

static void set_storage_progress(uint16_t stage)
{
    g_storage_status = DATASET_STORAGE_NOT_INITIALIZED;
    g_storage_error_detail = stage;
    dataset_storage_progress(stage);
}

static void reset_sd_session(void)
{
    (void) f_mount(NULL, "0:", 0U);
    if (g_sd_driver_open)
    {
        (void) g_sdmmc0.p_api->close(g_sdmmc0.p_ctrl);
    }
    g_sd_driver_open = 0U;
    g_sd_media_ready = 0U;
    g_fs_mounted = 0U;
    g_sd_transfer_done = 0U;
    g_sd_transfer_error = 0U;
}

static void put_u16_le(uint8_t *buffer, uint32_t offset, uint16_t value)
{
    buffer[offset] = (uint8_t) value;
    buffer[offset + 1U] = (uint8_t) (value >> 8U);
}

static void put_u32_le(uint8_t *buffer, uint32_t offset, uint32_t value)
{
    buffer[offset] = (uint8_t) value;
    buffer[offset + 1U] = (uint8_t) (value >> 8U);
    buffer[offset + 2U] = (uint8_t) (value >> 16U);
    buffer[offset + 3U] = (uint8_t) (value >> 24U);
}

static bool fat_write_exact(FIL *file, const void *data, UINT length)
{
    UINT written = 0U;
    FRESULT result = f_write(file, data, length, &written);
    if (FR_OK != result)
    {
        g_storage_error_detail = (uint16_t) (7000U + (uint16_t) result);
        return false;
    }
    if (written != length)
    {
        g_storage_error_detail = 7999U;
        return false;
    }
    return true;
}

static bool sd_wait_for_transfer(void)
{
    for (uint32_t elapsed = 0U; elapsed < SD_TRANSFER_TIMEOUT_MS; elapsed++)
    {
        if (g_sd_transfer_done)
        {
            return 0U == g_sd_transfer_error;
        }
        R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);
    }
    g_storage_error_detail = 8998U;
    return false;
}

static void path_append(char *path, uint32_t capacity, uint32_t *length, const char *text)
{
    while (('\0' != *text) && ((*length + 1U) < capacity))
    {
        path[(*length)++] = *text++;
    }
    path[*length] = '\0';
}

static void build_directory_path(char *path, uint32_t capacity, const char *image_group, uint8_t class_id)
{
    uint32_t length = 0U;
    path[0] = '\0';
    path_append(path, capacity, &length, "0:/DATASET/");
    path_append(path, capacity, &length, image_group);
    path_append(path, capacity, &length, "/");
    path_append(path, capacity, &length, g_class_directories[class_id]);
}

static void build_image_path(char *path, uint32_t capacity, const char *image_group,
                             uint8_t class_id, uint32_t index)
{
    uint32_t length = 0U;
    build_directory_path(path, capacity, image_group, class_id);
    while ('\0' != path[length])
    {
        length++;
    }
    path_append(path, capacity, &length, "/");

    uint32_t divisor = 10000000U;
    for (uint32_t digit = 0U; digit < 8U; digit++)
    {
        char value[2] = {(char) ('0' + ((index / divisor) % 10U)), '\0'};
        path_append(path, capacity, &length, value);
        divisor /= 10U;
    }
    path_append(path, capacity, &length, ".BMP");
}

static bool ensure_directory(const char *path)
{
    FRESULT result = f_mkdir(path);
    if ((FR_OK != result) && (FR_EXIST != result))
    {
        g_storage_error_detail = (uint16_t) (5000U + (uint16_t) result);
    }
    return (FR_OK == result) || (FR_EXIST == result);
}

static uint32_t filename_index(const char *filename)
{
    uint32_t value = 0U;
    for (uint32_t i = 0U; i < 8U; i++)
    {
        if ((filename[i] < '0') || (filename[i] > '9'))
        {
            return 0U;
        }
        value = value * 10U + (uint32_t) (filename[i] - '0');
    }
    return ('.' == filename[8]) ? value : 0U;
}

static uint32_t scan_next_index(uint8_t class_id)
{
    char path[48];
    DIR directory;
    FILINFO info;
    uint32_t maximum = 0U;
    build_directory_path(path, sizeof(path), "IMG192", class_id);
    if (FR_OK != f_opendir(&directory, path))
    {
        return 1U;
    }

    while (FR_OK == f_readdir(&directory, &info))
    {
        if ('\0' == info.fname[0])
        {
            break;
        }
        if (0U == (info.fattrib & AM_DIR))
        {
            uint32_t index = filename_index(info.fname);
            if (index > maximum)
            {
                maximum = index;
            }
        }
    }
    (void) f_closedir(&directory);
    return (maximum < 99999999U) ? (maximum + 1U) : 1U;
}

static bool create_dataset_directories(void)
{
    if (!ensure_directory("0:/DATASET") || !ensure_directory("0:/DATASET/IMG192"))
    {
        return false;
    }
#if DATASET_SAVE_RAW
    if (!ensure_directory("0:/DATASET/RAW"))
    {
        return false;
    }
#endif

    for (uint8_t class_id = 0U; class_id < DATASET_CLASS_COUNT; class_id++)
    {
        char path[48];
        build_directory_path(path, sizeof(path), "IMG192", class_id);
        if (!ensure_directory(path))
        {
            return false;
        }
#if DATASET_SAVE_RAW
        build_directory_path(path, sizeof(path), "RAW", class_id);
        if (!ensure_directory(path))
        {
            return false;
        }
#endif
    }
    return true;
}

static bool write_sd_test_file(void)
{
    FIL file;
    static const char message[] = "RA8P1 SD WRITE TEST OK\r\n";
    memset(g_sd_test_block, 0, sizeof(g_sd_test_block));
    memcpy(g_sd_test_block, message, sizeof(message) - 1U);

    set_storage_progress(1005U);
    FRESULT result = f_open(&file, "0:/SDTEST.TXT", FA_CREATE_ALWAYS | FA_WRITE);
    if (FR_OK != result)
    {
        g_storage_error_detail = (uint16_t) (6100U + (uint16_t) result);
        return false;
    }

    set_storage_progress(1006U);
    bool ok = fat_write_exact(&file, g_sd_test_block, sizeof(g_sd_test_block));
    if (ok)
    {
        set_storage_progress(1007U);
        result = f_sync(&file);
        if (FR_OK != result)
        {
            g_storage_error_detail = (uint16_t) (6200U + (uint16_t) result);
            ok = false;
        }
    }

    set_storage_progress(1008U);
    result = f_close(&file);
    if ((FR_OK != result) && (0U == g_storage_error_detail))
    {
        g_storage_error_detail = (uint16_t) (6300U + (uint16_t) result);
        ok = false;
    }
    return ok;
}

static bool write_bmp(const char *path, const uint8_t *frame_rgb565,
                      uint16_t output_width, uint16_t output_height)
{
    FIL file;
    uint8_t header[BMP_PIXEL_OFFSET] __attribute__((aligned(32))) = {0};
    uint32_t row_bytes = (uint32_t) output_width * 3U;
    uint32_t image_bytes = row_bytes * output_height;

    header[0] = 'B';
    header[1] = 'M';
    put_u32_le(header, 2U, BMP_PIXEL_OFFSET + image_bytes);
    put_u32_le(header, 10U, BMP_PIXEL_OFFSET);
    put_u32_le(header, 14U, 40U);
    put_u32_le(header, 18U, output_width);
    put_u32_le(header, 22U, output_height);
    put_u16_le(header, 26U, 1U);
    put_u16_le(header, 28U, 24U);
    put_u32_le(header, 34U, image_bytes);
    put_u32_le(header, 38U, 2835U);
    put_u32_le(header, 42U, 2835U);

    set_storage_progress(1101U);
    FRESULT result = f_open(&file, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (FR_OK != result)
    {
        g_storage_error_detail = (uint16_t) (6000U + (uint16_t) result);
        return false;
    }

    set_storage_progress(1102U);
    bool ok = fat_write_exact(&file, header, sizeof(header));
    uint16_t rows_buffered = 0U;
    for (uint16_t output_y = output_height; ok && (output_y > 0U); output_y--)
    {
        uint16_t y = (uint16_t) (output_y - 1U);
        uint32_t source_y = ((uint32_t) y * CAMERA_HEIGHT) / output_height;
        uint8_t *destination = &g_bmp_chunk[(uint32_t) rows_buffered * row_bytes];

        for (uint16_t x = 0U; x < output_width; x++)
        {
            uint32_t source_x = ((uint32_t) x * CAMERA_WIDTH) / output_width;
            uint32_t offset = (source_y * CAMERA_WIDTH + source_x) * 2U;
            uint16_t pixel = (uint16_t) (((uint16_t) frame_rgb565[offset + 1U] << 8U) |
                                         frame_rgb565[offset]);
            uint8_t r5 = (uint8_t) ((pixel >> 11U) & 0x1FU);
            uint8_t g6 = (uint8_t) ((pixel >> 5U) & 0x3FU);
            uint8_t b5 = (uint8_t) (pixel & 0x1FU);
            destination[(uint32_t) x * 3U] = (uint8_t) ((b5 << 3U) | (b5 >> 2U));
            destination[(uint32_t) x * 3U + 1U] = (uint8_t) ((g6 << 2U) | (g6 >> 4U));
            destination[(uint32_t) x * 3U + 2U] = (uint8_t) ((r5 << 3U) | (r5 >> 2U));
        }

        rows_buffered++;
        if ((BMP_CHUNK_ROWS == rows_buffered) || (1U == output_y))
        {
            set_storage_progress(1103U);
            ok = fat_write_exact(&file, g_bmp_chunk, (UINT) ((uint32_t) rows_buffered * row_bytes));
            rows_buffered = 0U;
        }
    }

    set_storage_progress(1104U);
    result = f_close(&file);
    if (FR_OK != result)
    {
        if (0U == g_storage_error_detail)
        {
            g_storage_error_detail = (uint16_t) (6500U + (uint16_t) result);
        }
        ok = false;
    }
    return ok;
}

void dataset_storage_sdhi_callback(sdmmc_callback_args_t *p_args)
{
    if (NULL == p_args)
    {
        return;
    }
    if (p_args->event & SDMMC_EVENT_CARD_INSERTED)
    {
        g_sd_card_inserted = 1U;
    }
    if (p_args->event & SDMMC_EVENT_CARD_REMOVED)
    {
        g_sd_card_inserted = 0U;
        g_sd_media_ready = 0U;
        g_fs_mounted = 0U;
        g_sd_transfer_error = 1U;
        g_sd_transfer_done = 1U;
        g_storage_status = DATASET_STORAGE_NO_CARD;
    }
    if (p_args->event & SDMMC_EVENT_TRANSFER_COMPLETE)
    {
        g_sd_transfer_error = 0U;
        g_sd_transfer_done = 1U;
    }
    if (p_args->event & SDMMC_EVENT_TRANSFER_ERROR)
    {
        g_storage_error_detail = 8999U;
        g_sd_transfer_error = 1U;
        g_sd_transfer_done = 1U;
    }
}

bool dataset_storage_mount(void)
{
    fsp_err_t error;
    sdmmc_status_t status;
    bsp_io_level_t cd_level = BSP_IO_LEVEL_HIGH;

    if (g_fs_mounted && g_sd_card_inserted)
    {
        return true;
    }
    if (!g_sd_driver_open)
    {
        set_storage_progress(1001U);
        error = g_sdmmc0.p_api->open(g_sdmmc0.p_ctrl, g_sdmmc0.p_cfg);
        if (FSP_SUCCESS != error)
        {
            g_storage_error_detail = (uint16_t) (1000U + ((uint16_t) error & 0x03FFU));
            g_storage_status = DATASET_STORAGE_CARD_ERROR;
            return false;
        }
        g_sd_driver_open = 1U;
    }

    set_storage_progress(1002U);
    error = g_sdmmc0.p_api->statusGet(g_sdmmc0.p_ctrl, &status);
    if (FSP_SUCCESS != error)
    {
        g_storage_error_detail = (uint16_t) (2000U + ((uint16_t) error & 0x03FFU));
        g_storage_status = DATASET_STORAGE_CARD_ERROR;
        return false;
    }

    /* 在本板上 JTF 第 9 脚为低电平有效。它还通过反相器 U602 使能 U601。
     * 直接读取 PD07 作为兜底，以防 SDCDMON 尚未锁存已经插入的卡。 */
    bool cd_active_low = (FSP_SUCCESS == R_IOPORT_PinRead(&g_ioport_ctrl,
                                                          BSP_IO_PORT_13_PIN_07,
                                                          &cd_level)) &&
                         (BSP_IO_LEVEL_LOW == cd_level);
    if (!status.card_inserted && !cd_active_low)
    {
        g_sd_card_inserted = 0U;
        g_storage_status = DATASET_STORAGE_NO_CARD;
        return false;
    }
    g_sd_card_inserted = 1U;

    if (!g_sd_media_ready)
    {
        /* 插卡检测还会使能板载的 TXB0108。在发出 CMD0 之前，
         * 给它的电源和 OE 通路留出稳定时间。 */
        R_BSP_SoftwareDelay(SD_POWER_STABLE_MS, BSP_DELAY_UNITS_MILLISECONDS);
        error = FSP_ERR_CARD_INIT_FAILED;
        for (uint32_t attempt = 0U; attempt < SD_MEDIA_INIT_ATTEMPTS; attempt++)
        {
            /* STEP 1031/1032/1033 表示当前 mediaInit 尝试次数。FSP 的
             * mediaInit 每次都会重新复位 SDHI 并从低速初始化命令开始。 */
            set_storage_progress((uint16_t) (1031U + attempt));
            error = g_sdmmc0.p_api->mediaInit(g_sdmmc0.p_ctrl, &g_sd_device);
            if (FSP_SUCCESS == error)
            {
                break;
            }
            if ((attempt + 1U) < SD_MEDIA_INIT_ATTEMPTS)
            {
                R_BSP_SoftwareDelay(SD_MEDIA_RETRY_DELAY_MS,
                                    BSP_DELAY_UNITS_MILLISECONDS);
            }
        }
        if (FSP_SUCCESS != error)
        {
            g_storage_error_detail = sd_media_error_detail(error);
            g_storage_status = DATASET_STORAGE_CARD_ERROR;
            return false;
        }
        g_sd_media_ready = 1U;
    }

    (void) f_mount(NULL, "0:", 0U);
    memset(&g_dataset_fs, 0, sizeof(g_dataset_fs));
    set_storage_progress(1004U);
    FRESULT mount_result = f_mount(&g_dataset_fs, "0:", 1U);
    if (FR_OK != mount_result)
    {
        g_storage_error_detail = (uint16_t) (4000U + (uint16_t) mount_result);
        g_storage_status = DATASET_STORAGE_FAT_ERROR;
        reset_sd_session();
        return false;
    }
    if (!write_sd_test_file())
    {
        g_storage_status = DATASET_STORAGE_WRITE_ERROR;
        reset_sd_session();
        return false;
    }
    set_storage_progress(1009U);
    if (!create_dataset_directories())
    {
        g_storage_status = DATASET_STORAGE_FAT_ERROR;
        reset_sd_session();
        return false;
    }

    set_storage_progress(1010U);
    for (uint8_t class_id = 0U; class_id < DATASET_CLASS_COUNT; class_id++)
    {
        g_next_index[class_id] = scan_next_index(class_id);
    }
    g_fs_mounted = 1U;
    g_storage_error_detail = 0U;
    g_storage_status = DATASET_STORAGE_READY;
    return true;
}

bool dataset_storage_save_frame(const uint8_t *frame_rgb565, uint8_t class_id)
{
    char path[56];
    g_storage_error_detail = 0U;
    if ((NULL == frame_rgb565) || (class_id >= DATASET_CLASS_COUNT))
    {
        return false;
    }
    if (!dataset_storage_mount())
    {
        return false;
    }

    uint32_t index = g_next_index[class_id];
    build_image_path(path, sizeof(path), "IMG192", class_id, index);
    bool ok = write_bmp(path, frame_rgb565, MODEL_WIDTH, MODEL_HEIGHT);
#if DATASET_SAVE_RAW
    if (ok)
    {
        build_image_path(path, sizeof(path), "RAW", class_id, index);
        ok = write_bmp(path, frame_rgb565, CAMERA_WIDTH, CAMERA_HEIGHT);
    }
#endif

    if (ok)
    {
        g_next_index[class_id] = (index < 99999999U) ? (index + 1U) : 1U;
        /* write_bmp() 会用 1101~1104 显示写入进度；成功完成后必须清零，
         * 否则仪表盘会把最后的“关闭文件”阶段误显示成 ERROR 1104。 */
        g_storage_error_detail = 0U;
        g_storage_status = DATASET_STORAGE_READY;
    }
    else
    {
        if (0U == g_storage_error_detail)
        {
            g_storage_error_detail = 7999U;
        }
        g_storage_status = DATASET_STORAGE_WRITE_ERROR;
    }
    return ok;
}

bool dataset_storage_is_ready(void)
{
    return g_fs_mounted && g_sd_card_inserted && (DATASET_STORAGE_READY == g_storage_status);
}

dataset_storage_status_t dataset_storage_get_status(void)
{
    return g_storage_status;
}

uint32_t dataset_storage_get_saved_count(uint8_t class_id)
{
    if ((class_id >= DATASET_CLASS_COUNT) || (0U == g_next_index[class_id]))
    {
        return 0U;
    }
    return g_next_index[class_id] - 1U;
}

uint16_t dataset_storage_get_error_detail(void)
{
    return g_storage_error_detail;
}

DSTATUS disk_initialize(BYTE physical_drive)
{
    if (0U != physical_drive)
    {
        return STA_NOINIT;
    }
    return g_sd_media_ready ? 0U : STA_NOINIT;
}

DSTATUS disk_status(BYTE physical_drive)
{
    if (0U != physical_drive)
    {
        return STA_NOINIT;
    }
    if (!g_sd_card_inserted)
    {
        return STA_NODISK | STA_NOINIT;
    }
    return g_sd_media_ready ? 0U : STA_NOINIT;
}

DRESULT disk_read(BYTE physical_drive, BYTE *buffer, LBA_t sector, UINT count)
{
    if ((0U != physical_drive) || (NULL == buffer) || (0U == count) || !g_sd_media_ready)
    {
        return RES_PARERR;
    }
    g_sd_transfer_done = 0U;
    g_sd_transfer_error = 0U;
    fsp_err_t error = g_sdmmc0.p_api->read(g_sdmmc0.p_ctrl, buffer, (uint32_t) sector, count);
    if (FSP_SUCCESS != error)
    {
        g_storage_error_detail = (uint16_t) (8100U + ((uint16_t) error & 0x00FFU));
        return RES_ERROR;
    }
    return sd_wait_for_transfer() ? RES_OK : RES_ERROR;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE physical_drive, const BYTE *buffer, LBA_t sector, UINT count)
{
    if ((0U != physical_drive) || (NULL == buffer) || (0U == count) || !g_sd_media_ready)
    {
        return RES_PARERR;
    }
    g_sd_transfer_done = 0U;
    g_sd_transfer_error = 0U;
    fsp_err_t error = g_sdmmc0.p_api->write(g_sdmmc0.p_ctrl, buffer, (uint32_t) sector, count);
    if (FSP_SUCCESS != error)
    {
        g_storage_error_detail = (uint16_t) (8200U + ((uint16_t) error & 0x00FFU));
        return RES_ERROR;
    }
    return sd_wait_for_transfer() ? RES_OK : RES_ERROR;
}
#endif

DRESULT disk_ioctl(BYTE physical_drive, BYTE command, void *buffer)
{
    if ((0U != physical_drive) || !g_sd_media_ready)
    {
        return RES_NOTRDY;
    }
    switch (command)
    {
        case CTRL_SYNC:
            return RES_OK;
        case GET_SECTOR_COUNT:
            if (NULL == buffer) return RES_PARERR;
            *(LBA_t *) buffer = (LBA_t) g_sd_device.sector_count;
            return RES_OK;
        case GET_SECTOR_SIZE:
            if (NULL == buffer) return RES_PARERR;
            *(WORD *) buffer = SD_SECTOR_SIZE;
            return RES_OK;
        case GET_BLOCK_SIZE:
            if (NULL == buffer) return RES_PARERR;
            *(DWORD *) buffer = g_sd_device.erase_sector_count ? g_sd_device.erase_sector_count : 1U;
            return RES_OK;
        default:
            return RES_PARERR;
    }
}
