/**
  ******************************************************************************
  * @file    tim_network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-04-19T22:09:26+0100
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
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

#include "ai_lite_inspect.h"
#include "ai_platform_interface.h"
#include "layers.h"
#include "core_convert.h"
#include "tim_network.h"
#include "tim_network_details.h"
#include "tim_network_data.h"
#include "stai_events.h"

#include "ai_lite_inspect.h"

#include "lite_operators.h"
/*****************************************************************************/
#define STAI_INTERNAL_API_MAJOR               (1)
#define STAI_INTERNAL_API_MINOR               (0)
#define STAI_INTERNAL_API_MICRO               (0)

#define STAI_MAGIC                            (0xB1C00100)

/*****************************************************************************/
#define _STAI_CONCAT_ARG(a, b)     a ## b
#define STAI_CONCAT(a, b)         _STAI_CONCAT_ARG(a, b)

/*!  STAI_CAST SECTION                       *********************************/
#define STAI_CAST(type, expr) \
  ((type)(expr))


/*****************************************************************************/
#define STAI_SIZE(_size) \
  ((stai_size)(_size))

/*****************************************************************************/
#define STAI_INIT_BUFFER(_flags, _size, _address) \
  { \
    .size = (_size), \
    .address = (uintptr_t)(_address), \
    .flags = (_flags), \
  }

#define STAI_INIT_TENSOR(_name, _flags, _fmt, _size_bytes, _shape, _scale, _zeropoint) \
  { \
    .size_bytes = (_size_bytes), \
    .flags = (_flags), \
    .format = (stai_format)(_fmt), \
    .shape = STAI_PACK(_shape), \
    .scale = STAI_PACK(_scale), \
    .zeropoint = STAI_PACK(_zeropoint), \
    .name = (_name) \
  }

#define STAI_INIT_ARRAY(_size, _ptr) \
  { .size = STAI_SIZE(_size), .data = STAI_PACK(_ptr) }


#define STAI_CAST_ARRAY(_type, _size, _ptr) \
  { .size = STAI_SIZE(_size), .data = (_type)STAI_PACK(_ptr) }


#define STAI_DECLARE_ARRAY(_type, _size, ...) \
  { .size = STAI_SIZE(_size), .data = (_type[_size]) { STAI_PACK(__VA_ARGS__) } }


#define STAI_EMPTY_ARRAY() \
  { .size = 0, .data = NULL }


#define STAI_INIT_VERSION(_major, _minor, _micro) \
  { .major = (_major), .minor = (_minor), .micro = (_micro), .reserved = 0x0 }

/*****************************************************************************/
/**  Getters and setters  **/

#define STAI_GET_ARRAY_SIZE(nd_array) \
  (nd_array.size)


#define STAI_GET_ARRAY_ELEM(nd_array, pos) \
  (nd_array.data[(pos)])

#define _STAI_SET_ERROR(net_ctx, cond, value, exit) { \
  if (!(net_ctx)) { return STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE; } \
  if (((uintptr_t)net_ctx) & (_STAI_CONTEXT_ALIGNMENT-1)) { return STAI_ERROR_NETWORK_INVALID_CONTEXT_ALIGNMENT; } \
  if (((value) >= STAI_ERROR_GENERIC) && (cond)) { \
    if ((net_ctx)->_return_code == STAI_SUCCESS) { \
      (net_ctx)->_return_code = (value); \
    } \
    return (exit); \
  } \
}

/*****************************************************************************/
/* TODO REMOVE THESE TWO MACROS */
#define STAI_EVENT_NODE_START_CB
#define STAI_EVENT_NODE_STOP_CB

#ifdef STAI_EVENT_NODE_START_CB
#ifndef _STAI_TIM_NETWORK_EVENT_NODE_START_CB
  #define _STAI_TIM_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...) \
  if (net_ctx->_callback) { \
    const stai_event_node_start_stop _start_event = { \
      .node_id=(_node_id), \
      .buffers={ \
        .size=(_buffers_size), \
        .data=(stai_ptr const*)(const stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__) \
      } \
    }; \
    net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_START, (const void*)&_start_event); \
  }
#endif
#else
  #define _STAI_TIM_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...) \
    do { /* _STAI_TIM_NETWORK_EVENT_NODE_START_CB() */ } while(0);
#endif      /* STAI_EVENT_NODE_START_CB */

#ifdef STAI_EVENT_NODE_STOP_CB
#ifndef _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB
  #define _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...) \
  if (net_ctx->_callback) { \
    const stai_event_node_start_stop _stop_event = { \
      .node_id=(_node_id), \
      .buffers={ \
        .size=(_buffers_size), \
        .data=(stai_ptr const*)(stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__) \
      } \
    }; \
    net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_STOP, (const void*)&_stop_event); \
  }
#endif
#else
  #define _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...) \
    do { /* _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB() */ } while(0);
#endif      /* STAI_EVENT_NODE_STOP_CB */


/*****************************************************************************/
#define _STAI_TIM_NETWORK_MODEL_SIGNATURE     "0x36f243b0f1982cb31e58dc9125d44415"
#define _STAI_TIM_NETWORK_DATETIME            "2026-04-19T22:09:26+0100"
#define _STAI_TIM_NETWORK_COMPILE_DATETIME    __DATE__ " " __TIME__

#define _STAI_CONTEXT_ALIGNMENT        STAI_TIM_NETWORK_CONTEXT_ALIGNMENT

/*****************************************************************************/
#define g_tim_network_activations_1     (NULL)




