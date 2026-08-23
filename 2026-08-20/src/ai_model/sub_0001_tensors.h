#ifndef __SUB_0001_TENSORS_H__
#define __SUB_0001_TENSORS_H__

#include <stddef.h>
#include <stdint.h>
#include "ethosu_common.h"

extern const TensorInfo sub_0001_tensors[];
extern const size_t sub_0001_tensors_count;

#define kArenaSize_sub_0001 405504

// Addresses for each input and output buffer inside of the arena
extern const uint32_t sub_0001_address_images_70692_11487_70629;
extern const uint32_t sub_0001_address__backbone_stage3_stage3_7_Concat_output_0_70367_11551;
extern const uint32_t sub_0001_address__backbone_stage4_stage4_3_Concat_output_0_70430_11567;
extern const uint32_t sub_0001_address_reg_s11_70465_70749_11499;
extern const uint32_t sub_0001_address_obj_s11_70466_70747_11491;
extern const uint32_t sub_0001_address_cls_s11_70467_70745_11479;


#endif // __SUB_0001_TENSORS_H__
