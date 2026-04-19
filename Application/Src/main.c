/**
 ******************************************************************************
 * @file    main.c
 * @brief   STM32N6570-DK Face Recognition - merged stable startup version
 *
 * Architecture:
 *   Pipe1 (display)  : camera preview -> LTDC Layer 1
 *   Pipe2 (NN input) : camera snapshot -> BlazeFace -> Face Recognition
 *
 * LTDC layers:
 *   Layer 1 : RGB565 camera background
 *   Layer 2 : ARGB4444 UI overlay
 *
 * Notes:
 *   - Startup path is kept close to the ST reference example for stability.
 *   - Risky direct MEMSYSCTL cache register pokes have been removed.
 *   - Full security/slave attribute configuration has been restored.
 *   - Full sleep clock configuration has been restored.
 *   - Camera display pipe is started again.
 *   - NN input capture uses a uint8 staging buffer, then normalizes into nn_in.
 *
 ******************************************************************************
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>

#include "cmw_camera.h"
#include "stm32n6570_discovery_bus.h"
#include "stm32n6570_discovery_lcd.h"
#include "stm32n6570_discovery_xspi.h"
#include "stm32n6570_discovery.h"
#include "stm32_lcd.h"
#include "stm32_lcd_ex.h"

#include "app_fuseprogramming.h"
#include "app_postprocess.h"
#include "app_camerapipeline.h"
#include "app_config.h"
#include "main.h"
#include "crop_img.h"
#include "stlogo.h"

#include "stai.h"
#include "stai_network.h"

#include "app_ui.h"
#include "app_touch.h"
#include "app_buzzer.h"

/*
 * Keep these includes aligned with the modules you actually have in the project.
 * If your project uses face_store.h / face_recog.h instead of app_facerec.h,
 * keep those versions and adapt the marked blocks below.
 */
#include "face_store.h"
#include "face_recog.h"

CLASSES_TABLE;

/* -------------------------------------------------------------------------- */
/* Version strings                                                            */
/* -------------------------------------------------------------------------- */

#ifndef APP_GIT_SHA1_STRING
#define APP_GIT_SHA1_STRING "dev"
#endif

#ifndef APP_VERSION_STRING
#define APP_VERSION_STRING "unversioned"
#endif

/* -------------------------------------------------------------------------- */
/* Display configuration                                                      */
/* -------------------------------------------------------------------------- */

#define LCD_FG_WIDTH   SCREEN_WIDTH
#define LCD_FG_HEIGHT  SCREEN_HEIGHT
#define LCD_FG_FRAMEBUFFER_SIZE  (LCD_FG_WIDTH * LCD_FG_HEIGHT * 2U)

#define LCD_PREVIEW_WIDTH   256U
#define LCD_PREVIEW_HEIGHT  212U
#define LCD_PREVIEW_X0      (SCREEN_WIDTH - LCD_PREVIEW_WIDTH)
#define LCD_PREVIEW_Y0      0U
#define LCD_PREVIEW_FRAMEBUFFER_SIZE  (LCD_PREVIEW_WIDTH * LCD_PREVIEW_HEIGHT * 2U)

typedef struct
{
  uint32_t X0;
  uint32_t Y0;
  uint32_t XSize;
  uint32_t YSize;
} Rectangle_TypeDef;
volatile uint32_t g_fault_cfsr = 0U;
volatile uint32_t g_fault_hfsr = 0U;
volatile uint32_t g_fault_bfar = 0U;
volatile uint32_t g_fault_mmfar = 0U;
volatile uint32_t g_fault_break_seen = 0U;
volatile uint32_t g_nn_step = 0U;
volatile int32_t g_nn_ret = 0;
volatile uintptr_t g_nn_context_addr = 0U;
volatile uintptr_t g_nn_input_addr = 0U;
volatile uintptr_t g_nn_output0_addr = 0U;
volatile uint32_t g_nn_input_count = 0U;
volatile uint32_t g_nn_output_count = 0U;
/* Camera preview area */
Rectangle_TypeDef lcd_bg_area = {
  .X0 = 0U,
  .Y0 = 0U,
  .XSize = 0U,
  .YSize = 0U,
};

/* Full-screen UI overlay area */
Rectangle_TypeDef lcd_fg_area = {
  .X0 = 0U,
  .Y0 = 0U,
  .XSize = LCD_FG_WIDTH,
  .YSize = LCD_FG_HEIGHT,
};

#define NUMBER_COLORS 10
const uint32_t colors[NUMBER_COLORS] = {
    UTIL_LCD_COLOR_GREEN,
    UTIL_LCD_COLOR_RED,
    UTIL_LCD_COLOR_CYAN,
    UTIL_LCD_COLOR_MAGENTA,
    UTIL_LCD_COLOR_YELLOW,
    UTIL_LCD_COLOR_GRAY,
    UTIL_LCD_COLOR_BLACK,
    UTIL_LCD_COLOR_BROWN,
    UTIL_LCD_COLOR_BLUE,
    UTIL_LCD_COLOR_ORANGE
};

/* -------------------------------------------------------------------------- */
/* Post-processing                                                            */
/* -------------------------------------------------------------------------- */

#if POSTPROCESS_TYPE == POSTPROCESS_OD_BLAZEFACE_UI
  od_blazeface_pp_static_param_t pp_params;
#elif POSTPROCESS_TYPE == POSTPROCESS_OD_BLAZEFACE_UF
  od_blazeface_pp_static_param_t pp_params;
#else
  #error "Only BlazeFace postprocessing is supported in this project"
#endif

/* -------------------------------------------------------------------------- */
/* Globals                                                                    */
/* -------------------------------------------------------------------------- */

UART_HandleTypeDef huart1;
volatile int32_t cameraFrameReceived = 0;

