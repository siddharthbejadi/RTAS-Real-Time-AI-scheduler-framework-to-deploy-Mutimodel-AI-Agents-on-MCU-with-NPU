/**
  ******************************************************************************
  * @file    network_embed.h
  * @author  STEdgeAI
  * @date    2026-04-16 14:55:34
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
#ifndef LL_ATON_NETWORK_EMBED_H
#define LL_ATON_NETWORK_EMBED_H

/******************************************************************************/
#define LL_ATON_NETWORK_EMBED_C_MODEL_NAME        "network_embed"
#define LL_ATON_NETWORK_EMBED_ORIGIN_MODEL_NAME   "mobilefacenet_int8_faces"

/************************** USER ALLOCATED IOs ********************************/
// No user allocated inputs
// No user allocated outputs

/************************** INPUTS ********************************************/
#define LL_ATON_NETWORK_EMBED_IN_NUM        (1)    // Total number of input buffers
// Input buffer 1 -- Input_0_out_0
#define LL_ATON_NETWORK_EMBED_IN_1_ALIGNMENT   (32)
#define LL_ATON_NETWORK_EMBED_IN_1_SIZE_BYTES  (37632)

/************************** OUTPUTS *******************************************/
#define LL_ATON_NETWORK_EMBED_OUT_NUM        (1)    // Total number of output buffers
// Output buffer 1 -- BatchNormalization_290_out_0
#define LL_ATON_NETWORK_EMBED_OUT_1_ALIGNMENT   (32)
#define LL_ATON_NETWORK_EMBED_OUT_1_SIZE_BYTES  (512)

#endif /* LL_ATON_NETWORK_EMBED_H */
