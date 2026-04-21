/**
 ******************************************************************************
 * @file    app_config.h
 * @author  GPM Application Team
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
#ifndef APP_CONFIG
#define APP_CONFIG
#include "arm_math.h"
#define USE_DCACHE

/*Defines: CMW_MIRRORFLIP_NONE; CMW_MIRRORFLIP_FLIP; CMW_MIRRORFLIP_MIRROR; CMW_MIRRORFLIP_FLIP_MIRROR;*/
#define CAMERA_FLIP CMW_MIRRORFLIP_MIRROR

#define ASPECT_RATIO_CROP       (1)
#define ASPECT_RATIO_FIT        (2)
#define ASPECT_RATIO_FULLSCREEN (3)
#define ASPECT_RATIO_MODE ASPECT_RATIO_CROP

/* Model Related Info */
#define POSTPROCESS_TYPE    POSTPROCESS_OD_BLAZEFACE_UI

#define COLOR_BGR (0)
#define COLOR_RGB (1)
#define COLOR_MODE    COLOR_RGB

/* Classes */
#define NB_CLASSES   (1)
#define CLASSES_TABLE const char* classes_table[NB_CLASSES] = {\
   "face"}\

/* Postprocessing BlazeFace configuration */
#define AI_FD_BLAZEFACE_PP_CONF_THRESHOLD    (0.55f)
#define AI_FD_BLAZEFACE_PP_IOU_THRESHOLD     (0.55f)
#define AI_FD_BLAZEFACE_PP_MAX_BOXES_LIMIT   (5)

/* Display */
#define WELCOME_MSG_1         "Face Recognition"
#define WELCOME_MSG_2         "BlazeFace + MobileFaceNet on STM32N6 NPU"

/* ─────────────────────────────────────────────────────────────────────────── */
/*  Two-model pipeline configuration                                          */
/* ─────────────────────────────────────────────────────────────────────────── */

/* Set HAVE_RECOG_NETWORK to 1 once you have generated the MobileFaceNet      */
/* stedgeai outputs and copied them into Model/STM32N6570-DK/                 */
/* (specifically: stai_network_embed.h, stai_network_embed.c,                 */
/*  network_embed.c, network_embed_ecblobs.h, network_embed_data.hex,         */
/*  network_embed_atonbuf.xSPI2.bin).                                         */
/* Until then, leave it at 0 — the project still builds and detection runs;  */
/* recognition simply returns FACE_RECOG_ERR_NOT_READY.                       */
#define HAVE_RECOG_NETWORK    1

/* Cosine-similarity threshold to accept a face match. 0.50–0.70 is typical  */
/* for MobileFaceNet; tune on a small validation set with your own faces.    */
#define FACE_RECOG_MATCH_THRESHOLD    0.70f

/* Detection confidence required before we even attempt recognition.         */
#define FACE_RECOG_MIN_DET_CONF       0.60f

/* Depth-template similarity required when an enrolled 224x224 depth map exists. */
#define FACE_DEPTH_MATCH_THRESHOLD    0.20f

#endif