#if defined(HAVE_TIM_NETWORK_INFO)
/*****************************************************************************/
static const stai_network_info g_tim_network_info = {
  .model_signature = _STAI_TIM_NETWORK_MODEL_SIGNATURE,
  .c_compile_datetime = _STAI_TIM_NETWORK_COMPILE_DATETIME,
  .c_model_name = STAI_TIM_NETWORK_MODEL_NAME,
  .c_model_datetime = _STAI_TIM_NETWORK_DATETIME,
  .c_model_signature = 0x0,
  .runtime_version = STAI_INIT_VERSION(12, 0, 0),
  .tool_version = STAI_INIT_VERSION(4, 0, 0),
  .api_version = STAI_INIT_VERSION(1, 0, 0),
  .n_macc = STAI_TIM_NETWORK_MACC_NUM,
  .n_nodes = STAI_TIM_NETWORK_NODES_NUM,
  .flags = STAI_TIM_NETWORK_FLAGS,
  .n_inputs = STAI_TIM_NETWORK_IN_NUM,
  .n_outputs = STAI_TIM_NETWORK_OUT_NUM,
  .n_activations = STAI_TIM_NETWORK_ACTIVATIONS_NUM,
  .n_weights = STAI_TIM_NETWORK_WEIGHTS_NUM,
  .n_states = STAI_TIM_NETWORK_STATES_NUM,
  .inputs = (stai_tensor[STAI_TIM_NETWORK_IN_NUM]) {
    STAI_INIT_TENSOR(
      STAI_TIM_NETWORK_IN_1_NAME,
      STAI_TIM_NETWORK_IN_1_FLAGS,
      STAI_TIM_NETWORK_IN_1_FORMAT,
      STAI_TIM_NETWORK_IN_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 2, 1, 32),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
    .outputs = (stai_tensor[STAI_TIM_NETWORK_OUT_NUM]) {
    STAI_INIT_TENSOR(
      STAI_TIM_NETWORK_OUT_1_NAME,
      STAI_TIM_NETWORK_OUT_1_FLAGS,
      STAI_TIM_NETWORK_OUT_1_FORMAT,
      STAI_TIM_NETWORK_OUT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 2, 1, 9),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
  .activations = (stai_tensor[STAI_TIM_NETWORK_ACTIVATIONS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_TIM_NETWORK_ACTIVATION_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_TIM_NETWORK_ACTIVATION_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 114688),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
  .weights = (stai_tensor[STAI_TIM_NETWORK_WEIGHTS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_TIM_NETWORK_WEIGHT_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_TIM_NETWORK_WEIGHT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1253712),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },

  .states = NULL
};
#endif

#define _STAI_CONTEXT_ACQUIRE(_net_ctx, _net_handle) \
  _stai_tim_network_context* _net_ctx = (_stai_tim_network_context*)(_net_handle); \
  STAI_ASSERT(_net_ctx != NULL) \
  _STAI_SET_ERROR(_net_ctx, _net_ctx->_magic != STAI_MAGIC, \
                  STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE, _net_ctx->_return_code)


/*****************************************************************************/
static
void _stai_tim_network_check(_stai_tim_network_context* net_ctx)
{
  stai_size idx;

// Check activations status
  for (idx=0; idx<STAI_TIM_NETWORK_ACTIVATIONS_NUM; idx++) {
    if (net_ctx->_activations[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_TIM_NETWORK_ACTIVATIONS_NUM) ? STAI_FLAG_ACTIVATIONS : STAI_FLAG_NONE;
// Check inputs status
  for (idx=0; idx<STAI_TIM_NETWORK_IN_NUM; idx++) {
    if (net_ctx->_inputs[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_TIM_NETWORK_IN_NUM) ? STAI_FLAG_INPUTS : STAI_FLAG_NONE;

  // Check outputs status
  for (idx=0; idx<STAI_TIM_NETWORK_OUT_NUM; idx++) {
    if (net_ctx->_outputs[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_TIM_NETWORK_OUT_NUM) ? STAI_FLAG_OUTPUTS : STAI_FLAG_NONE;

// Check weights status
  for (idx=0; idx<STAI_TIM_NETWORK_WEIGHTS_NUM; idx++) {
    if (net_ctx->_weights[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_TIM_NETWORK_WEIGHTS_NUM) ? STAI_FLAG_WEIGHTS : STAI_FLAG_NONE;
STAI_PRINT("  [_stai_network_check] flags: 0x%08x\n", net_ctx->_flags)
}


/*****************************************************************************/
STAI_API_ENTRY
stai_return_code stai_tim_network_init(
  stai_network* network)
{
  /* Memory where to store internal context is provided by applications as a raw byte buffer */
  _stai_tim_network_context* net_ctx = (_stai_tim_network_context*)(network);
  net_ctx->_return_code = STAI_SUCCESS;
  STAI_PRINT("[Entering Network Init] network(%p) context_size(%d)\n", net_ctx, (int32_t)sizeof(_stai_tim_network_context))

  _STAI_SET_ERROR(net_ctx, STAI_TIM_NETWORK_CONTEXT_SIZE != sizeof(_stai_tim_network_context),
                 STAI_ERROR_NETWORK_INVALID_CONTEXT_SIZE, net_ctx->_return_code)

  {
    const _stai_tim_network_context _tim_network_context = {
      ._magic = STAI_MAGIC,
      ._signature = STAI_TIM_NETWORK_MODEL_SIGNATURE,
      ._flags = STAI_TIM_NETWORK_FLAGS,
      ._return_code = STAI_SUCCESS,
      ._callback = NULL,
      ._callback_cookie = NULL,
      ._activations = {
      (stai_ptr)g_tim_network_activations_1
      },
      ._weights = {
      (stai_ptr)g_tim_network_weights_array
      },
      ._inputs = {
    NULL},
      ._outputs = {
    NULL},
    };

    // Deep copy of internal context to opaque buffer provided by app
    *net_ctx = _tim_network_context;

    _stai_tim_network_check(net_ctx);
  }

  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_tim_network_deinit(
  stai_network* network)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  /*  Reset flags to initial state  */
  net_ctx->_flags = STAI_TIM_NETWORK_FLAGS;
  return net_ctx->_return_code;
}

/*****************************************************************************/





/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  input_ids_output_array, AI_ARRAY_FORMAT_S32|AI_FMT_FLAG_IS_IO,
  NULL, NULL, 32, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  embedding_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  token_embedding_weight_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 42368, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  add_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  embedding_1_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  transpose_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  val_11_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12288, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  val_11_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 49152, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  val_11_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 384, AI_STATIC)

/* Array#9 */
AI_ARRAY_OBJ_DECLARE(
  transpose_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12288, AI_STATIC)

/* Array#10 */
AI_ARRAY_OBJ_DECLARE(
  select_2_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#11 */
AI_ARRAY_OBJ_DECLARE(
  val_21_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 1, AI_STATIC)

/* Array#12 */
AI_ARRAY_OBJ_DECLARE(
  transpose_4_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#13 */
AI_ARRAY_OBJ_DECLARE(
  select_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#14 */
AI_ARRAY_OBJ_DECLARE(
  val_20_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 1, AI_STATIC)

/* Array#15 */
AI_ARRAY_OBJ_DECLARE(
  val_74_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#16 */
AI_ARRAY_OBJ_DECLARE(
  select_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#17 */
AI_ARRAY_OBJ_DECLARE(
  val_19_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 1, AI_STATIC)

/* Array#18 */
AI_ARRAY_OBJ_DECLARE(
  transpose_2_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#19 */
AI_ARRAY_OBJ_DECLARE(
  val_81_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#20 */
AI_ARRAY_OBJ_DECLARE(
  val_78_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#21 */
AI_ARRAY_OBJ_DECLARE(
  val_82_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#22 */
AI_ARRAY_OBJ_DECLARE(
  val_82_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1, AI_STATIC)

/* Array#23 */
AI_ARRAY_OBJ_DECLARE(
  val_83_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#24 */
AI_ARRAY_OBJ_DECLARE(
  scaled_dot_product_attention_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#25 */
AI_ARRAY_OBJ_DECLARE(
  scaled_dot_product_attention_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1, AI_STATIC)

/* Array#26 */
AI_ARRAY_OBJ_DECLARE(
  permute_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#27 */
AI_ARRAY_OBJ_DECLARE(
  linear_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#28 */
AI_ARRAY_OBJ_DECLARE(
  linear_1_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 16384, AI_STATIC)

/* Array#29 */
AI_ARRAY_OBJ_DECLARE(
  linear_1_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 128, AI_STATIC)

/* Array#30 */
AI_ARRAY_OBJ_DECLARE(
  add_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#31 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_Reduce_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#32 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_Reduce_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#33 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_Sub_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#34 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#35 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_Reduce_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#36 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_Reciprocal_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#37 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_Mul_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#38 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_Mul_2_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#39 */
AI_ARRAY_OBJ_DECLARE(
  val_96_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8192, AI_STATIC)

/* Array#40 */
AI_ARRAY_OBJ_DECLARE(
  val_96_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32768, AI_STATIC)

/* Array#41 */
AI_ARRAY_OBJ_DECLARE(
  val_96_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 256, AI_STATIC)

/* Array#42 */
AI_ARRAY_OBJ_DECLARE(
  val_98_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8192, AI_STATIC)

/* Array#43 */
AI_ARRAY_OBJ_DECLARE(
  val_97_3D_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1, AI_STATIC)

/* Array#44 */
AI_ARRAY_OBJ_DECLARE(
  val_101_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8192, AI_STATIC)

/* Array#45 */
AI_ARRAY_OBJ_DECLARE(
  gelu_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8192, AI_STATIC)

/* Array#46 */
AI_ARRAY_OBJ_DECLARE(
  val_105_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#47 */
AI_ARRAY_OBJ_DECLARE(
  val_105_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32768, AI_STATIC)

/* Array#48 */
AI_ARRAY_OBJ_DECLARE(
  val_105_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 128, AI_STATIC)

/* Array#49 */
AI_ARRAY_OBJ_DECLARE(
  add_2_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#50 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_1_Reduce_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#51 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_1_Reduce_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#52 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_1_Sub_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#53 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_1_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#54 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_1_Reduce_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#55 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_1_Reciprocal_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#56 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_1_Mul_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#57 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_1_Mul_2_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#58 */
AI_ARRAY_OBJ_DECLARE(
  transpose_6_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#59 */
AI_ARRAY_OBJ_DECLARE(
  val_109_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12288, AI_STATIC)

/* Array#60 */
AI_ARRAY_OBJ_DECLARE(
  val_109_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 49152, AI_STATIC)

/* Array#61 */
AI_ARRAY_OBJ_DECLARE(
  val_109_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 384, AI_STATIC)

/* Array#62 */
AI_ARRAY_OBJ_DECLARE(
  transpose_7_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12288, AI_STATIC)

/* Array#63 */
AI_ARRAY_OBJ_DECLARE(
  select_5_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#64 */
AI_ARRAY_OBJ_DECLARE(
  transpose_10_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#65 */
AI_ARRAY_OBJ_DECLARE(
  select_4_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#66 */
AI_ARRAY_OBJ_DECLARE(
  val_167_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#67 */
AI_ARRAY_OBJ_DECLARE(
  select_3_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#68 */
AI_ARRAY_OBJ_DECLARE(
  transpose_8_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#69 */
AI_ARRAY_OBJ_DECLARE(
  val_174_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#70 */
AI_ARRAY_OBJ_DECLARE(
  val_171_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#71 */
AI_ARRAY_OBJ_DECLARE(
  val_175_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#72 */
AI_ARRAY_OBJ_DECLARE(
  val_175_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1, AI_STATIC)

/* Array#73 */
AI_ARRAY_OBJ_DECLARE(
  val_176_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#74 */
AI_ARRAY_OBJ_DECLARE(
  scaled_dot_product_attention_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#75 */
AI_ARRAY_OBJ_DECLARE(
  scaled_dot_product_attention_1_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1, AI_STATIC)

/* Array#76 */
AI_ARRAY_OBJ_DECLARE(
  permute_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#77 */
AI_ARRAY_OBJ_DECLARE(
  linear_5_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#78 */
AI_ARRAY_OBJ_DECLARE(
  linear_5_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 16384, AI_STATIC)

/* Array#79 */
AI_ARRAY_OBJ_DECLARE(
  linear_5_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 128, AI_STATIC)

/* Array#80 */
AI_ARRAY_OBJ_DECLARE(
  add_3_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#81 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_2_Reduce_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#82 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_2_Reduce_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#83 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_2_Sub_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#84 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_2_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#85 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_2_Reduce_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#86 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_2_Reciprocal_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#87 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_2_Mul_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#88 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_2_Mul_2_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#89 */
AI_ARRAY_OBJ_DECLARE(
  val_189_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8192, AI_STATIC)

/* Array#90 */
AI_ARRAY_OBJ_DECLARE(
  val_189_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32768, AI_STATIC)

/* Array#91 */
AI_ARRAY_OBJ_DECLARE(
  val_189_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 256, AI_STATIC)

/* Array#92 */
AI_ARRAY_OBJ_DECLARE(
  val_191_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8192, AI_STATIC)

/* Array#93 */
AI_ARRAY_OBJ_DECLARE(
  val_194_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8192, AI_STATIC)

/* Array#94 */
AI_ARRAY_OBJ_DECLARE(
  gelu_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8192, AI_STATIC)

/* Array#95 */
AI_ARRAY_OBJ_DECLARE(
  val_198_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#96 */
AI_ARRAY_OBJ_DECLARE(
  val_198_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32768, AI_STATIC)

/* Array#97 */
AI_ARRAY_OBJ_DECLARE(
  val_198_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 128, AI_STATIC)

/* Array#98 */
AI_ARRAY_OBJ_DECLARE(
  add_4_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#99 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_3_Reduce_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#100 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_3_Reduce_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#101 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_3_Sub_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#102 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_3_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#103 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_3_Reduce_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#104 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_3_Reciprocal_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#105 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_3_Mul_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#106 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_3_Mul_2_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4096, AI_STATIC)

/* Array#107 */
AI_ARRAY_OBJ_DECLARE(
  select_6_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 128, AI_STATIC)

/* Array#108 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_4_Transpose_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 128, AI_STATIC)

/* Array#109 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_4_Reduce_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1, AI_STATIC)

/* Array#110 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_4_Reduce_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1, AI_STATIC)

/* Array#111 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_4_Sub_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 128, AI_STATIC)

/* Array#112 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_4_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 128, AI_STATIC)

/* Array#113 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_4_Reduce_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1, AI_STATIC)

/* Array#114 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_4_Reciprocal_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1, AI_STATIC)

/* Array#115 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_4_Mul_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 128, AI_STATIC)

/* Array#116 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_4_Mul_2_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 128, AI_STATIC)

/* Array#117 */
AI_ARRAY_OBJ_DECLARE(
  layer_norm_4_Transpose_out_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 128, AI_STATIC)



/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  embedding_output, AI_STATIC,
  7, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 32, 1), AI_STRIDE_INIT(4, 4, 4, 512, 16384),
  1, &embedding_output_array, NULL)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  input_ids_output, AI_STATIC,
  11, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &input_ids_output_array, NULL)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  token_embedding_weight, AI_STATIC,
  110, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 331), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &token_embedding_weight_array, NULL)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  add_output, AI_STATIC,
  4, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &add_output_array, NULL)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  embedding_1, AI_STATIC,
  6, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &embedding_1_array, NULL)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  embedding_output0, AI_STATIC,
  8, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &embedding_output_array, NULL)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  add_output0, AI_STATIC,
  5, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 32, 1), AI_STRIDE_INIT(4, 4, 4, 512, 16384),
  1, &add_output_array, NULL)

/* Tensor #7 */
AI_TENSOR_OBJ_DECLARE(
  transpose_output, AI_STATIC,
  120, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &transpose_output_array, NULL)

/* Tensor #8 */
AI_TENSOR_OBJ_DECLARE(
  val_11_bias, AI_STATIC,
  131, 0x0,
  AI_SHAPE_INIT(4, 1, 384, 1, 1), AI_STRIDE_INIT(4, 4, 4, 1536, 1536),
  1, &val_11_bias_array, NULL)

/* Tensor #9 */
AI_TENSOR_OBJ_DECLARE(
  val_11_output, AI_STATIC,
  132, 0x0,
  AI_SHAPE_INIT(4, 1, 384, 1, 32), AI_STRIDE_INIT(4, 4, 4, 1536, 1536),
  1, &val_11_output_array, NULL)

/* Tensor #10 */
AI_TENSOR_OBJ_DECLARE(
  val_11_weights, AI_STATIC,
  134, 0x0,
  AI_SHAPE_INIT(4, 128, 384, 1, 1), AI_STRIDE_INIT(4, 4, 512, 196608, 196608),
  1, &val_11_weights_array, NULL)

/* Tensor #11 */
AI_TENSOR_OBJ_DECLARE(
  transpose_1_output, AI_STATIC,
  112, 0x0,
  AI_SHAPE_INIT(6, 1, 128, 32, 3, 1, 1), AI_STRIDE_INIT(6, 4, 4, 512, 16384, 512, 512),
  1, &transpose_1_output_array, NULL)

/* Tensor #12 */
AI_TENSOR_OBJ_DECLARE(
  val_11_output0, AI_STATIC,
  133, 0x0,
  AI_SHAPE_INIT(6, 1, 128, 32, 1, 1, 3), AI_STRIDE_INIT(6, 4, 4, 1536, 49152, 1536, 512),
  1, &val_11_output_array, NULL)

/* Tensor #13 */
AI_TENSOR_OBJ_DECLARE(
  select_2_output, AI_STATIC,
  99, 0x0,
  AI_SHAPE_INIT(5, 1, 128, 32, 1, 1), AI_STRIDE_INIT(5, 4, 4, 512, 16384, 512),
  1, &select_2_output_array, NULL)

/* Tensor #14 */
AI_TENSOR_OBJ_DECLARE(
  transpose_1_output0, AI_STATIC,
  113, 0x0,
  AI_SHAPE_INIT(5, 1, 128, 32, 3, 1), AI_STRIDE_INIT(5, 4, 4, 512, 16384, 512),
  1, &transpose_1_output_array, NULL)

/* Tensor #15 */
AI_TENSOR_OBJ_DECLARE(
  val_21, AI_STATIC,
  153, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &val_21_array, NULL)

/* Tensor #16 */
AI_TENSOR_OBJ_DECLARE(
  select_2_output0, AI_STATIC,
  100, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 4, 32), AI_STRIDE_INIT(4, 4, 4, 128, 512),
  1, &select_2_output_array, NULL)

/* Tensor #17 */
AI_TENSOR_OBJ_DECLARE(
  transpose_4_output, AI_STATIC,
  115, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &transpose_4_output_array, NULL)

/* Tensor #18 */
AI_TENSOR_OBJ_DECLARE(
  select_1_output, AI_STATIC,
  97, 0x0,
  AI_SHAPE_INIT(5, 1, 128, 32, 1, 1), AI_STRIDE_INIT(5, 4, 4, 512, 16384, 512),
  1, &select_1_output_array, NULL)

/* Tensor #19 */
AI_TENSOR_OBJ_DECLARE(
  val_20, AI_STATIC,
  152, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &val_20_array, NULL)

/* Tensor #20 */
AI_TENSOR_OBJ_DECLARE(
  select_1_output0, AI_STATIC,
  98, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 4, 32), AI_STRIDE_INIT(4, 4, 4, 128, 512),
  1, &select_1_output_array, NULL)

/* Tensor #21 */
AI_TENSOR_OBJ_DECLARE(
  val_74_output, AI_STATIC,
  154, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &val_74_output_array, NULL)

/* Tensor #22 */
AI_TENSOR_OBJ_DECLARE(
  select_output, AI_STATIC,
  108, 0x0,
  AI_SHAPE_INIT(5, 1, 128, 32, 1, 1), AI_STRIDE_INIT(5, 4, 4, 512, 16384, 512),
  1, &select_output_array, NULL)

/* Tensor #23 */
AI_TENSOR_OBJ_DECLARE(
  val_19, AI_STATIC,
  144, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &val_19_array, NULL)

/* Tensor #24 */
AI_TENSOR_OBJ_DECLARE(
  select_output0, AI_STATIC,
  109, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 4, 32), AI_STRIDE_INIT(4, 4, 4, 128, 512),
  1, &select_output_array, NULL)

/* Tensor #25 */
AI_TENSOR_OBJ_DECLARE(
  transpose_2_output, AI_STATIC,
  114, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &transpose_2_output_array, NULL)

/* Tensor #26 */
AI_TENSOR_OBJ_DECLARE(
  val_78_output, AI_STATIC,
  155, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &val_78_output_array, NULL)

/* Tensor #27 */
AI_TENSOR_OBJ_DECLARE(
  val_81_output, AI_STATIC,
  157, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &val_81_output_array, NULL)

/* Tensor #28 */
AI_TENSOR_OBJ_DECLARE(
  val_82_bias, AI_STATIC,
  159, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &val_82_bias_array, NULL)

/* Tensor #29 */
AI_TENSOR_OBJ_DECLARE(
  val_82_output, AI_STATIC,
  160, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &val_82_output_array, NULL)

/* Tensor #30 */
AI_TENSOR_OBJ_DECLARE(
  scaled_dot_product_attention_bias, AI_STATIC,
  94, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &scaled_dot_product_attention_bias_array, NULL)

/* Tensor #31 */
AI_TENSOR_OBJ_DECLARE(
  scaled_dot_product_attention_output, AI_STATIC,
  95, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &scaled_dot_product_attention_output_array, NULL)

/* Tensor #32 */
AI_TENSOR_OBJ_DECLARE(
  val_83_output, AI_STATIC,
  161, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &val_83_output_array, NULL)

/* Tensor #33 */
AI_TENSOR_OBJ_DECLARE(
  permute_output, AI_STATIC,
  89, 0x0,
  AI_SHAPE_INIT(5, 1, 32, 1, 32, 4), AI_STRIDE_INIT(5, 4, 4, 512, 512, 128),
  1, &permute_output_array, NULL)

/* Tensor #34 */
AI_TENSOR_OBJ_DECLARE(
  scaled_dot_product_attention_output0, AI_STATIC,
  96, 0x0,
  AI_SHAPE_INIT(5, 1, 32, 4, 1, 32), AI_STRIDE_INIT(5, 4, 4, 4096, 16384, 128),
  1, &scaled_dot_product_attention_output_array, NULL)

/* Tensor #35 */
AI_TENSOR_OBJ_DECLARE(
  linear_1_bias, AI_STATIC,
  78, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &linear_1_bias_array, NULL)

/* Tensor #36 */
AI_TENSOR_OBJ_DECLARE(
  linear_1_output, AI_STATIC,
  79, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &linear_1_output_array, NULL)

/* Tensor #37 */
AI_TENSOR_OBJ_DECLARE(
  linear_1_weights, AI_STATIC,
  80, 0x0,
  AI_SHAPE_INIT(4, 128, 128, 1, 1), AI_STRIDE_INIT(4, 4, 512, 65536, 65536),
  1, &linear_1_weights_array, NULL)

/* Tensor #38 */
AI_TENSOR_OBJ_DECLARE(
  permute_output0, AI_STATIC,
  90, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &permute_output_array, NULL)

/* Tensor #39 */
AI_TENSOR_OBJ_DECLARE(
  add_1_output, AI_STATIC,
  0, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &add_1_output_array, NULL)

/* Tensor #40 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_Reduce_output, AI_STATIC,
  75, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_Reduce_output_array, NULL)

/* Tensor #41 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_Reduce_Mul_output, AI_STATIC,
  73, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_Reduce_Mul_output_array, NULL)

/* Tensor #42 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_Sub_output, AI_STATIC,
  77, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_Sub_output_array, NULL)

/* Tensor #43 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_Mul_output, AI_STATIC,
  67, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_Mul_output_array, NULL)

/* Tensor #44 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_Reduce_1_output, AI_STATIC,
  71, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_Reduce_1_output_array, NULL)

/* Tensor #45 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_Mul_1_output, AI_STATIC,
  63, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_Mul_1_output_array, NULL)

/* Tensor #46 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_Reciprocal_output, AI_STATIC,
  68, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_Reciprocal_output_array, NULL)

/* Tensor #47 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_Mul_2_output, AI_STATIC,
  65, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_Mul_2_output_array, NULL)

/* Tensor #48 */
AI_TENSOR_OBJ_DECLARE(
  val_96_bias, AI_STATIC,
  162, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 1, 1), AI_STRIDE_INIT(4, 4, 4, 1024, 1024),
  1, &val_96_bias_array, NULL)

/* Tensor #49 */
AI_TENSOR_OBJ_DECLARE(
  val_96_output, AI_STATIC,
  163, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 1, 32), AI_STRIDE_INIT(4, 4, 4, 1024, 1024),
  1, &val_96_output_array, NULL)

/* Tensor #50 */
AI_TENSOR_OBJ_DECLARE(
  val_96_weights, AI_STATIC,
  164, 0x0,
  AI_SHAPE_INIT(4, 128, 256, 1, 1), AI_STRIDE_INIT(4, 4, 512, 131072, 131072),
  1, &val_96_weights_array, NULL)

/* Tensor #51 */
AI_TENSOR_OBJ_DECLARE(
  val_97_3D, AI_STATIC,
  165, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &val_97_3D_array, NULL)

/* Tensor #52 */
AI_TENSOR_OBJ_DECLARE(
  val_98_output, AI_STATIC,
  166, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 1, 32), AI_STRIDE_INIT(4, 4, 4, 1024, 1024),
  1, &val_98_output_array, NULL)

/* Tensor #53 */
AI_TENSOR_OBJ_DECLARE(
  gelu_output, AI_STATIC,
  10, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 1, 32), AI_STRIDE_INIT(4, 4, 4, 1024, 1024),
  1, &gelu_output_array, NULL)

/* Tensor #54 */
AI_TENSOR_OBJ_DECLARE(
  val_101_output, AI_STATIC,
  122, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 1, 32), AI_STRIDE_INIT(4, 4, 4, 1024, 1024),
  1, &val_101_output_array, NULL)

/* Tensor #55 */
AI_TENSOR_OBJ_DECLARE(
  val_105_bias, AI_STATIC,
  124, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &val_105_bias_array, NULL)

/* Tensor #56 */
AI_TENSOR_OBJ_DECLARE(
  val_105_output, AI_STATIC,
  125, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &val_105_output_array, NULL)

/* Tensor #57 */
AI_TENSOR_OBJ_DECLARE(
  val_105_weights, AI_STATIC,
  126, 0x0,
  AI_SHAPE_INIT(4, 256, 128, 1, 1), AI_STRIDE_INIT(4, 4, 1024, 131072, 131072),
  1, &val_105_weights_array, NULL)

/* Tensor #58 */
AI_TENSOR_OBJ_DECLARE(
  add_2_output, AI_STATIC,
  1, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &add_2_output_array, NULL)

/* Tensor #59 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_1_Reduce_output, AI_STATIC,
  22, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_1_Reduce_output_array, NULL)

/* Tensor #60 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_1_Reduce_Mul_output, AI_STATIC,
  21, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_1_Reduce_Mul_output_array, NULL)

/* Tensor #61 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_1_Sub_output, AI_STATIC,
  24, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_1_Sub_output_array, NULL)

/* Tensor #62 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_1_Mul_output, AI_STATIC,
  17, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_1_Mul_output_array, NULL)

/* Tensor #63 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_1_Reduce_1_output, AI_STATIC,
  20, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_1_Reduce_1_output_array, NULL)

/* Tensor #64 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_1_Mul_1_output, AI_STATIC,
  12, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_1_Mul_1_output_array, NULL)

/* Tensor #65 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_1_Reciprocal_output, AI_STATIC,
  18, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_1_Reciprocal_output_array, NULL)

/* Tensor #66 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_1_Mul_2_output0, AI_STATIC,
  15, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 32, 1), AI_STRIDE_INIT(4, 4, 4, 512, 16384),
  1, &layer_norm_1_Mul_2_output_array, NULL)

/* Tensor #67 */
AI_TENSOR_OBJ_DECLARE(
  transpose_6_output, AI_STATIC,
  116, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &transpose_6_output_array, NULL)

/* Tensor #68 */
AI_TENSOR_OBJ_DECLARE(
  val_109_bias, AI_STATIC,
  127, 0x0,
  AI_SHAPE_INIT(4, 1, 384, 1, 1), AI_STRIDE_INIT(4, 4, 4, 1536, 1536),
  1, &val_109_bias_array, NULL)

/* Tensor #69 */
AI_TENSOR_OBJ_DECLARE(
  val_109_output, AI_STATIC,
  128, 0x0,
  AI_SHAPE_INIT(4, 1, 384, 1, 32), AI_STRIDE_INIT(4, 4, 4, 1536, 1536),
  1, &val_109_output_array, NULL)

/* Tensor #70 */
AI_TENSOR_OBJ_DECLARE(
  val_109_weights, AI_STATIC,
  130, 0x0,
  AI_SHAPE_INIT(4, 128, 384, 1, 1), AI_STRIDE_INIT(4, 4, 512, 196608, 196608),
  1, &val_109_weights_array, NULL)

/* Tensor #71 */
AI_TENSOR_OBJ_DECLARE(
  transpose_7_output, AI_STATIC,
  117, 0x0,
  AI_SHAPE_INIT(6, 1, 128, 32, 3, 1, 1), AI_STRIDE_INIT(6, 4, 4, 512, 16384, 512, 512),
  1, &transpose_7_output_array, NULL)

/* Tensor #72 */
AI_TENSOR_OBJ_DECLARE(
  val_109_output0, AI_STATIC,
  129, 0x0,
  AI_SHAPE_INIT(6, 1, 128, 32, 1, 1, 3), AI_STRIDE_INIT(6, 4, 4, 1536, 49152, 1536, 512),
  1, &val_109_output_array, NULL)

/* Tensor #73 */
AI_TENSOR_OBJ_DECLARE(
  select_5_output, AI_STATIC,
  105, 0x0,
  AI_SHAPE_INIT(5, 1, 128, 32, 1, 1), AI_STRIDE_INIT(5, 4, 4, 512, 16384, 512),
  1, &select_5_output_array, NULL)

/* Tensor #74 */
AI_TENSOR_OBJ_DECLARE(
  transpose_7_output0, AI_STATIC,
  118, 0x0,
  AI_SHAPE_INIT(5, 1, 128, 32, 3, 1), AI_STRIDE_INIT(5, 4, 4, 512, 16384, 512),
  1, &transpose_7_output_array, NULL)

/* Tensor #75 */
AI_TENSOR_OBJ_DECLARE(
  select_5_output0, AI_STATIC,
  106, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 4, 32), AI_STRIDE_INIT(4, 4, 4, 128, 512),
  1, &select_5_output_array, NULL)

/* Tensor #76 */
AI_TENSOR_OBJ_DECLARE(
  transpose_10_output, AI_STATIC,
  111, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &transpose_10_output_array, NULL)

/* Tensor #77 */
AI_TENSOR_OBJ_DECLARE(
  select_4_output, AI_STATIC,
  103, 0x0,
  AI_SHAPE_INIT(5, 1, 128, 32, 1, 1), AI_STRIDE_INIT(5, 4, 4, 512, 16384, 512),
  1, &select_4_output_array, NULL)

/* Tensor #78 */
AI_TENSOR_OBJ_DECLARE(
  select_4_output0, AI_STATIC,
  104, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 4, 32), AI_STRIDE_INIT(4, 4, 4, 128, 512),
  1, &select_4_output_array, NULL)

/* Tensor #79 */
AI_TENSOR_OBJ_DECLARE(
  val_167_output, AI_STATIC,
  135, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &val_167_output_array, NULL)

/* Tensor #80 */
AI_TENSOR_OBJ_DECLARE(
  select_3_output, AI_STATIC,
  101, 0x0,
  AI_SHAPE_INIT(5, 1, 128, 32, 1, 1), AI_STRIDE_INIT(5, 4, 4, 512, 16384, 512),
  1, &select_3_output_array, NULL)

/* Tensor #81 */
AI_TENSOR_OBJ_DECLARE(
  select_3_output0, AI_STATIC,
  102, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 4, 32), AI_STRIDE_INIT(4, 4, 4, 128, 512),
  1, &select_3_output_array, NULL)

/* Tensor #82 */
AI_TENSOR_OBJ_DECLARE(
  transpose_8_output, AI_STATIC,
  119, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &transpose_8_output_array, NULL)

/* Tensor #83 */
AI_TENSOR_OBJ_DECLARE(
  val_171_output, AI_STATIC,
  136, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &val_171_output_array, NULL)

/* Tensor #84 */
AI_TENSOR_OBJ_DECLARE(
  val_174_output, AI_STATIC,
  137, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &val_174_output_array, NULL)

/* Tensor #85 */
AI_TENSOR_OBJ_DECLARE(
  val_175_bias, AI_STATIC,
  138, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &val_175_bias_array, NULL)

/* Tensor #86 */
AI_TENSOR_OBJ_DECLARE(
  val_175_output, AI_STATIC,
  139, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &val_175_output_array, NULL)

/* Tensor #87 */
AI_TENSOR_OBJ_DECLARE(
  scaled_dot_product_attention_1_bias, AI_STATIC,
  91, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &scaled_dot_product_attention_1_bias_array, NULL)

/* Tensor #88 */
AI_TENSOR_OBJ_DECLARE(
  scaled_dot_product_attention_1_output, AI_STATIC,
  92, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &scaled_dot_product_attention_1_output_array, NULL)

/* Tensor #89 */
AI_TENSOR_OBJ_DECLARE(
  val_176_output, AI_STATIC,
  140, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 32, 4), AI_STRIDE_INIT(4, 4, 4, 128, 4096),
  1, &val_176_output_array, NULL)

/* Tensor #90 */
AI_TENSOR_OBJ_DECLARE(
  permute_1_output, AI_STATIC,
  87, 0x0,
  AI_SHAPE_INIT(5, 1, 32, 1, 32, 4), AI_STRIDE_INIT(5, 4, 4, 512, 512, 128),
  1, &permute_1_output_array, NULL)

/* Tensor #91 */
AI_TENSOR_OBJ_DECLARE(
  scaled_dot_product_attention_1_output0, AI_STATIC,
  93, 0x0,
  AI_SHAPE_INIT(5, 1, 32, 4, 1, 32), AI_STRIDE_INIT(5, 4, 4, 4096, 16384, 128),
  1, &scaled_dot_product_attention_1_output_array, NULL)

/* Tensor #92 */
AI_TENSOR_OBJ_DECLARE(
  linear_5_bias, AI_STATIC,
  81, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &linear_5_bias_array, NULL)

/* Tensor #93 */
AI_TENSOR_OBJ_DECLARE(
  linear_5_output, AI_STATIC,
  82, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &linear_5_output_array, NULL)

/* Tensor #94 */
AI_TENSOR_OBJ_DECLARE(
  linear_5_weights, AI_STATIC,
  83, 0x0,
  AI_SHAPE_INIT(4, 128, 128, 1, 1), AI_STRIDE_INIT(4, 4, 512, 65536, 65536),
  1, &linear_5_weights_array, NULL)

/* Tensor #95 */
AI_TENSOR_OBJ_DECLARE(
  permute_1_output0, AI_STATIC,
  88, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &permute_1_output_array, NULL)

/* Tensor #96 */
AI_TENSOR_OBJ_DECLARE(
  add_3_output, AI_STATIC,
  2, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &add_3_output_array, NULL)

/* Tensor #97 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_1_Mul_2_output, AI_STATIC,
  14, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_1_Mul_2_output_array, NULL)

/* Tensor #98 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_2_Reduce_output, AI_STATIC,
  34, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_2_Reduce_output_array, NULL)

/* Tensor #99 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_2_Reduce_Mul_output, AI_STATIC,
  33, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_2_Reduce_Mul_output_array, NULL)

/* Tensor #100 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_2_Sub_output, AI_STATIC,
  36, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_2_Sub_output_array, NULL)

/* Tensor #101 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_2_Mul_output, AI_STATIC,
  29, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_2_Mul_output_array, NULL)

/* Tensor #102 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_2_Reduce_1_output, AI_STATIC,
  32, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_2_Reduce_1_output_array, NULL)

/* Tensor #103 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_2_Mul_1_output, AI_STATIC,
  25, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_2_Mul_1_output_array, NULL)

/* Tensor #104 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_2_Reciprocal_output, AI_STATIC,
  30, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_2_Reciprocal_output_array, NULL)

/* Tensor #105 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_2_Mul_2_output, AI_STATIC,
  27, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_2_Mul_2_output_array, NULL)

/* Tensor #106 */
AI_TENSOR_OBJ_DECLARE(
  val_189_bias, AI_STATIC,
  141, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 1, 1), AI_STRIDE_INIT(4, 4, 4, 1024, 1024),
  1, &val_189_bias_array, NULL)

/* Tensor #107 */
AI_TENSOR_OBJ_DECLARE(
  val_189_output, AI_STATIC,
  142, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 1, 32), AI_STRIDE_INIT(4, 4, 4, 1024, 1024),
  1, &val_189_output_array, NULL)

/* Tensor #108 */
AI_TENSOR_OBJ_DECLARE(
  val_189_weights, AI_STATIC,
  143, 0x0,
  AI_SHAPE_INIT(4, 128, 256, 1, 1), AI_STRIDE_INIT(4, 4, 512, 131072, 131072),
  1, &val_189_weights_array, NULL)

/* Tensor #109 */
AI_TENSOR_OBJ_DECLARE(
  val_191_output, AI_STATIC,
  145, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 1, 32), AI_STRIDE_INIT(4, 4, 4, 1024, 1024),
  1, &val_191_output_array, NULL)

/* Tensor #110 */
AI_TENSOR_OBJ_DECLARE(
  gelu_1_output, AI_STATIC,
  9, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 1, 32), AI_STRIDE_INIT(4, 4, 4, 1024, 1024),
  1, &gelu_1_output_array, NULL)

/* Tensor #111 */
AI_TENSOR_OBJ_DECLARE(
  val_194_output, AI_STATIC,
  147, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 1, 32), AI_STRIDE_INIT(4, 4, 4, 1024, 1024),
  1, &val_194_output_array, NULL)

/* Tensor #112 */
AI_TENSOR_OBJ_DECLARE(
  val_198_bias, AI_STATIC,
  148, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &val_198_bias_array, NULL)

/* Tensor #113 */
AI_TENSOR_OBJ_DECLARE(
  val_198_output, AI_STATIC,
  149, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &val_198_output_array, NULL)

/* Tensor #114 */
AI_TENSOR_OBJ_DECLARE(
  val_198_weights, AI_STATIC,
  150, 0x0,
  AI_SHAPE_INIT(4, 256, 128, 1, 1), AI_STRIDE_INIT(4, 4, 1024, 131072, 131072),
  1, &val_198_weights_array, NULL)

/* Tensor #115 */
AI_TENSOR_OBJ_DECLARE(
  add_4_output, AI_STATIC,
  3, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &add_4_output_array, NULL)

/* Tensor #116 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_3_Reduce_output, AI_STATIC,
  46, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_3_Reduce_output_array, NULL)

/* Tensor #117 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_3_Reduce_Mul_output, AI_STATIC,
  45, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_3_Reduce_Mul_output_array, NULL)

/* Tensor #118 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_3_Sub_output, AI_STATIC,
  48, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_3_Sub_output_array, NULL)

/* Tensor #119 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_3_Mul_output, AI_STATIC,
  41, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_3_Mul_output_array, NULL)

/* Tensor #120 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_3_Reduce_1_output, AI_STATIC,
  44, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_3_Reduce_1_output_array, NULL)

/* Tensor #121 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_3_Mul_1_output, AI_STATIC,
  37, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_3_Mul_1_output_array, NULL)

/* Tensor #122 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_3_Reciprocal_output, AI_STATIC,
  42, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 32), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_3_Reciprocal_output_array, NULL)

/* Tensor #123 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_3_Mul_2_output, AI_STATIC,
  39, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 32), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_3_Mul_2_output_array, NULL)

/* Tensor #124 */
AI_TENSOR_OBJ_DECLARE(
  select_6_output, AI_STATIC,
  107, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &select_6_output_array, NULL)

/* Tensor #125 */
AI_TENSOR_OBJ_DECLARE(
  val_19_1, AI_STATIC,
  151, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &val_19_array, NULL)

/* Tensor #126 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_4_Transpose_output, AI_STATIC,
  62, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_4_Transpose_output_array, NULL)

/* Tensor #127 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_4_Reduce_output, AI_STATIC,
  58, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_4_Reduce_output_array, NULL)

/* Tensor #128 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_4_Reduce_Mul_output, AI_STATIC,
  57, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_4_Reduce_Mul_output_array, NULL)

/* Tensor #129 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_4_Sub_output, AI_STATIC,
  60, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_4_Sub_output_array, NULL)

/* Tensor #130 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_4_Mul_output, AI_STATIC,
  53, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_4_Mul_output_array, NULL)

/* Tensor #131 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_4_Reduce_1_output, AI_STATIC,
  56, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_4_Reduce_1_output_array, NULL)

/* Tensor #132 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_4_Mul_1_output, AI_STATIC,
  49, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_4_Mul_1_output_array, NULL)

/* Tensor #133 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_4_Reciprocal_output, AI_STATIC,
  54, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &layer_norm_4_Reciprocal_output_array, NULL)

/* Tensor #134 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_4_Mul_2_output, AI_STATIC,
  51, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_4_Mul_2_output_array, NULL)

/* Tensor #135 */
AI_TENSOR_OBJ_DECLARE(
  layer_norm_4_Transpose_out_0_output, AI_STATIC,
  61, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &layer_norm_4_Transpose_out_0_output_array, NULL)


AI_TENSOR_CHAIN_OBJ_DECLARE(
  embedding_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &token_embedding_weight, &input_ids_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &embedding_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  embedding_layer, 1,
  GATHER_TYPE, 0x0, NULL,
  gather, forward_gather,
  &embedding_chain,
  NULL, &embedding_layer, AI_STATIC, 
  .axis = AI_SHAPE_HEIGHT, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  add_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &embedding_output0, &embedding_1),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &add_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  add_layer, 2,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &add_chain,
  NULL, &add_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &add_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_layer, 3,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_chain,
  NULL, &transpose_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_WIDTH, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  val_11_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &val_11_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &val_11_weights, &val_11_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  val_11_layer, 5,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &val_11_chain,
  NULL, &val_11_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &val_11_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_1_layer, 8,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_1_chain,
  NULL, &transpose_1_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_EXTENSION, AI_SHAPE_DEPTH, AI_SHAPE_HEIGHT), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  select_2_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &transpose_1_output0, &val_21),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &select_2_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  select_2_layer, 12,
  GATHER_TYPE, 0x0, NULL,
  gather, forward_gather,
  &select_2_chain,
  NULL, &select_2_layer, AI_STATIC, 
  .axis = AI_SHAPE_HEIGHT, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_4_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &select_2_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_4_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_4_layer, 18,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_4_chain,
  NULL, &transpose_4_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_WIDTH, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  select_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &transpose_1_output0, &val_20),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &select_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  select_1_layer, 11,
  GATHER_TYPE, 0x0, NULL,
  gather, forward_gather,
  &select_1_chain,
  NULL, &select_1_layer, AI_STATIC, 
  .axis = AI_SHAPE_HEIGHT, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  val_74_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &select_1_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &val_74_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  val_74_layer, 21,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &val_74_chain,
  NULL, &val_74_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  select_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &transpose_1_output0, &val_19),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &select_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  select_layer, 10,
  GATHER_TYPE, 0x0, NULL,
  gather, forward_gather,
  &select_chain,
  NULL, &select_layer, AI_STATIC, 
  .axis = AI_SHAPE_HEIGHT, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_2_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &select_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_2_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_2_layer, 14,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_2_chain,
  NULL, &transpose_2_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_WIDTH, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  val_82_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &val_78_output, &val_81_output, &val_82_bias),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &val_82_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  val_82_layer, 25,
  MATMUL_TYPE, 0x0, NULL,
  matmul, forward_matmul,
  &val_82_chain,
  NULL, &val_82_layer, AI_STATIC, 
  .alpha = 1.0, 
  .beta = 1.0, 
  .tA = 0, 
  .tB = 0, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  scaled_dot_product_attention_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &val_83_output, &transpose_4_output, &scaled_dot_product_attention_bias),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &scaled_dot_product_attention_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  scaled_dot_product_attention_layer, 27,
  MATMUL_TYPE, 0x0, NULL,
  matmul, forward_matmul,
  &scaled_dot_product_attention_chain,
  NULL, &scaled_dot_product_attention_layer, AI_STATIC, 
  .alpha = 1.0, 
  .beta = 1.0, 
  .tA = 0, 
  .tB = 0, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  permute_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &scaled_dot_product_attention_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &permute_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  permute_layer, 28,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &permute_chain,
  NULL, &permute_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_DEPTH, AI_SHAPE_WIDTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  linear_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &permute_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &linear_1_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &linear_1_weights, &linear_1_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  linear_1_layer, 30,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &linear_1_chain,
  NULL, &linear_1_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  add_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &add_output, &linear_1_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &add_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  add_1_layer, 33,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &add_1_chain,
  NULL, &add_1_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)


AI_STATIC_CONST ai_float layer_norm_Reduce_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    layer_norm_Reduce_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    layer_norm_Reduce_neutral_value_data, layer_norm_Reduce_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_Reduce_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &add_1_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_Reduce_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_Reduce_layer, 34,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &layer_norm_Reduce_chain,
  NULL, &layer_norm_Reduce_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &layer_norm_Reduce_neutral_value, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_Sub_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &add_1_output, &layer_norm_Reduce_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_Sub_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_Sub_layer, 34,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_Sub_chain,
  NULL, &layer_norm_Sub_layer, AI_STATIC, 
  .operation = ai_sub_f32, 
  .buffer_operation = ai_sub_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_Mul_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_Sub_output, &layer_norm_Sub_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_Mul_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_Mul_layer, 34,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_Mul_chain,
  NULL, &layer_norm_Mul_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)


AI_STATIC_CONST ai_float layer_norm_Reduce_1_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    layer_norm_Reduce_1_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    layer_norm_Reduce_1_neutral_value_data, layer_norm_Reduce_1_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_Reduce_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_Reduce_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_Reduce_1_layer, 34,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &layer_norm_Reduce_1_chain,
  NULL, &layer_norm_Reduce_1_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &layer_norm_Reduce_1_neutral_value, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_Mul_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_Sub_output, &layer_norm_Reciprocal_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_Mul_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_Mul_1_layer, 34,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_Mul_1_chain,
  NULL, &layer_norm_Mul_1_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  val_96_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_Mul_2_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &val_96_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &val_96_weights, &val_96_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  val_96_layer, 36,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &val_96_chain,
  NULL, &val_96_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  val_98_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &val_96_output, &val_97_3D),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &val_98_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  val_98_layer, 37,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &val_98_chain,
  NULL, &val_98_layer, AI_STATIC, 
  .operation = ai_div_f32, 
  .buffer_operation = ai_div_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  gelu_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &val_96_output, &val_101_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gelu_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  gelu_layer, 41,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &gelu_chain,
  NULL, &gelu_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  val_105_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gelu_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &val_105_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &val_105_weights, &val_105_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  val_105_layer, 43,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &val_105_chain,
  NULL, &val_105_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  add_2_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_Mul_2_output, &val_105_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &add_2_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  add_2_layer, 44,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &add_2_chain,
  NULL, &add_2_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)


AI_STATIC_CONST ai_float layer_norm_1_Reduce_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    layer_norm_1_Reduce_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    layer_norm_1_Reduce_neutral_value_data, layer_norm_1_Reduce_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_1_Reduce_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &add_2_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_1_Reduce_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_1_Reduce_layer, 45,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &layer_norm_1_Reduce_chain,
  NULL, &layer_norm_1_Reduce_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &layer_norm_1_Reduce_neutral_value, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_1_Sub_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &add_2_output, &layer_norm_1_Reduce_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_1_Sub_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_1_Sub_layer, 45,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_1_Sub_chain,
  NULL, &layer_norm_1_Sub_layer, AI_STATIC, 
  .operation = ai_sub_f32, 
  .buffer_operation = ai_sub_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_1_Mul_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_1_Sub_output, &layer_norm_1_Sub_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_1_Mul_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_1_Mul_layer, 45,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_1_Mul_chain,
  NULL, &layer_norm_1_Mul_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)


AI_STATIC_CONST ai_float layer_norm_1_Reduce_1_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    layer_norm_1_Reduce_1_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    layer_norm_1_Reduce_1_neutral_value_data, layer_norm_1_Reduce_1_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_1_Reduce_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_1_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_1_Reduce_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_1_Reduce_1_layer, 45,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &layer_norm_1_Reduce_1_chain,
  NULL, &layer_norm_1_Reduce_1_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &layer_norm_1_Reduce_1_neutral_value, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_1_Mul_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_1_Sub_output, &layer_norm_1_Reciprocal_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_1_Mul_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_1_Mul_1_layer, 45,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_1_Mul_1_chain,
  NULL, &layer_norm_1_Mul_1_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_6_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_1_Mul_2_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_6_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_6_layer, 46,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_6_chain,
  NULL, &transpose_6_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_WIDTH, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  val_109_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_6_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &val_109_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &val_109_weights, &val_109_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  val_109_layer, 48,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &val_109_chain,
  NULL, &val_109_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_7_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &val_109_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_7_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_7_layer, 51,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_7_chain,
  NULL, &transpose_7_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_EXTENSION, AI_SHAPE_DEPTH, AI_SHAPE_HEIGHT), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  select_5_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &transpose_7_output0, &val_21),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &select_5_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  select_5_layer, 55,
  GATHER_TYPE, 0x0, NULL,
  gather, forward_gather,
  &select_5_chain,
  NULL, &select_5_layer, AI_STATIC, 
  .axis = AI_SHAPE_HEIGHT, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_10_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &select_5_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_10_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_10_layer, 61,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_10_chain,
  NULL, &transpose_10_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_WIDTH, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  select_4_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &transpose_7_output0, &val_20),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &select_4_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  select_4_layer, 54,
  GATHER_TYPE, 0x0, NULL,
  gather, forward_gather,
  &select_4_chain,
  NULL, &select_4_layer, AI_STATIC, 
  .axis = AI_SHAPE_HEIGHT, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  val_167_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &select_4_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &val_167_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  val_167_layer, 64,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &val_167_chain,
  NULL, &val_167_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  select_3_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &transpose_7_output0, &val_19),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &select_3_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  select_3_layer, 53,
  GATHER_TYPE, 0x0, NULL,
  gather, forward_gather,
  &select_3_chain,
  NULL, &select_3_layer, AI_STATIC, 
  .axis = AI_SHAPE_HEIGHT, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_8_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &select_3_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_8_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_8_layer, 57,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_8_chain,
  NULL, &transpose_8_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_WIDTH, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  val_175_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &val_171_output, &val_174_output, &val_175_bias),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &val_175_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  val_175_layer, 68,
  MATMUL_TYPE, 0x0, NULL,
  matmul, forward_matmul,
  &val_175_chain,
  NULL, &val_175_layer, AI_STATIC, 
  .alpha = 1.0, 
  .beta = 1.0, 
  .tA = 0, 
  .tB = 0, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  scaled_dot_product_attention_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &val_176_output, &transpose_10_output, &scaled_dot_product_attention_1_bias),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &scaled_dot_product_attention_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  scaled_dot_product_attention_1_layer, 70,
  MATMUL_TYPE, 0x0, NULL,
  matmul, forward_matmul,
  &scaled_dot_product_attention_1_chain,
  NULL, &scaled_dot_product_attention_1_layer, AI_STATIC, 
  .alpha = 1.0, 
  .beta = 1.0, 
  .tA = 0, 
  .tB = 0, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  permute_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &scaled_dot_product_attention_1_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &permute_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  permute_1_layer, 71,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &permute_1_chain,
  NULL, &permute_1_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_DEPTH, AI_SHAPE_WIDTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  linear_5_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &permute_1_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &linear_5_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &linear_5_weights, &linear_5_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  linear_5_layer, 73,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &linear_5_chain,
  NULL, &linear_5_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  add_3_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_1_Mul_2_output, &linear_5_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &add_3_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  add_3_layer, 76,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &add_3_chain,
  NULL, &add_3_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)


