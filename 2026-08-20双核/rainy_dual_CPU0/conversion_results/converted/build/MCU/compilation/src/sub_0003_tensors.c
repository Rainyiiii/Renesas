#include "sub_0003_tensors.h"

const TensorInfo sub_0003_tensors[] = {
  { "_split_1_command_stream", 2, 2036, "COMMAND_STREAM", 0xffffffff },
  { "_split_1_flash", 3, 68544, "MODEL", 0xffffffff },
  { "_split_1_scratch", 4, 82944, "ARENA", 0x0 },
  { "_split_1_scratch_fast", 5, 82944, "FAST_SCRATCH", 0x0 },
  { "_fpn_Resize_output_0_70436_11129", 1, 27648, "INPUT_TENSOR", 0xd800 },
  { "_backbone_stage3_stage3_7_Concat_output_0_70357_11551", 0, 13824, "INPUT_TENSOR", 0x0 },
  { "reg_s22_70452_70740_11503", 8, 1728, "OUTPUT_TENSOR", 0x2d00 },
  { "obj_s22_70453_70738_11495", 7, 432, "OUTPUT_TENSOR", 0x33c0 },
  { "cls_s22_70454_70736_11483", 6, 576, "OUTPUT_TENSOR", 0x0 },
};

const size_t sub_0003_tensors_count = sizeof(sub_0003_tensors) / sizeof(sub_0003_tensors[0]);

// Addresses for each input and output buffer inside of the arena
const uint32_t sub_0003_address__fpn_Resize_output_0_70436_11129 = 0xd800;
const uint32_t sub_0003_address__backbone_stage3_stage3_7_Concat_output_0_70357_11551 = 0x0;
const uint32_t sub_0003_address_reg_s22_70452_70740_11503 = 0x2d00;
const uint32_t sub_0003_address_obj_s22_70453_70738_11495 = 0x33c0;
const uint32_t sub_0003_address_cls_s22_70454_70736_11483 = 0x0;