stai_ptr nn_in;                          /* Network input buffer from STAI */
BSP_LCD_LayerConfig_t LayerConfig = {0};
od_pp_out_t pp_output;

/* Shared recognition result for UI */
int32_t  last_recog_idx   = -1;
float    last_recog_score = 0.0f;
float    last_det_conf    = 0.0f;
uint32_t g_cpu_frame_ms   = 0U;
uint32_t g_npu_infer_ms   = 0U;
extern uint32_t g_embed_npu_ms;

/* Enrollment request flags from UI */
extern volatile uint8_t g_enroll_requested;
extern volatile uint8_t g_enroll_done_flag;
extern volatile uint8_t g_enroll_fail_flag;
extern volatile uint8_t g_enroll_duplicate_flag;
extern char g_enroll_name[FACE_STORE_NAME_LEN];
static uint32_t enroll_toast_ts = 0U;

/* -------------------------------------------------------------------------- */
/* NN input staging buffer                                                    */
/* -------------------------------------------------------------------------- */

/*
 * Capture pipe writes uint8 RGB pixels into a staging buffer.
 * Then CPU normalizes into nn_in if the model input is float32.
 *
 * Important:
 * - This assumes your BlazeFace model expects float32 normalized input.
 * - If your deployed model is uint8 / int8, REMOVE the normalization block
 *   and feed the buffer format expected by the generated model.
 */

#define ALIGN_TO_16(value) (((value) + 15U) & ~15U)

#define NN_U8_SIZE \
  (STAI_NETWORK_IN_1_WIDTH * STAI_NETWORK_IN_1_HEIGHT * STAI_NETWORK_IN_1_CHANNEL)

#define NN_U8_SIZE_PADDED  ((NN_U8_SIZE + 31U) & ~31U)

/*
 * DCMIPP may pad each line to a multiple-of-16 pixel rule depending on config.
 * Reserve a staging region large enough for the raw NN snapshot path.
 */
#define DCMIPP_OUT_NN_LEN \
  (ALIGN_TO_16(STAI_NETWORK_IN_1_WIDTH * STAI_NETWORK_IN_1_CHANNEL) * STAI_NETWORK_IN_1_HEIGHT)

#define DCMIPP_OUT_NN_BUFF_LEN ((DCMIPP_OUT_NN_LEN + 31U) & ~31U)

__attribute__((aligned(32)))
static uint8_t nn_in_u8[DCMIPP_OUT_NN_BUFF_LEN];

/*
 * Optional separate crop buffer. This avoids in-place crop corruption risk
 * if img_crop() is not guaranteed to be safe with src == dst.
 */
__attribute__((aligned(32)))
static uint8_t nn_crop_u8[NN_U8_SIZE_PADDED];

/* -------------------------------------------------------------------------- */
/* Network context                                                            */
/* -------------------------------------------------------------------------- */

STAI_NETWORK_CONTEXT_DECLARE(network_context, STAI_NETWORK_CONTEXT_SIZE)

/* -------------------------------------------------------------------------- */
/* LCD framebuffers                                                           */
/* -------------------------------------------------------------------------- */

__attribute__((section(".psram_bss")))
__attribute__((aligned(32)))
static uint8_t lcd_bg_buffer[800U * 480U * 2U];

__attribute__((section(".psram_bss")))
__attribute__((aligned(32)))
static uint8_t lcd_preview_buffer[2][LCD_PREVIEW_FRAMEBUFFER_SIZE];

__attribute__((section(".psram_bss")))
__attribute__((aligned(32)))
static uint8_t lcd_fg_buffer[2][LCD_FG_WIDTH * LCD_FG_HEIGHT * 2U];

static int lcd_fg_buffer_rd_idx = 0;
static int lcd_preview_buffer_rd_idx = 0;

typedef enum
{
  DISPLAY_LAYOUT_FULLSCREEN = 0,
  DISPLAY_LAYOUT_MAIN_PREVIEW = 1
} DisplayLayout_t;

static DisplayLayout_t display_layout = DISPLAY_LAYOUT_FULLSCREEN;

/* -------------------------------------------------------------------------- */
/* Forward declarations                                                       */
/* -------------------------------------------------------------------------- */

static void SystemClock_Config(void);
static void CONSOLE_Config(void);
static void NPURam_enable(void);
static void NPUCache_config(void);
static void Display_NetworkOutput(od_pp_out_t *p_postprocess, uint32_t inference_ms);
static void Display_SetLayout(DisplayLayout_t layout);
static void Display_UpdateMainPreview(void);
static void LCD_init(void);
static void Security_Config(void);
static void set_clk_sleep_mode(void);
static void IAC_Config(void);
static void Hardware_init(void);
static void Run_Inference(stai_network *network_instance);
static void NeuralNetwork_init(uint32_t *nn_in_length,
                               stai_ptr *nn_out,
                               stai_size *number_output,
                               int32_t nn_out_len[],
                               stai_network_info *nn_info);

/* -------------------------------------------------------------------------- */
/* Main                                                                       */
/* -------------------------------------------------------------------------- */

