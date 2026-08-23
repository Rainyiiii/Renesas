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
 */

#include <stdint.h>

#include "compute_sub_0002.h"

#include "arm_nn_types.h"
#include "arm_nnfunctions.h"
#include "kernel_library_utils.h"

#include "kernel_library_int.h" 

 

void compute_sub_0002(
  // buffer for intermediate results
  uint8_t* main_storage, // should provide at least 9 bytes of storage

  // inputs
  
  const int8_t _backbone_stage4_stage4_3_Concat_output_0_70420_11567[6912], // 1,6,6,192
  
  const int8_t cls_s11_70457_70735_11479[144], // 1,4,6,6
  
  const int8_t obj_s11_70456_70737_11491[108], // 1,3,6,6
  
  const int8_t reg_s11_70455_70739_11499[432], // 1,12,6,6
  

  // outputs
  
  int8_t _fpn_Resize_output_0_70436_11129[27648] , // 1,12,12,192
  
  float cls_s11_70457_70735[144] , // 1,4,6,6
  
  float obj_s11_70456_70737[108] , // 1,3,6,6
  
  float reg_s11_70455_70739[432]  // 1,12,6,6
  
) {
  // Buffers allocated on the main storage (note: depends on the execution order)
  

  // Parameters
  







//
// Dequantize
//
// Input  reg_s11_70455_70739_11499: int8_t - 1,12,6,6
// Output reg_s11_70455_70739: float - 1,12,6,6
AffineDequantizeInt8ToFloat(reg_s11_70455_70739_11499, reg_s11_70455_70739, 432, 0, 0.016774863004684448);


//
// Dequantize
//
// Input  obj_s11_70456_70737_11491: int8_t - 1,3,6,6
// Output obj_s11_70456_70737: float - 1,3,6,6
AffineDequantizeInt8ToFloat(obj_s11_70456_70737_11491, obj_s11_70456_70737, 108, 0, 0.12191332876682281);


//
// Dequantize
//
// Input  cls_s11_70457_70735_11479: int8_t - 1,4,6,6
// Output cls_s11_70457_70735: float - 1,4,6,6
AffineDequantizeInt8ToFloat(cls_s11_70457_70735_11479, cls_s11_70457_70735, 144, 0, 0.0477587953209877);



//
// Upsampling Nearest Neighbor
//

// Input _backbone_stage4_stage4_3_Concat_output_0_70420_11567: int8_t - 1,6,6,192
// Output _fpn_Resize_output_0_70436_11129: int8_t - 1,12,12,192

const int32_t in_shape__fpn_Resize_output_0_70436_11129[4] = { 1, 6, 6, 192,  };

const int32_t out_shape__fpn_Resize_output_0_70436_11129[4] = { 1, 12, 12, 192,  };


UpsamplingNearestNeighbor(
      _backbone_stage4_stage4_3_Concat_output_0_70420_11567
    , _fpn_Resize_output_0_70436_11129
    , in_shape__fpn_Resize_output_0_70436_11129
    , out_shape__fpn_Resize_output_0_70436_11129
    , false
    , false
);

}
