
#ifndef FACE_RECOG_H
#define FACE_RECOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "app_postprocess.h"
#include "face_store.h"


#define FACE_RECOG_IN_W      112u
#define FACE_RECOG_IN_H      112u
#define FACE_RECOG_IN_C      3u


#ifndef FACE_RECOG_MATCH_THRESHOLD
#define FACE_RECOG_MATCH_THRESHOLD   0.60f
#endif

typedef enum {
    FACE_RECOG_OK            = 0,
    FACE_RECOG_ERR_NOT_READY = -1,
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


FaceRecog_Status_t FaceRecog_Init(void);


bool FaceRecog_IsReady(void);


FaceRecog_Status_t FaceRecog_Extract(const uint8_t *rgb_frame,
                                     uint32_t frame_w,
                                     uint32_t frame_h,
                                     const od_pp_outBuffer_t *bbox,
                                     float *out_emb);


FaceRecog_Status_t FaceRecog_Identify(const uint8_t *rgb_frame,
                                      uint32_t frame_w,
                                      uint32_t frame_h,
                                      const od_pp_outBuffer_t *bbox,
                                      int32_t *out_index,
                                      float *out_score);


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
#endif
