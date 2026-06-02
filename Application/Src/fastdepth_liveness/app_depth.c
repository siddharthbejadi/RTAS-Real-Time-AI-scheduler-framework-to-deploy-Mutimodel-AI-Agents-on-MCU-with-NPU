#include "app_depth.h"

#include <string.h>
#include <stdio.h>

#include "stm32n6xx_hal.h"
#include "app_processes.h"
#include "stai.h"
#include "stai_depth.h"

STAI_NETWORK_CONTEXT_DECLARE(s_depth_ctx, STAI_DEPTH_CONTEXT_SIZE)

static stai_ptr s_depth_in = NULL;
static stai_ptr s_depth_out[STAI_DEPTH_OUT_NUM];
static bool s_depth_ready = false;

uint32_t g_depth_npu_ms = 0U;
uint32_t g_depth_npu_us = 0U;
float g_depth_live_score = 0.0f;
uint8_t g_depth_preview[DEPTH_PREVIEW_W * DEPTH_PREVIEW_H];
volatile uint8_t g_depth_preview_ready = 0U;

static void resize_rgb_to_depth_input(const uint8_t *src,
                                      uint32_t src_w,
                                      uint32_t src_h,
                                      uint8_t *dst)
{
    const uint32_t dst_w = STAI_DEPTH_IN_1_WIDTH;
    const uint32_t dst_h = STAI_DEPTH_IN_1_HEIGHT;
    uint32_t sy = 0U;
    uint32_t y_acc = 0U;

    for (uint32_t y = 0U; y < dst_h; y++)
    {
        const uint8_t *src_row = src + (sy * src_w * 3U);
        uint8_t *dp = dst + (y * dst_w * 3U);
        uint32_t sx = 0U;
        uint32_t x_acc = 0U;

        for (uint32_t x = 0U; x < dst_w; x++)
        {
            const uint8_t *sp = src_row + (sx * 3U);
            *dp++ = sp[0];
            *dp++ = sp[1];
            *dp++ = sp[2];

            x_acc += src_w;
            while ((x_acc >= dst_w) && (sx < (src_w - 1U)))
            {
                x_acc -= dst_w;
                sx++;
            }
        }

        y_acc += src_h;
        while ((y_acc >= dst_h) && (sy < (src_h - 1U)))
        {
            y_acc -= dst_h;
            sy++;
        }
    }
}

static void build_depth_preview(const uint8_t *depth)
{
#if (STAI_DEPTH_OUT_1_WIDTH == DEPTH_PREVIEW_W) && (STAI_DEPTH_OUT_1_HEIGHT == DEPTH_PREVIEW_H)
    memcpy(g_depth_preview, depth, sizeof(g_depth_preview));
#else
    uint32_t sy = 0U;
    uint32_t y_acc = 0U;

    for (uint32_t y = 0U; y < DEPTH_PREVIEW_H; y++)
    {
        const uint8_t *src_row = depth + (sy * STAI_DEPTH_OUT_1_WIDTH);
        uint8_t *dp = &g_depth_preview[y * DEPTH_PREVIEW_W];
        uint32_t sx = 0U;
        uint32_t x_acc = 0U;

        for (uint32_t x = 0U; x < DEPTH_PREVIEW_W; x++)
        {
            *dp++ = src_row[sx];

            x_acc += STAI_DEPTH_OUT_1_WIDTH;
            while ((x_acc >= DEPTH_PREVIEW_W) && (sx < (STAI_DEPTH_OUT_1_WIDTH - 1U)))
            {
                x_acc -= DEPTH_PREVIEW_W;
                sx++;
            }
        }

        y_acc += STAI_DEPTH_OUT_1_HEIGHT;
        while ((y_acc >= DEPTH_PREVIEW_H) && (sy < (STAI_DEPTH_OUT_1_HEIGHT - 1U)))
        {
            y_acc -= DEPTH_PREVIEW_H;
            sy++;
        }
    }
#endif
    g_depth_preview_ready = 1U;
}

