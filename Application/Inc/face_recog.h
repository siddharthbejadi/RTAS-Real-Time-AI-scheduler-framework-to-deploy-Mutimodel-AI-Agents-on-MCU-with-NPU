/**
 ******************************************************************************
 * @file    face_recog.h
 * @brief   Face recognition pipeline (MobileFaceNet embedding extractor).
 *
 *  Pipeline (per detected face):
 *    1. Crop face from 480x480 display-pipe frame using the bounding box
 *       returned by BlazeFace postprocess.
 *    2. Resize to 112x112 (MobileFaceNet default input) — bilinear.
 *    3. Run MobileFaceNet on NPU → 128-d embedding.
 *    4. FaceStore_Match() against enrolled embeddings.
 *
 *  Build flag: HAVE_RECOG_NETWORK
 *    Set to 1 (in app_config.h) once you have generated the MobileFaceNet
 *    stedgeai C files (e.g. stai_network_embed.[ch]) and added them to the
 *    project. Until then, FaceRecog_Extract() returns an error and the UI
 *    simply falls back to "enrolled-by-default" behaviour.
 ******************************************************************************
 */
#ifndef FACE_RECOG_H
#define FACE_RECOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "app_postprocess.h"
#include "face_store.h"

/* Recognition-input dimensions — match your MobileFaceNet variant. */
#define FACE_RECOG_IN_W      112u
#define FACE_RECOG_IN_H      112u
#define FACE_RECOG_IN_C      3u     /* RGB */

/* Cosine-similarity threshold for a positive match (tune on validation set). */
#ifndef FACE_RECOG_MATCH_THRESHOLD
#define FACE_RECOG_MATCH_THRESHOLD   0.60f
#endif

typedef enum {
    FACE_RECOG_OK            = 0,
    FACE_RECOG_ERR_NOT_READY = -1,   /* 2nd model not linked in */
    FACE_RECOG_ERR_BAD_BOX   = -2,
    FACE_RECOG_ERR_NPU       = -3,
} FaceRecog_Status_t;

typedef enum {
    FACE_ENROLL_OK            = 0,
    FACE_ENROLL_ERR_EXTRACT   = -1,
    FACE_ENROLL_ERR_DUPLICATE = -2,
    FACE_ENROLL_ERR_STORE     = -3,
    FACE_ENROLL_ERR_COMMIT    = -4,
} FaceEnroll_Status_t;

/**
 * @brief  One-time initialisation.  Boots the 2nd stai_network_* context
 *         when HAVE_RECOG_NETWORK is defined.  No-op otherwise.
 * @return FACE_RECOG_OK on success (or when not-ready and that's acceptable).
 */
FaceRecog_Status_t FaceRecog_Init(void);

/** @brief  True when the 2nd model is linked in and successfully initialised. */
bool FaceRecog_IsReady(void);

/**
 * @brief  Extract an embedding for a single detection.
 * @param  rgb_frame         pointer to the display-pipe frame (RGB888, tight-packed)
 * @param  frame_w,frame_h   frame size (typically 480x480 post-crop)
 * @param  bbox              detection bounding box in normalised [0..1] coords
 *                           (as produced by BlazeFace postprocess)
 * @param  out_emb           [out] FACE_STORE_EMB_DIM floats
 * @return FACE_RECOG_OK on success.
 */
FaceRecog_Status_t FaceRecog_Extract(const uint8_t *rgb_frame,
                                     uint32_t frame_w,
                                     uint32_t frame_h,
                                     const od_pp_outBuffer_t *bbox,
                                     float *out_emb);

/**
 * @brief  Convenience: extract + match against the face store in one call.
 * @param  out_index  [out] matched person index, or -1 if no match / not-ready
 * @param  out_score  [out] best cosine similarity (for UI display)
 * @return FACE_RECOG_OK + *out_index>=0 on a positive match.
 */
FaceRecog_Status_t FaceRecog_Identify(const uint8_t *rgb_frame,
                                      uint32_t frame_w,
                                      uint32_t frame_h,
                                      const od_pp_outBuffer_t *bbox,
                                      int32_t *out_index,
                                      float *out_score);

/**
 * @brief  Extract embedding for the given bbox, then save as a new enrolment
 *         via FaceStore_Add() + FaceStore_Commit().
 * @return true on success.
 */
bool FaceRecog_EnrollFromFrame(const char *name,
                               const uint8_t *rgb_frame,
                               uint32_t frame_w,
                               uint32_t frame_h,
                               const od_pp_outBuffer_t *bbox);

FaceEnroll_Status_t FaceRecog_EnrollFromFrameEx(const char *name,
                                                const uint8_t *rgb_frame,
                                                uint32_t frame_w,
                                                uint32_t frame_h,
                                                const od_pp_outBuffer_t *bbox,
                                                uint32_t *matched_index,
                                                float *matched_score);

#ifdef __cplusplus
}
#endif
#endif /* FACE_RECOG_H */
