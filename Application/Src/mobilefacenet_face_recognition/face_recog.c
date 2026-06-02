
#include "app_config.h"
#include "face_recog.h"
#include "app_depth.h"
#include "app_processes.h"

#include <string.h>
#include <stdio.h>

#include "stm32n6xx_hal.h"

#if defined(HAVE_RECOG_NETWORK) && (HAVE_RECOG_NETWORK != 0)
  #include "stai.h"
  #include "stai_network_embed.h"
#endif


#if defined(HAVE_RECOG_NETWORK) && (HAVE_RECOG_NETWORK != 0)
STAI_NETWORK_CONTEXT_DECLARE(s_recog_ctx, STAI_NETWORK_EMBED_CONTEXT_SIZE)
static stai_ptr  s_recog_in;
static stai_ptr  s_recog_out[STAI_NETWORK_EMBED_OUT_NUM];
static uint32_t  s_recog_in_bytes;

static bool s_ready = false;
#else
static bool s_ready = false;
#endif

uint32_t g_embed_npu_ms = 0U;
uint32_t g_embed_npu_us = 0U;


#if defined(HAVE_RECOG_NETWORK) && (HAVE_RECOG_NETWORK != 0)

static void resize_face_bilinear(const uint8_t *src, uint32_t src_w, uint32_t src_h,
                                 uint32_t sx, uint32_t sy,
                                 uint32_t sw, uint32_t sh,
                                 int8_t *dst)
{
    static uint16_t x_idx[FACE_RECOG_IN_W];
    static uint16_t y_idx[FACE_RECOG_IN_H];
    static uint8_t x_frac[FACE_RECOG_IN_W];
    static uint8_t y_frac[FACE_RECOG_IN_H];

    const uint32_t max_x_base = (src_w > 1U) ? (src_w - 2U) : 0U;
    const uint32_t max_y_base = (src_h > 1U) ? (src_h - 2U) : 0U;

    for (uint32_t x = 0U; x < FACE_RECOG_IN_W; x++)
    {
        int32_t fx_q16 = (int32_t)(sx << 16) +
                         ((((int32_t)((2U * x) + 1U) * (int32_t)sw) << 15) /
                          (int32_t)FACE_RECOG_IN_W) -
                         (1 << 15);
        if (fx_q16 <= 0)
        {
            x_idx[x] = 0U;
            x_frac[x] = 0U;
        }
        else
        {
            uint32_t fx = (uint32_t)fx_q16;
            uint32_t ix = fx >> 16;
            if (ix >= (src_w - 1U))
            {
                ix = max_x_base;
                x_frac[x] = 255U;
            }
            else
            {
                x_frac[x] = (uint8_t)((fx >> 8) & 0xFFU);
            }
            x_idx[x] = (uint16_t)ix;
        }
    }

    for (uint32_t y = 0U; y < FACE_RECOG_IN_H; y++)
    {
        int32_t fy_q16 = (int32_t)(sy << 16) +
                         ((((int32_t)((2U * y) + 1U) * (int32_t)sh) << 15) /
                          (int32_t)FACE_RECOG_IN_H) -
                         (1 << 15);
        if (fy_q16 <= 0)
        {
            y_idx[y] = 0U;
            y_frac[y] = 0U;
        }
        else
        {
            uint32_t fy = (uint32_t)fy_q16;
            uint32_t iy = fy >> 16;
            if (iy >= (src_h - 1U))
            {
                iy = max_y_base;
                y_frac[y] = 255U;
            }
            else
            {
                y_frac[y] = (uint8_t)((fy >> 8) & 0xFFU);
            }
            y_idx[y] = (uint16_t)iy;
        }
    }

    for (uint32_t y = 0U; y < FACE_RECOG_IN_H; y++)
    {
        const uint8_t *row0 = src + (y_idx[y] * src_w * 3U);
        const uint8_t *row1 = row0 + (src_w * 3U);
        int8_t *dst_r = dst + (y * FACE_RECOG_IN_W);
        int8_t *dst_g = dst + (FACE_RECOG_IN_H * FACE_RECOG_IN_W) + (y * FACE_RECOG_IN_W);
        int8_t *dst_b = dst + (2U * FACE_RECOG_IN_H * FACE_RECOG_IN_W) + (y * FACE_RECOG_IN_W);
        uint32_t wy1 = y_frac[y];
        uint32_t wy0 = 256U - wy1;

        for (uint32_t x = 0U; x < FACE_RECOG_IN_W; x++)
        {
            const uint8_t *p00 = row0 + (x_idx[x] * 3U);
            const uint8_t *p10 = p00 + 3U;
            const uint8_t *p01 = row1 + (x_idx[x] * 3U);
            const uint8_t *p11 = p01 + 3U;
            uint32_t wx1 = x_frac[x];
            uint32_t wx0 = 256U - wx1;
            uint32_t w00 = wx0 * wy0;
            uint32_t w10 = wx1 * wy0;
            uint32_t w01 = wx0 * wy1;
            uint32_t w11 = wx1 * wy1;

            uint32_t r = ((uint32_t)p00[0] * w00) + ((uint32_t)p10[0] * w10) +
                         ((uint32_t)p01[0] * w01) + ((uint32_t)p11[0] * w11);
            uint32_t g = ((uint32_t)p00[1] * w00) + ((uint32_t)p10[1] * w10) +
                         ((uint32_t)p01[1] * w01) + ((uint32_t)p11[1] * w11);
            uint32_t b = ((uint32_t)p00[2] * w00) + ((uint32_t)p10[2] * w10) +
                         ((uint32_t)p01[2] * w01) + ((uint32_t)p11[2] * w11);

            dst_r[x] = (int8_t)((int32_t)((r + 32768U) >> 16) - 128);
            dst_g[x] = (int8_t)((int32_t)((g + 32768U) >> 16) - 128);
            dst_b[x] = (int8_t)((int32_t)((b + 32768U) >> 16) - 128);
        }
    }
}
#endif


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
    return FACE_RECOG_OK;
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

    int sx = (int)(bbox->x_center * frame_w - bbox->width  * frame_w * 0.5f);
    int sy = (int)(bbox->y_center * frame_h - bbox->height * frame_h * 0.5f);
    int sw = (int)(bbox->width    * frame_w);
    int sh = (int)(bbox->height   * frame_h);
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sx + sw > (int)frame_w) sw = frame_w - sx;
    if (sy + sh > (int)frame_h) sh = frame_h - sy;
    if (sw < 8 || sh < 8) return FACE_RECOG_ERR_BAD_BOX;


    resize_face_bilinear(rgb_frame, frame_w, frame_h,
                         (uint32_t)sx, (uint32_t)sy,
                         (uint32_t)sw, (uint32_t)sh,
                         (int8_t *)s_recog_in);


    SCB_CleanDCache_by_Addr((void *)s_recog_in, s_recog_in_bytes);


    uint32_t npu_start = AppProcesses_MetricsNowUs();
    stai_return_code ret;
    do {
        ret = stai_network_embed_run(s_recog_ctx, STAI_MODE_SYNC);
        if (ret == STAI_RUNNING_WFE)
        {
            uint32_t sleep_start;
            AppProcesses_MetricsSleepBegin(&sleep_start);
            LL_ATON_OSAL_WFE();
            AppProcesses_MetricsSleepEnd(sleep_start);
        }
    } while ((ret == STAI_RUNNING_WFE) || (ret == STAI_RUNNING_NO_WFE));
    g_embed_npu_us = AppProcesses_MetricsElapsedUs(npu_start);
    g_embed_npu_ms = (g_embed_npu_us + 500U) / 1000U;
    if (ret != STAI_SUCCESS) {
        printf("[FaceRecog] extract run failed ret=%d box=%.3f,%.3f,%.3f,%.3f input=%ld,%ld,%ld,%ld\r\n",
               (int)ret,
               bbox->x_center,
               bbox->y_center,
               bbox->width,
               bbox->height,
               (long)sx,
               (long)sy,
               (long)sw,
               (long)sh);
        return FACE_RECOG_ERR_NPU;
    }


    SCB_InvalidateDCache_by_Addr((void *)s_recog_out[0],
                                 STAI_NETWORK_EMBED_OUT_1_SIZE_BYTES);
    memcpy(out_emb, (void *)s_recog_out[0],
           FACE_STORE_EMB_DIM * sizeof(float));


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
