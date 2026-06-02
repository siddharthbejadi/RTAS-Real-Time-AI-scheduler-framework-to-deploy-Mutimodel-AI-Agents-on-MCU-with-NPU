/**
 ******************************************************************************
 * @file    stai_network.h
 * @brief   Wrapper header - maps generic stai_network API to the face model
 *          (stai_network_face). This allows main.c and other generic code to
 *          use STAI_NETWORK_* macros and stai_network_*() functions without
 *          needing to know the specific model name.
 ******************************************************************************
 */

#ifndef __STAI_NETWORK_H_WRAPPER
#define __STAI_NETWORK_H_WRAPPER

#include "stai_network_face.h"

/* ── Context ─────────────────────────────────────────────────────────────── */
#define STAI_NETWORK_CONTEXT_SIZE         STAI_NETWORK_FACE_CONTEXT_SIZE
#define STAI_NETWORK_CONTEXT_ALIGNMENT    STAI_NETWORK_FACE_CONTEXT_ALIGNMENT

/* ── Input descriptors ───────────────────────────────────────────────────── */
#define STAI_NETWORK_IN_NUM               STAI_NETWORK_FACE_IN_NUM
#define STAI_NETWORK_IN_1_WIDTH           STAI_NETWORK_FACE_IN_1_WIDTH
#define STAI_NETWORK_IN_1_HEIGHT          STAI_NETWORK_FACE_IN_1_HEIGHT
#define STAI_NETWORK_IN_1_CHANNEL         STAI_NETWORK_FACE_IN_1_CHANNEL
#define STAI_NETWORK_IN_1_FORMAT          STAI_NETWORK_FACE_IN_1_FORMAT
#define STAI_NETWORK_IN_1_SIZE            STAI_NETWORK_FACE_IN_1_SIZE
#define STAI_NETWORK_IN_1_SIZE_BYTES      STAI_NETWORK_FACE_IN_1_SIZE_BYTES
#define STAI_NETWORK_IN_1_ALIGNMENT       STAI_NETWORK_FACE_IN_1_ALIGNMENT
#define STAI_NETWORK_IN_1_BATCH           STAI_NETWORK_FACE_IN_1_BATCH
#define STAI_NETWORK_IN_1_RANK            STAI_NETWORK_FACE_IN_1_RANK
#define STAI_NETWORK_IN_1_FLAGS           STAI_NETWORK_FACE_IN_1_FLAGS
#define STAI_NETWORK_IN_1_NAME            STAI_NETWORK_FACE_IN_1_NAME

/* ── Output descriptors ──────────────────────────────────────────────────── */
#define STAI_NETWORK_OUT_NUM              STAI_NETWORK_FACE_OUT_NUM

#define STAI_NETWORK_OUT_1_SIZE           STAI_NETWORK_FACE_OUT_1_SIZE
#define STAI_NETWORK_OUT_1_SIZE_BYTES     STAI_NETWORK_FACE_OUT_1_SIZE_BYTES
#define STAI_NETWORK_OUT_1_ALIGNMENT      STAI_NETWORK_FACE_OUT_1_ALIGNMENT
#define STAI_NETWORK_OUT_1_FORMAT         STAI_NETWORK_FACE_OUT_1_FORMAT
#define STAI_NETWORK_OUT_1_CHANNEL        STAI_NETWORK_FACE_OUT_1_CHANNEL
#define STAI_NETWORK_OUT_1_HEIGHT         STAI_NETWORK_FACE_OUT_1_HEIGHT
#define STAI_NETWORK_OUT_1_WIDTH          STAI_NETWORK_FACE_OUT_1_WIDTH

#define STAI_NETWORK_OUT_2_SIZE           STAI_NETWORK_FACE_OUT_2_SIZE
#define STAI_NETWORK_OUT_2_SIZE_BYTES     STAI_NETWORK_FACE_OUT_2_SIZE_BYTES
#define STAI_NETWORK_OUT_2_ALIGNMENT      STAI_NETWORK_FACE_OUT_2_ALIGNMENT
#define STAI_NETWORK_OUT_2_FORMAT         STAI_NETWORK_FACE_OUT_2_FORMAT

#define STAI_NETWORK_OUT_3_SIZE           STAI_NETWORK_FACE_OUT_3_SIZE
#define STAI_NETWORK_OUT_3_SIZE_BYTES     STAI_NETWORK_FACE_OUT_3_SIZE_BYTES
#define STAI_NETWORK_OUT_3_ALIGNMENT      STAI_NETWORK_FACE_OUT_3_ALIGNMENT
#define STAI_NETWORK_OUT_3_FORMAT         STAI_NETWORK_FACE_OUT_3_FORMAT

#define STAI_NETWORK_OUT_4_SIZE           STAI_NETWORK_FACE_OUT_4_SIZE
#define STAI_NETWORK_OUT_4_SIZE_BYTES     STAI_NETWORK_FACE_OUT_4_SIZE_BYTES
#define STAI_NETWORK_OUT_4_ALIGNMENT      STAI_NETWORK_FACE_OUT_4_ALIGNMENT
#define STAI_NETWORK_OUT_4_FORMAT         STAI_NETWORK_FACE_OUT_4_FORMAT

/* ── Model info ──────────────────────────────────────────────────────────── */
#define STAI_NETWORK_ORIGIN_MODEL_NAME    STAI_NETWORK_FACE_ORIGIN_MODEL_NAME
#define STAI_NETWORK_FLAGS                STAI_NETWORK_FACE_FLAGS

/* ── Function mapping ────────────────────────────────────────────────────── */
#define stai_network_init                 stai_network_face_init
#define stai_network_deinit               stai_network_face_deinit
#define stai_network_run                  stai_network_face_run
#define stai_network_get_info             stai_network_face_get_info
#define stai_network_get_inputs           stai_network_face_get_inputs
#define stai_network_get_outputs          stai_network_face_get_outputs
#define stai_network_get_error            stai_network_face_get_error
#define stai_network_get_weights          stai_network_face_get_weights
#define stai_network_get_activations      stai_network_face_get_activations
#define stai_network_get_states           stai_network_face_get_states
#define stai_network_set_inputs           stai_network_face_set_inputs
#define stai_network_set_outputs          stai_network_face_set_outputs
#define stai_network_set_weights          stai_network_face_set_weights
#define stai_network_set_activations      stai_network_face_set_activations
#define stai_network_set_states           stai_network_face_set_states
#define stai_network_set_callback         stai_network_face_set_callback
#define stai_ext_network_new_inference    stai_ext_network_face_new_inference

#endif /* __STAI_NETWORK_H_WRAPPER */