AI_STATIC_CONST ai_float layer_norm_2_Reduce_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    layer_norm_2_Reduce_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    layer_norm_2_Reduce_neutral_value_data, layer_norm_2_Reduce_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_2_Reduce_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &add_3_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_2_Reduce_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_2_Reduce_layer, 77,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &layer_norm_2_Reduce_chain,
  NULL, &layer_norm_2_Reduce_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &layer_norm_2_Reduce_neutral_value, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_2_Sub_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &add_3_output, &layer_norm_2_Reduce_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_2_Sub_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_2_Sub_layer, 77,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_2_Sub_chain,
  NULL, &layer_norm_2_Sub_layer, AI_STATIC, 
  .operation = ai_sub_f32, 
  .buffer_operation = ai_sub_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_2_Mul_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_2_Sub_output, &layer_norm_2_Sub_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_2_Mul_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_2_Mul_layer, 77,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_2_Mul_chain,
  NULL, &layer_norm_2_Mul_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)


AI_STATIC_CONST ai_float layer_norm_2_Reduce_1_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    layer_norm_2_Reduce_1_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    layer_norm_2_Reduce_1_neutral_value_data, layer_norm_2_Reduce_1_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_2_Reduce_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_2_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_2_Reduce_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_2_Reduce_1_layer, 77,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &layer_norm_2_Reduce_1_chain,
  NULL, &layer_norm_2_Reduce_1_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &layer_norm_2_Reduce_1_neutral_value, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_2_Mul_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_2_Sub_output, &layer_norm_2_Reciprocal_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_2_Mul_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_2_Mul_1_layer, 77,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_2_Mul_1_chain,
  NULL, &layer_norm_2_Mul_1_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  val_189_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_2_Mul_2_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &val_189_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &val_189_weights, &val_189_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  val_189_layer, 79,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &val_189_chain,
  NULL, &val_189_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  val_191_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &val_189_output, &val_97_3D),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &val_191_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  val_191_layer, 80,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &val_191_chain,
  NULL, &val_191_layer, AI_STATIC, 
  .operation = ai_div_f32, 
  .buffer_operation = ai_div_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  gelu_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &val_189_output, &val_194_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gelu_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  gelu_1_layer, 84,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &gelu_1_chain,
  NULL, &gelu_1_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  val_198_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gelu_1_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &val_198_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &val_198_weights, &val_198_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  val_198_layer, 86,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &val_198_chain,
  NULL, &val_198_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  add_4_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_2_Mul_2_output, &val_198_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &add_4_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  add_4_layer, 87,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &add_4_chain,
  NULL, &add_4_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)


