#ifndef __SUB_0003_TENSORS_H__
#define __SUB_0003_TENSORS_H__

#include <stddef.h>
#include <stdint.h>
#include "ethosu_common.h"

extern const TensorInfo sub_0003_tensors[];
extern const size_t sub_0003_tensors_count;

#define kArenaSize_sub_0003 82944

// Addresses for each input and output buffer inside of the arena
extern const uint32_t sub_0003_address__fpn_Resize_output_0_70436_11129;
extern const uint32_t sub_0003_address__backbone_stage3_stage3_7_Concat_output_0_70357_11551;
extern const uint32_t sub_0003_address_reg_s22_70452_70740_11503;
extern const uint32_t sub_0003_address_obj_s22_70453_70738_11495;
extern const uint32_t sub_0003_address_cls_s22_70454_70736_11483;


#endif // __SUB_0003_TENSORS_H__
