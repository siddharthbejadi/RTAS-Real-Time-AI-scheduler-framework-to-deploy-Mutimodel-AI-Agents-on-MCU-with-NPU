#ifndef APP_DEPTH_H
#define APP_DEPTH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "app_postprocess.h"

#define DEPTH_PREVIEW_W 224U
#define DEPTH_PREVIEW_H 224U

extern uint32_t g_depth_npu_ms;
extern float g_depth_live_score;
extern uint8_t g_depth_preview[DEPTH_PREVIEW_W * DEPTH_PREVIEW_H];
extern volatile uint8_t g_depth_preview_ready;

bool Depth_Init(void);
bool Depth_IsReady(void);
bool Depth_RunFrame(const uint8_t *rgb_frame,
                    uint32_t frame_w,
                    uint32_t frame_h,
                    const od_pp_outBuffer_t *bbox);

#ifdef __cplusplus
}
#endif

#endif /* APP_DEPTH_H */
