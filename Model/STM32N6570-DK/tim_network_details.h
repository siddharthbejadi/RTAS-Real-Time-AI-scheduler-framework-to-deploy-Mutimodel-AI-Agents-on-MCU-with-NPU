/**
  ******************************************************************************
  * @file    tim_network.h
  * @date    2026-04-19T22:09:26+0100
  * @brief   ST.AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */
#ifndef STAI_TIM_NETWORK_DETAILS_H
#define STAI_TIM_NETWORK_DETAILS_H

#include "stai.h"
#include "layers.h"

const stai_network_details g_tim_network_details = {
  .tensors = (const stai_tensor[105]) {
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S32, .shape = {2, (const int32_t[2]){1, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "input_ids_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "embedding_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "add_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 32, 1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "transpose_output" },
   { .size_bytes = 49152, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 32, 1, 384}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_11_output" },
   { .size_bytes = 49152, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {6, (const int32_t[6]){1, 3, 32, 1, 1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "transpose_1_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {5, (const int32_t[5]){1, 1, 32, 1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "select_2_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "transpose_4_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {5, (const int32_t[5]){1, 1, 32, 1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "select_1_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_74_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_81_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {5, (const int32_t[5]){1, 1, 32, 1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "select_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "transpose_2_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_78_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_82_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_83_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "scaled_dot_product_attention_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {5, (const int32_t[5]){1, 32, 1, 4, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "permute_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "linear_1_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "add_1_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_Reduce_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_Reduce_Mul_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_Sub_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_Mul_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_Reduce_1_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_Reduce_1_Mul_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_Sqrt_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_Reciprocal_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_Mul_1_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_Mul_2_output" },
   { .size_bytes = 32768, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 256}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_96_output" },
   { .size_bytes = 32768, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 256}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_98_output" },
   { .size_bytes = 32768, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 256}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_99_output" },
   { .size_bytes = 32768, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 256}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_101_output" },
   { .size_bytes = 32768, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 256}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "gelu_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_105_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "add_2_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_1_Reduce_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_1_Reduce_Mul_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_1_Sub_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_1_Mul_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_1_Reduce_1_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_1_Reduce_1_Mul_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_1_Sqrt_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_1_Reciprocal_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_1_Mul_1_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_1_Mul_2_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 32, 1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "transpose_6_output" },
   { .size_bytes = 49152, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 32, 1, 384}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_109_output" },
   { .size_bytes = 49152, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {6, (const int32_t[6]){1, 3, 32, 1, 1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "transpose_7_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {5, (const int32_t[5]){1, 1, 32, 1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "select_5_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "transpose_10_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {5, (const int32_t[5]){1, 1, 32, 1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "select_4_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_167_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_174_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {5, (const int32_t[5]){1, 1, 32, 1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "select_3_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "transpose_8_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_171_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_175_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_176_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {4, (const int32_t[4]){1, 4, 32, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "scaled_dot_product_attention_1_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {5, (const int32_t[5]){1, 32, 1, 4, 32}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "permute_1_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "linear_5_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "add_3_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_2_Reduce_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_2_Reduce_Mul_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_2_Sub_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_2_Mul_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_2_Reduce_1_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_2_Reduce_1_Mul_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_2_Sqrt_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_2_Reciprocal_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_2_Mul_1_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_2_Mul_2_output" },
   { .size_bytes = 32768, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 256}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_189_output" },
   { .size_bytes = 32768, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 256}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_191_output" },
   { .size_bytes = 32768, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 256}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_192_output" },
   { .size_bytes = 32768, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 256}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_194_output" },
   { .size_bytes = 32768, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 256}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "gelu_1_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "val_198_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "add_4_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_3_Reduce_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_3_Reduce_Mul_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_3_Sub_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_3_Mul_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_3_Reduce_1_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_3_Reduce_1_Mul_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_3_Sqrt_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_3_Reciprocal_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_3_Mul_1_output" },
   { .size_bytes = 16384, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 32, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_3_Mul_2_output" },
   { .size_bytes = 512, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {3, (const int32_t[3]){1, 1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "select_6_output" },
   { .size_bytes = 512, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_4_Transpose_output" },
   { .size_bytes = 4, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_4_Reduce_output" },
   { .size_bytes = 4, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_4_Reduce_Mul_output" },
   { .size_bytes = 512, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_4_Sub_output" },
   { .size_bytes = 512, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_4_Mul_output" },
   { .size_bytes = 4, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_4_Reduce_1_output" },
   { .size_bytes = 4, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_4_Reduce_1_Mul_output" },
   { .size_bytes = 4, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_4_Sqrt_output" },
   { .size_bytes = 4, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 1}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_4_Reciprocal_output" },
   { .size_bytes = 512, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_4_Mul_1_output" },
   { .size_bytes = 512, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_4_Mul_2_output" },
   { .size_bytes = 512, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 128}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "layer_norm_4_Transpose_out_0_output" },
   { .size_bytes = 36, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_FLOAT32, .shape = {2, (const int32_t[2]){1, 9}}, .scale = {0, NULL}, .zeropoint = {0, NULL}, .name = "logits_output" }
  },
  .nodes = (const stai_node_details[104]){
    {.id = 1, .type = AI_LAYER_GATHER_TYPE, .input_tensors = {1, (const int32_t[1]){0}}, .output_tensors = {1, (const int32_t[1]){1}} }, /* embedding */
    {.id = 2, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {1, (const int32_t[1]){1}}, .output_tensors = {1, (const int32_t[1]){2}} }, /* add */
    {.id = 3, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){2}}, .output_tensors = {1, (const int32_t[1]){3}} }, /* transpose */
    {.id = 5, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){3}}, .output_tensors = {1, (const int32_t[1]){4}} }, /* val_11 */
    {.id = 8, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){4}}, .output_tensors = {1, (const int32_t[1]){5}} }, /* transpose_1 */
    {.id = 12, .type = AI_LAYER_GATHER_TYPE, .input_tensors = {1, (const int32_t[1]){5}}, .output_tensors = {1, (const int32_t[1]){6}} }, /* select_2 */
    {.id = 18, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){6}}, .output_tensors = {1, (const int32_t[1]){7}} }, /* transpose_4 */
    {.id = 11, .type = AI_LAYER_GATHER_TYPE, .input_tensors = {1, (const int32_t[1]){5}}, .output_tensors = {1, (const int32_t[1]){8}} }, /* select_1 */
    {.id = 21, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){8}}, .output_tensors = {1, (const int32_t[1]){9}} }, /* val_74 */
    {.id = 24, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){9}}, .output_tensors = {1, (const int32_t[1]){10}} }, /* val_81 */
    {.id = 10, .type = AI_LAYER_GATHER_TYPE, .input_tensors = {1, (const int32_t[1]){5}}, .output_tensors = {1, (const int32_t[1]){11}} }, /* select */
    {.id = 14, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){11}}, .output_tensors = {1, (const int32_t[1]){12}} }, /* transpose_2 */
    {.id = 23, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){12}}, .output_tensors = {1, (const int32_t[1]){13}} }, /* val_78 */
    {.id = 25, .type = AI_LAYER_MATMUL_TYPE, .input_tensors = {2, (const int32_t[2]){13, 10}}, .output_tensors = {1, (const int32_t[1]){14}} }, /* val_82 */
    {.id = 26, .type = AI_LAYER_SM_TYPE, .input_tensors = {1, (const int32_t[1]){14}}, .output_tensors = {1, (const int32_t[1]){15}} }, /* val_83 */
    {.id = 27, .type = AI_LAYER_MATMUL_TYPE, .input_tensors = {2, (const int32_t[2]){15, 7}}, .output_tensors = {1, (const int32_t[1]){16}} }, /* scaled_dot_product_attention */
    {.id = 28, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){16}}, .output_tensors = {1, (const int32_t[1]){17}} }, /* permute */
    {.id = 30, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){17}}, .output_tensors = {1, (const int32_t[1]){18}} }, /* linear_1 */
    {.id = 33, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){2, 18}}, .output_tensors = {1, (const int32_t[1]){19}} }, /* add_1 */
    {.id = 34, .type = AI_LAYER_REDUCE_TYPE, .input_tensors = {1, (const int32_t[1]){19}}, .output_tensors = {1, (const int32_t[1]){20}} }, /* layer_norm_Reduce */
    {.id = 34, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){20}}, .output_tensors = {1, (const int32_t[1]){21}} }, /* layer_norm_Reduce_Mul */
    {.id = 34, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){19, 21}}, .output_tensors = {1, (const int32_t[1]){22}} }, /* layer_norm_Sub */
    {.id = 34, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){22, 22}}, .output_tensors = {1, (const int32_t[1]){23}} }, /* layer_norm_Mul */
    {.id = 34, .type = AI_LAYER_REDUCE_TYPE, .input_tensors = {1, (const int32_t[1]){23}}, .output_tensors = {1, (const int32_t[1]){24}} }, /* layer_norm_Reduce_1 */
    {.id = 34, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){24}}, .output_tensors = {1, (const int32_t[1]){25}} }, /* layer_norm_Reduce_1_Mul */
    {.id = 34, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){25}}, .output_tensors = {1, (const int32_t[1]){26}} }, /* layer_norm_Sqrt */
    {.id = 34, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){26}}, .output_tensors = {1, (const int32_t[1]){27}} }, /* layer_norm_Reciprocal */
    {.id = 34, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){22, 27}}, .output_tensors = {1, (const int32_t[1]){28}} }, /* layer_norm_Mul_1 */
    {.id = 34, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){28}}, .output_tensors = {1, (const int32_t[1]){29}} }, /* layer_norm_Mul_2 */
    {.id = 36, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){29}}, .output_tensors = {1, (const int32_t[1]){30}} }, /* val_96 */
    {.id = 37, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {1, (const int32_t[1]){30}}, .output_tensors = {1, (const int32_t[1]){31}} }, /* val_98 */
    {.id = 38, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){31}}, .output_tensors = {1, (const int32_t[1]){32}} }, /* val_99 */
    {.id = 40, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){32}}, .output_tensors = {1, (const int32_t[1]){33}} }, /* val_101 */
    {.id = 41, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){30, 33}}, .output_tensors = {1, (const int32_t[1]){34}} }, /* gelu */
    {.id = 43, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){34}}, .output_tensors = {1, (const int32_t[1]){35}} }, /* val_105 */
    {.id = 44, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){29, 35}}, .output_tensors = {1, (const int32_t[1]){36}} }, /* add_2 */
    {.id = 45, .type = AI_LAYER_REDUCE_TYPE, .input_tensors = {1, (const int32_t[1]){36}}, .output_tensors = {1, (const int32_t[1]){37}} }, /* layer_norm_1_Reduce */
    {.id = 45, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){37}}, .output_tensors = {1, (const int32_t[1]){38}} }, /* layer_norm_1_Reduce_Mul */
    {.id = 45, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){36, 38}}, .output_tensors = {1, (const int32_t[1]){39}} }, /* layer_norm_1_Sub */
    {.id = 45, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){39, 39}}, .output_tensors = {1, (const int32_t[1]){40}} }, /* layer_norm_1_Mul */
    {.id = 45, .type = AI_LAYER_REDUCE_TYPE, .input_tensors = {1, (const int32_t[1]){40}}, .output_tensors = {1, (const int32_t[1]){41}} }, /* layer_norm_1_Reduce_1 */
    {.id = 45, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){41}}, .output_tensors = {1, (const int32_t[1]){42}} }, /* layer_norm_1_Reduce_1_Mul */
    {.id = 45, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){42}}, .output_tensors = {1, (const int32_t[1]){43}} }, /* layer_norm_1_Sqrt */
    {.id = 45, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){43}}, .output_tensors = {1, (const int32_t[1]){44}} }, /* layer_norm_1_Reciprocal */
    {.id = 45, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){39, 44}}, .output_tensors = {1, (const int32_t[1]){45}} }, /* layer_norm_1_Mul_1 */
    {.id = 45, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){45}}, .output_tensors = {1, (const int32_t[1]){46}} }, /* layer_norm_1_Mul_2 */
    {.id = 46, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){46}}, .output_tensors = {1, (const int32_t[1]){47}} }, /* transpose_6 */
    {.id = 48, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){47}}, .output_tensors = {1, (const int32_t[1]){48}} }, /* val_109 */
    {.id = 51, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){48}}, .output_tensors = {1, (const int32_t[1]){49}} }, /* transpose_7 */
    {.id = 55, .type = AI_LAYER_GATHER_TYPE, .input_tensors = {1, (const int32_t[1]){49}}, .output_tensors = {1, (const int32_t[1]){50}} }, /* select_5 */
    {.id = 61, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){50}}, .output_tensors = {1, (const int32_t[1]){51}} }, /* transpose_10 */
    {.id = 54, .type = AI_LAYER_GATHER_TYPE, .input_tensors = {1, (const int32_t[1]){49}}, .output_tensors = {1, (const int32_t[1]){52}} }, /* select_4 */
    {.id = 64, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){52}}, .output_tensors = {1, (const int32_t[1]){53}} }, /* val_167 */
    {.id = 67, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){53}}, .output_tensors = {1, (const int32_t[1]){54}} }, /* val_174 */
    {.id = 53, .type = AI_LAYER_GATHER_TYPE, .input_tensors = {1, (const int32_t[1]){49}}, .output_tensors = {1, (const int32_t[1]){55}} }, /* select_3 */
    {.id = 57, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){55}}, .output_tensors = {1, (const int32_t[1]){56}} }, /* transpose_8 */
    {.id = 66, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){56}}, .output_tensors = {1, (const int32_t[1]){57}} }, /* val_171 */
    {.id = 68, .type = AI_LAYER_MATMUL_TYPE, .input_tensors = {2, (const int32_t[2]){57, 54}}, .output_tensors = {1, (const int32_t[1]){58}} }, /* val_175 */
    {.id = 69, .type = AI_LAYER_SM_TYPE, .input_tensors = {1, (const int32_t[1]){58}}, .output_tensors = {1, (const int32_t[1]){59}} }, /* val_176 */
    {.id = 70, .type = AI_LAYER_MATMUL_TYPE, .input_tensors = {2, (const int32_t[2]){59, 51}}, .output_tensors = {1, (const int32_t[1]){60}} }, /* scaled_dot_product_attention_1 */
    {.id = 71, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){60}}, .output_tensors = {1, (const int32_t[1]){61}} }, /* permute_1 */
    {.id = 73, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){61}}, .output_tensors = {1, (const int32_t[1]){62}} }, /* linear_5 */
    {.id = 76, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){46, 62}}, .output_tensors = {1, (const int32_t[1]){63}} }, /* add_3 */
    {.id = 77, .type = AI_LAYER_REDUCE_TYPE, .input_tensors = {1, (const int32_t[1]){63}}, .output_tensors = {1, (const int32_t[1]){64}} }, /* layer_norm_2_Reduce */
    {.id = 77, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){64}}, .output_tensors = {1, (const int32_t[1]){65}} }, /* layer_norm_2_Reduce_Mul */
    {.id = 77, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){63, 65}}, .output_tensors = {1, (const int32_t[1]){66}} }, /* layer_norm_2_Sub */
    {.id = 77, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){66, 66}}, .output_tensors = {1, (const int32_t[1]){67}} }, /* layer_norm_2_Mul */
    {.id = 77, .type = AI_LAYER_REDUCE_TYPE, .input_tensors = {1, (const int32_t[1]){67}}, .output_tensors = {1, (const int32_t[1]){68}} }, /* layer_norm_2_Reduce_1 */
    {.id = 77, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){68}}, .output_tensors = {1, (const int32_t[1]){69}} }, /* layer_norm_2_Reduce_1_Mul */
    {.id = 77, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){69}}, .output_tensors = {1, (const int32_t[1]){70}} }, /* layer_norm_2_Sqrt */
    {.id = 77, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){70}}, .output_tensors = {1, (const int32_t[1]){71}} }, /* layer_norm_2_Reciprocal */
    {.id = 77, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){66, 71}}, .output_tensors = {1, (const int32_t[1]){72}} }, /* layer_norm_2_Mul_1 */
    {.id = 77, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){72}}, .output_tensors = {1, (const int32_t[1]){73}} }, /* layer_norm_2_Mul_2 */
    {.id = 79, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){73}}, .output_tensors = {1, (const int32_t[1]){74}} }, /* val_189 */
    {.id = 80, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {1, (const int32_t[1]){74}}, .output_tensors = {1, (const int32_t[1]){75}} }, /* val_191 */
    {.id = 81, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){75}}, .output_tensors = {1, (const int32_t[1]){76}} }, /* val_192 */
    {.id = 83, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){76}}, .output_tensors = {1, (const int32_t[1]){77}} }, /* val_194 */
    {.id = 84, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){74, 77}}, .output_tensors = {1, (const int32_t[1]){78}} }, /* gelu_1 */
    {.id = 86, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){78}}, .output_tensors = {1, (const int32_t[1]){79}} }, /* val_198 */
    {.id = 87, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){73, 79}}, .output_tensors = {1, (const int32_t[1]){80}} }, /* add_4 */
    {.id = 88, .type = AI_LAYER_REDUCE_TYPE, .input_tensors = {1, (const int32_t[1]){80}}, .output_tensors = {1, (const int32_t[1]){81}} }, /* layer_norm_3_Reduce */
    {.id = 88, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){81}}, .output_tensors = {1, (const int32_t[1]){82}} }, /* layer_norm_3_Reduce_Mul */
    {.id = 88, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){80, 82}}, .output_tensors = {1, (const int32_t[1]){83}} }, /* layer_norm_3_Sub */
    {.id = 88, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){83, 83}}, .output_tensors = {1, (const int32_t[1]){84}} }, /* layer_norm_3_Mul */
    {.id = 88, .type = AI_LAYER_REDUCE_TYPE, .input_tensors = {1, (const int32_t[1]){84}}, .output_tensors = {1, (const int32_t[1]){85}} }, /* layer_norm_3_Reduce_1 */
    {.id = 88, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){85}}, .output_tensors = {1, (const int32_t[1]){86}} }, /* layer_norm_3_Reduce_1_Mul */
    {.id = 88, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){86}}, .output_tensors = {1, (const int32_t[1]){87}} }, /* layer_norm_3_Sqrt */
    {.id = 88, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){87}}, .output_tensors = {1, (const int32_t[1]){88}} }, /* layer_norm_3_Reciprocal */
    {.id = 88, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){83, 88}}, .output_tensors = {1, (const int32_t[1]){89}} }, /* layer_norm_3_Mul_1 */
    {.id = 88, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){89}}, .output_tensors = {1, (const int32_t[1]){90}} }, /* layer_norm_3_Mul_2 */
    {.id = 89, .type = AI_LAYER_GATHER_TYPE, .input_tensors = {1, (const int32_t[1]){90}}, .output_tensors = {1, (const int32_t[1]){91}} }, /* select_6 */
    {.id = 90, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){91}}, .output_tensors = {1, (const int32_t[1]){92}} }, /* layer_norm_4_Transpose */
    {.id = 90, .type = AI_LAYER_REDUCE_TYPE, .input_tensors = {1, (const int32_t[1]){92}}, .output_tensors = {1, (const int32_t[1]){93}} }, /* layer_norm_4_Reduce */
    {.id = 90, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){93}}, .output_tensors = {1, (const int32_t[1]){94}} }, /* layer_norm_4_Reduce_Mul */
    {.id = 90, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){92, 94}}, .output_tensors = {1, (const int32_t[1]){95}} }, /* layer_norm_4_Sub */
    {.id = 90, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){95, 95}}, .output_tensors = {1, (const int32_t[1]){96}} }, /* layer_norm_4_Mul */
    {.id = 90, .type = AI_LAYER_REDUCE_TYPE, .input_tensors = {1, (const int32_t[1]){96}}, .output_tensors = {1, (const int32_t[1]){97}} }, /* layer_norm_4_Reduce_1 */
    {.id = 90, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){97}}, .output_tensors = {1, (const int32_t[1]){98}} }, /* layer_norm_4_Reduce_1_Mul */
    {.id = 90, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){98}}, .output_tensors = {1, (const int32_t[1]){99}} }, /* layer_norm_4_Sqrt */
    {.id = 90, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){99}}, .output_tensors = {1, (const int32_t[1]){100}} }, /* layer_norm_4_Reciprocal */
    {.id = 90, .type = AI_LAYER_ELTWISE_TYPE, .input_tensors = {2, (const int32_t[2]){95, 100}}, .output_tensors = {1, (const int32_t[1]){101}} }, /* layer_norm_4_Mul_1 */
    {.id = 90, .type = AI_LAYER_BN_TYPE, .input_tensors = {1, (const int32_t[1]){101}}, .output_tensors = {1, (const int32_t[1]){102}} }, /* layer_norm_4_Mul_2 */
    {.id = 90, .type = AI_LAYER_TRANSPOSE_TYPE, .input_tensors = {1, (const int32_t[1]){102}}, .output_tensors = {1, (const int32_t[1]){103}} }, /* layer_norm_4_Transpose_out_0 */
    {.id = 91, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){103}}, .output_tensors = {1, (const int32_t[1]){104}} } /* logits */
  },
  .n_nodes = 104
};
#endif

