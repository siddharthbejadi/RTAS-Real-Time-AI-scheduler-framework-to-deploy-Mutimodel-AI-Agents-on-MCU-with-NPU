/**
 ******************************************************************************
 * @file    face_recog.c
 * @brief   MobileFaceNet embedding extractor + enrolment helper.
 *
 * Until you generate and add the MobileFaceNet stedgeai outputs, this file
 * compiles as a collection of no-ops that return FACE_RECOG_ERR_NOT_READY.
 * As soon as you set HAVE_RECOG_NETWORK=1 in app_config.h (and the corresponding
 * stai_network_embed.[ch] files exist), the real NPU path is enabled.
 ******************************************************************************
 */
#include "app_config.h"
#include "face_recog.h"
#include "app_depth.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "stm32n6xx_hal.h"

#if defined(HAVE_RECOG_NETWORK) && (HAVE_RECOG_NETWORK != 0)
  #include "stai.h"
  #include "stai_network_embed.h"
#endif

/* ───── Private state ─────────────────────────────────────────────────────── */

#if defined(HAVE_RECOG_NETWORK) && (HAVE_RECOG_NETWORK != 0)
STAI_NETWORK_CONTEXT_DECLARE(s_recog_ctx, STAI_NETWORK_EMBED_CONTEXT_SIZE)
static stai_ptr  s_recog_in;
static stai_ptr  s_recog_out[STAI_NETWORK_EMBED_OUT_NUM];
static uint32_t  s_recog_in_bytes;

/* Scratch face crop (pre-resize): 112x112 RGB888 */
__attribute__((section(".psram_bss")))
__attribute__((aligned(32)))
static int8_t    s_crop_buf[FACE_RECOG_IN_W * FACE_RECOG_IN_H * FACE_RECOG_IN_C];

static bool s_ready = false;
#else
static bool s_ready = false;
#endif

uint32_t g_embed_npu_ms = 0U;

/* ───── Helpers ───────────────────────────────────────────────────────────── */

#if defined(HAVE_RECOG_NETWORK) && (HAVE_RECOG_NETWORK != 0)
/* Bilinear resize into MobileFaceNet S8 CHW input. */
static void resize_face_bilinear(const uint8_t *src, uint32_t src_w, uint32_t src_h,
                                 uint32_t sx, uint32_t sy,
                                 uint32_t sw, uint32_t sh,
                                 int8_t *dst)
{
    const float rx = (float)sw / (float)FACE_RECOG_IN_W;
    const float ry = (float)sh / (float)FACE_RECOG_IN_H;
    for (uint32_t y = 0; y < FACE_RECOG_IN_H; y++) {
        for (uint32_t x = 0; x < FACE_RECOG_IN_W; x++) {
            float fx = sx + (x + 0.5f) * rx - 0.5f;
            float fy = sy + (y + 0.5f) * ry - 0.5f;
            int   ix = (int)fx;  if (ix < 0) ix = 0; if ((uint32_t)ix >= src_w - 1) ix = src_w - 2;
            int   iy = (int)fy;  if (iy < 0) iy = 0; if ((uint32_t)iy >= src_h - 1) iy = src_h - 2;
            float dx = fx - ix;  if (dx < 0) dx = 0; if (dx > 1) dx = 1;
            float dy = fy - iy;  if (dy < 0) dy = 0; if (dy > 1) dy = 1;

            for (int c = 0; c < 3; c++) {
                const uint8_t *row0 = src + (iy * src_w + ix) * 3 + c;
                const uint8_t *row1 = row0 + src_w * 3;
                float v = (1 - dx) * (1 - dy) * row0[0] +
                               dx  * (1 - dy) * row0[3] +
                          (1 - dx) *      dy  * row1[0] +
                               dx  *      dy  * row1[3];
                int q = (int)(v + 0.5f) - 128;
                if (q < -128) q = -128;
                if (q > 127) q = 127;
                dst[(c * FACE_RECOG_IN_H + y) * FACE_RECOG_IN_W + x] = (int8_t)q;
            }
        }
    }
}
#endif /* HAVE_RECOG_NETWORK */