AI_STATIC_CONST ai_float layer_norm_3_Reduce_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    layer_norm_3_Reduce_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    layer_norm_3_Reduce_neutral_value_data, layer_norm_3_Reduce_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_3_Reduce_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &add_4_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_3_Reduce_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_3_Reduce_layer, 88,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &layer_norm_3_Reduce_chain,
  NULL, &layer_norm_3_Reduce_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &layer_norm_3_Reduce_neutral_value, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_3_Sub_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &add_4_output, &layer_norm_3_Reduce_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_3_Sub_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_3_Sub_layer, 88,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_3_Sub_chain,
  NULL, &layer_norm_3_Sub_layer, AI_STATIC, 
  .operation = ai_sub_f32, 
  .buffer_operation = ai_sub_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_3_Mul_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_3_Sub_output, &layer_norm_3_Sub_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_3_Mul_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_3_Mul_layer, 88,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_3_Mul_chain,
  NULL, &layer_norm_3_Mul_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)


AI_STATIC_CONST ai_float layer_norm_3_Reduce_1_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    layer_norm_3_Reduce_1_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    layer_norm_3_Reduce_1_neutral_value_data, layer_norm_3_Reduce_1_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_3_Reduce_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_3_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_3_Reduce_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_3_Reduce_1_layer, 88,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &layer_norm_3_Reduce_1_chain,
  NULL, &layer_norm_3_Reduce_1_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &layer_norm_3_Reduce_1_neutral_value, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_3_Mul_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_3_Sub_output, &layer_norm_3_Reciprocal_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_3_Mul_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_3_Mul_1_layer, 88,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_3_Mul_1_chain,
  NULL, &layer_norm_3_Mul_1_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  select_6_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_3_Mul_2_output, &val_19_1),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &select_6_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  select_6_layer, 89,
  GATHER_TYPE, 0x0, NULL,
  gather, forward_gather,
  &select_6_chain,
  NULL, &select_6_layer, AI_STATIC, 
  .axis = AI_SHAPE_HEIGHT, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_4_Transpose_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &select_6_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_4_Transpose_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_4_Transpose_layer, 90,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &layer_norm_4_Transpose_chain,
  NULL, &layer_norm_4_Transpose_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_HEIGHT, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)


AI_STATIC_CONST ai_float layer_norm_4_Reduce_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    layer_norm_4_Reduce_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    layer_norm_4_Reduce_neutral_value_data, layer_norm_4_Reduce_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_4_Reduce_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_4_Transpose_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_4_Reduce_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_4_Reduce_layer, 90,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &layer_norm_4_Reduce_chain,
  NULL, &layer_norm_4_Reduce_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &layer_norm_4_Reduce_neutral_value, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_4_Sub_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_4_Transpose_output, &layer_norm_4_Reduce_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_4_Sub_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_4_Sub_layer, 90,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_4_Sub_chain,
  NULL, &layer_norm_4_Sub_layer, AI_STATIC, 
  .operation = ai_sub_f32, 
  .buffer_operation = ai_sub_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_4_Mul_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_4_Sub_output, &layer_norm_4_Sub_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_4_Mul_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_4_Mul_layer, 90,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_4_Mul_chain,
  NULL, &layer_norm_4_Mul_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)


AI_STATIC_CONST ai_float layer_norm_4_Reduce_1_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    layer_norm_4_Reduce_1_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    layer_norm_4_Reduce_1_neutral_value_data, layer_norm_4_Reduce_1_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_4_Reduce_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_4_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_4_Reduce_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_4_Reduce_1_layer, 90,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &layer_norm_4_Reduce_1_chain,
  NULL, &layer_norm_4_Reduce_1_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &layer_norm_4_Reduce_1_neutral_value, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_4_Mul_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &layer_norm_4_Sub_output, &layer_norm_4_Reciprocal_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_4_Mul_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_4_Mul_1_layer, 90,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &layer_norm_4_Mul_1_chain,
  NULL, &layer_norm_4_Mul_1_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  layer_norm_4_Transpose_out_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_4_Mul_2_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &layer_norm_4_Transpose_out_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  layer_norm_4_Transpose_out_0_layer, 90,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &layer_norm_4_Transpose_out_0_chain,
  NULL, &layer_norm_4_Transpose_out_0_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_HEIGHT, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)