int main(void)
{
  Hardware_init();

  printf("H1: hardware init done\r\n");
  printf("========================================\n");
  printf("STM32N6 Face Recognition %s (%s)\n", APP_VERSION_STRING, APP_GIT_SHA1_STRING);
  printf("Build date & time: %s %s\n", __DATE__, __TIME__);
#if defined(__GNUC__)
  printf("Compiler: GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(__ICCARM__)
  printf("Compiler: IAR EWARM %d.%d.%d\n", __VER__ / 1000000, (__VER__ / 1000) % 1000, __VER__ % 1000);
#else
  printf("Compiler: Unknown\n");
#endif
  printf("HAL: %lu.%lu.%lu\n",
         __STM32N6xx_HAL_VERSION_MAIN,
         __STM32N6xx_HAL_VERSION_SUB1,
         __STM32N6xx_HAL_VERSION_SUB2);
  printf("STEdgeAI Tools: %d.%d.%d\n",
         STAI_TOOLS_VERSION_MAJOR,
         STAI_TOOLS_VERSION_MINOR,
         STAI_TOOLS_VERSION_MICRO);
  printf("NN model: %s\n", STAI_NETWORK_ORIGIN_MODEL_NAME);
  printf("========================================\n");

  /* ---------------- NN init ---------------- */
  uint32_t pitch_nn = 0U;
  uint32_t nn_in_len = 0U;
  stai_size number_output = 0U;
  stai_ptr nn_out[STAI_NETWORK_OUT_NUM] = {0};
  int32_t nn_out_len[STAI_NETWORK_OUT_NUM] = {0};
  stai_network_info info;
  int ret = 0;

  printf("H2: starting NeuralNetwork_init\r\n");
  NeuralNetwork_init(&nn_in_len, nn_out, &number_output, nn_out_len, &info);
  printf("H3: NeuralNetwork_init done, nn_in=%p len=%lu outputs=%lu out0=%p\r\n",
         nn_in,
         nn_in_len,
         (uint32_t)number_output,
         (number_output > 0U) ? nn_out[0] : NULL);

  /* ---------------- Postprocess init ---------------- */
  printf("H4: app_postprocess_init inputs=%lu outputs=%lu\r\n",
         (uint32_t)info.n_inputs,
         (uint32_t)info.n_outputs);
  app_postprocess_init(&pp_params, &info);
  printf("H5: postprocess init done\r\n");

  /* ---------------- Face recognition init ---------------- */
  FaceStore_Init();
  FaceRecog_Init();

  /* ---------------- Camera init ---------------- */
  CameraPipeline_Init(&lcd_bg_area.XSize, &lcd_bg_area.YSize, &pitch_nn);

  LCD_init();
  UI_Init();
  Touch_Init();
  Buzzer_Init();

  /* Restore stable camera preview startup */
  CameraPipeline_DisplayPipe_Start(lcd_bg_buffer, CMW_MODE_CONTINUOUS);

  printf("Camera preview started. BG area: %lux%lu, NN pitch=%lu\n",
         lcd_bg_area.XSize, lcd_bg_area.YSize, pitch_nn);

  /* ---------------- Main loop ---------------- */
  while (1)
  {
    CameraPipeline_IspUpdate();

    /* Snapshot into uint8 staging buffer */
    CameraPipeline_NNPipe_Start(nn_in_u8, CMW_MODE_SNAPSHOT);

    while (cameraFrameReceived == 0)
    {
      /* wait */
    }
    cameraFrameReceived = 0;

    uint32_t ts[2] = {0U, 0U};
    uint32_t frame_cpu_start = HAL_GetTick();

    /* Ensure CPU sees the fresh DCMIPP-written buffer */
    SCB_InvalidateDCache_by_Addr((void *)nn_in_u8, DCMIPP_OUT_NN_BUFF_LEN);

    /*
     * If DCMIPP row pitch differs from exact NN row width, crop/pack into a
     * contiguous buffer before normalization.
     */
    uint8_t *nn_src_u8 = nn_in_u8;

    if (pitch_nn != (STAI_NETWORK_IN_1_WIDTH * STAI_NETWORK_IN_1_CHANNEL))
    {
      memset(nn_crop_u8, 0, sizeof(nn_crop_u8));
      img_crop(nn_in_u8,
               nn_crop_u8,
               pitch_nn,
               STAI_NETWORK_IN_1_WIDTH,
               STAI_NETWORK_IN_1_HEIGHT,
               STAI_NETWORK_IN_1_CHANNEL);

      SCB_CleanInvalidateDCache_by_Addr((void *)nn_crop_u8, NN_U8_SIZE_PADDED);
      nn_src_u8 = nn_crop_u8;
    }

    /*
     * Normalize uint8 [0..255] -> float32 [-1, 1]
     * Keep this only if your generated model input really is float32.
     */
    {
      float *dst = (float *)nn_in;
      const uint32_t n = NN_U8_SIZE;

      memset((void *)nn_in, 0, nn_in_len);

      for (uint32_t i = 0U; i < n; i++)
      {
        dst[i] = ((float)nn_src_u8[i] * (1.0f / 127.5f)) - 1.0f;
      }
    }

    SCB_CleanInvalidateDCache_by_Addr((void *)nn_in, nn_in_len);

    ts[0] = HAL_GetTick();
    Run_Inference(network_context);
    ts[1] = HAL_GetTick();
    g_npu_infer_ms = ts[1] - ts[0];

    ret = app_postprocess_run((void **)nn_out, number_output, &pp_output, &pp_params);
    assert(ret == 0);

    /* ---------------- Recognition stage ---------------- */
    last_recog_idx = -1;
    last_recog_score = 0.0f;
    last_det_conf = 0.0f;
    g_embed_npu_ms = 0U;

    int32_t best_idx = -1;

    if (pp_output.nb_detect >= 1U)
    {
      uint32_t best = 0U;
      for (uint32_t i = 1U; i < pp_output.nb_detect; i++)
      {
        if (pp_output.pOutBuff[i].conf > pp_output.pOutBuff[best].conf)
        {
          best = i;
        }
      }

      best_idx = (int32_t)best;
      last_det_conf = pp_output.pOutBuff[best].conf;

      if ((last_det_conf >= FACE_RECOG_MIN_DET_CONF) && FaceRecog_IsReady())
      {
        (void)FaceRecog_Identify((const uint8_t *)nn_src_u8,
                                 STAI_NETWORK_IN_1_WIDTH,
                                 STAI_NETWORK_IN_1_HEIGHT,
                                 &pp_output.pOutBuff[best],
                                 &last_recog_idx,
                                 &last_recog_score);
      }
    }

    /* ---------------- Enrollment stage ---------------- */
    if (g_enroll_requested)
    {
      g_enroll_requested = 0U;
      g_enroll_done_flag = 0U;
      g_enroll_fail_flag = 0U;
      g_enroll_duplicate_flag = 0U;

      if ((best_idx >= 0) &&
          (last_det_conf >= FACE_RECOG_MIN_DET_CONF) &&
          FaceRecog_IsReady())
      {
        char default_name[FACE_STORE_NAME_LEN];
        snprintf(default_name,
                 sizeof(default_name),
                 "Person %lu",
                 (unsigned long)(FaceStore_Count() + 1U));
        if (g_enroll_name[0] != '\0')
        {
          strncpy(default_name, g_enroll_name, sizeof(default_name) - 1U);
          default_name[sizeof(default_name) - 1U] = '\0';
          g_enroll_name[0] = '\0';
        }

        uint32_t matched_index = UINT32_MAX;
        float matched_score = 0.0f;
        FaceEnroll_Status_t enroll_status =
            FaceRecog_EnrollFromFrameEx(default_name,
                                        (const uint8_t *)nn_src_u8,
                                        STAI_NETWORK_IN_1_WIDTH,
                                        STAI_NETWORK_IN_1_HEIGHT,
                                        &pp_output.pOutBuff[best_idx],
                                        &matched_index,
                                        &matched_score);

        if (enroll_status == FACE_ENROLL_OK)
        {
          g_enroll_done_flag = 1U;
          enroll_toast_ts = HAL_GetTick();
          printf("[Enroll] saved '%s' (total=%lu)\r\n",
                 default_name,
                 (unsigned long)FaceStore_Count());
        }
        else if (enroll_status == FACE_ENROLL_ERR_DUPLICATE)
        {
          g_enroll_duplicate_flag = 1U;
          enroll_toast_ts = HAL_GetTick();
          printf("[Enroll] duplicate face, matched index=%lu score=%.3f\r\n",
                 (unsigned long)matched_index,
                 matched_score);
        }
        else
        {
          g_enroll_fail_flag = 1U;
          enroll_toast_ts = HAL_GetTick();
          printf("[Enroll] failed during save\r\n");
        }
      }
      else
      {
        g_enroll_name[0] = '\0';
        g_enroll_fail_flag = 1U;
        enroll_toast_ts = HAL_GetTick();
        printf("[Enroll] failed: no suitable face or recognizer not ready\r\n");
      }
    }

    if ((g_enroll_done_flag || g_enroll_fail_flag || g_enroll_duplicate_flag) &&
        ((HAL_GetTick() - enroll_toast_ts) > 2000U))
    {
      g_enroll_done_flag = 0U;
      g_enroll_fail_flag = 0U;
      g_enroll_duplicate_flag = 0U;
    }

    {
      uint32_t frame_elapsed = HAL_GetTick() - frame_cpu_start;
      uint32_t npu_total_ms = g_npu_infer_ms + g_embed_npu_ms;
      g_cpu_frame_ms = (frame_elapsed > npu_total_ms) ?
                       (frame_elapsed - npu_total_ms) : frame_elapsed;
    }

    Display_NetworkOutput(&pp_output, g_npu_infer_ms);

    /*
     * Invalidate network outputs after use, so next inference/postprocess
     * sees fresh values.
     */
    for (int i = 0; i < (int)number_output; i++)
    {
      SCB_InvalidateDCache_by_Addr((void *)nn_out[i], nn_out_len[i]);
    }
  }
}

/* -------------------------------------------------------------------------- */
/* Hardware init                                                              */
/* -------------------------------------------------------------------------- */

static void Hardware_init(void)
{
  /* Return temporarily to safe HSI sources before re-clocking */
  __HAL_RCC_CPUCLK_CONFIG(RCC_CPUCLKSOURCE_HSI);
  __HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_HSI);

  HAL_Init();

  SCB_EnableICache();

#if defined(USE_DCACHE)
  SCB_EnableDCache();
#endif

  SystemClock_Config();
  CONSOLE_Config();

  NPURam_enable();
  Fuse_Programming();
  NPUCache_config();

  /* External PSRAM */
  BSP_XSPI_RAM_Init(0);
  BSP_XSPI_RAM_EnableMemoryMappedMode(0);

  /* External NOR */
  BSP_XSPI_NOR_Init_t NOR_Init;
  NOR_Init.InterfaceMode = BSP_XSPI_NOR_OPI_MODE;
  NOR_Init.TransferRate  = BSP_XSPI_NOR_DTR_TRANSFER;

  BSP_XSPI_NOR_Init(0, &NOR_Init);
  BSP_XSPI_NOR_EnableMemoryMappedMode(0);

  Security_Config();
  IAC_Config();
  set_clk_sleep_mode();
}

