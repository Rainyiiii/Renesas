/*
 * This file is developed by EdgeCortix Inc. to be used with certain Renesas Electronics Hardware only.
 *
 * Copyright © 2025 EdgeCortix Inc. Licensed to Renesas Electronics Corporation with the
 * right to sublicense under the Apache License, Version 2.0.
 *
 * This file also includes source code originally developed by the Renesas Electronics Corporation.
 * The Renesas disclaimer below applies to any Renesas-originated portions for usage of the code.
 *
 * The Renesas Electronics Corporation
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED 'AS IS' AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Changed from original python code to C source code.
 * Copyright (C) 2017 Renesas Electronics Corporation. All rights reserved.
 *
 * This file also includes source codes originally developed by the TensorFlow Authors which were distributed under the following conditions.
 *
 * The TensorFlow Authors
 * Copyright 2023 The Apache Software Foundation
 *
 * This product includes software developed at
 * The Apache Software Foundation (http://www.apache.org/).
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "model.h"

// CPU compute declarations
#include "compute_sub_0000.h"
#include "sub_0001_invoke.h"
#include "compute_sub_0002.h"
#include "sub_0003_invoke.h"
#include "compute_sub_0004.h"

// Buffers for CPU units
float buf_images[110592];
int8_t buf_images_70682_11487_70629[110592];
int8_t buf__fpn_Resize_output_0_70436_11129[27648];
float buf_reg_s22_70452_70740[1728];
float buf_obj_s22_70453_70738[432];
float buf_cls_s22_70454_70736[576];
float buf_reg_s11_70455_70739[432];
float buf_obj_s11_70456_70737[108];
float buf_cls_s11_70457_70735[144];

// Arenas for CPU units
uint8_t compute_arena_sub_0000[kBufferSize_sub_0000];
uint8_t compute_arena_sub_0002[kBufferSize_sub_0002];
uint8_t compute_arena_sub_0004[kBufferSize_sub_0004];

  // Model input pointers
float* GetModelInputPtr_images() {
  return buf_images;
}


  // Model output pointers
float* GetModelOutputPtr_reg_s22_70452_70740() {
  return buf_reg_s22_70452_70740;
}

float* GetModelOutputPtr_obj_s22_70453_70738() {
  return buf_obj_s22_70453_70738;
}

float* GetModelOutputPtr_cls_s22_70454_70736() {
  return buf_cls_s22_70454_70736;
}

float* GetModelOutputPtr_reg_s11_70455_70739() {
  return buf_reg_s11_70455_70739;
}

float* GetModelOutputPtr_obj_s11_70456_70737() {
  return buf_obj_s11_70456_70737;
}

float* GetModelOutputPtr_cls_s11_70457_70735() {
  return buf_cls_s11_70457_70735;
}


void RunModel(bool clean_outputs) {
  // Buffers for NPU units
  int8_t* buf__backbone_stage3_stage3_7_Concat_output_0_70357_11551 = (int8_t*) (sub_0001_arena + sub_0001_address__backbone_stage3_stage3_7_Concat_output_0_70357_11551);
  int8_t* buf__backbone_stage4_stage4_3_Concat_output_0_70420_11567 = (int8_t*) (sub_0001_arena + sub_0001_address__backbone_stage4_stage4_3_Concat_output_0_70420_11567);
  int8_t* buf_cls_s11_70457_70735_11479 = (int8_t*) (sub_0001_arena + sub_0001_address_cls_s11_70457_70735_11479);
  int8_t* buf_obj_s11_70456_70737_11491 = (int8_t*) (sub_0001_arena + sub_0001_address_obj_s11_70456_70737_11491);
  int8_t* buf_reg_s11_70455_70739_11499 = (int8_t*) (sub_0001_arena + sub_0001_address_reg_s11_70455_70739_11499);
  int8_t* buf_cls_s22_70454_70736_11483 = (int8_t*) (sub_0003_arena + sub_0003_address_cls_s22_70454_70736_11483);
  int8_t* buf_obj_s22_70453_70738_11495 = (int8_t*) (sub_0003_arena + sub_0003_address_obj_s22_70453_70738_11495);
  int8_t* buf_reg_s22_70452_70740_11503 = (int8_t*) (sub_0003_arena + sub_0003_address_reg_s22_70452_70740_11503);

  // CPU Unit
  compute_sub_0000(compute_arena_sub_0000, buf_images, buf_images_70682_11487_70629  );

  memcpy((sub_0001_arena + sub_0001_address_images_70682_11487_70629), buf_images_70682_11487_70629, 110592);
  // NPU Unit
  sub_0001_invoke(clean_outputs);

  // CPU Unit
  compute_sub_0002(compute_arena_sub_0002, buf__backbone_stage4_stage4_3_Concat_output_0_70420_11567, buf_cls_s11_70457_70735_11479, buf_obj_s11_70456_70737_11491, buf_reg_s11_70455_70739_11499, buf__fpn_Resize_output_0_70436_11129, buf_cls_s11_70457_70735, buf_obj_s11_70456_70737, buf_reg_s11_70455_70739  );

  memcpy((sub_0003_arena + sub_0003_address__backbone_stage3_stage3_7_Concat_output_0_70357_11551), buf__backbone_stage3_stage3_7_Concat_output_0_70357_11551, 13824);
  memcpy((sub_0003_arena + sub_0003_address__fpn_Resize_output_0_70436_11129), buf__fpn_Resize_output_0_70436_11129, 27648);
  // NPU Unit
  sub_0003_invoke(clean_outputs);

  // CPU Unit
  compute_sub_0004(compute_arena_sub_0004, buf_cls_s22_70454_70736_11483, buf_obj_s22_70453_70738_11495, buf_reg_s22_70452_70740_11503, buf_cls_s22_70454_70736, buf_obj_s22_70453_70738, buf_reg_s22_70452_70740  );

}
