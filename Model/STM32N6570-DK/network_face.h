/**
  ******************************************************************************
  * @file    network_face.h
  * @author  STEdgeAI
  * @date    2026-04-16 14:34:33
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
#ifndef LL_ATON_NETWORK_FACE_H
#define LL_ATON_NETWORK_FACE_H

/******************************************************************************/
#define LL_ATON_NETWORK_FACE_C_MODEL_NAME        "network_face"
#define LL_ATON_NETWORK_FACE_ORIGIN_MODEL_NAME   "blazeface_front_128_int8"

/************************** USER ALLOCATED IOs ********************************/
// No user allocated inputs
// No user allocated outputs

/************************** INPUTS ********************************************/
#define LL_ATON_NETWORK_FACE_IN_NUM        (1)    // Total number of input buffers
// Input buffer 1 -- Input_0_out_0
#define LL_ATON_NETWORK_FACE_IN_1_ALIGNMENT   (32)
#define LL_ATON_NETWORK_FACE_IN_1_SIZE_BYTES  (196608)

/************************** OUTPUTS *******************************************/
#define LL_ATON_NETWORK_FACE_OUT_NUM        (4)    // Total number of output buffers
// Output buffer 1 -- Transpose_246_out_0
#define LL_ATON_NETWORK_FACE_OUT_1_ALIGNMENT   (32)
#define LL_ATON_NETWORK_FACE_OUT_1_SIZE_BYTES  (32768)
// Output buffer 2 -- Transpose_255_out_0
#define LL_ATON_NETWORK_FACE_OUT_2_ALIGNMENT   (32)
#define LL_ATON_NETWORK_FACE_OUT_2_SIZE_BYTES  (2048)
// Output buffer 3 -- Transpose_237_out_0
#define LL_ATON_NETWORK_FACE_OUT_3_ALIGNMENT   (32)
#define LL_ATON_NETWORK_FACE_OUT_3_SIZE_BYTES  (1536)
// Output buffer 4 -- Transpose_228_out_0
#define LL_ATON_NETWORK_FACE_OUT_4_ALIGNMENT   (32)
#define LL_ATON_NETWORK_FACE_OUT_4_SIZE_BYTES  (24576)

#endif /* LL_ATON_NETWORK_FACE_H */