/* -------------------------------------------------------------------------- */
/* Inference                                                                  */
/* -------------------------------------------------------------------------- */

static void Run_Inference(stai_network *network_instance)
{
  stai_return_code ret;

  do
  {
    ret = stai_network_run(network_instance, STAI_MODE_ASYNC);
    if (ret == STAI_RUNNING_WFE)
    {
      LL_ATON_OSAL_WFE();
    }
  } while ((ret == STAI_RUNNING_WFE) || (ret == STAI_RUNNING_NO_WFE));

  ret = stai_ext_network_new_inference(network_instance);
  assert(ret == STAI_SUCCESS);
}

static void NeuralNetwork_init(uint32_t *nn_in_length,
                               stai_ptr *nn_out,
                               stai_size *number_output,
                               int32_t nn_out_len[],
                               stai_network_info *nn_info)
{
  stai_network_info info;
  stai_size n_inputs = STAI_NETWORK_IN_NUM;
  stai_size n_outputs = STAI_NETWORK_OUT_NUM;
  int ret;

  g_nn_step = 10U;
  g_nn_context_addr = (uintptr_t)network_context;
  printf("NN10: stai_runtime_init\r\n");
  ret = stai_runtime_init();
  g_nn_ret = ret;
  printf("NN11: stai_runtime_init ret=%d\r\n", ret);
  if (ret != STAI_SUCCESS)
  {
    while (1)
    {
    }
  }

  g_nn_step = 20U;
  printf("NN20: stai_network_init ctx=%p\r\n", network_context);
  ret = stai_network_init(network_context);
  g_nn_ret = ret;
  printf("NN21: stai_network_init ret=%d\r\n", ret);
  if (ret != STAI_SUCCESS)
  {
    while (1)
    {
    }
  }

  g_nn_step = 30U;
  printf("NN30: stai_network_get_info\r\n");
  ret = stai_network_get_info(network_context, &info);
  g_nn_ret = ret;
  printf("NN31: stai_network_get_info ret=%d inputs=%lu outputs=%lu info.inputs=%p info.outputs=%p\r\n",
         ret,
         (uint32_t)info.n_inputs,
         (uint32_t)info.n_outputs,
         info.inputs,
         info.outputs);
  if ((ret != STAI_SUCCESS) || (info.n_inputs == 0U) || (info.inputs == NULL) ||
      (info.n_outputs == 0U) || (info.outputs == NULL) || (nn_info == NULL))
  {
    while (1)
    {
    }
  }

  g_nn_step = 40U;
  *number_output = n_outputs;
  *nn_in_length = info.inputs[0].size_bytes;
  printf("NN40: input bytes=%lu, expected inputs=%lu outputs=%lu\r\n",
         *nn_in_length,
         (uint32_t)n_inputs,
         (uint32_t)n_outputs);

  g_nn_step = 50U;
  printf("NN50: stai_network_get_inputs\r\n");
  ret = stai_network_get_inputs(network_context, &nn_in, &n_inputs);
  g_nn_ret = ret;
  g_nn_input_addr = (uintptr_t)nn_in;
  g_nn_input_count = (uint32_t)n_inputs;
  printf("NN51: stai_network_get_inputs ret=%d n_inputs=%lu nn_in=%p\r\n",
         ret,
         (uint32_t)n_inputs,
         nn_in);
  if ((ret != STAI_SUCCESS) || (n_inputs == 0U) || (nn_in == NULL))
  {
    while (1)
    {
    }
  }

  g_nn_step = 60U;
  printf("NN60: stai_network_get_outputs\r\n");
  ret = stai_network_get_outputs(network_context, nn_out, &n_outputs);
  g_nn_ret = ret;
  g_nn_output_count = (uint32_t)n_outputs;
  g_nn_output0_addr = (n_outputs > 0U) ? (uintptr_t)nn_out[0] : 0U;
  printf("NN61: stai_network_get_outputs ret=%d n_outputs=%lu out0=%p\r\n",
         ret,
         (uint32_t)n_outputs,
         (n_outputs > 0U) ? nn_out[0] : NULL);
  if ((ret != STAI_SUCCESS) || (n_outputs == 0U) || (nn_out[0] == NULL))
  {
    while (1)
    {
    }
  }

  *number_output = n_outputs;

  for (int i = 0; i < (int)(*number_output); i++)
  {
    nn_out_len[i] = info.outputs[i].size_bytes;
  }

  *nn_info = info;

  g_nn_step = 70U;
  printf("NN70: NeuralNetwork_init complete\r\n");
}