/**  Hybrid layers declarations section  *************************************/
void forward_lite_gather_embedding(_stai_tim_network_context* net_ctx)
{
  token_embedding_weight_array.data = AI_PTR(net_ctx->_weights[0] + 16416);
  token_embedding_weight_array.data_start = AI_PTR(net_ctx->_weights[0] + 16416);
  input_ids_output_array.data = AI_PTR(net_ctx->_inputs[0] + 0);
  input_ids_output_array.data_start = AI_PTR(net_ctx->_inputs[0] + 0);
  embedding_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  embedding_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(1, 2, { token_embedding_weight.data->data,input_ids_output.data->data});
  forward_gather(&embedding_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(1, 1, { embedding_output.data->data});
}
void forward_lite_eltwise_add(_stai_tim_network_context* net_ctx)
{
  embedding_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  embedding_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  embedding_1_array.data = AI_PTR(net_ctx->_weights[0] + 32);
  embedding_1_array.data_start = AI_PTR(net_ctx->_weights[0] + 32);
  add_output_array.data = AI_PTR(net_ctx->_activations[0] + 49152);
  add_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 49152);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(2, 2, { embedding_output0.data->data,embedding_1.data->data});
  forward_eltwise(&add_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(2, 1, { add_output.data->data});
}
void forward_lite_transpose_transpose(_stai_tim_network_context* net_ctx)
{
  add_output_array.data = AI_PTR(net_ctx->_activations[0] + 49152);
  add_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 49152);
  transpose_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  transpose_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(3, 1, { add_output0.data->data});
  forward_transpose(&transpose_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(3, 1, { transpose_output.data->data});
}
void forward_lite_dense_val_11(_stai_tim_network_context* net_ctx)
{
  transpose_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  transpose_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  val_11_weights_array.data = AI_PTR(net_ctx->_weights[0] + 185888);
  val_11_weights_array.data_start = AI_PTR(net_ctx->_weights[0] + 185888);
  val_11_bias_array.data = AI_PTR(net_ctx->_weights[0] + 382496);
  val_11_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 382496);
  val_11_output_array.data = AI_PTR(net_ctx->_activations[0] + 65536);
  val_11_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65536);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(5, 1, { transpose_output.data->data});
  forward_dense(&val_11_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(5, 1, { val_11_output.data->data});
}
void forward_lite_transpose_transpose_1(_stai_tim_network_context* net_ctx)
{
  val_11_output_array.data = AI_PTR(net_ctx->_activations[0] + 65536);
  val_11_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65536);
  transpose_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  transpose_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(8, 1, { val_11_output0.data->data});
  forward_transpose(&transpose_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(8, 1, { transpose_1_output.data->data});
}
void forward_lite_gather_select_2(_stai_tim_network_context* net_ctx)
{
  transpose_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  transpose_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  val_21_array.data = AI_PTR(net_ctx->_weights[0] + 20);
  val_21_array.data_start = AI_PTR(net_ctx->_weights[0] + 20);
  select_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 65536);
  select_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65536);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(12, 2, { transpose_1_output0.data->data,val_21.data->data});
  forward_gather(&select_2_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(12, 1, { select_2_output.data->data});
}
void forward_lite_transpose_transpose_4(_stai_tim_network_context* net_ctx)
{
  select_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 65536);
  select_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65536);
  transpose_4_output_array.data = AI_PTR(net_ctx->_activations[0] + 81920);
  transpose_4_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 81920);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(18, 1, { select_2_output0.data->data});
  forward_transpose(&transpose_4_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(18, 1, { transpose_4_output.data->data});
}
void forward_lite_gather_select_1(_stai_tim_network_context* net_ctx)
{
  transpose_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  transpose_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  val_20_array.data = AI_PTR(net_ctx->_weights[0] + 24);
  val_20_array.data_start = AI_PTR(net_ctx->_weights[0] + 24);
  select_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 65536);
  select_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65536);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(11, 2, { transpose_1_output0.data->data,val_20.data->data});
  forward_gather(&select_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(11, 1, { select_1_output.data->data});
}
void forward_lite_transpose_val_74(_stai_tim_network_context* net_ctx)
{
  select_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 65536);
  select_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65536);
  val_74_output_array.data = AI_PTR(net_ctx->_activations[0] + 98304);
  val_74_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 98304);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(21, 1, { select_1_output0.data->data});
  forward_transpose(&val_74_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(21, 1, { val_74_output.data->data});
}
void forward_lite_gather_select(_stai_tim_network_context* net_ctx)
{
  transpose_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  transpose_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  val_19_array.data = AI_PTR(net_ctx->_weights[0] + 28);
  val_19_array.data_start = AI_PTR(net_ctx->_weights[0] + 28);
  select_output_array.data = AI_PTR(net_ctx->_activations[0] + 98304);
  select_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 98304);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(10, 2, { transpose_1_output0.data->data,val_19.data->data});
  forward_gather(&select_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(10, 1, { select_output.data->data});
}
void forward_lite_transpose_transpose_2(_stai_tim_network_context* net_ctx)
{
  select_output_array.data = AI_PTR(net_ctx->_activations[0] + 98304);
  select_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 98304);
  transpose_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  transpose_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(14, 1, { select_output0.data->data});
  forward_transpose(&transpose_2_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(14, 1, { transpose_2_output.data->data});
}
void forward_lite_matmul_val_82(_stai_tim_network_context* net_ctx)
{
  val_78_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  val_78_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  val_81_output_array.data = AI_PTR(net_ctx->_activations[0] + 65536);
  val_81_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65536);
  val_82_bias_array.data = AI_PTR(net_ctx->_weights[0] + 12);
  val_82_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 12);
  val_82_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  val_82_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(25, 3, { val_78_output.data->data,val_81_output.data->data,val_82_bias.data->data});
  forward_matmul(&val_82_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(25, 1, { val_82_output.data->data});
}
void forward_lite_matmul_scaled_dot_product_attention(_stai_tim_network_context* net_ctx)
{
  val_83_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  val_83_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  transpose_4_output_array.data = AI_PTR(net_ctx->_activations[0] + 81920);
  transpose_4_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 81920);
  scaled_dot_product_attention_bias_array.data = AI_PTR(net_ctx->_weights[0] + 8);
  scaled_dot_product_attention_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 8);
  scaled_dot_product_attention_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  scaled_dot_product_attention_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(27, 3, { val_83_output.data->data,transpose_4_output.data->data,scaled_dot_product_attention_bias.data->data});
  forward_matmul(&scaled_dot_product_attention_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(27, 1, { scaled_dot_product_attention_output.data->data});
}
void forward_lite_transpose_permute(_stai_tim_network_context* net_ctx)
{
  scaled_dot_product_attention_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  scaled_dot_product_attention_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  permute_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  permute_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(28, 1, { scaled_dot_product_attention_output0.data->data});
  forward_transpose(&permute_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(28, 1, { permute_output.data->data});
}
void forward_lite_dense_linear_1(_stai_tim_network_context* net_ctx)
{
  permute_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  permute_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  linear_1_weights_array.data = AI_PTR(net_ctx->_weights[0] + 384288);
  linear_1_weights_array.data_start = AI_PTR(net_ctx->_weights[0] + 384288);
  linear_1_bias_array.data = AI_PTR(net_ctx->_weights[0] + 449824);
  linear_1_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 449824);
  linear_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  linear_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(30, 1, { permute_output0.data->data});
  forward_dense(&linear_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(30, 1, { linear_1_output.data->data});
}
void forward_lite_eltwise_add_1(_stai_tim_network_context* net_ctx)
{
  add_output_array.data = AI_PTR(net_ctx->_activations[0] + 49152);
  add_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 49152);
  linear_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  linear_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  add_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  add_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(33, 2, { add_output.data->data,linear_1_output.data->data});
  forward_eltwise(&add_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(33, 1, { add_1_output.data->data});
}
void forward_lite_reduce_layer_norm_Reduce(_stai_tim_network_context* net_ctx)
{
  add_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  add_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  layer_norm_Reduce_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_Reduce_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(34, 1, { add_1_output.data->data});
  forward_reduce(&layer_norm_Reduce_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(34, 1, { layer_norm_Reduce_output.data->data});
}
void forward_lite_eltwise_layer_norm_Sub(_stai_tim_network_context* net_ctx)
{
  add_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  add_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  layer_norm_Reduce_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_Reduce_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  layer_norm_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(34, 2, { add_1_output.data->data,layer_norm_Reduce_Mul_output.data->data});
  forward_eltwise(&layer_norm_Sub_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(34, 1, { layer_norm_Sub_output.data->data});
}
void forward_lite_eltwise_layer_norm_Mul(_stai_tim_network_context* net_ctx)
{
  layer_norm_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  layer_norm_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  layer_norm_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(34, 2, { layer_norm_Sub_output.data->data,layer_norm_Sub_output.data->data});
  forward_eltwise(&layer_norm_Mul_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(34, 1, { layer_norm_Mul_output.data->data});
}
void forward_lite_reduce_layer_norm_Reduce_1(_stai_tim_network_context* net_ctx)
{
  layer_norm_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_Reduce_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  layer_norm_Reduce_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(34, 1, { layer_norm_Mul_output.data->data});
  forward_reduce(&layer_norm_Reduce_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(34, 1, { layer_norm_Reduce_1_output.data->data});
}
void forward_lite_eltwise_layer_norm_Mul_1(_stai_tim_network_context* net_ctx)
{
  layer_norm_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  layer_norm_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  layer_norm_Reciprocal_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_Reciprocal_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_Mul_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_Mul_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(34, 2, { layer_norm_Sub_output.data->data,layer_norm_Reciprocal_output.data->data});
  forward_eltwise(&layer_norm_Mul_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(34, 1, { layer_norm_Mul_1_output.data->data});
}
void forward_lite_dense_val_96(_stai_tim_network_context* net_ctx)
{
  layer_norm_Mul_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 16512);
  layer_norm_Mul_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16512);
  val_96_weights_array.data = AI_PTR(net_ctx->_weights[0] + 451372);
  val_96_weights_array.data_start = AI_PTR(net_ctx->_weights[0] + 451372);
  val_96_bias_array.data = AI_PTR(net_ctx->_weights[0] + 582444);
  val_96_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 582444);
  val_96_output_array.data = AI_PTR(net_ctx->_activations[0] + 32896);
  val_96_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32896);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(36, 1, { layer_norm_Mul_2_output.data->data});
  forward_dense(&val_96_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(36, 1, { val_96_output.data->data});
}
void forward_lite_eltwise_val_98(_stai_tim_network_context* net_ctx)
{
  val_96_output_array.data = AI_PTR(net_ctx->_activations[0] + 32896);
  val_96_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32896);
  val_97_3D_array.data = AI_PTR(net_ctx->_weights[0] + 16);
  val_97_3D_array.data_start = AI_PTR(net_ctx->_weights[0] + 16);
  val_98_output_array.data = AI_PTR(net_ctx->_activations[0] + 65664);
  val_98_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65664);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(37, 2, { val_96_output.data->data,val_97_3D.data->data});
  forward_eltwise(&val_98_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(37, 1, { val_98_output.data->data});
}
void forward_lite_eltwise_gelu(_stai_tim_network_context* net_ctx)
{
  val_96_output_array.data = AI_PTR(net_ctx->_activations[0] + 32896);
  val_96_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32896);
  val_101_output_array.data = AI_PTR(net_ctx->_activations[0] + 65664);
  val_101_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65664);
  gelu_output_array.data = AI_PTR(net_ctx->_activations[0] + 32896);
  gelu_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32896);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(41, 2, { val_96_output.data->data,val_101_output.data->data});
  forward_eltwise(&gelu_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(41, 1, { gelu_output.data->data});
}
void forward_lite_dense_val_105(_stai_tim_network_context* net_ctx)
{
  gelu_output_array.data = AI_PTR(net_ctx->_activations[0] + 32896);
  gelu_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32896);
  val_105_weights_array.data = AI_PTR(net_ctx->_weights[0] + 585516);
  val_105_weights_array.data_start = AI_PTR(net_ctx->_weights[0] + 585516);
  val_105_bias_array.data = AI_PTR(net_ctx->_weights[0] + 716588);
  val_105_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 716588);
  val_105_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  val_105_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(43, 1, { gelu_output.data->data});
  forward_dense(&val_105_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(43, 1, { val_105_output.data->data});
}
void forward_lite_eltwise_add_2(_stai_tim_network_context* net_ctx)
{
  layer_norm_Mul_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 16512);
  layer_norm_Mul_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16512);
  val_105_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  val_105_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  add_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 32896);
  add_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32896);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(44, 2, { layer_norm_Mul_2_output.data->data,val_105_output.data->data});
  forward_eltwise(&add_2_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(44, 1, { add_2_output.data->data});
}
void forward_lite_reduce_layer_norm_1_Reduce(_stai_tim_network_context* net_ctx)
{
  add_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 32896);
  add_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32896);
  layer_norm_1_Reduce_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_1_Reduce_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(45, 1, { add_2_output.data->data});
  forward_reduce(&layer_norm_1_Reduce_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(45, 1, { layer_norm_1_Reduce_output.data->data});
}
void forward_lite_eltwise_layer_norm_1_Sub(_stai_tim_network_context* net_ctx)
{
  add_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 32896);
  add_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32896);
  layer_norm_1_Reduce_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_1_Reduce_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_1_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_1_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 256);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(45, 2, { add_2_output.data->data,layer_norm_1_Reduce_Mul_output.data->data});
  forward_eltwise(&layer_norm_1_Sub_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(45, 1, { layer_norm_1_Sub_output.data->data});
}
void forward_lite_eltwise_layer_norm_1_Mul(_stai_tim_network_context* net_ctx)
{
  layer_norm_1_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_1_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_1_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 16640);
  layer_norm_1_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16640);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(45, 2, { layer_norm_1_Sub_output.data->data,layer_norm_1_Sub_output.data->data});
  forward_eltwise(&layer_norm_1_Mul_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(45, 1, { layer_norm_1_Mul_output.data->data});
}
void forward_lite_reduce_layer_norm_1_Reduce_1(_stai_tim_network_context* net_ctx)
{
  layer_norm_1_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 16640);
  layer_norm_1_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16640);
  layer_norm_1_Reduce_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_1_Reduce_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(45, 1, { layer_norm_1_Mul_output.data->data});
  forward_reduce(&layer_norm_1_Reduce_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(45, 1, { layer_norm_1_Reduce_1_output.data->data});
}
void forward_lite_eltwise_layer_norm_1_Mul_1(_stai_tim_network_context* net_ctx)
{
  layer_norm_1_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_1_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_1_Reciprocal_output_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_1_Reciprocal_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_1_Mul_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 16640);
  layer_norm_1_Mul_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16640);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(45, 2, { layer_norm_1_Sub_output.data->data,layer_norm_1_Reciprocal_output.data->data});
  forward_eltwise(&layer_norm_1_Mul_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(45, 1, { layer_norm_1_Mul_1_output.data->data});
}
void forward_lite_transpose_transpose_6(_stai_tim_network_context* net_ctx)
{
  layer_norm_1_Mul_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_1_Mul_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  transpose_6_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  transpose_6_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(46, 1, { layer_norm_1_Mul_2_output0.data->data});
  forward_transpose(&transpose_6_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(46, 1, { transpose_6_output.data->data});
}
void forward_lite_dense_val_109(_stai_tim_network_context* net_ctx)
{
  transpose_6_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  transpose_6_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  val_109_weights_array.data = AI_PTR(net_ctx->_weights[0] + 718124);
  val_109_weights_array.data_start = AI_PTR(net_ctx->_weights[0] + 718124);
  val_109_bias_array.data = AI_PTR(net_ctx->_weights[0] + 914732);
  val_109_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 914732);
  val_109_output_array.data = AI_PTR(net_ctx->_activations[0] + 65536);
  val_109_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65536);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(48, 1, { transpose_6_output.data->data});
  forward_dense(&val_109_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(48, 1, { val_109_output.data->data});
}
void forward_lite_transpose_transpose_7(_stai_tim_network_context* net_ctx)
{
  val_109_output_array.data = AI_PTR(net_ctx->_activations[0] + 65536);
  val_109_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65536);
  transpose_7_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  transpose_7_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(51, 1, { val_109_output0.data->data});
  forward_transpose(&transpose_7_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(51, 1, { transpose_7_output.data->data});
}
void forward_lite_gather_select_5(_stai_tim_network_context* net_ctx)
{
  transpose_7_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  transpose_7_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  val_21_array.data = AI_PTR(net_ctx->_weights[0] + 20);
  val_21_array.data_start = AI_PTR(net_ctx->_weights[0] + 20);
  select_5_output_array.data = AI_PTR(net_ctx->_activations[0] + 65536);
  select_5_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65536);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(55, 2, { transpose_7_output0.data->data,val_21.data->data});
  forward_gather(&select_5_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(55, 1, { select_5_output.data->data});
}
void forward_lite_transpose_transpose_10(_stai_tim_network_context* net_ctx)
{
  select_5_output_array.data = AI_PTR(net_ctx->_activations[0] + 65536);
  select_5_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65536);
  transpose_10_output_array.data = AI_PTR(net_ctx->_activations[0] + 81920);
  transpose_10_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 81920);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(61, 1, { select_5_output0.data->data});
  forward_transpose(&transpose_10_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(61, 1, { transpose_10_output.data->data});
}
void forward_lite_gather_select_4(_stai_tim_network_context* net_ctx)
{
  transpose_7_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  transpose_7_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  val_20_array.data = AI_PTR(net_ctx->_weights[0] + 24);
  val_20_array.data_start = AI_PTR(net_ctx->_weights[0] + 24);
  select_4_output_array.data = AI_PTR(net_ctx->_activations[0] + 65536);
  select_4_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65536);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(54, 2, { transpose_7_output0.data->data,val_20.data->data});
  forward_gather(&select_4_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(54, 1, { select_4_output.data->data});
}
void forward_lite_transpose_val_167(_stai_tim_network_context* net_ctx)
{
  select_4_output_array.data = AI_PTR(net_ctx->_activations[0] + 65536);
  select_4_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65536);
  val_167_output_array.data = AI_PTR(net_ctx->_activations[0] + 98304);
  val_167_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 98304);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(64, 1, { select_4_output0.data->data});
  forward_transpose(&val_167_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(64, 1, { val_167_output.data->data});
}
void forward_lite_gather_select_3(_stai_tim_network_context* net_ctx)
{
  transpose_7_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  transpose_7_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  val_19_array.data = AI_PTR(net_ctx->_weights[0] + 28);
  val_19_array.data_start = AI_PTR(net_ctx->_weights[0] + 28);
  select_3_output_array.data = AI_PTR(net_ctx->_activations[0] + 98304);
  select_3_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 98304);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(53, 2, { transpose_7_output0.data->data,val_19.data->data});
  forward_gather(&select_3_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(53, 1, { select_3_output.data->data});
}
void forward_lite_transpose_transpose_8(_stai_tim_network_context* net_ctx)
{
  select_3_output_array.data = AI_PTR(net_ctx->_activations[0] + 98304);
  select_3_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 98304);
  transpose_8_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  transpose_8_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(57, 1, { select_3_output0.data->data});
  forward_transpose(&transpose_8_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(57, 1, { transpose_8_output.data->data});
}
void forward_lite_matmul_val_175(_stai_tim_network_context* net_ctx)
{
  val_171_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  val_171_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  val_174_output_array.data = AI_PTR(net_ctx->_activations[0] + 65536);
  val_174_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 65536);
  val_175_bias_array.data = AI_PTR(net_ctx->_weights[0] + 4);
  val_175_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 4);
  val_175_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  val_175_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(68, 3, { val_171_output.data->data,val_174_output.data->data,val_175_bias.data->data});
  forward_matmul(&val_175_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(68, 1, { val_175_output.data->data});
}
void forward_lite_matmul_scaled_dot_product_attention_1(_stai_tim_network_context* net_ctx)
{
  val_176_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  val_176_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  transpose_10_output_array.data = AI_PTR(net_ctx->_activations[0] + 81920);
  transpose_10_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 81920);
  scaled_dot_product_attention_1_bias_array.data = AI_PTR(net_ctx->_weights[0] + 0);
  scaled_dot_product_attention_1_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 0);
  scaled_dot_product_attention_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  scaled_dot_product_attention_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(70, 3, { val_176_output.data->data,transpose_10_output.data->data,scaled_dot_product_attention_1_bias.data->data});
  forward_matmul(&scaled_dot_product_attention_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(70, 1, { scaled_dot_product_attention_1_output.data->data});
}
void forward_lite_transpose_permute_1(_stai_tim_network_context* net_ctx)
{
  scaled_dot_product_attention_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  scaled_dot_product_attention_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  permute_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  permute_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(71, 1, { scaled_dot_product_attention_1_output0.data->data});
  forward_transpose(&permute_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(71, 1, { permute_1_output.data->data});
}
void forward_lite_dense_linear_5(_stai_tim_network_context* net_ctx)
{
  permute_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  permute_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  linear_5_weights_array.data = AI_PTR(net_ctx->_weights[0] + 916268);
  linear_5_weights_array.data_start = AI_PTR(net_ctx->_weights[0] + 916268);
  linear_5_bias_array.data = AI_PTR(net_ctx->_weights[0] + 981804);
  linear_5_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 981804);
  linear_5_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  linear_5_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(73, 1, { permute_1_output0.data->data});
  forward_dense(&linear_5_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(73, 1, { linear_5_output.data->data});
}
void forward_lite_eltwise_add_3(_stai_tim_network_context* net_ctx)
{
  layer_norm_1_Mul_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_1_Mul_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  linear_5_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  linear_5_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  add_3_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  add_3_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(76, 2, { layer_norm_1_Mul_2_output.data->data,linear_5_output.data->data});
  forward_eltwise(&add_3_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(76, 1, { add_3_output.data->data});
}
void forward_lite_reduce_layer_norm_2_Reduce(_stai_tim_network_context* net_ctx)
{
  add_3_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  add_3_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  layer_norm_2_Reduce_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_2_Reduce_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(77, 1, { add_3_output.data->data});
  forward_reduce(&layer_norm_2_Reduce_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(77, 1, { layer_norm_2_Reduce_output.data->data});
}
void forward_lite_eltwise_layer_norm_2_Sub(_stai_tim_network_context* net_ctx)
{
  add_3_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  add_3_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  layer_norm_2_Reduce_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_2_Reduce_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_2_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_2_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 256);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(77, 2, { add_3_output.data->data,layer_norm_2_Reduce_Mul_output.data->data});
  forward_eltwise(&layer_norm_2_Sub_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(77, 1, { layer_norm_2_Sub_output.data->data});
}
void forward_lite_eltwise_layer_norm_2_Mul(_stai_tim_network_context* net_ctx)
{
  layer_norm_2_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_2_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_2_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 16640);
  layer_norm_2_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16640);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(77, 2, { layer_norm_2_Sub_output.data->data,layer_norm_2_Sub_output.data->data});
  forward_eltwise(&layer_norm_2_Mul_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(77, 1, { layer_norm_2_Mul_output.data->data});
}
void forward_lite_reduce_layer_norm_2_Reduce_1(_stai_tim_network_context* net_ctx)
{
  layer_norm_2_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 16640);
  layer_norm_2_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16640);
  layer_norm_2_Reduce_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_2_Reduce_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(77, 1, { layer_norm_2_Mul_output.data->data});
  forward_reduce(&layer_norm_2_Reduce_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(77, 1, { layer_norm_2_Reduce_1_output.data->data});
}
void forward_lite_eltwise_layer_norm_2_Mul_1(_stai_tim_network_context* net_ctx)
{
  layer_norm_2_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_2_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_2_Reciprocal_output_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_2_Reciprocal_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_2_Mul_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 16640);
  layer_norm_2_Mul_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16640);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(77, 2, { layer_norm_2_Sub_output.data->data,layer_norm_2_Reciprocal_output.data->data});
  forward_eltwise(&layer_norm_2_Mul_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(77, 1, { layer_norm_2_Mul_1_output.data->data});
}
void forward_lite_dense_val_189(_stai_tim_network_context* net_ctx)
{
  layer_norm_2_Mul_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_2_Mul_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  val_189_weights_array.data = AI_PTR(net_ctx->_weights[0] + 983340);
  val_189_weights_array.data_start = AI_PTR(net_ctx->_weights[0] + 983340);
  val_189_bias_array.data = AI_PTR(net_ctx->_weights[0] + 1114412);
  val_189_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 1114412);
  val_189_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  val_189_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(79, 1, { layer_norm_2_Mul_2_output.data->data});
  forward_dense(&val_189_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(79, 1, { val_189_output.data->data});
}
void forward_lite_eltwise_val_191(_stai_tim_network_context* net_ctx)
{
  val_189_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  val_189_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  val_97_3D_array.data = AI_PTR(net_ctx->_weights[0] + 16);
  val_97_3D_array.data_start = AI_PTR(net_ctx->_weights[0] + 16);
  val_191_output_array.data = AI_PTR(net_ctx->_activations[0] + 49152);
  val_191_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 49152);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(80, 2, { val_189_output.data->data,val_97_3D.data->data});
  forward_eltwise(&val_191_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(80, 1, { val_191_output.data->data});
}
void forward_lite_eltwise_gelu_1(_stai_tim_network_context* net_ctx)
{
  val_189_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  val_189_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  val_194_output_array.data = AI_PTR(net_ctx->_activations[0] + 49152);
  val_194_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 49152);
  gelu_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 81920);
  gelu_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 81920);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(84, 2, { val_189_output.data->data,val_194_output.data->data});
  forward_eltwise(&gelu_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(84, 1, { gelu_1_output.data->data});
}
void forward_lite_dense_val_198(_stai_tim_network_context* net_ctx)
{
  gelu_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 81920);
  gelu_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 81920);
  val_198_weights_array.data = AI_PTR(net_ctx->_weights[0] + 1115436);
  val_198_weights_array.data_start = AI_PTR(net_ctx->_weights[0] + 1115436);
  val_198_bias_array.data = AI_PTR(net_ctx->_weights[0] + 1246508);
  val_198_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 1246508);
  val_198_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  val_198_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(86, 1, { gelu_1_output.data->data});
  forward_dense(&val_198_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(86, 1, { val_198_output.data->data});
}
void forward_lite_eltwise_add_4(_stai_tim_network_context* net_ctx)
{
  layer_norm_2_Mul_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_2_Mul_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  val_198_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  val_198_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  add_4_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  add_4_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(87, 2, { layer_norm_2_Mul_2_output.data->data,val_198_output.data->data});
  forward_eltwise(&add_4_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(87, 1, { add_4_output.data->data});
}
void forward_lite_reduce_layer_norm_3_Reduce(_stai_tim_network_context* net_ctx)
{
  add_4_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  add_4_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  layer_norm_3_Reduce_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_3_Reduce_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(88, 1, { add_4_output.data->data});
  forward_reduce(&layer_norm_3_Reduce_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(88, 1, { layer_norm_3_Reduce_output.data->data});
}
void forward_lite_eltwise_layer_norm_3_Sub(_stai_tim_network_context* net_ctx)
{
  add_4_output_array.data = AI_PTR(net_ctx->_activations[0] + 32768);
  add_4_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 32768);
  layer_norm_3_Reduce_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_3_Reduce_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_3_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_3_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 256);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(88, 2, { add_4_output.data->data,layer_norm_3_Reduce_Mul_output.data->data});
  forward_eltwise(&layer_norm_3_Sub_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(88, 1, { layer_norm_3_Sub_output.data->data});
}
void forward_lite_eltwise_layer_norm_3_Mul(_stai_tim_network_context* net_ctx)
{
  layer_norm_3_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_3_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_3_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 16640);
  layer_norm_3_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16640);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(88, 2, { layer_norm_3_Sub_output.data->data,layer_norm_3_Sub_output.data->data});
  forward_eltwise(&layer_norm_3_Mul_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(88, 1, { layer_norm_3_Mul_output.data->data});
}
void forward_lite_reduce_layer_norm_3_Reduce_1(_stai_tim_network_context* net_ctx)
{
  layer_norm_3_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 16640);
  layer_norm_3_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16640);
  layer_norm_3_Reduce_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_3_Reduce_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(88, 1, { layer_norm_3_Mul_output.data->data});
  forward_reduce(&layer_norm_3_Reduce_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(88, 1, { layer_norm_3_Reduce_1_output.data->data});
}
void forward_lite_eltwise_layer_norm_3_Mul_1(_stai_tim_network_context* net_ctx)
{
  layer_norm_3_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_3_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 256);
  layer_norm_3_Reciprocal_output_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_3_Reciprocal_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  layer_norm_3_Mul_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 16640);
  layer_norm_3_Mul_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16640);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(88, 2, { layer_norm_3_Sub_output.data->data,layer_norm_3_Reciprocal_output.data->data});
  forward_eltwise(&layer_norm_3_Mul_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(88, 1, { layer_norm_3_Mul_1_output.data->data});
}
void forward_lite_gather_select_6(_stai_tim_network_context* net_ctx)
{
  layer_norm_3_Mul_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_3_Mul_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  val_19_array.data = AI_PTR(net_ctx->_weights[0] + 28);
  val_19_array.data_start = AI_PTR(net_ctx->_weights[0] + 28);
  select_6_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  select_6_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(89, 2, { layer_norm_3_Mul_2_output.data->data,val_19_1.data->data});
  forward_gather(&select_6_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(89, 1, { select_6_output.data->data});
}
void forward_lite_transpose_layer_norm_4_Transpose(_stai_tim_network_context* net_ctx)
{
  select_6_output_array.data = AI_PTR(net_ctx->_activations[0] + 16384);
  select_6_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 16384);
  layer_norm_4_Transpose_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_4_Transpose_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(90, 1, { select_6_output.data->data});
  forward_transpose(&layer_norm_4_Transpose_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(90, 1, { layer_norm_4_Transpose_output.data->data});
}
void forward_lite_reduce_layer_norm_4_Reduce(_stai_tim_network_context* net_ctx)
{
  layer_norm_4_Transpose_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_4_Transpose_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_4_Reduce_output_array.data = AI_PTR(net_ctx->_activations[0] + 512);
  layer_norm_4_Reduce_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 512);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(90, 1, { layer_norm_4_Transpose_output.data->data});
  forward_reduce(&layer_norm_4_Reduce_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(90, 1, { layer_norm_4_Reduce_output.data->data});
}
void forward_lite_eltwise_layer_norm_4_Sub(_stai_tim_network_context* net_ctx)
{
  layer_norm_4_Transpose_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_4_Transpose_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_4_Reduce_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 516);
  layer_norm_4_Reduce_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 516);
  layer_norm_4_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 520);
  layer_norm_4_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 520);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(90, 2, { layer_norm_4_Transpose_output.data->data,layer_norm_4_Reduce_Mul_output.data->data});
  forward_eltwise(&layer_norm_4_Sub_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(90, 1, { layer_norm_4_Sub_output.data->data});
}
void forward_lite_eltwise_layer_norm_4_Mul(_stai_tim_network_context* net_ctx)
{
  layer_norm_4_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 520);
  layer_norm_4_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 520);
  layer_norm_4_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_4_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(90, 2, { layer_norm_4_Sub_output.data->data,layer_norm_4_Sub_output.data->data});
  forward_eltwise(&layer_norm_4_Mul_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(90, 1, { layer_norm_4_Mul_output.data->data});
}
void forward_lite_reduce_layer_norm_4_Reduce_1(_stai_tim_network_context* net_ctx)
{
  layer_norm_4_Mul_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_4_Mul_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_4_Reduce_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 512);
  layer_norm_4_Reduce_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 512);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(90, 1, { layer_norm_4_Mul_output.data->data});
  forward_reduce(&layer_norm_4_Reduce_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(90, 1, { layer_norm_4_Reduce_1_output.data->data});
}
void forward_lite_eltwise_layer_norm_4_Mul_1(_stai_tim_network_context* net_ctx)
{
  layer_norm_4_Sub_output_array.data = AI_PTR(net_ctx->_activations[0] + 520);
  layer_norm_4_Sub_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 520);
  layer_norm_4_Reciprocal_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_4_Reciprocal_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_4_Mul_1_output_array.data = AI_PTR(net_ctx->_activations[0] + 4);
  layer_norm_4_Mul_1_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 4);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(90, 2, { layer_norm_4_Sub_output.data->data,layer_norm_4_Reciprocal_output.data->data});
  forward_eltwise(&layer_norm_4_Mul_1_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(90, 1, { layer_norm_4_Mul_1_output.data->data});
}
void forward_lite_transpose_layer_norm_4_Transpose_out_0(_stai_tim_network_context* net_ctx)
{
  layer_norm_4_Mul_2_output_array.data = AI_PTR(net_ctx->_activations[0] + 516);
  layer_norm_4_Mul_2_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 516);
  layer_norm_4_Transpose_out_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  layer_norm_4_Transpose_out_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(90, 1, { layer_norm_4_Mul_2_output.data->data});
  forward_transpose(&layer_norm_4_Transpose_out_0_layer);
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(90, 1, { layer_norm_4_Transpose_out_0_output.data->data});
}

