#include "sub_0001_tensors.h"

const TensorInfo sub_0001_tensors[] = {
  { "_split_1_command_stream", 2, 20968, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 3, 294336, "MODEL", 0xffffffff },
  { "_split_1_scratch", 4, 405504, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 5, 405504, "FAST_SCRATCH", 0x0 },
  { "images_70692_11487_70629", 7, 110592, "INPUT_TENSOR", 0x1b000 },
  { "_backbone_stage3_stage3_7_Concat_output_0_70367_11551", 0, 13824, "OUTPUT_TENSOR", 0x0 },
  { "_backbone_stage4_stage4_3_Concat_output_0_70430_11567", 1, 6912, "OUTPUT_TENSOR", 0x3600 },
  { "reg_s11_70465_70749_11499", 9, 432, "OUTPUT_TENSOR", 0x5c40 },
  { "obj_s11_70466_70747_11491", 8, 108, "OUTPUT_TENSOR", 0x5df0 },
  { "cls_s11_70467_70745_11479", 6, 144, "OUTPUT_TENSOR", 0x5100 },
};

const size_t sub_0001_tensors_count = sizeof(sub_0001_tensors) / sizeof(sub_0001_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0001_address_images_70692_11487_70629 = 0x1b000;
const uint32_t sub_0001_address__backbone_stage3_stage3_7_Concat_output_0_70367_11551 = 0x0;
const uint32_t sub_0001_address__backbone_stage4_stage4_3_Concat_output_0_70430_11567 = 0x3600;
const uint32_t sub_0001_address_reg_s11_70465_70749_11499 = 0x5c40;
const uint32_t sub_0001_address_obj_s11_70466_70747_11491 = 0x5df0;
const uint32_t sub_0001_address_cls_s11_70467_70745_11479 = 0x5100;