/* -------------------------------------------------------------------------- */
/* NPU RAM / Cache                                                            */
/* -------------------------------------------------------------------------- */

static void NPURam_enable(void)
{
  __HAL_RCC_NPU_CLK_ENABLE();
  __HAL_RCC_NPU_FORCE_RESET();
  __HAL_RCC_NPU_RELEASE_RESET();

  __HAL_RCC_AXISRAM3_MEM_CLK_ENABLE();
  __HAL_RCC_AXISRAM4_MEM_CLK_ENABLE();
  __HAL_RCC_AXISRAM5_MEM_CLK_ENABLE();
  __HAL_RCC_AXISRAM6_MEM_CLK_ENABLE();
  __HAL_RCC_RAMCFG_CLK_ENABLE();

  RAMCFG_HandleTypeDef hramcfg = {0};

  hramcfg.Instance = RAMCFG_SRAM3_AXI;
  HAL_RAMCFG_EnableAXISRAM(&hramcfg);

  hramcfg.Instance = RAMCFG_SRAM4_AXI;
  HAL_RAMCFG_EnableAXISRAM(&hramcfg);

  hramcfg.Instance = RAMCFG_SRAM5_AXI;
  HAL_RAMCFG_EnableAXISRAM(&hramcfg);

  hramcfg.Instance = RAMCFG_SRAM6_AXI;
  HAL_RAMCFG_EnableAXISRAM(&hramcfg);
}

static void NPUCache_config(void)
{
  npu_cache_enable();
}

/* -------------------------------------------------------------------------- */
/* Security / IAC                                                             */
/* -------------------------------------------------------------------------- */

static void Security_Config(void)
{
  __HAL_RCC_RIFSC_CLK_ENABLE();

  RIMC_MasterConfig_t RIMC_master = {0};
  RIMC_master.MasterCID = RIF_CID_1;
  RIMC_master.SecPriv   = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV;

  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_NPU,    &RIMC_master);
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_DMA2D,  &RIMC_master);
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_DCMIPP, &RIMC_master);
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_LTDC1,  &RIMC_master);
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_LTDC2,  &RIMC_master);

  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_NPU,    RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_DMA2D,  RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_CSI,    RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_DCMIPP, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDC,   RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDCL1, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDCL2, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
}

