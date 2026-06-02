/**
  ******************************************************************************
  * @file    depth.h
  * @author  STEdgeAI
  * @date    2026-04-21 18:02:09
  * @brief   Minimal description of the generated c-implemention of the network
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */
#ifndef LL_ATON_DEPTH_H
#define LL_ATON_DEPTH_H

/******************************************************************************/
#define LL_ATON_DEPTH_C_MODEL_NAME        "depth"
#define LL_ATON_DEPTH_ORIGIN_MODEL_NAME   "fastdepth_224_int8"

/************************** USER ALLOCATED IOs ********************************/
// No user allocated inputs
// No user allocated outputs

/************************** INPUTS ********************************************/
#define LL_ATON_DEPTH_IN_NUM        (1)    // Total number of input buffers
// Input buffer 1 -- Input_0_out_0
#define LL_ATON_DEPTH_IN_1_ALIGNMENT   (32)
#define LL_ATON_DEPTH_IN_1_SIZE_BYTES  (150528)

/************************** OUTPUTS *******************************************/
#define LL_ATON_DEPTH_OUT_NUM        (1)    // Total number of output buffers
// Output buffer 1 -- Transpose_195_out_0
#define LL_ATON_DEPTH_OUT_1_ALIGNMENT   (32)
#define LL_ATON_DEPTH_OUT_1_SIZE_BYTES  (50176)

#endif /* LL_ATON_DEPTH_H */