/* ───── Public API ────────────────────────────────────────────────────────── */

FaceRecog_Status_t FaceRecog_Init(void)
{
#if defined(HAVE_RECOG_NETWORK) && (HAVE_RECOG_NETWORK != 0)
    stai_return_code ret = stai_network_embed_init(s_recog_ctx);
    if (ret != STAI_SUCCESS) {
        printf("[FaceRecog] network_embed_init failed (%d)\r\n", (int)ret);
        s_ready = false;
        return FACE_RECOG_ERR_NPU;
    }

    stai_size n_in = STAI_NETWORK_EMBED_IN_NUM;
    ret = stai_network_embed_get_inputs(s_recog_ctx, &s_recog_in, &n_in);
    if (ret != STAI_SUCCESS) { s_ready = false; return FACE_RECOG_ERR_NPU; }

    stai_size n_out = STAI_NETWORK_EMBED_OUT_NUM;
    ret = stai_network_embed_get_outputs(s_recog_ctx, s_recog_out, &n_out);
    if (ret != STAI_SUCCESS) { s_ready = false; return FACE_RECOG_ERR_NPU; }

    s_recog_in_bytes = STAI_NETWORK_EMBED_IN_1_SIZE_BYTES;
    s_ready = true;
    printf("[FaceRecog] MobileFaceNet ready (in=%lu bytes, %u outputs)\r\n",
           (unsigned long)s_recog_in_bytes, (unsigned)n_out);
    return FACE_RECOG_OK;
#else
    printf("[FaceRecog] HAVE_RECOG_NETWORK not set — recognition disabled\r\n");
    s_ready = false;
    return FACE_RECOG_OK;  /* not an error — just not available */
#endif
}

bool FaceRecog_IsReady(void)
{
    return s_ready;
}

FaceRecog_Status_t FaceRecog_Extract(const uint8_t *rgb_frame,
                                     uint32_t frame_w,
                                     uint32_t frame_h,
                                     const od_pp_outBuffer_t *bbox,
                                     float *out_emb)
{
    if (!s_ready) return FACE_RECOG_ERR_NOT_READY;
    if (!rgb_frame || !bbox || !out_emb) return FACE_RECOG_ERR_BAD_BOX;

#if defined(HAVE_RECOG_NETWORK) && (HAVE_RECOG_NETWORK != 0)
    /* BlazeFace boxes are in normalised [0..1] coordinates. */
    int sx = (int)(bbox->x_center * frame_w - bbox->width  * frame_w * 0.5f);
    int sy = (int)(bbox->y_center * frame_h - bbox->height * frame_h * 0.5f);
    int sw = (int)(bbox->width    * frame_w);
    int sh = (int)(bbox->height   * frame_h);
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sx + sw > (int)frame_w) sw = frame_w - sx;
    if (sy + sh > (int)frame_h) sh = frame_h - sy;
    if (sw < 8 || sh < 8) return FACE_RECOG_ERR_BAD_BOX;

    /* Resize the face region into the network input buffer. */
    resize_face_bilinear(rgb_frame, frame_w, frame_h,
                         (uint32_t)sx, (uint32_t)sy,
                         (uint32_t)sw, (uint32_t)sh,
                         s_crop_buf);

    /* Copy into the NPU input (DCache-aligned). */
    memcpy((void *)s_recog_in, s_crop_buf, s_recog_in_bytes);
    SCB_CleanInvalidateDCache_by_Addr((void *)s_recog_in, s_recog_in_bytes);

    /* Run inference (sync). */
    uint32_t npu_start = HAL_GetTick();
    stai_return_code ret;
    do {
        ret = stai_network_embed_run(s_recog_ctx, STAI_MODE_SYNC);
    } while (ret == STAI_RUNNING_WFE || ret == STAI_RUNNING_NO_WFE);
    g_embed_npu_ms = HAL_GetTick() - npu_start;
    if (ret != STAI_SUCCESS) return FACE_RECOG_ERR_NPU;

    /* Copy out embedding (first output). */
    SCB_InvalidateDCache_by_Addr((void *)s_recog_out[0],
                                 STAI_NETWORK_EMBED_OUT_1_SIZE_BYTES);
    memcpy(out_emb, (void *)s_recog_out[0],
           FACE_STORE_EMB_DIM * sizeof(float));

    /* Prep next inference. */
    stai_ext_network_embed_new_inference(s_recog_ctx);
    return FACE_RECOG_OK;