static void IAC_Config(void)
{
  __HAL_RCC_IAC_CLK_ENABLE();
  __HAL_RCC_IAC_FORCE_RESET();
  __HAL_RCC_IAC_RELEASE_RESET();
}

void IAC_IRQHandler(void)
{
  while (1)
  {
    /* Illegal access trap */
  }
}

/* -------------------------------------------------------------------------- */
/* Sleep mode clock configuration                                             */
/* -------------------------------------------------------------------------- */

static void set_clk_sleep_mode(void)
{
  __HAL_RCC_XSPI1_CLK_SLEEP_ENABLE();
  __HAL_RCC_XSPI2_CLK_SLEEP_ENABLE();
  __HAL_RCC_NPU_CLK_SLEEP_ENABLE();
  __HAL_RCC_CACHEAXI_CLK_SLEEP_ENABLE();
  __HAL_RCC_LTDC_CLK_SLEEP_ENABLE();
  __HAL_RCC_DMA2D_CLK_SLEEP_ENABLE();
  __HAL_RCC_DCMIPP_CLK_SLEEP_ENABLE();
  __HAL_RCC_CSI_CLK_SLEEP_ENABLE();

  __HAL_RCC_FLEXRAM_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM1_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM2_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM3_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM4_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM5_MEM_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM6_MEM_CLK_SLEEP_ENABLE();
}

/* -------------------------------------------------------------------------- */
/* Display/UI                                                                 */
/* -------------------------------------------------------------------------- */

static void Display_NetworkOutput(od_pp_out_t *p_postprocess, uint32_t inference_ms)
{
  int ret;

  ret = HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc,
                                     (uint32_t)lcd_fg_buffer[lcd_fg_buffer_rd_idx],
                                     LTDC_LAYER_2);
  assert(ret == HAL_OK);

  UTIL_LCD_FillRect(lcd_fg_area.X0,
                    lcd_fg_area.Y0,
                    lcd_fg_area.XSize,
                    lcd_fg_area.YSize,
                    0x00000000u);

  TouchButton_t touch = Touch_GetButton(app_state);

  AppState_t prev_state = app_state;
  UI_UpdateState(p_postprocess, (uint32_t)touch);

  if (app_state != prev_state)
  {
    if (app_state == APP_STATE_MAIN)
    {
      Display_SetLayout(DISPLAY_LAYOUT_MAIN_PREVIEW);
    }
    else if ((app_state == APP_STATE_AUTH) || (app_state == APP_STATE_AUTH_SUCCESS) ||
             (app_state == APP_STATE_SPLASH))
    {
      Display_SetLayout(DISPLAY_LAYOUT_FULLSCREEN);
    }

    if (app_state == APP_STATE_MAIN)
    {
      Buzzer_Play(BEEP_AUTH_OK);
    }
    else if (app_state == APP_STATE_BACKING_OFF)
    {
      Buzzer_Play(BEEP_BACKING_OFF);
    }
    else if ((app_state == APP_STATE_AUTH) && (prev_state == APP_STATE_MAIN))
    {
      Buzzer_Play(BEEP_LOGOUT);
    }
    else if (app_state == APP_STATE_SETTINGS)
    {
      Buzzer_Play(BEEP_SETTINGS_OPEN);
    }
  }

  Buzzer_Update();

  /*
   * Assumes your UI layer reads:
   *   - app_state
   *   - last_recog_idx / last_recog_score / last_det_conf
   * If your UI_Render signature differs, adapt here.
   */
  (void)inference_ms;
  if (app_state == APP_STATE_MAIN)
  {
    Display_UpdateMainPreview();
  }

  UI_Render(p_postprocess, (UI_BgArea_t *)&lcd_bg_area);

  SCB_CleanDCache_by_Addr((void *)lcd_fg_buffer[lcd_fg_buffer_rd_idx],
                          LCD_FG_FRAMEBUFFER_SIZE);

  ret = HAL_LTDC_ReloadLayer(&hlcd_ltdc,
                             LTDC_RELOAD_VERTICAL_BLANKING,
                             LTDC_LAYER_2);
  assert(ret == HAL_OK);

  lcd_fg_buffer_rd_idx = 1 - lcd_fg_buffer_rd_idx;
}

static void Display_SetLayout(DisplayLayout_t layout)
{
  int ret;
  BSP_LCD_LayerConfig_t layer_config = {0};

  if (display_layout == layout)
  {
    return;
  }

  if (layout == DISPLAY_LAYOUT_MAIN_PREVIEW)
  {
    lcd_bg_area.X0 = LCD_PREVIEW_X0;
    lcd_bg_area.Y0 = LCD_PREVIEW_Y0;
    lcd_bg_area.XSize = LCD_PREVIEW_WIDTH;
    lcd_bg_area.YSize = LCD_PREVIEW_HEIGHT;
    lcd_preview_buffer_rd_idx = 0;
    ret = HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc,
                                       (uint32_t)lcd_preview_buffer[lcd_preview_buffer_rd_idx],
                                       LTDC_LAYER_1);
    assert(ret == HAL_OK);
  }
  else
  {
    lcd_bg_area.X0 = 0U;
    lcd_bg_area.Y0 = 0U;
    lcd_bg_area.XSize = SCREEN_WIDTH;
    lcd_bg_area.YSize = SCREEN_HEIGHT;

    SCB_InvalidateDCache_by_Addr((void *)lcd_bg_buffer, sizeof(lcd_bg_buffer));

    layer_config.X0          = lcd_bg_area.X0;
    layer_config.Y0          = lcd_bg_area.Y0;
    layer_config.X1          = lcd_bg_area.X0 + lcd_bg_area.XSize;
    layer_config.Y1          = lcd_bg_area.Y0 + lcd_bg_area.YSize;
    layer_config.PixelFormat = LCD_PIXEL_FORMAT_RGB565;
    layer_config.Address     = (uint32_t)lcd_bg_buffer;
    ret = BSP_LCD_ConfigLayer(0, LTDC_LAYER_1, &layer_config);
    assert(ret == BSP_ERROR_NONE);

    display_layout = layout;
    return;
  }

  ret = HAL_LTDC_SetWindowSize_NoReload(&hlcd_ltdc,
                                        lcd_bg_area.XSize,
                                        lcd_bg_area.YSize,
                                        LTDC_LAYER_1);
  assert(ret == HAL_OK);

  ret = HAL_LTDC_SetWindowPosition_NoReload(&hlcd_ltdc,
                                            lcd_bg_area.X0,
                                            lcd_bg_area.Y0,
                                            LTDC_LAYER_1);
  assert(ret == HAL_OK);

  ret = HAL_LTDC_ReloadLayer(&hlcd_ltdc,
                             LTDC_RELOAD_VERTICAL_BLANKING,
                             LTDC_LAYER_1);
  assert(ret == HAL_OK);

  display_layout = layout;
}

