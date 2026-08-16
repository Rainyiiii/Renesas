#include "yolo_detector.h"
#include "ai_model/model.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#define CAMERA_WIDTH       (320U)
#define CAMERA_HEIGHT      (240U)
#define YOLO_CLASS_COUNT   (4U)
#define YOLO_ANCHOR_COUNT  (3U)
#define YOLO_CANDIDATE_MAX (24U)
#define YOLO_CONFIDENCE    (0.35f)
#define YOLO_NMS_IOU       (0.40f)

typedef struct st_detection
{
    float x0;
    float y0;
    float x1;
    float y1;
    float score;
    uint8_t class_id;
    bool suppressed;
} detection_t;

static const float g_anchors[2][YOLO_ANCHOR_COUNT][2] =
{
    {{20.53f, 31.49f}, {32.09f, 39.43f}, {37.46f, 21.01f}},
    {{42.19f, 68.39f}, {56.39f, 28.33f}, {61.69f, 74.24f}},
};

static const uint16_t g_class_colors[YOLO_CLASS_COUNT] =
{
    0xF800U, /* harm：红色 */
    0xFD20U, /* kitchen：橙色 */
    0x001FU, /* other：蓝色 */
    0x07E0U, /* recover：绿色 */
};

static const char * const g_class_names[YOLO_CLASS_COUNT] =
{
    "HARM", "KITCHEN", "OTHER", "RECOVER"
};

static detection_t g_candidates[YOLO_CANDIDATE_MAX];
static detection_t g_detections[YOLO_MAX_DETECTIONS];
static uint32_t g_detection_count = 0U;

static float sigmoidf_fast(float x)
{
    if (x > 16.0f) return 1.0f;
    if (x < -16.0f) return 0.0f;
    return 1.0f / (1.0f + expf(-x));
}