#else
    (void)rgb_frame; (void)frame_w; (void)frame_h; (void)bbox; (void)out_emb;
    return FACE_RECOG_ERR_NOT_READY;
#endif
}

FaceRecog_Status_t FaceRecog_Identify(const uint8_t *rgb_frame,
                                      uint32_t frame_w,
                                      uint32_t frame_h,
                                      const od_pp_outBuffer_t *bbox,
                                      int32_t *out_index,
                                      float *out_score)
{
    float emb[FACE_STORE_EMB_DIM];
    FaceRecog_Status_t st = FaceRecog_Extract(rgb_frame, frame_w, frame_h,
                                              bbox, emb);
    if (out_index) *out_index = -1;
    if (out_score) *out_score = 0.0f;
    if (st != FACE_RECOG_OK) return st;

    uint32_t idx = 0;
    float    score = 0.0f;
    bool matched = FaceStore_Match(emb, &idx, &score,
                                   FACE_RECOG_MATCH_THRESHOLD);
    if (out_score) *out_score = score;
    if (matched) {
        if (out_index) *out_index = (int32_t)idx;
    }
    return FACE_RECOG_OK;
}

bool FaceRecog_EnrollFromFrame(const char *name,
                               const uint8_t *rgb_frame,
                               uint32_t frame_w,
                               uint32_t frame_h,
                               const od_pp_outBuffer_t *bbox)
{
    return FaceRecog_EnrollFromFrameEx(name, rgb_frame, frame_w, frame_h,
                                       bbox, NULL, NULL) == FACE_ENROLL_OK;
}

FaceEnroll_Status_t FaceRecog_EnrollFromFrameEx(const char *name,
                                                const uint8_t *rgb_frame,
                                                uint32_t frame_w,
                                                uint32_t frame_h,
                                                const od_pp_outBuffer_t *bbox,
                                                uint32_t *matched_index,
                                                float *matched_score)
{
    float emb[FACE_STORE_EMB_DIM];
    FaceRecog_Status_t st = FaceRecog_Extract(rgb_frame, frame_w, frame_h,
                                              bbox, emb);
    if (matched_index) *matched_index = UINT32_MAX;
    if (matched_score) *matched_score = 0.0f;

    if (st != FACE_RECOG_OK) {
        printf("[FaceRecog] Enrol: extract failed (%d)\r\n", (int)st);
        return FACE_ENROLL_ERR_EXTRACT;
    }

    uint32_t dup_idx = 0U;
    float dup_score = 0.0f;
    if (FaceStore_Match(emb, &dup_idx, &dup_score, FACE_RECOG_MATCH_THRESHOLD)) {
        if (matched_index) *matched_index = dup_idx;
        if (matched_score) *matched_score = dup_score;
        printf("[FaceRecog] Enrol: duplicate face index=%lu score=%.3f\r\n",
               (unsigned long)dup_idx,
               dup_score);
        return FACE_ENROLL_ERR_DUPLICATE;
    }

    uint32_t saved_index = UINT32_MAX;
    if (!FaceStore_Add(name, emb, NULL, &saved_index)) {
        printf("[FaceRecog] Enrol: store full\r\n");
        return FACE_ENROLL_ERR_STORE;
    }
    if ((saved_index != UINT32_MAX) && (g_depth_preview_ready != 0U)) {
        (void)FaceStore_SetDepthTemplate(saved_index, g_depth_preview);
    }
    if (!FaceStore_Commit()) {
        return FACE_ENROLL_ERR_COMMIT;
    }
    return FACE_ENROLL_OK;
}