static float compute_depth_score(const uint8_t *depth,
                                 const od_pp_outBuffer_t *bbox)
{
    uint32_t x0 = 0U;
    uint32_t y0 = 0U;
    uint32_t x1 = STAI_DEPTH_OUT_1_WIDTH;
    uint32_t y1 = STAI_DEPTH_OUT_1_HEIGHT;

    if (bbox != NULL)
    {
        float fx0 = bbox->x_center - (bbox->width * 0.5f);
        float fy0 = bbox->y_center - (bbox->height * 0.5f);
        float fx1 = bbox->x_center + (bbox->width * 0.5f);
        float fy1 = bbox->y_center + (bbox->height * 0.5f);

        if (fx0 < 0.0f) fx0 = 0.0f;
        if (fy0 < 0.0f) fy0 = 0.0f;
        if (fx1 > 1.0f) fx1 = 1.0f;
        if (fy1 > 1.0f) fy1 = 1.0f;

        x0 = (uint32_t)(fx0 * (float)STAI_DEPTH_OUT_1_WIDTH);
        y0 = (uint32_t)(fy0 * (float)STAI_DEPTH_OUT_1_HEIGHT);
        x1 = (uint32_t)(fx1 * (float)STAI_DEPTH_OUT_1_WIDTH);
        y1 = (uint32_t)(fy1 * (float)STAI_DEPTH_OUT_1_HEIGHT);
    }

    if ((x1 <= x0 + 4U) || (y1 <= y0 + 4U))
    {
        return 0.0f;
    }

    uint32_t min_v = 255U;
    uint32_t max_v = 0U;
    uint32_t sum = 0U;
    uint32_t count = 0U;

    for (uint32_t y = y0; y < y1; y += 2U)
    {
        const uint8_t *p = depth + (y * STAI_DEPTH_OUT_1_WIDTH) + x0;
        for (uint32_t x = x0; x < x1; x += 2U)
        {
            uint32_t v = *p;
            if (v < min_v) min_v = v;
            if (v > max_v) max_v = v;
            sum += v;
            count++;
            p += 2U;
        }
    }

    if (count == 0U)
    {
        return 0.0f;
    }

    uint32_t mean = sum / count;
    uint32_t mad = 0U;
    uint32_t mad_count = 0U;
    for (uint32_t y = y0; y < y1; y += 4U)
    {
        const uint8_t *p = depth + (y * STAI_DEPTH_OUT_1_WIDTH) + x0;
        for (uint32_t x = x0; x < x1; x += 4U)
        {
            uint32_t v = *p;
            mad += (v > mean) ? (v - mean) : (mean - v);
            mad_count++;
            p += 4U;
        }
    }

    uint32_t range = max_v - min_v;
    float range_score = (float)range / 80.0f;
    float texture_score = (mad_count > 0U) ? ((float)mad / (float)mad_count) / 32.0f : 0.0f;
    float score = (range_score * 0.65f) + (texture_score * 0.35f);

    if (score > 1.0f) score = 1.0f;
    if (score < 0.0f) score = 0.0f;
    return score;
}

bool Depth_Init(void)
{
    stai_return_code ret = stai_depth_init(s_depth_ctx);
    if (ret != STAI_SUCCESS)
    {
        printf("[Depth] stai_depth_init failed (%d)\r\n", (int)ret);
        s_depth_ready = false;
        return false;
    }

    stai_size n_in = STAI_DEPTH_IN_NUM;
    ret = stai_depth_get_inputs(s_depth_ctx, &s_depth_in, &n_in);
    if ((ret != STAI_SUCCESS) || (n_in == 0U) || (s_depth_in == NULL))
    {
        printf("[Depth] get_inputs failed (%d)\r\n", (int)ret);
        s_depth_ready = false;
        return false;
    }

    stai_size n_out = STAI_DEPTH_OUT_NUM;
    ret = stai_depth_get_outputs(s_depth_ctx, s_depth_out, &n_out);
    if ((ret != STAI_SUCCESS) || (n_out == 0U) || (s_depth_out[0] == NULL))
    {
        printf("[Depth] get_outputs failed (%d)\r\n", (int)ret);
        s_depth_ready = false;
        return false;
    }

    s_depth_ready = true;
    printf("[Depth] FastDepth ready (in=%lu bytes, out=%lu bytes)\r\n",
           (unsigned long)STAI_DEPTH_IN_1_SIZE_BYTES,
           (unsigned long)STAI_DEPTH_OUT_1_SIZE_BYTES);
    return true;
}

bool Depth_IsReady(void)
{
    return s_depth_ready;
}

bool Depth_RunFrame(const uint8_t *rgb_frame,
                    uint32_t frame_w,
                    uint32_t frame_h,
                    const od_pp_outBuffer_t *bbox)
{
    if (!s_depth_ready || (rgb_frame == NULL) || (frame_w == 0U) || (frame_h == 0U))
    {
        g_depth_npu_us = 0U;
        g_depth_npu_ms = 0U;
        g_depth_live_score = 0.0f;
        return false;
    }

    resize_rgb_to_depth_input(rgb_frame, frame_w, frame_h, (uint8_t *)s_depth_in);
    SCB_CleanDCache_by_Addr((void *)s_depth_in, STAI_DEPTH_IN_1_SIZE_BYTES);

    uint32_t npu_start = AppProcesses_MetricsNowUs();
    stai_return_code ret;
    do
    {
        ret = stai_depth_run(s_depth_ctx, STAI_MODE_SYNC);
        if (ret == STAI_RUNNING_WFE)
        {
            uint32_t sleep_start;
            AppProcesses_MetricsSleepBegin(&sleep_start);
            LL_ATON_OSAL_WFE();
            AppProcesses_MetricsSleepEnd(sleep_start);
        }
    } while ((ret == STAI_RUNNING_WFE) || (ret == STAI_RUNNING_NO_WFE));
    g_depth_npu_us = AppProcesses_MetricsElapsedUs(npu_start);
    g_depth_npu_ms = (g_depth_npu_us + 500U) / 1000U;

    if (ret != STAI_SUCCESS)
    {
        g_depth_live_score = 0.0f;
        printf("[Depth] run failed ret=%d\r\n", (int)ret);
        return false;
    }

    SCB_InvalidateDCache_by_Addr((void *)s_depth_out[0],
                                 STAI_DEPTH_OUT_1_SIZE_BYTES);

    const uint8_t *depth = (const uint8_t *)s_depth_out[0];
    build_depth_preview(depth);
    g_depth_live_score = compute_depth_score(depth, bbox);

    (void)stai_ext_depth_new_inference(s_depth_ctx);
    return true;
}