static float clampf(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static int8_t quantize_camera_channel(uint32_t value)
{
    int32_t quantized = (int32_t) ((value + 1U) >> 1) - 1;
    if (quantized > 126) quantized = 126;
    return (int8_t) quantized;
}

static void prepare_input(const uint8_t *src)
{
    int8_t *input = GetModelInputQuantizedPtr_images();
    const uint32_t plane = YOLO_INPUT_W * YOLO_INPUT_H;

    for (uint32_t y = 0; y < YOLO_INPUT_H; y++)
    {
        uint32_t sy = (y * CAMERA_HEIGHT) / YOLO_INPUT_H;
        for (uint32_t x = 0; x < YOLO_INPUT_W; x++)
        {
            uint32_t sx = (x * CAMERA_WIDTH) / YOLO_INPUT_W;
            uint32_t offset = (sy * CAMERA_WIDTH + sx) * 2U;
            uint16_t pixel = (uint16_t) (((uint16_t) src[offset + 1U] << 8) | src[offset]);
            uint32_t r5 = (pixel >> 11) & 0x1FU;
            uint32_t g6 = (pixel >> 5) & 0x3FU;
            uint32_t b5 = pixel & 0x1FU;
            uint32_t index = y * YOLO_INPUT_W + x;

            /* 训练图像由 OpenCV 读取，因此模型输入为 BGR NCHW 格式。 */
            uint32_t b8 = (b5 << 3) | (b5 >> 2);
            uint32_t g8 = (g6 << 2) | (g6 >> 4);
            uint32_t r8 = (r5 << 3) | (r5 >> 2);
            input[index]              = quantize_camera_channel(b8);
            input[plane + index]      = quantize_camera_channel(g8);
            input[2U * plane + index] = quantize_camera_channel(r8);
        }
    }
}

static void insert_candidate(detection_t candidate, uint32_t *count)
{
    if (*count < YOLO_CANDIDATE_MAX)
    {
        g_candidates[*count] = candidate;
        (*count)++;
        return;
    }

    uint32_t minimum = 0U;
    for (uint32_t i = 1U; i < YOLO_CANDIDATE_MAX; i++)
    {
        if (g_candidates[i].score < g_candidates[minimum].score) minimum = i;
    }
    if (candidate.score > g_candidates[minimum].score) g_candidates[minimum] = candidate;
}

static void decode_scale(const float *reg,
                         const float *obj,
                         const float *cls,
                         uint32_t grid_size,
                         uint32_t anchor_set,
                         uint32_t *candidate_count)
{
    uint32_t cells = grid_size * grid_size;
    float stride = (float) YOLO_INPUT_W / (float) grid_size;

    for (uint32_t y = 0U; y < grid_size; y++)
    {
        for (uint32_t x = 0U; x < grid_size; x++)
        {
            uint32_t cell = y * grid_size + x;
            float maximum = cls[cell];
            uint8_t class_id = 0U;
            for (uint8_t c = 1U; c < YOLO_CLASS_COUNT; c++)
            {
                float value = cls[(uint32_t) c * cells + cell];
                if (value > maximum)
                {
                    maximum = value;
                    class_id = c;
                }
            }

            float sum = 0.0f;
            for (uint8_t c = 0U; c < YOLO_CLASS_COUNT; c++)
            {
                sum += expf(cls[(uint32_t) c * cells + cell] - maximum);
            }
            float class_probability = 1.0f / sum;

            for (uint32_t a = 0U; a < YOLO_ANCHOR_COUNT; a++)
            {
                float objectness = sigmoidf_fast(obj[a * cells + cell]);
                float score = objectness * class_probability;
                if (score < YOLO_CONFIDENCE) continue;

                uint32_t channel = a * 4U;
                float cx = (sigmoidf_fast(reg[(channel + 0U) * cells + cell]) * 2.0f - 0.5f + (float) x) * stride;
                float cy = (sigmoidf_fast(reg[(channel + 1U) * cells + cell]) * 2.0f - 0.5f + (float) y) * stride;
                float sw = sigmoidf_fast(reg[(channel + 2U) * cells + cell]) * 2.0f;
                float sh = sigmoidf_fast(reg[(channel + 3U) * cells + cell]) * 2.0f;
                float width = sw * sw * g_anchors[anchor_set][a][0];
                float height = sh * sh * g_anchors[anchor_set][a][1];

                detection_t candidate;
                candidate.x0 = clampf((cx - width * 0.5f) * ((float) CAMERA_WIDTH / (float) YOLO_INPUT_W), 0.0f, 319.0f);
                candidate.y0 = clampf((cy - height * 0.5f) * ((float) CAMERA_HEIGHT / (float) YOLO_INPUT_H), 0.0f, 239.0f);
                candidate.x1 = clampf((cx + width * 0.5f) * ((float) CAMERA_WIDTH / (float) YOLO_INPUT_W), 0.0f, 319.0f);
                candidate.y1 = clampf((cy + height * 0.5f) * ((float) CAMERA_HEIGHT / (float) YOLO_INPUT_H), 0.0f, 239.0f);
                candidate.score = score;
                candidate.class_id = class_id;
                candidate.suppressed = false;
                insert_candidate(candidate, candidate_count);
            }
        }
    }
}

static float intersection_over_union(const detection_t *a, const detection_t *b)
{
    float x0 = (a->x0 > b->x0) ? a->x0 : b->x0;
    float y0 = (a->y0 > b->y0) ? a->y0 : b->y0;
    float x1 = (a->x1 < b->x1) ? a->x1 : b->x1;
    float y1 = (a->y1 < b->y1) ? a->y1 : b->y1;
    float iw = x1 - x0;
    float ih = y1 - y0;
    if ((iw <= 0.0f) || (ih <= 0.0f)) return 0.0f;
    float intersection = iw * ih;
    float area_a = (a->x1 - a->x0) * (a->y1 - a->y0);
    float area_b = (b->x1 - b->x0) * (b->y1 - b->y0);
    return intersection / (area_a + area_b - intersection);
}

static uint32_t apply_nms(uint32_t candidate_count)
{
    for (uint32_t i = 0U; i < candidate_count; i++)
    {
        for (uint32_t j = i + 1U; j < candidate_count; j++)
        {
            if (g_candidates[j].score > g_candidates[i].score)
            {
                detection_t temporary = g_candidates[i];
                g_candidates[i] = g_candidates[j];
                g_candidates[j] = temporary;
            }
        }
    }

    uint32_t output_count = 0U;
    for (uint32_t i = 0U; (i < candidate_count) && (output_count < YOLO_MAX_DETECTIONS); i++)
    {
        if (g_candidates[i].suppressed) continue;
        g_detections[output_count++] = g_candidates[i];
        for (uint32_t j = i + 1U; j < candidate_count; j++)
        {
            if ((!g_candidates[j].suppressed) &&
                (g_candidates[j].class_id == g_candidates[i].class_id) &&
                (intersection_over_union(&g_candidates[i], &g_candidates[j]) > YOLO_NMS_IOU))
            {
                g_candidates[j].suppressed = true;
            }
        }
    }
    return output_count;
}

static void put_pixel(uint8_t *frame, int x, int y, uint16_t color)
{
    if ((x < 0) || (x >= (int) CAMERA_WIDTH) || (y < 0) || (y >= (int) CAMERA_HEIGHT)) return;
    uint32_t offset = ((uint32_t) y * CAMERA_WIDTH + (uint32_t) x) * 2U;
    frame[offset] = (uint8_t) color;
    frame[offset + 1U] = (uint8_t) (color >> 8);
}

static void fill_rect(uint8_t *frame, int x0, int y0, int x1, int y1, uint16_t color)
{
    for (int y = y0; y < y1; y++)
    {
        for (int x = x0; x < x1; x++) put_pixel(frame, x, y, color);
    }
}

static const uint8_t *glyph_for(char c)
{
    static const uint8_t blank[5] = {0, 0, 0, 0, 0};
    static const uint8_t glyph_minus[5] = {0x08,0x08,0x08,0x08,0x08};
    static const uint8_t glyph_c[5] = {0x3E,0x41,0x41,0x41,0x22};
    static const uint8_t glyph_e[5] = {0x7F,0x49,0x49,0x49,0x41};
    static const uint8_t glyph_n[5] = {0x7F,0x04,0x08,0x10,0x7F};
    static const uint8_t glyphs[][5] =
    {
        {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
        {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
        {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
        {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x08,0x08,0x08,0x7F},
        {0x41,0x7F,0x41,0x00,0x00}, {0x7F,0x08,0x14,0x22,0x41},
        {0x7F,0x02,0x0C,0x02,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},
        {0x7F,0x09,0x19,0x29,0x46}, {0x7F,0x09,0x09,0x09,0x06},
        {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
        {0x26,0x49,0x49,0x49,0x32}, {0x01,0x01,0x7F,0x01,0x01},
        {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F},
        {0x7F,0x20,0x18,0x20,0x7F}, {0x63,0x14,0x08,0x14,0x63},
        {0x03,0x04,0x78,0x04,0x03}, {0x61,0x51,0x49,0x45,0x43},
        {0x62,0x64,0x08,0x13,0x23}
    };
    if ((c >= '0') && (c <= '9')) return glyphs[(uint32_t) (c - '0')];
    switch (c)
    {
        case 'A': return glyphs[10]; case 'C': return glyph_c; case 'E': return glyph_e;
        case 'H': return glyphs[11]; case 'I': return glyphs[12];
        case 'K': return glyphs[13]; case 'M': return glyphs[14]; case 'O': return glyphs[15];
        case 'N': return glyph_n; case 'R': return glyphs[16]; case 'P': return glyphs[17];
        case 'Q': return glyphs[18];
        case 'S': return glyphs[19]; case 'T': return glyphs[20]; case 'U': return glyphs[21];
        case 'V': return glyphs[22]; case 'W': return glyphs[23]; case 'X': return glyphs[24];
        case 'Y': return glyphs[25]; case 'Z': return glyphs[26]; case '%': return glyphs[27];
        case '-': return glyph_minus;
        default: return blank;
    }
}

static void draw_text(uint8_t *frame, int x, int y, const char *text, uint16_t color)
{
    while (*text)
    {
        const uint8_t *glyph = glyph_for(*text++);
        for (int col = 0; col < 5; col++)
        {
            for (int row = 0; row < 7; row++)
            {
                if (glyph[col] & (1U << row)) put_pixel(frame, x + col, y + row, color);
            }
        }
        x += 6;
    }
}

static void draw_detection(uint8_t *frame, const detection_t *detection)
{
    int x0 = (int) detection->x0;
    int y0 = (int) detection->y0;
    int x1 = (int) detection->x1;
    int y1 = (int) detection->y1;
    uint16_t color = g_class_colors[detection->class_id];
    for (int thickness = 0; thickness < 2; thickness++)
    {
        for (int x = x0; x <= x1; x++)
        {
            put_pixel(frame, x, y0 + thickness, color);
            put_pixel(frame, x, y1 - thickness, color);
        }
        for (int y = y0; y <= y1; y++)
        {
            put_pixel(frame, x0 + thickness, y, color);
            put_pixel(frame, x1 - thickness, y, color);
        }
    }

    int label_y = (y0 >= 10) ? y0 - 10 : y0 + 2;
    int percent = (int) (detection->score * 100.0f + 0.5f);
    if (percent > 99) percent = 99;
    char score_text[4] = {(char) ('0' + (percent / 10) % 10), (char) ('0' + percent % 10), '%', '\0'};
    int label_width = ((int) 6 * ((int) __builtin_strlen(g_class_names[detection->class_id]) + 4));
    if (x0 + label_width > (int) CAMERA_WIDTH) x0 = (int) CAMERA_WIDTH - label_width;
    fill_rect(frame, x0, label_y, x0 + label_width, label_y + 9, 0x0000U);
    draw_text(frame, x0 + 1, label_y + 1, g_class_names[detection->class_id], color);
    draw_text(frame, x0 + 1 + (int) __builtin_strlen(g_class_names[detection->class_id]) * 6, label_y + 1, score_text, color);
}

fsp_err_t yolo_detector_open(void)
{
    return RM_ETHOSU_Open(&g_rm_ethosu0_ctrl, &g_rm_ethosu0_cfg);
}

uint32_t yolo_detector_infer(const uint8_t *frame_rgb565)
{
    detection_t previous[YOLO_MAX_DETECTIONS];
    uint32_t previous_count = g_detection_count;
    for (uint32_t i = 0U; i < previous_count; i++) previous[i] = g_detections[i];

    uint32_t candidate_count = 0U;
    prepare_input(frame_rgb565);
    RunModelQuantized(false);

    decode_scale(GetModelOutputPtr_reg_s22_70462_70750(),
                 GetModelOutputPtr_obj_s22_70463_70748(),
                 GetModelOutputPtr_cls_s22_70464_70746(), 12U, 0U, &candidate_count);
    decode_scale(GetModelOutputPtr_reg_s11_70465_70749(),
                 GetModelOutputPtr_obj_s11_70466_70747(),
                 GetModelOutputPtr_cls_s11_70467_70745(), 6U, 1U, &candidate_count);

    g_detection_count = apply_nms(candidate_count);
    for (uint32_t i = 0U; i < g_detection_count; i++)
    {
        int best = -1;
        float best_iou = 0.20f;
        for (uint32_t j = 0U; j < previous_count; j++)
        {
            if (g_detections[i].class_id != previous[j].class_id) continue;
            float iou = intersection_over_union(&g_detections[i], &previous[j]);
            if (iou > best_iou)
            {
                best_iou = iou;
                best = (int) j;
            }
        }
        if (best >= 0)
        {
            const float old_weight = 0.30f;
            const float new_weight = 0.70f;
            g_detections[i].x0 = new_weight * g_detections[i].x0 + old_weight * previous[best].x0;
            g_detections[i].y0 = new_weight * g_detections[i].y0 + old_weight * previous[best].y0;
            g_detections[i].x1 = new_weight * g_detections[i].x1 + old_weight * previous[best].x1;
            g_detections[i].y1 = new_weight * g_detections[i].y1 + old_weight * previous[best].y1;
            g_detections[i].score = new_weight * g_detections[i].score + old_weight * previous[best].score;
        }
    }
    return g_detection_count;
}

void yolo_detector_draw_latest(uint8_t *frame_rgb565)
{
    if (0U == g_detection_count)
    {
        return;
    }

    /* 界面和分拣控制保持一致：全画面只采用置信度最高的一个目标。 */
    uint32_t best = 0U;
    for (uint32_t i = 1U; i < g_detection_count; i++)
    {
        if (g_detections[i].score > g_detections[best].score)
        {
            best = i;
        }
    }
    draw_detection(frame_rgb565, &g_detections[best]);
}

bool yolo_detector_get_best(yolo_detection_result_t *result)
{
    if ((NULL == result) || (0U == g_detection_count))
    {
        return false;
    }

    uint32_t best = 0U;
    for (uint32_t i = 1U; i < g_detection_count; i++)
    {
        if (g_detections[i].score > g_detections[best].score)
        {
            best = i;
        }
    }
    result->class_id = g_detections[best].class_id;
    result->confidence = g_detections[best].score;
    result->x0 = g_detections[best].x0;
    result->y0 = g_detections[best].y0;
    result->x1 = g_detections[best].x1;
    result->y1 = g_detections[best].y1;
    return true;
}

uint32_t yolo_detector_run_and_draw(uint8_t *frame_rgb565)
{
    uint32_t count = yolo_detector_infer(frame_rgb565);
    yolo_detector_draw_latest(frame_rgb565);
    return count;
}