static void Display_UpdateMainPreview(void)
{
  int ret;
  const uint32_t dst_w = LCD_PREVIEW_WIDTH;
  const uint32_t dst_h = LCD_PREVIEW_HEIGHT;
  const uint16_t *src = (const uint16_t *)lcd_bg_buffer;
  int wr_idx = 1 - lcd_preview_buffer_rd_idx;
  uint16_t *dst = (uint16_t *)lcd_preview_buffer[wr_idx];

  SCB_InvalidateDCache_by_Addr((void *)lcd_bg_buffer, sizeof(lcd_bg_buffer));

  for (uint32_t y = 0U; y < dst_h; y++)
  {
    uint32_t src_y = (y * SCREEN_HEIGHT) / dst_h;
    for (uint32_t x = 0U; x < dst_w; x++)
    {
      uint32_t src_x = (x * SCREEN_WIDTH) / dst_w;
      dst[(y * dst_w) + x] = src[(src_y * SCREEN_WIDTH) + src_x];
    }
  }

  SCB_CleanDCache_by_Addr((void *)lcd_preview_buffer[wr_idx],
                          sizeof(lcd_preview_buffer[wr_idx]));

  ret = HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc,
                                     (uint32_t)lcd_preview_buffer[wr_idx],
                                     LTDC_LAYER_1);
  assert(ret == HAL_OK);

  ret = HAL_LTDC_ReloadLayer(&hlcd_ltdc,
                             LTDC_RELOAD_VERTICAL_BLANKING,
                             LTDC_LAYER_1);
  assert(ret == HAL_OK);

  lcd_preview_buffer_rd_idx = wr_idx;
}

static void LCD_init(void)
{
  BSP_LCD_Init(0, LCD_ORIENTATION_LANDSCAPE);

  /* Layer 1: camera preview */
  LayerConfig.X0          = lcd_bg_area.X0;
  LayerConfig.Y0          = lcd_bg_area.Y0;
  LayerConfig.X1          = lcd_bg_area.X0 + lcd_bg_area.XSize;
  LayerConfig.Y1          = lcd_bg_area.Y0 + lcd_bg_area.YSize;
  LayerConfig.PixelFormat = LCD_PIXEL_FORMAT_RGB565;
  LayerConfig.Address     = (uint32_t)lcd_bg_buffer;
  BSP_LCD_ConfigLayer(0, LTDC_LAYER_1, &LayerConfig);

  /* Layer 2: transparent UI overlay */
  LayerConfig.X0          = lcd_fg_area.X0;
  LayerConfig.Y0          = lcd_fg_area.Y0;
  LayerConfig.X1          = lcd_fg_area.X0 + lcd_fg_area.XSize;
  LayerConfig.Y1          = lcd_fg_area.Y0 + lcd_fg_area.YSize;
  LayerConfig.PixelFormat = LCD_PIXEL_FORMAT_ARGB4444;
  LayerConfig.Address     = (uint32_t)lcd_fg_buffer[0];
  BSP_LCD_ConfigLayer(0, LTDC_LAYER_2, &LayerConfig);

  UTIL_LCD_SetFuncDriver(&LCD_Driver);
  UTIL_LCD_SetLayer(LTDC_LAYER_2);
  UTIL_LCD_Clear(0x00000000);
  UTIL_LCD_SetFont(&Font20);
  UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
}

/* -------------------------------------------------------------------------- */
/* DCMIPP clock                                                               */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp)
{
  (void)hdcmipp;

  RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct = {0};
  HAL_StatusTypeDef ret = HAL_OK;

  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_DCMIPP;
  RCC_PeriphCLKInitStruct.DcmippClockSelection = RCC_DCMIPPCLKSOURCE_IC17;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC17].ClockSelection = RCC_ICCLKSOURCE_PLL2;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC17].ClockDivider = 3;
  ret = HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
  if (ret != HAL_OK)
  {
    return ret;
  }

  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CSI;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC18].ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC18].ClockDivider = 40;
  ret = HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);

  return ret;
}

/* -------------------------------------------------------------------------- */
/* System clocks                                                              */
/* -------------------------------------------------------------------------- */

