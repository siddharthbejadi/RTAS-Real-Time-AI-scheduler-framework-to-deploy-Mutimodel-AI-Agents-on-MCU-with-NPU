
#ifndef APP_CONFIG
#define APP_CONFIG
#include "arm_math.h"
#include "rtas_generated_config.h"
#define USE_DCACHE


#define CAMERA_FLIP CMW_MIRRORFLIP_MIRROR

#define ASPECT_RATIO_CROP       (1)
#define ASPECT_RATIO_FIT        (2)
#define ASPECT_RATIO_FULLSCREEN (3)
#define ASPECT_RATIO_MODE ASPECT_RATIO_CROP


#define POSTPROCESS_TYPE    POSTPROCESS_OD_BLAZEFACE_UF

#define COLOR_BGR (0)
#define COLOR_RGB (1)
#define COLOR_MODE    COLOR_RGB


#define NB_CLASSES   (1)
#define CLASSES_TABLE const char* classes_table[NB_CLASSES] = {\
   "face"}\


#define AI_FD_BLAZEFACE_PP_CONF_THRESHOLD    (0.55f)
#define AI_FD_BLAZEFACE_PP_IOU_THRESHOLD     (0.55f)
#define AI_FD_BLAZEFACE_PP_MAX_BOXES_LIMIT   (5)


#define WELCOME_MSG_1         "Face Recognition"
#define WELCOME_MSG_2         "BlazeFace + MobileFaceNet on STM32N6 NPU"


/* Keep this set to 1 only when the embedding network files are present in Model/STM32N6570-DK. */
#define HAVE_RECOG_NETWORK    1

/* Enable only while debugging model I/O correctness.
 * Keep disabled for benchmark/FOL/CPU measurements to avoid observer-effect loops.
 */
#ifndef PIPELINE_DEBUG_ENABLE
#define PIPELINE_DEBUG_ENABLE  0
#endif

/* Let DCMIPP DMA write the camera NN pipe directly into the STAI input buffer
 * when the camera pitch and generated model input size are an exact match.
 * Set to 0 to force the older staging-buffer path for bring-up/debug.
 */
#ifndef NN_INPUT_ZERO_COPY_ENABLE
#define NN_INPUT_ZERO_COPY_ENABLE  RTAS_NN_INPUT_ZERO_COPY_ENABLE
#endif


/* Face match thresholds are intentionally kept together because they usually need tuning as a set. */
#define FACE_RECOG_MATCH_THRESHOLD    0.70f


#define FACE_RECOG_MIN_DET_CONF       AI_FD_BLAZEFACE_PP_CONF_THRESHOLD


#define FACE_DEPTH_MATCH_THRESHOLD    0.20f

#endif