/*****************************************************************************/
















static const ai_i32 val_83_t_in_0_shape_ch_h_w_prod_const_s32 = 4096;











static const ai_i32 layer_norm_Sqrt_t_in_0_shape_ch_h_prod_const_s32 = 32;

static const ai_i32 layer_norm_Reciprocal_t_in_0_shape_ch_h_prod_const_s32 = 32;





static const ai_i32 val_99_t_in_0_shape_ch_h_prod_const_s32 = 8192;











static const ai_i32 layer_norm_1_Sqrt_t_in_0_shape_ch_h_prod_const_s32 = 32;

static const ai_i32 layer_norm_1_Reciprocal_t_in_0_shape_ch_h_prod_const_s32 = 32;















static const ai_i32 val_176_t_in_0_shape_ch_h_w_prod_const_s32 = 4096;











static const ai_i32 layer_norm_2_Sqrt_t_in_0_shape_ch_h_prod_const_s32 = 32;

static const ai_i32 layer_norm_2_Reciprocal_t_in_0_shape_ch_h_prod_const_s32 = 32;





static const ai_i32 val_192_t_in_0_shape_ch_h_prod_const_s32 = 8192;











static const ai_i32 layer_norm_3_Sqrt_t_in_0_shape_ch_h_prod_const_s32 = 32;

static const ai_i32 layer_norm_3_Reciprocal_t_in_0_shape_ch_h_prod_const_s32 = 32;











static const ai_i32 layer_norm_4_Sqrt_t_in_0_shape_ch_prod_const_s32 = 1;

static const ai_i32 layer_norm_4_Reciprocal_t_in_0_shape_ch_prod_const_s32 = 1;