static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct = {0};

  BSP_SMPS_Init(SMPS_VOLTAGE_OVERDRIVE);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;

  /* PLL1 = 800 MHz */
  RCC_OscInitStruct.PLL1.PLLState      = RCC_PLL_ON;
  RCC_OscInitStruct.PLL1.PLLSource     = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL1.PLLM          = 2;
  RCC_OscInitStruct.PLL1.PLLN          = 25;
  RCC_OscInitStruct.PLL1.PLLFractional = 0;
  RCC_OscInitStruct.PLL1.PLLP1         = 1;
  RCC_OscInitStruct.PLL1.PLLP2         = 1;

  /* PLL2 = 1000 MHz */
  RCC_OscInitStruct.PLL2.PLLState      = RCC_PLL_ON;
  RCC_OscInitStruct.PLL2.PLLSource     = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL2.PLLM          = 8;
  RCC_OscInitStruct.PLL2.PLLFractional = 0;
  RCC_OscInitStruct.PLL2.PLLN          = 125;
  RCC_OscInitStruct.PLL2.PLLP1         = 1;
  RCC_OscInitStruct.PLL2.PLLP2         = 1;

  /* PLL3 = 900 MHz */
  RCC_OscInitStruct.PLL3.PLLState      = RCC_PLL_ON;
  RCC_OscInitStruct.PLL3.PLLSource     = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL3.PLLM          = 8;
  RCC_OscInitStruct.PLL3.PLLN          = 225;
  RCC_OscInitStruct.PLL3.PLLFractional = 0;
  RCC_OscInitStruct.PLL3.PLLP1         = 1;
  RCC_OscInitStruct.PLL3.PLLP2         = 2;

  /* PLL4 = 50 MHz */
  RCC_OscInitStruct.PLL4.PLLState      = RCC_PLL_ON;
  RCC_OscInitStruct.PLL4.PLLSource     = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL4.PLLM          = 8;
  RCC_OscInitStruct.PLL4.PLLFractional = 0;
  RCC_OscInitStruct.PLL4.PLLN          = 225;
  RCC_OscInitStruct.PLL4.PLLP1         = 6;
  RCC_OscInitStruct.PLL4.PLLP2         = 6;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    while (1) {}
  }

  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_CPUCLK |
                                 RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_HCLK |
                                 RCC_CLOCKTYPE_PCLK1 |
                                 RCC_CLOCKTYPE_PCLK2 |
                                 RCC_CLOCKTYPE_PCLK4 |
                                 RCC_CLOCKTYPE_PCLK5);

  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_IC1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;

  RCC_ClkInitStruct.IC1Selection.ClockSelection  = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC1Selection.ClockDivider    = 1;

  RCC_ClkInitStruct.IC2Selection.ClockSelection  = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC2Selection.ClockDivider    = 2;

  RCC_ClkInitStruct.IC6Selection.ClockSelection  = RCC_ICCLKSOURCE_PLL2;
  RCC_ClkInitStruct.IC6Selection.ClockDivider    = 1;

  RCC_ClkInitStruct.IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL3;
  RCC_ClkInitStruct.IC11Selection.ClockDivider   = 1;

  RCC_ClkInitStruct.AHBCLKDivider  = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;
  RCC_ClkInitStruct.APB5CLKDivider = RCC_APB5_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
  {
    while (1) {}
  }

  RCC_PeriphCLKInitStruct.PeriphClockSelection = 0U;

  RCC_PeriphCLKInitStruct.PeriphClockSelection |= RCC_PERIPHCLK_XSPI1;
  RCC_PeriphCLKInitStruct.Xspi1ClockSelection   = RCC_XSPI1CLKSOURCE_HCLK;

  RCC_PeriphCLKInitStruct.PeriphClockSelection |= RCC_PERIPHCLK_XSPI2;
  RCC_PeriphCLKInitStruct.Xspi2ClockSelection   = RCC_XSPI2CLKSOURCE_HCLK;

  if (HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct) != HAL_OK)
  {
    while (1) {}
  }
}

/* -------------------------------------------------------------------------- */
/* UART console                                                               */
/* -------------------------------------------------------------------------- */

static void CONSOLE_Config(void)
{
  GPIO_InitTypeDef gpio_init;

  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  gpio_init.Mode      = GPIO_MODE_AF_PP;
  gpio_init.Pull      = GPIO_PULLUP;
  gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
  gpio_init.Pin       = GPIO_PIN_5 | GPIO_PIN_6;
  gpio_init.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOE, &gpio_init);

  huart1.Instance          = USART1;
  huart1.Init.BaudRate     = 115200;
  huart1.Init.Mode         = UART_MODE_TX_RX;
  huart1.Init.Parity       = UART_PARITY_NONE;
  huart1.Init.WordLength   = UART_WORDLENGTH_8B;
  huart1.Init.StopBits     = UART_STOPBITS_1;
  huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_8;

  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    while (1) {}
  }
}

int _write(int file, char *ptr, int len)
{
  HAL_StatusTypeDef status;

  if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
  {
    errno = EBADF;
    return -1;
  }

  status = HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, ~0U);
  return (status == HAL_OK) ? len : 0;
}

/* -------------------------------------------------------------------------- */
/* Cache helper hooks                                                         */
/* -------------------------------------------------------------------------- */

void npu_cache_enable_clocks_and_reset(void)
{
  __HAL_RCC_CACHEAXIRAM_MEM_CLK_ENABLE();
  __HAL_RCC_CACHEAXI_CLK_ENABLE();
  __HAL_RCC_CACHEAXI_FORCE_RESET();
  __HAL_RCC_CACHEAXI_RELEASE_RESET();
}

void npu_cache_disable_clocks_and_reset(void)
{
  __HAL_RCC_CACHEAXIRAM_MEM_CLK_DISABLE();
  __HAL_RCC_CACHEAXI_CLK_DISABLE();
  __HAL_RCC_CACHEAXI_FORCE_RESET();
}

/* -------------------------------------------------------------------------- */
/* Assert                                                                     */
/* -------------------------------------------------------------------------- */

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  UNUSED(file);
  UNUSED(line);
  __BKPT(0);
  while (1)
  {
  }
}
#endif