STAI_API_ENTRY
stai_return_code stai_tim_network_run(
  stai_network* network,
  const stai_run_mode mode)
{
   STAI_UNUSED(mode)
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_ACTIVATIONS) != STAI_FLAG_ACTIVATIONS,
        STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_INPUTS) != STAI_FLAG_INPUTS,
                  STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_OUTPUTS) != STAI_FLAG_OUTPUTS,
                  STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_WEIGHTS) != STAI_FLAG_WEIGHTS,
                  STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)


  /* LITE_KERNEL_SECTION BEGIN embedding */
  {
    
  forward_lite_gather_embedding(net_ctx);
  }
  /* LITE_KERNEL_SECTION END embedding */
  /* LITE_KERNEL_SECTION BEGIN add */
  {
    
  forward_lite_eltwise_add(net_ctx);
  }
  /* LITE_KERNEL_SECTION END add */
  /* LITE_KERNEL_SECTION BEGIN transpose */
  {
    
  forward_lite_transpose_transpose(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose */
  /* LITE_KERNEL_SECTION BEGIN val_11 */
  {
    
  forward_lite_dense_val_11(net_ctx);
  }
  /* LITE_KERNEL_SECTION END val_11 */
  /* LITE_KERNEL_SECTION BEGIN transpose_1 */
  {
    
  forward_lite_transpose_transpose_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_1 */
  /* LITE_KERNEL_SECTION BEGIN select_2 */
  {
    
  forward_lite_gather_select_2(net_ctx);
  }
  /* LITE_KERNEL_SECTION END select_2 */
  /* LITE_KERNEL_SECTION BEGIN transpose_4 */
  {
    
  forward_lite_transpose_transpose_4(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_4 */
  /* LITE_KERNEL_SECTION BEGIN select_1 */
  {
    
  forward_lite_gather_select_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END select_1 */
  /* LITE_KERNEL_SECTION BEGIN val_74 */
  {
    
  forward_lite_transpose_val_74(net_ctx);
  }
  /* LITE_KERNEL_SECTION END val_74 */
  /* LITE_KERNEL_SECTION BEGIN val_81 */
  {
      ai_float* val_81_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 65536);
    const ai_float* val_81_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 98304);
    const ai_float* val_81_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 384032);
    const ai_float* val_81_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 384160);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(24, 1, {(stai_ptr) val_81_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(val_81_t_out_0_ptr_f32, val_81_t_in_0_ptr_const_f32, val_81_t_weight_0_ptr_const_f32, val_81_t_weight_1_ptr_const_f32, (ai_u32)(4096), (ai_size)(32));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(24, 1, {(stai_ptr) val_81_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END val_81 */
  /* LITE_KERNEL_SECTION BEGIN select */
  {
    
  forward_lite_gather_select(net_ctx);
  }
  /* LITE_KERNEL_SECTION END select */
  /* LITE_KERNEL_SECTION BEGIN transpose_2 */
  {
    
  forward_lite_transpose_transpose_2(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_2 */
  /* LITE_KERNEL_SECTION BEGIN val_78 */
  {
      ai_float* val_78_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 16384);
    const ai_float* val_78_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
    const ai_float* val_78_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 384032);
    const ai_float* val_78_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 384160);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(23, 1, {(stai_ptr) val_78_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(val_78_t_out_0_ptr_f32, val_78_t_in_0_ptr_const_f32, val_78_t_weight_0_ptr_const_f32, val_78_t_weight_1_ptr_const_f32, (ai_u32)(4096), (ai_size)(32));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(23, 1, {(stai_ptr) val_78_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END val_78 */
  /* LITE_KERNEL_SECTION BEGIN val_82 */
  {
    
  forward_lite_matmul_val_82(net_ctx);
  }
  /* LITE_KERNEL_SECTION END val_82 */
  /* LITE_KERNEL_SECTION BEGIN val_83 */
  {
      ai_handle val_83_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 16384);
    const ai_handle val_83_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 0);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(26, 1, {(stai_ptr) val_83_t_in_0_ptr_const_handle});
    
  forward_lite_nl_softmax_if32of32(val_83_t_out_0_ptr_handle, val_83_t_in_0_ptr_const_handle, val_83_t_in_0_shape_ch_h_w_prod_const_s32, 1, 32);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(26, 1, {(stai_ptr) val_83_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END val_83 */
  /* LITE_KERNEL_SECTION BEGIN scaled_dot_product_attention */
  {
    
  forward_lite_matmul_scaled_dot_product_attention(net_ctx);
  }
  /* LITE_KERNEL_SECTION END scaled_dot_product_attention */
  /* LITE_KERNEL_SECTION BEGIN permute */
  {
    
  forward_lite_transpose_permute(net_ctx);
  }
  /* LITE_KERNEL_SECTION END permute */
  /* LITE_KERNEL_SECTION BEGIN linear_1 */
  {
    
  forward_lite_dense_linear_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END linear_1 */
  /* LITE_KERNEL_SECTION BEGIN add_1 */
  {
    
  forward_lite_eltwise_add_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END add_1 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_Reduce */
  {
    
  forward_lite_reduce_layer_norm_Reduce(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_Reduce */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_Reduce_Mul */
  {
      ai_float* layer_norm_Reduce_Mul_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 128);
    const ai_float* layer_norm_Reduce_Mul_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
    const ai_float* layer_norm_Reduce_Mul_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450336);
    const ai_float* layer_norm_Reduce_Mul_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450340);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(34, 1, {(stai_ptr) layer_norm_Reduce_Mul_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_Reduce_Mul_t_out_0_ptr_f32, layer_norm_Reduce_Mul_t_in_0_ptr_const_f32, layer_norm_Reduce_Mul_t_weight_0_ptr_const_f32, layer_norm_Reduce_Mul_t_weight_1_ptr_const_f32, (ai_u32)(32), (ai_size)(1));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(34, 1, {(stai_ptr) layer_norm_Reduce_Mul_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_Reduce_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_Sub */
  {
    
  forward_lite_eltwise_layer_norm_Sub(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_Sub */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_Mul */
  {
    
  forward_lite_eltwise_layer_norm_Mul(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_Reduce_1 */
  {
    
  forward_lite_reduce_layer_norm_Reduce_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_Reduce_1 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_Reduce_1_Mul */
  {
      ai_float* layer_norm_Reduce_1_Mul_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
    const ai_float* layer_norm_Reduce_1_Mul_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 16384);
    const ai_float* layer_norm_Reduce_1_Mul_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450336);
    const ai_float* layer_norm_Reduce_1_Mul_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450344);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(34, 1, {(stai_ptr) layer_norm_Reduce_1_Mul_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_Reduce_1_Mul_t_out_0_ptr_f32, layer_norm_Reduce_1_Mul_t_in_0_ptr_const_f32, layer_norm_Reduce_1_Mul_t_weight_0_ptr_const_f32, layer_norm_Reduce_1_Mul_t_weight_1_ptr_const_f32, (ai_u32)(32), (ai_size)(1));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(34, 1, {(stai_ptr) layer_norm_Reduce_1_Mul_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_Reduce_1_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_Sqrt */
  {
      ai_handle layer_norm_Sqrt_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 128);
    const ai_handle layer_norm_Sqrt_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 0);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(34, 1, {(stai_ptr) layer_norm_Sqrt_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sqrt_if32of32(layer_norm_Sqrt_t_out_0_ptr_handle, layer_norm_Sqrt_t_in_0_ptr_const_handle, layer_norm_Sqrt_t_in_0_shape_ch_h_prod_const_s32, NULL);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(34, 1, {(stai_ptr) layer_norm_Sqrt_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END layer_norm_Sqrt */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_Reciprocal */
  {
      ai_handle layer_norm_Reciprocal_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 0);
    const ai_handle layer_norm_Reciprocal_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 128);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(34, 1, {(stai_ptr) layer_norm_Reciprocal_t_in_0_ptr_const_handle});
    
  forward_lite_nl_reciprocal_if32of32(layer_norm_Reciprocal_t_out_0_ptr_handle, layer_norm_Reciprocal_t_in_0_ptr_const_handle, layer_norm_Reciprocal_t_in_0_shape_ch_h_prod_const_s32, NULL);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(34, 1, {(stai_ptr) layer_norm_Reciprocal_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END layer_norm_Reciprocal */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_Mul_1 */
  {
    
  forward_lite_eltwise_layer_norm_Mul_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_Mul_1 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_Mul_2 */
  {
      ai_float* layer_norm_Mul_2_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 16512);
    const ai_float* layer_norm_Mul_2_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 128);
    const ai_float* layer_norm_Mul_2_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450348);
    const ai_float* layer_norm_Mul_2_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450860);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(34, 1, {(stai_ptr) layer_norm_Mul_2_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_Mul_2_t_out_0_ptr_f32, layer_norm_Mul_2_t_in_0_ptr_const_f32, layer_norm_Mul_2_t_weight_0_ptr_const_f32, layer_norm_Mul_2_t_weight_1_ptr_const_f32, (ai_u32)(4096), (ai_size)(128));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(34, 1, {(stai_ptr) layer_norm_Mul_2_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_Mul_2 */
  /* LITE_KERNEL_SECTION BEGIN val_96 */
  {
    
  forward_lite_dense_val_96(net_ctx);
  }
  /* LITE_KERNEL_SECTION END val_96 */
  /* LITE_KERNEL_SECTION BEGIN val_98 */
  {
    
  forward_lite_eltwise_val_98(net_ctx);
  }
  /* LITE_KERNEL_SECTION END val_98 */
  /* LITE_KERNEL_SECTION BEGIN val_99 */
  {
      ai_handle val_99_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 65664);
    const ai_handle val_99_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 65664);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(38, 1, {(stai_ptr) val_99_t_in_0_ptr_const_handle});
    
  forward_lite_nl_erf_if32of32(val_99_t_out_0_ptr_handle, val_99_t_in_0_ptr_const_handle, val_99_t_in_0_shape_ch_h_prod_const_s32, NULL);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(38, 1, {(stai_ptr) val_99_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END val_99 */
  /* LITE_KERNEL_SECTION BEGIN val_101 */
  {
      ai_float* val_101_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 65664);
    const ai_float* val_101_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 65664);
    const ai_float* val_101_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 583468);
    const ai_float* val_101_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 584492);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(40, 1, {(stai_ptr) val_101_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(val_101_t_out_0_ptr_f32, val_101_t_in_0_ptr_const_f32, val_101_t_weight_0_ptr_const_f32, val_101_t_weight_1_ptr_const_f32, (ai_u32)(8192), (ai_size)(256));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(40, 1, {(stai_ptr) val_101_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END val_101 */
  /* LITE_KERNEL_SECTION BEGIN gelu */
  {
    
  forward_lite_eltwise_gelu(net_ctx);
  }
  /* LITE_KERNEL_SECTION END gelu */
  /* LITE_KERNEL_SECTION BEGIN val_105 */
  {
    
  forward_lite_dense_val_105(net_ctx);
  }
  /* LITE_KERNEL_SECTION END val_105 */
  /* LITE_KERNEL_SECTION BEGIN add_2 */
  {
    
  forward_lite_eltwise_add_2(net_ctx);
  }
  /* LITE_KERNEL_SECTION END add_2 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_1_Reduce */
  {
    
  forward_lite_reduce_layer_norm_1_Reduce(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_1_Reduce */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_1_Reduce_Mul */
  {
      ai_float* layer_norm_1_Reduce_Mul_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 128);
    const ai_float* layer_norm_1_Reduce_Mul_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
    const ai_float* layer_norm_1_Reduce_Mul_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450336);
    const ai_float* layer_norm_1_Reduce_Mul_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450340);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(45, 1, {(stai_ptr) layer_norm_1_Reduce_Mul_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_1_Reduce_Mul_t_out_0_ptr_f32, layer_norm_1_Reduce_Mul_t_in_0_ptr_const_f32, layer_norm_1_Reduce_Mul_t_weight_0_ptr_const_f32, layer_norm_1_Reduce_Mul_t_weight_1_ptr_const_f32, (ai_u32)(32), (ai_size)(1));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(45, 1, {(stai_ptr) layer_norm_1_Reduce_Mul_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_1_Reduce_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_1_Sub */
  {
    
  forward_lite_eltwise_layer_norm_1_Sub(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_1_Sub */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_1_Mul */
  {
    
  forward_lite_eltwise_layer_norm_1_Mul(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_1_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_1_Reduce_1 */
  {
    
  forward_lite_reduce_layer_norm_1_Reduce_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_1_Reduce_1 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_1_Reduce_1_Mul */
  {
      ai_float* layer_norm_1_Reduce_1_Mul_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 128);
    const ai_float* layer_norm_1_Reduce_1_Mul_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
    const ai_float* layer_norm_1_Reduce_1_Mul_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450336);
    const ai_float* layer_norm_1_Reduce_1_Mul_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450344);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(45, 1, {(stai_ptr) layer_norm_1_Reduce_1_Mul_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_1_Reduce_1_Mul_t_out_0_ptr_f32, layer_norm_1_Reduce_1_Mul_t_in_0_ptr_const_f32, layer_norm_1_Reduce_1_Mul_t_weight_0_ptr_const_f32, layer_norm_1_Reduce_1_Mul_t_weight_1_ptr_const_f32, (ai_u32)(32), (ai_size)(1));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(45, 1, {(stai_ptr) layer_norm_1_Reduce_1_Mul_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_1_Reduce_1_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_1_Sqrt */
  {
      ai_handle layer_norm_1_Sqrt_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 0);
    const ai_handle layer_norm_1_Sqrt_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 128);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(45, 1, {(stai_ptr) layer_norm_1_Sqrt_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sqrt_if32of32(layer_norm_1_Sqrt_t_out_0_ptr_handle, layer_norm_1_Sqrt_t_in_0_ptr_const_handle, layer_norm_1_Sqrt_t_in_0_shape_ch_h_prod_const_s32, NULL);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(45, 1, {(stai_ptr) layer_norm_1_Sqrt_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END layer_norm_1_Sqrt */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_1_Reciprocal */
  {
      ai_handle layer_norm_1_Reciprocal_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 128);
    const ai_handle layer_norm_1_Reciprocal_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 0);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(45, 1, {(stai_ptr) layer_norm_1_Reciprocal_t_in_0_ptr_const_handle});
    
  forward_lite_nl_reciprocal_if32of32(layer_norm_1_Reciprocal_t_out_0_ptr_handle, layer_norm_1_Reciprocal_t_in_0_ptr_const_handle, layer_norm_1_Reciprocal_t_in_0_shape_ch_h_prod_const_s32, NULL);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(45, 1, {(stai_ptr) layer_norm_1_Reciprocal_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END layer_norm_1_Reciprocal */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_1_Mul_1 */
  {
    
  forward_lite_eltwise_layer_norm_1_Mul_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_1_Mul_1 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_1_Mul_2 */
  {
      ai_float* layer_norm_1_Mul_2_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
    const ai_float* layer_norm_1_Mul_2_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 16640);
    const ai_float* layer_norm_1_Mul_2_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 717100);
    const ai_float* layer_norm_1_Mul_2_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 717612);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(45, 1, {(stai_ptr) layer_norm_1_Mul_2_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_1_Mul_2_t_out_0_ptr_f32, layer_norm_1_Mul_2_t_in_0_ptr_const_f32, layer_norm_1_Mul_2_t_weight_0_ptr_const_f32, layer_norm_1_Mul_2_t_weight_1_ptr_const_f32, (ai_u32)(4096), (ai_size)(128));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(45, 1, {(stai_ptr) layer_norm_1_Mul_2_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_1_Mul_2 */
  /* LITE_KERNEL_SECTION BEGIN transpose_6 */
  {
    
  forward_lite_transpose_transpose_6(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_6 */
  /* LITE_KERNEL_SECTION BEGIN val_109 */
  {
    
  forward_lite_dense_val_109(net_ctx);
  }
  /* LITE_KERNEL_SECTION END val_109 */
  /* LITE_KERNEL_SECTION BEGIN transpose_7 */
  {
    
  forward_lite_transpose_transpose_7(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_7 */
  /* LITE_KERNEL_SECTION BEGIN select_5 */
  {
    
  forward_lite_gather_select_5(net_ctx);
  }
  /* LITE_KERNEL_SECTION END select_5 */
  /* LITE_KERNEL_SECTION BEGIN transpose_10 */
  {
    
  forward_lite_transpose_transpose_10(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_10 */
  /* LITE_KERNEL_SECTION BEGIN select_4 */
  {
    
  forward_lite_gather_select_4(net_ctx);
  }
  /* LITE_KERNEL_SECTION END select_4 */
  /* LITE_KERNEL_SECTION BEGIN val_167 */
  {
    
  forward_lite_transpose_val_167(net_ctx);
  }
  /* LITE_KERNEL_SECTION END val_167 */
  /* LITE_KERNEL_SECTION BEGIN val_174 */
  {
      ai_float* val_174_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 65536);
    const ai_float* val_174_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 98304);
    const ai_float* val_174_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 384032);
    const ai_float* val_174_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 384160);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(67, 1, {(stai_ptr) val_174_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(val_174_t_out_0_ptr_f32, val_174_t_in_0_ptr_const_f32, val_174_t_weight_0_ptr_const_f32, val_174_t_weight_1_ptr_const_f32, (ai_u32)(4096), (ai_size)(32));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(67, 1, {(stai_ptr) val_174_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END val_174 */
  /* LITE_KERNEL_SECTION BEGIN select_3 */
  {
    
  forward_lite_gather_select_3(net_ctx);
  }
  /* LITE_KERNEL_SECTION END select_3 */
  /* LITE_KERNEL_SECTION BEGIN transpose_8 */
  {
    
  forward_lite_transpose_transpose_8(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_8 */
  /* LITE_KERNEL_SECTION BEGIN val_171 */
  {
      ai_float* val_171_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 32768);
    const ai_float* val_171_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 16384);
    const ai_float* val_171_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 384032);
    const ai_float* val_171_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 384160);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(66, 1, {(stai_ptr) val_171_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(val_171_t_out_0_ptr_f32, val_171_t_in_0_ptr_const_f32, val_171_t_weight_0_ptr_const_f32, val_171_t_weight_1_ptr_const_f32, (ai_u32)(4096), (ai_size)(32));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(66, 1, {(stai_ptr) val_171_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END val_171 */
  /* LITE_KERNEL_SECTION BEGIN val_175 */
  {
    
  forward_lite_matmul_val_175(net_ctx);
  }
  /* LITE_KERNEL_SECTION END val_175 */
  /* LITE_KERNEL_SECTION BEGIN val_176 */
  {
      ai_handle val_176_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 32768);
    const ai_handle val_176_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 16384);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(69, 1, {(stai_ptr) val_176_t_in_0_ptr_const_handle});
    
  forward_lite_nl_softmax_if32of32(val_176_t_out_0_ptr_handle, val_176_t_in_0_ptr_const_handle, val_176_t_in_0_shape_ch_h_w_prod_const_s32, 1, 32);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(69, 1, {(stai_ptr) val_176_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END val_176 */
  /* LITE_KERNEL_SECTION BEGIN scaled_dot_product_attention_1 */
  {
    
  forward_lite_matmul_scaled_dot_product_attention_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END scaled_dot_product_attention_1 */
  /* LITE_KERNEL_SECTION BEGIN permute_1 */
  {
    
  forward_lite_transpose_permute_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END permute_1 */
  /* LITE_KERNEL_SECTION BEGIN linear_5 */
  {
    
  forward_lite_dense_linear_5(net_ctx);
  }
  /* LITE_KERNEL_SECTION END linear_5 */
  /* LITE_KERNEL_SECTION BEGIN add_3 */
  {
    
  forward_lite_eltwise_add_3(net_ctx);
  }
  /* LITE_KERNEL_SECTION END add_3 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_2_Reduce */
  {
    
  forward_lite_reduce_layer_norm_2_Reduce(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_2_Reduce */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_2_Reduce_Mul */
  {
      ai_float* layer_norm_2_Reduce_Mul_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 128);
    const ai_float* layer_norm_2_Reduce_Mul_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
    const ai_float* layer_norm_2_Reduce_Mul_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450336);
    const ai_float* layer_norm_2_Reduce_Mul_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450340);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(77, 1, {(stai_ptr) layer_norm_2_Reduce_Mul_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_2_Reduce_Mul_t_out_0_ptr_f32, layer_norm_2_Reduce_Mul_t_in_0_ptr_const_f32, layer_norm_2_Reduce_Mul_t_weight_0_ptr_const_f32, layer_norm_2_Reduce_Mul_t_weight_1_ptr_const_f32, (ai_u32)(32), (ai_size)(1));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(77, 1, {(stai_ptr) layer_norm_2_Reduce_Mul_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_2_Reduce_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_2_Sub */
  {
    
  forward_lite_eltwise_layer_norm_2_Sub(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_2_Sub */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_2_Mul */
  {
    
  forward_lite_eltwise_layer_norm_2_Mul(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_2_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_2_Reduce_1 */
  {
    
  forward_lite_reduce_layer_norm_2_Reduce_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_2_Reduce_1 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_2_Reduce_1_Mul */
  {
      ai_float* layer_norm_2_Reduce_1_Mul_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 128);
    const ai_float* layer_norm_2_Reduce_1_Mul_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
    const ai_float* layer_norm_2_Reduce_1_Mul_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450336);
    const ai_float* layer_norm_2_Reduce_1_Mul_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450344);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(77, 1, {(stai_ptr) layer_norm_2_Reduce_1_Mul_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_2_Reduce_1_Mul_t_out_0_ptr_f32, layer_norm_2_Reduce_1_Mul_t_in_0_ptr_const_f32, layer_norm_2_Reduce_1_Mul_t_weight_0_ptr_const_f32, layer_norm_2_Reduce_1_Mul_t_weight_1_ptr_const_f32, (ai_u32)(32), (ai_size)(1));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(77, 1, {(stai_ptr) layer_norm_2_Reduce_1_Mul_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_2_Reduce_1_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_2_Sqrt */
  {
      ai_handle layer_norm_2_Sqrt_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 0);
    const ai_handle layer_norm_2_Sqrt_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 128);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(77, 1, {(stai_ptr) layer_norm_2_Sqrt_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sqrt_if32of32(layer_norm_2_Sqrt_t_out_0_ptr_handle, layer_norm_2_Sqrt_t_in_0_ptr_const_handle, layer_norm_2_Sqrt_t_in_0_shape_ch_h_prod_const_s32, NULL);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(77, 1, {(stai_ptr) layer_norm_2_Sqrt_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END layer_norm_2_Sqrt */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_2_Reciprocal */
  {
      ai_handle layer_norm_2_Reciprocal_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 128);
    const ai_handle layer_norm_2_Reciprocal_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 0);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(77, 1, {(stai_ptr) layer_norm_2_Reciprocal_t_in_0_ptr_const_handle});
    
  forward_lite_nl_reciprocal_if32of32(layer_norm_2_Reciprocal_t_out_0_ptr_handle, layer_norm_2_Reciprocal_t_in_0_ptr_const_handle, layer_norm_2_Reciprocal_t_in_0_shape_ch_h_prod_const_s32, NULL);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(77, 1, {(stai_ptr) layer_norm_2_Reciprocal_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END layer_norm_2_Reciprocal */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_2_Mul_1 */
  {
    
  forward_lite_eltwise_layer_norm_2_Mul_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_2_Mul_1 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_2_Mul_2 */
  {
      ai_float* layer_norm_2_Mul_2_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
    const ai_float* layer_norm_2_Mul_2_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 16640);
    const ai_float* layer_norm_2_Mul_2_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 982316);
    const ai_float* layer_norm_2_Mul_2_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 982828);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(77, 1, {(stai_ptr) layer_norm_2_Mul_2_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_2_Mul_2_t_out_0_ptr_f32, layer_norm_2_Mul_2_t_in_0_ptr_const_f32, layer_norm_2_Mul_2_t_weight_0_ptr_const_f32, layer_norm_2_Mul_2_t_weight_1_ptr_const_f32, (ai_u32)(4096), (ai_size)(128));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(77, 1, {(stai_ptr) layer_norm_2_Mul_2_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_2_Mul_2 */
  /* LITE_KERNEL_SECTION BEGIN val_189 */
  {
    
  forward_lite_dense_val_189(net_ctx);
  }
  /* LITE_KERNEL_SECTION END val_189 */
  /* LITE_KERNEL_SECTION BEGIN val_191 */
  {
    
  forward_lite_eltwise_val_191(net_ctx);
  }
  /* LITE_KERNEL_SECTION END val_191 */
  /* LITE_KERNEL_SECTION BEGIN val_192 */
  {
      ai_handle val_192_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 81920);
    const ai_handle val_192_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 49152);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(81, 1, {(stai_ptr) val_192_t_in_0_ptr_const_handle});
    
  forward_lite_nl_erf_if32of32(val_192_t_out_0_ptr_handle, val_192_t_in_0_ptr_const_handle, val_192_t_in_0_shape_ch_h_prod_const_s32, NULL);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(81, 1, {(stai_ptr) val_192_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END val_192 */
  /* LITE_KERNEL_SECTION BEGIN val_194 */
  {
      ai_float* val_194_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 49152);
    const ai_float* val_194_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 81920);
    const ai_float* val_194_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 583468);
    const ai_float* val_194_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 584492);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(83, 1, {(stai_ptr) val_194_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(val_194_t_out_0_ptr_f32, val_194_t_in_0_ptr_const_f32, val_194_t_weight_0_ptr_const_f32, val_194_t_weight_1_ptr_const_f32, (ai_u32)(8192), (ai_size)(256));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(83, 1, {(stai_ptr) val_194_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END val_194 */
  /* LITE_KERNEL_SECTION BEGIN gelu_1 */
  {
    
  forward_lite_eltwise_gelu_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END gelu_1 */
  /* LITE_KERNEL_SECTION BEGIN val_198 */
  {
    
  forward_lite_dense_val_198(net_ctx);
  }
  /* LITE_KERNEL_SECTION END val_198 */
  /* LITE_KERNEL_SECTION BEGIN add_4 */
  {
    
  forward_lite_eltwise_add_4(net_ctx);
  }
  /* LITE_KERNEL_SECTION END add_4 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_3_Reduce */
  {
    
  forward_lite_reduce_layer_norm_3_Reduce(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_3_Reduce */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_3_Reduce_Mul */
  {
      ai_float* layer_norm_3_Reduce_Mul_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 128);
    const ai_float* layer_norm_3_Reduce_Mul_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
    const ai_float* layer_norm_3_Reduce_Mul_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450336);
    const ai_float* layer_norm_3_Reduce_Mul_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450340);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(88, 1, {(stai_ptr) layer_norm_3_Reduce_Mul_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_3_Reduce_Mul_t_out_0_ptr_f32, layer_norm_3_Reduce_Mul_t_in_0_ptr_const_f32, layer_norm_3_Reduce_Mul_t_weight_0_ptr_const_f32, layer_norm_3_Reduce_Mul_t_weight_1_ptr_const_f32, (ai_u32)(32), (ai_size)(1));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(88, 1, {(stai_ptr) layer_norm_3_Reduce_Mul_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_3_Reduce_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_3_Sub */
  {
    
  forward_lite_eltwise_layer_norm_3_Sub(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_3_Sub */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_3_Mul */
  {
    
  forward_lite_eltwise_layer_norm_3_Mul(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_3_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_3_Reduce_1 */
  {
    
  forward_lite_reduce_layer_norm_3_Reduce_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_3_Reduce_1 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_3_Reduce_1_Mul */
  {
      ai_float* layer_norm_3_Reduce_1_Mul_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 128);
    const ai_float* layer_norm_3_Reduce_1_Mul_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
    const ai_float* layer_norm_3_Reduce_1_Mul_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450336);
    const ai_float* layer_norm_3_Reduce_1_Mul_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450344);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(88, 1, {(stai_ptr) layer_norm_3_Reduce_1_Mul_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_3_Reduce_1_Mul_t_out_0_ptr_f32, layer_norm_3_Reduce_1_Mul_t_in_0_ptr_const_f32, layer_norm_3_Reduce_1_Mul_t_weight_0_ptr_const_f32, layer_norm_3_Reduce_1_Mul_t_weight_1_ptr_const_f32, (ai_u32)(32), (ai_size)(1));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(88, 1, {(stai_ptr) layer_norm_3_Reduce_1_Mul_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_3_Reduce_1_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_3_Sqrt */
  {
      ai_handle layer_norm_3_Sqrt_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 0);
    const ai_handle layer_norm_3_Sqrt_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 128);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(88, 1, {(stai_ptr) layer_norm_3_Sqrt_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sqrt_if32of32(layer_norm_3_Sqrt_t_out_0_ptr_handle, layer_norm_3_Sqrt_t_in_0_ptr_const_handle, layer_norm_3_Sqrt_t_in_0_shape_ch_h_prod_const_s32, NULL);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(88, 1, {(stai_ptr) layer_norm_3_Sqrt_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END layer_norm_3_Sqrt */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_3_Reciprocal */
  {
      ai_handle layer_norm_3_Reciprocal_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 128);
    const ai_handle layer_norm_3_Reciprocal_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 0);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(88, 1, {(stai_ptr) layer_norm_3_Reciprocal_t_in_0_ptr_const_handle});
    
  forward_lite_nl_reciprocal_if32of32(layer_norm_3_Reciprocal_t_out_0_ptr_handle, layer_norm_3_Reciprocal_t_in_0_ptr_const_handle, layer_norm_3_Reciprocal_t_in_0_shape_ch_h_prod_const_s32, NULL);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(88, 1, {(stai_ptr) layer_norm_3_Reciprocal_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END layer_norm_3_Reciprocal */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_3_Mul_1 */
  {
    
  forward_lite_eltwise_layer_norm_3_Mul_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_3_Mul_1 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_3_Mul_2 */
  {
      ai_float* layer_norm_3_Mul_2_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
    const ai_float* layer_norm_3_Mul_2_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 16640);
    const ai_float* layer_norm_3_Mul_2_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 1247020);
    const ai_float* layer_norm_3_Mul_2_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 1247532);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(88, 1, {(stai_ptr) layer_norm_3_Mul_2_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_3_Mul_2_t_out_0_ptr_f32, layer_norm_3_Mul_2_t_in_0_ptr_const_f32, layer_norm_3_Mul_2_t_weight_0_ptr_const_f32, layer_norm_3_Mul_2_t_weight_1_ptr_const_f32, (ai_u32)(4096), (ai_size)(128));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(88, 1, {(stai_ptr) layer_norm_3_Mul_2_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_3_Mul_2 */
  /* LITE_KERNEL_SECTION BEGIN select_6 */
  {
    
  forward_lite_gather_select_6(net_ctx);
  }
  /* LITE_KERNEL_SECTION END select_6 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_4_Transpose */
  {
    
  forward_lite_transpose_layer_norm_4_Transpose(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_4_Transpose */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_4_Reduce */
  {
    
  forward_lite_reduce_layer_norm_4_Reduce(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_4_Reduce */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_4_Reduce_Mul */
  {
      ai_float* layer_norm_4_Reduce_Mul_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 516);
    const ai_float* layer_norm_4_Reduce_Mul_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 512);
    const ai_float* layer_norm_4_Reduce_Mul_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450336);
    const ai_float* layer_norm_4_Reduce_Mul_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450340);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(90, 1, {(stai_ptr) layer_norm_4_Reduce_Mul_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_4_Reduce_Mul_t_out_0_ptr_f32, layer_norm_4_Reduce_Mul_t_in_0_ptr_const_f32, layer_norm_4_Reduce_Mul_t_weight_0_ptr_const_f32, layer_norm_4_Reduce_Mul_t_weight_1_ptr_const_f32, (ai_u32)(1), (ai_size)(1));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(90, 1, {(stai_ptr) layer_norm_4_Reduce_Mul_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_4_Reduce_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_4_Sub */
  {
    
  forward_lite_eltwise_layer_norm_4_Sub(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_4_Sub */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_4_Mul */
  {
    
  forward_lite_eltwise_layer_norm_4_Mul(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_4_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_4_Reduce_1 */
  {
    
  forward_lite_reduce_layer_norm_4_Reduce_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_4_Reduce_1 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_4_Reduce_1_Mul */
  {
      ai_float* layer_norm_4_Reduce_1_Mul_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
    const ai_float* layer_norm_4_Reduce_1_Mul_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 512);
    const ai_float* layer_norm_4_Reduce_1_Mul_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450336);
    const ai_float* layer_norm_4_Reduce_1_Mul_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 450344);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(90, 1, {(stai_ptr) layer_norm_4_Reduce_1_Mul_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_4_Reduce_1_Mul_t_out_0_ptr_f32, layer_norm_4_Reduce_1_Mul_t_in_0_ptr_const_f32, layer_norm_4_Reduce_1_Mul_t_weight_0_ptr_const_f32, layer_norm_4_Reduce_1_Mul_t_weight_1_ptr_const_f32, (ai_u32)(1), (ai_size)(1));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(90, 1, {(stai_ptr) layer_norm_4_Reduce_1_Mul_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_4_Reduce_1_Mul */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_4_Sqrt */
  {
      ai_handle layer_norm_4_Sqrt_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 4);
    const ai_handle layer_norm_4_Sqrt_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 0);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(90, 1, {(stai_ptr) layer_norm_4_Sqrt_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sqrt_if32of32(layer_norm_4_Sqrt_t_out_0_ptr_handle, layer_norm_4_Sqrt_t_in_0_ptr_const_handle, layer_norm_4_Sqrt_t_in_0_shape_ch_prod_const_s32, NULL);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(90, 1, {(stai_ptr) layer_norm_4_Sqrt_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END layer_norm_4_Sqrt */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_4_Reciprocal */
  {
      ai_handle layer_norm_4_Reciprocal_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 0);
    const ai_handle layer_norm_4_Reciprocal_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 4);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(90, 1, {(stai_ptr) layer_norm_4_Reciprocal_t_in_0_ptr_const_handle});
    
  forward_lite_nl_reciprocal_if32of32(layer_norm_4_Reciprocal_t_out_0_ptr_handle, layer_norm_4_Reciprocal_t_in_0_ptr_const_handle, layer_norm_4_Reciprocal_t_in_0_shape_ch_prod_const_s32, NULL);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(90, 1, {(stai_ptr) layer_norm_4_Reciprocal_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END layer_norm_4_Reciprocal */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_4_Mul_1 */
  {
    
  forward_lite_eltwise_layer_norm_4_Mul_1(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_4_Mul_1 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_4_Mul_2 */
  {
      ai_float* layer_norm_4_Mul_2_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 516);
    const ai_float* layer_norm_4_Mul_2_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 4);
    const ai_float* layer_norm_4_Mul_2_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 1248044);
    const ai_float* layer_norm_4_Mul_2_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 1248556);
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(90, 1, {(stai_ptr) layer_norm_4_Mul_2_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(layer_norm_4_Mul_2_t_out_0_ptr_f32, layer_norm_4_Mul_2_t_in_0_ptr_const_f32, layer_norm_4_Mul_2_t_weight_0_ptr_const_f32, layer_norm_4_Mul_2_t_weight_1_ptr_const_f32, (ai_u32)(128), (ai_size)(128));
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(90, 1, {(stai_ptr) layer_norm_4_Mul_2_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END layer_norm_4_Mul_2 */
  /* LITE_KERNEL_SECTION BEGIN layer_norm_4_Transpose_out_0 */
  {
    
  forward_lite_transpose_layer_norm_4_Transpose_out_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END layer_norm_4_Transpose_out_0 */
  /* LITE_KERNEL_SECTION BEGIN logits */
  {
      forward_lite_dense_if32of32wf32_args arg_30f51e = {
      .output = (float*)(net_ctx->_outputs[0] + 0),
      .input = (float*)(net_ctx->_activations[0] + 0),
      .weights = (float*)(net_ctx->_weights[0] + 1249068),
      .bias = (float*)(net_ctx->_weights[0] + 1253676),
      .n_channel_in = 128,
      .n_channel_out = 9,
      .n_elements = 1,
    };
  
  _STAI_TIM_NETWORK_EVENT_NODE_START_CB(91, 1, {(stai_ptr) (float*)(net_ctx->_activations[0] + 0)});
    
  forward_lite_dense_if32of32wf32((forward_lite_dense_if32of32wf32_args*)&arg_30f51e);
    
  _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB(91, 1, {(stai_ptr) (float*)(net_ctx->_outputs[0] + 0)});
  }
  /* LITE_KERNEL_SECTION END logits */
  return net_ctx->_return_code;
}

/*****************************************************************************/
/*  Getters APIs Section  */
STAI_API_ENTRY
stai_size stai_tim_network_get_context_size()
{
  return (stai_size)STAI_TIM_NETWORK_CONTEXT_SIZE;
}

#if defined(HAVE_TIM_NETWORK_INFO)
STAI_API_ENTRY
stai_return_code stai_tim_network_get_info(
  stai_network* network,
  stai_network_info* info)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, info==NULL, STAI_ERROR_NETWORK_INVALID_INFO, net_ctx->_return_code)

  // Copy of network info struct
  *info = g_tim_network_info;

  return STAI_SUCCESS;
}
#endif


STAI_API_ENTRY
stai_return_code stai_tim_network_get_activations(
  stai_network* network, stai_ptr* activations, stai_size* n_activations)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  _STAI_SET_ERROR(net_ctx, !n_activations, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_activations = STAI_TIM_NETWORK_ACTIVATIONS_NUM;
for (stai_size idx=0; activations && (idx<STAI_TIM_NETWORK_ACTIVATIONS_NUM); idx++) {
    // get address of the activations buffers
    activations[idx] = net_ctx->_activations[idx];
  }return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_tim_network_get_weights(
  stai_network* network, stai_ptr* weights, stai_size* n_weights)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_weights, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_weights = STAI_TIM_NETWORK_WEIGHTS_NUM;
for (stai_size idx=0; weights && (idx<STAI_TIM_NETWORK_WEIGHTS_NUM); idx++) {
    // get address of the weights buffers
    weights[idx] = net_ctx->_weights[idx];
  }return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_tim_network_get_inputs(
  stai_network* network, stai_ptr* inputs, stai_size* n_inputs)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_inputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_inputs = STAI_TIM_NETWORK_IN_NUM;
  for (stai_size idx=0; inputs && (idx<STAI_TIM_NETWORK_IN_NUM); idx++) {
    inputs[idx] = net_ctx->_inputs[idx];
  }
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_tim_network_get_outputs(
  stai_network* network, stai_ptr* outputs, stai_size* n_outputs)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_outputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_outputs = STAI_TIM_NETWORK_OUT_NUM;
  for (stai_size idx=0; outputs && (idx<STAI_TIM_NETWORK_OUT_NUM); idx++) {
    outputs[idx] = net_ctx->_outputs[idx];
  }
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_tim_network_get_error(
  stai_network* network)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  /* return 1st generated error or STAI_SUCCESS if no errors so far */
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_tim_network_get_states(
  stai_network* network, stai_ptr* states, stai_size* n_states)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_states, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  /* get the number of internals states (supporting multi-heap also for internal states) */
  *n_states = STAI_TIM_NETWORK_STATES_NUM;

  STAI_UNUSED(states)
return net_ctx->_return_code;
}


/*****************************************************************************/
/*  Setters APIs Section  */

STAI_API_ENTRY
stai_return_code stai_tim_network_set_activations(
  stai_network* network,
  const stai_ptr* activations,
  const stai_size n_activations)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
const uintptr_t _activations_alignment[] = STAI_TIM_NETWORK_ACTIVATIONS_ALIGNMENTS;
  STAI_PRINT("  [stai_tim_network_set_activations] network(%p) activations[%d]: %p\n\n", net_ctx, n_activations, activations)
  _STAI_SET_ERROR(net_ctx, !activations,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_activations!=STAI_TIM_NETWORK_ACTIVATIONS_NUM,
                  STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_NUM, net_ctx->_return_code)

  for (stai_size idx=0; activations && idx<STAI_TIM_NETWORK_ACTIVATIONS_NUM; idx++) {
    STAI_PRINT("  activation[%d]: %p\n", idx, activations[idx])
    _STAI_SET_ERROR(net_ctx, activations[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)activations[idx]) & (_activations_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_activations[idx] = activations[idx];
  }
  net_ctx->_inputs[0] = activations[0] + 32640;

  net_ctx->_outputs[0] = activations[0] + 512;
_stai_tim_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_tim_network_set_weights(
  stai_network* network,
  const stai_ptr* weights,
  const stai_size n_weights)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
const uintptr_t _weights_alignment[] = STAI_TIM_NETWORK_WEIGHTS_ALIGNMENTS;
  _STAI_SET_ERROR(net_ctx, !weights,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_weights!=STAI_TIM_NETWORK_WEIGHTS_NUM,
                  STAI_ERROR_NETWORK_INVALID_WEIGHTS_NUM, net_ctx->_return_code)
  for (stai_size idx=0; weights && idx<STAI_TIM_NETWORK_WEIGHTS_NUM; idx++) {
    STAI_PRINT("  weight[%d]: %p\n", idx, weights[idx])
    _STAI_SET_ERROR(net_ctx, weights[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)weights[idx]) & (_weights_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_weights[idx] = weights[idx];
  }_stai_tim_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_tim_network_set_inputs(
  stai_network* network,
  const stai_ptr* inputs,
  const stai_size n_inputs)
{
  const uintptr_t _inputs_alignment[] = STAI_TIM_NETWORK_IN_ALIGNMENTS;
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !inputs,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_inputs!=STAI_TIM_NETWORK_IN_NUM,
                  STAI_ERROR_NETWORK_INVALID_IN_NUM, net_ctx->_return_code)

  for (stai_size idx=0; inputs && idx<STAI_TIM_NETWORK_IN_NUM; idx++) {
    STAI_PRINT("  input[%d]: %p\n", idx, inputs[idx])
    _STAI_SET_ERROR(net_ctx, inputs[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)inputs[idx]) & (_inputs_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_inputs[idx] = inputs[idx];
  }

  _stai_tim_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_tim_network_set_outputs(
  stai_network* network,
  const stai_ptr* outputs,
  const stai_size n_outputs)
{
  const uintptr_t _outputs_alignment[] = STAI_TIM_NETWORK_OUT_ALIGNMENTS;
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !outputs,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_outputs!=STAI_TIM_NETWORK_OUT_NUM,
                  STAI_ERROR_NETWORK_INVALID_OUT_NUM, net_ctx->_return_code)

  for (stai_size idx=0; outputs && idx<n_outputs; idx++) {
    STAI_PRINT("  output[%d]: %p\n", idx, outputs[idx])
    _STAI_SET_ERROR(net_ctx, outputs[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)outputs[idx]) & (_outputs_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_outputs[idx] = outputs[idx];
  }

  _stai_tim_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_tim_network_set_states(
  stai_network* network,
  const stai_ptr* states,
  const stai_size n_states)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  STAI_UNUSED(states)
  STAI_UNUSED(n_states)
_stai_tim_network_check(net_ctx);
  return net_ctx->_return_code;
}

STAI_API_ENTRY
stai_return_code stai_tim_network_set_callback(
  stai_network* network, const stai_event_cb cb, void* cb_cookie)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  STAI_PRINT("  set_callback %p cb %p cookie %p\n", net_ctx, cb, cb_cookie)
  // _STAI_SET_ERROR(net_ctx, cb==NULL, STAI_ERROR_NETWORK_INVALID_CALLBACK, net_ctx->_return_code)
  net_ctx->_callback = cb;
  net_ctx->_callback_cookie = cb_cookie;
  return net_ctx->_return_code;
}

#undef _STAI_SET_ERROR
#undef _STAI_CONTEXT_ALIGNMENT
#undef _STAI_CONTEXT_ACQUIRE
#undef _STAI_TIM_NETWORK_EVENT_NODE_START_CB
#undef _STAI_TIM_NETWORK_EVENT_NODE_STOP_CB
#undef _STAI_TIM_NETWORK_MODEL_SIGNATURE
#undef _STAI_TIM_NETWORK_DATETIME
#undef _STAI_TIM_NETWORK_COMPILE_DATETIME

