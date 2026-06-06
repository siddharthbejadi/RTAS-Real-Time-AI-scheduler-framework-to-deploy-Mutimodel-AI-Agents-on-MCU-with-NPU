	#include "app_processes.h"

	#include <assert.h>
	#include <stdbool.h>
	#include <stdio.h>
	#include <string.h>

	#include "cmw_camera.h"
	#include "app_camerapipeline.h"
	#include "app_config.h"
	#include "app_depth.h"
	#include "app_postprocess.h"
	#include "app_ui.h"
	#include "crop_img.h"
	#include "face_recog.h"
	#include "face_store.h"
	#include "main.h"
	#include "stai.h"
	#include "stai_network.h"
	#include "tim_app.h"

	#define ALIGN_TO_16(value) (((value) + 15U) & ~15U)

#define NN_U8_SIZE \
  (STAI_NETWORK_IN_1_WIDTH * STAI_NETWORK_IN_1_HEIGHT * STAI_NETWORK_IN_1_CHANNEL)

#define NN_INPUT_ROW_BYTES \
  (STAI_NETWORK_IN_1_WIDTH * STAI_NETWORK_IN_1_CHANNEL)

#define NN_U8_SIZE_PADDED  ((NN_U8_SIZE + 31U) & ~31U)

#define NN_INPUT_BUFFER_COUNT RTAS_NN_INPUT_BUFFER_COUNT

#define DCMIPP_OUT_NN_LEN \
  (ALIGN_TO_16(NN_INPUT_ROW_BYTES) * STAI_NETWORK_IN_1_HEIGHT)

	#define DCMIPP_OUT_NN_BUFF_LEN ((DCMIPP_OUT_NN_LEN + 31U) & ~31U)

	typedef enum
	{
	  MODEL_SCHED_RUN_DETECT = 1U << 0,
	  MODEL_SCHED_RUN_RECOG  = 1U << 1,
	  MODEL_SCHED_RUN_DEPTH  = 1U << 2
	} ModelSchedFlags_t;

	typedef struct
	{
	  uint32_t pitch_nn;
	  uint32_t nn_in_len;
	  stai_size number_output;
	  stai_ptr nn_out[STAI_NETWORK_OUT_NUM];
	  int32_t nn_out_len[STAI_NETWORK_OUT_NUM];
	  stai_network_info nn_info;
	  uint8_t *nn_src_u8;
	  int32_t best_idx;
	  uint32_t frame_cpu_start;
	  uint32_t frame_epoch_start_us;
	  uint32_t frame_sleep_us;
	} AppProcessContext_t;

	#if PIPELINE_DEBUG_ENABLE
	typedef struct
	{
	  uint32_t frame_id;
	  uint32_t input_min;
	  uint32_t input_max;
	  uint32_t input_mean;
	  uint32_t input_checksum;
	  uint32_t input_len;
	  uint32_t input_expected_len;
	  uint32_t pitch_nn;
	  uint32_t npu_ms;
	  int32_t run_ret;
	  int32_t reset_ret;
	  int32_t postprocess_ret;
	  uint32_t output_count;
	  float output_min[STAI_NETWORK_OUT_NUM];
	  float output_max[STAI_NETWORK_OUT_NUM];
	  float output_mean[STAI_NETWORK_OUT_NUM];
	  uint32_t output_nan_count[STAI_NETWORK_OUT_NUM];
	  uint32_t detections;
	  uint32_t drawable_detections;
	  uint32_t invalid_detections;
	  float best_conf;
	  float best_x;
	  float best_y;
	  float best_w;
	  float best_h;
	  uint32_t schedule_flags;
	  uint32_t recog_attempted;
	  uint32_t depth_attempted;
	} AppPipelineDebug_t;
	#endif

	#if POSTPROCESS_TYPE == POSTPROCESS_OD_BLAZEFACE_UI
	  od_blazeface_pp_static_param_t pp_params;
	#elif POSTPROCESS_TYPE == POSTPROCESS_OD_BLAZEFACE_UF
	  od_blazeface_pp_static_param_t pp_params;
	#else
	  #error "Only BlazeFace postprocessing is supported in this project"
	#endif

	STAI_NETWORK_CONTEXT_DECLARE(network_context, STAI_NETWORK_CONTEXT_SIZE)

	stai_ptr nn_in;
	od_pp_out_t pp_output;

	int32_t  last_recog_idx   = -1;
	float    last_recog_score = 0.0f;
	float    last_det_conf    = 0.0f;
	float    last_depth_match_score = 0.0f;
	uint8_t  last_depth_template_seen = 0U;
	uint32_t g_frame_output_ms = 0U;
	uint32_t g_cpu_frame_ms   = 0U;
	uint32_t g_npu_infer_ms   = 0U;
	uint32_t g_npu_infer_us   = 0U;
	uint32_t g_hw_cpu_active_us = 0U;
	uint32_t g_hw_cpu_sleep_us  = 0U;
	uint32_t g_hw_cpu_pct     = 0U;
	uint32_t g_hw_npu_run_pct = 0U;
	#if PIPELINE_DEBUG_ENABLE
	volatile AppPipelineDebug_t g_pipe_dbg;
	#endif

	extern volatile int32_t cameraFrameReceived;
	extern uint32_t g_embed_npu_ms;
	extern uint32_t g_embed_npu_us;
	extern uint32_t g_depth_npu_ms;
	extern uint32_t g_depth_npu_us;

	extern volatile uint8_t g_enroll_requested;
	extern volatile uint8_t g_enroll_done_flag;
	extern volatile uint8_t g_enroll_fail_flag;
	extern volatile uint8_t g_enroll_duplicate_flag;
	extern char g_enroll_name[FACE_STORE_NAME_LEN];
	extern char g_enroll_fail_reason[64];

	extern volatile uint32_t g_nn_step;
	extern volatile int32_t g_nn_ret;
	extern volatile uintptr_t g_nn_context_addr;
	extern volatile uintptr_t g_nn_input_addr;
	extern volatile uintptr_t g_nn_output0_addr;
	extern volatile uint32_t g_nn_input_count;
	extern volatile uint32_t g_nn_output_count;

	extern void AppProcesses_RenderOutput(od_pp_out_t *p_postprocess, uint32_t inference_ms);

	static AppProcessContext_t s_proc;

	__attribute__((aligned(32)))
	static uint8_t nn_in_u8[DCMIPP_OUT_NN_BUFF_LEN];

	/* Separate crop buffer keeps padded camera rows from overwriting pixels still needed later. */
	__attribute__((aligned(32)))
	static uint8_t nn_crop_u8[NN_U8_SIZE_PADDED];

	/* External-input model buffers: ping-pong DMA targets for --no-inputs-allocation. */
	__attribute__((aligned(STAI_CACHE_USER_BUFFER_ALIGNMENT)))
	static uint8_t nn_user_input_u8[NN_INPUT_BUFFER_COUNT][NN_U8_SIZE_PADDED];

#define MODEL_SCHED_STABLE_FACE_FRAMES     RTAS_MODEL_SCHED_STABLE_FACE_FRAMES
#define MODEL_SCHED_AUTH_RECHECK_MS        RTAS_MODEL_SCHED_AUTH_RECHECK_MS
#define MODEL_SCHED_MAIN_RECHECK_MS        RTAS_MODEL_SCHED_MAIN_RECHECK_MS
#define MODEL_SCHED_DEPTH_RECHECK_MS       RTAS_MODEL_SCHED_DEPTH_RECHECK_MS
	#define APP_PIPE_DEBUG_PRINT_PERIOD     30U
	#define HW_METRICS_TIMER_HZ             1000000U
#define TELEMETRY_PRINT_PERIOD_FRAMES      RTAS_TELEMETRY_PRINT_PERIOD_FRAMES

	static uint8_t s_model_stable_face_frames = 0U;
	static uint32_t s_last_recognition_ms = 0U;
	static uint32_t s_last_depth_ms = 0U;
	static uint32_t g_model_schedule_flags = 0U;
	static uint32_t enroll_toast_ts = 0U;
	static uint8_t s_hw_metrics_ready = 0U;
	static uint8_t s_hw_metrics_epoch_open = 0U;
	static uint32_t s_telemetry_frame_count = 0U;
	static uint64_t s_telemetry_npu_us_sum = 0ULL;
	static uint64_t s_telemetry_detect_npu_us_sum = 0ULL;
	static uint64_t s_telemetry_embed_npu_us_sum = 0ULL;
	static uint64_t s_telemetry_depth_npu_us_sum = 0ULL;
	static uint64_t s_telemetry_cpu_active_us_sum = 0ULL;
	static uint64_t s_telemetry_cpu_sleep_us_sum = 0ULL;
	static uint64_t s_telemetry_total_us_sum = 0ULL;
	static uint32_t s_nn_capture_buf_idx = 0U;
	static uint32_t s_nn_infer_buf_idx = 0U;
	static uint8_t s_nn_capture_inflight = 0U;
	static uint8_t *s_nn_capture_inflight_u8 = NULL;

	static stai_return_code Run_Inference(stai_network *network_instance);
	static void HwMetrics_Init(void);
	static uint32_t HwMetrics_ReadUs(void);
	static uint32_t HwMetrics_UsDelta(uint32_t start, uint32_t end);
	static void Telemetry_RecordFrame(uint32_t npu_total_us,
									  uint32_t detect_npu_us,
									  uint32_t embed_npu_us,
									  uint32_t depth_npu_us,
									  uint32_t cpu_active_us,
									  uint32_t cpu_sleep_us,
									  uint32_t total_us);
static void NeuralNetwork_init(uint32_t *nn_in_length,
							   stai_ptr *nn_out,
							   stai_size *number_output,
							   int32_t nn_out_len[],
							   stai_network_info *nn_info);
static bool CameraFrame_CanZeroCopy(const AppProcessContext_t *ctx);
static bool CameraFrame_NeedsPostRgbCopy(void);
static int NeuralNetwork_SetInputBuffer(uint8_t *input_u8);
static void ModelScheduler_ClearMatch(void);
	#if PIPELINE_DEBUG_ENABLE
	static void PipelineDebug_CaptureInput(const AppProcessContext_t *ctx);
	static void PipelineDebug_CaptureOutputs(const AppProcessContext_t *ctx);
	static void PipelineDebug_CapturePostprocess(const AppProcessContext_t *ctx,
												 int postprocess_ret);
	static void PipelineDebug_PrintIfDue(void);
	#else
	#define PipelineDebug_CaptureInput(ctx)              ((void)0)
	#define PipelineDebug_CaptureOutputs(ctx)            ((void)0)
	#define PipelineDebug_CapturePostprocess(ctx, ret)   ((void)0)
	#define PipelineDebug_PrintIfDue()                   ((void)0)
	#endif
	static void Enroll_SetFailReason(const char *reason);
	static uint32_t ModelScheduler_SelectRecognition(uint32_t now,
													 const od_pp_out_t *pp,
													 int32_t best_idx,
													 float best_conf);
	static bool ModelScheduler_ShouldRunDepth(const AppProcessContext_t *ctx,
											  uint32_t now);

	void AppProcesses_Init(uint32_t *lcd_bg_width, uint32_t *lcd_bg_height)
	{
	  memset(&s_proc, 0, sizeof(s_proc));
	  s_proc.best_idx = -1;

	  printf("H2: starting NeuralNetwork_init\r\n");
	  NeuralNetwork_init(&s_proc.nn_in_len,
						 s_proc.nn_out,
						 &s_proc.number_output,
						 s_proc.nn_out_len,
						 &s_proc.nn_info);
	  printf("H3: NeuralNetwork_init done, nn_in=%p len=%lu outputs=%lu out0=%p\r\n",
			 nn_in,
			 s_proc.nn_in_len,
			 (uint32_t)s_proc.number_output,
			 (s_proc.number_output > 0U) ? s_proc.nn_out[0] : NULL);

	  printf("H4: app_postprocess_init inputs=%lu outputs=%lu\r\n",
			 (uint32_t)s_proc.nn_info.n_inputs,
			 (uint32_t)s_proc.nn_info.n_outputs);
	  app_postprocess_init(&pp_params, &s_proc.nn_info);
	  printf("H5: postprocess init done\r\n");

	  FaceStore_Init();
	  FaceRecog_Init();
	  Depth_Init();
	  TIM_AppInit();
	  HwMetrics_Init();

	  CameraPipeline_Init(lcd_bg_width, lcd_bg_height, &s_proc.pitch_nn);
	  if (CameraFrame_CanZeroCopy(&s_proc))
	  {
		printf("[Pipeline] NN double-buffer zero-copy DMA enabled pitch=%lu len=%lu buffers=%lu preserve=%u faces=%lu\r\n",
			   (unsigned long)s_proc.pitch_nn,
			   (unsigned long)s_proc.nn_in_len,
			   (unsigned long)NN_INPUT_BUFFER_COUNT,
			   CameraFrame_NeedsPostRgbCopy() ? 1U : 0U,
			   (unsigned long)FaceStore_Count());
	  }
	  else
	  {
		printf("[Pipeline] NN zero-copy DMA fallback pitch=%lu row=%lu len=%lu expected=%lu enable=%u\r\n",
			   (unsigned long)s_proc.pitch_nn,
			   (unsigned long)NN_INPUT_ROW_BYTES,
			   (unsigned long)s_proc.nn_in_len,
			   (unsigned long)NN_U8_SIZE,
			   (unsigned int)NN_INPUT_ZERO_COPY_ENABLE);
	  }
	}

uint32_t AppProcesses_GetNnPitch(void)
{
  return s_proc.pitch_nn;
}

static bool CameraFrame_CanZeroCopy(const AppProcessContext_t *ctx)
{
#if NN_INPUT_ZERO_COPY_ENABLE
  return (ctx != NULL) &&
		 (nn_in != NULL) &&
		 (ctx->pitch_nn == NN_INPUT_ROW_BYTES) &&
		 (ctx->nn_in_len == NN_U8_SIZE) &&
		 (ctx->nn_in_len == STAI_NETWORK_IN_1_SIZE_BYTES);
#else
  (void)ctx;
  return false;
#endif
}

static bool CameraFrame_NeedsPostRgbCopy(void)
{
#if (NN_INPUT_BUFFER_COUNT >= 2U)
  return false;
#else
  return FaceRecog_IsReady() &&
		 ((FaceStore_Count() > 0U) || (g_enroll_requested != 0U));
#endif
}

static int NeuralNetwork_SetInputBuffer(uint8_t *input_u8)
{
  stai_ptr input_buffers[STAI_NETWORK_IN_NUM] = {0};
  stai_size n_inputs = STAI_NETWORK_IN_NUM;
  int ret;

  assert(input_u8 != NULL);

  if (nn_in == (stai_ptr)input_u8)
  {
	return STAI_SUCCESS;
  }

  input_buffers[0] = (stai_ptr)input_u8;
  ret = stai_network_set_inputs(network_context, input_buffers, n_inputs);
  g_nn_ret = ret;

  if (ret == STAI_SUCCESS)
  {
	nn_in = input_buffers[0];
	g_nn_input_addr = (uintptr_t)nn_in;
	g_nn_input_count = (uint32_t)n_inputs;
  }

  return ret;
}

/* Process 1: capture one camera frame and prepare the NN input buffer. */
void CameraFrame_Process(void)
{
  AppProcessContext_t *ctx = &s_proc;
  bool zero_copy_capture;
  bool preserve_post_rgb;
  uint8_t *capture_dst;
  uint32_t completed_buf_idx = 0U;

  ctx->frame_cpu_start = HAL_GetTick();
  ctx->frame_epoch_start_us = HwMetrics_ReadUs();
  ctx->frame_sleep_us = 0U;
  s_hw_metrics_epoch_open = 1U;

  zero_copy_capture = CameraFrame_CanZeroCopy(ctx);
  preserve_post_rgb = zero_copy_capture && CameraFrame_NeedsPostRgbCopy();

  if (zero_copy_capture)
  {
	if (s_nn_capture_inflight == 0U)
	{
	  capture_dst = nn_user_input_u8[s_nn_capture_buf_idx];
	  SCB_InvalidateDCache_by_Addr((void *)capture_dst, ctx->nn_in_len);
	  cameraFrameReceived = 0;
	  CameraPipeline_IspUpdate();
	  CameraPipeline_NNPipe_Start(capture_dst, CMW_MODE_SNAPSHOT);
	  s_nn_capture_inflight = 1U;
	  s_nn_capture_inflight_u8 = capture_dst;
	}

	while (cameraFrameReceived == 0)
	{
	  uint32_t sleep_start;
	  uint32_t primask;

	  AppProcesses_MetricsSleepBegin(&sleep_start);
	  primask = __get_PRIMASK();
	  __disable_irq();
	  if (cameraFrameReceived == 0)
	  {
		__DSB();
		__WFI();
	  }
	  AppProcesses_MetricsSleepEnd(sleep_start);
	  if (primask == 0U)
	  {
		__enable_irq();
	  }
	}
	cameraFrameReceived = 0;

	capture_dst = s_nn_capture_inflight_u8;
	s_nn_capture_inflight = 0U;
	s_nn_capture_inflight_u8 = NULL;

	SCB_InvalidateDCache_by_Addr((void *)capture_dst, ctx->nn_in_len);
	completed_buf_idx = s_nn_capture_buf_idx;
	s_nn_infer_buf_idx = completed_buf_idx;
	s_nn_capture_buf_idx = (s_nn_capture_buf_idx + 1U) % NN_INPUT_BUFFER_COUNT;

	if (NeuralNetwork_SetInputBuffer(nn_user_input_u8[s_nn_infer_buf_idx]) != STAI_SUCCESS)
	{
	  while (1)
	  {
	  }
	}

	if (preserve_post_rgb)
	{
	  memcpy(nn_in_u8, (const void *)capture_dst, NN_U8_SIZE);
	  ctx->nn_src_u8 = nn_in_u8;
	}
	else
	{
	  ctx->nn_src_u8 = capture_dst;
	}
	PipelineDebug_CaptureInput(ctx);

	capture_dst = nn_user_input_u8[s_nn_capture_buf_idx];
	SCB_InvalidateDCache_by_Addr((void *)capture_dst, ctx->nn_in_len);
	cameraFrameReceived = 0;
	CameraPipeline_IspUpdate();
	CameraPipeline_NNPipe_Start(capture_dst, CMW_MODE_SNAPSHOT);
	s_nn_capture_inflight = 1U;
	s_nn_capture_inflight_u8 = capture_dst;
	return;
  }

  capture_dst = nn_in_u8;

  CameraPipeline_IspUpdate();
  CameraPipeline_NNPipe_Start(capture_dst, CMW_MODE_SNAPSHOT);

  while (cameraFrameReceived == 0)
  {
	uint32_t sleep_start;
	uint32_t primask;

	AppProcesses_MetricsSleepBegin(&sleep_start);
	primask = __get_PRIMASK();
	__disable_irq();
	if (cameraFrameReceived == 0)
	{
	  __DSB();
	  __WFI();
	}
	AppProcesses_MetricsSleepEnd(sleep_start);
	if (primask == 0U)
	{
	  __enable_irq();
	}
  }
  cameraFrameReceived = 0;

  SCB_InvalidateDCache_by_Addr((void *)nn_in_u8, DCMIPP_OUT_NN_BUFF_LEN);
  ctx->nn_src_u8 = nn_in_u8;

  if (ctx->pitch_nn != NN_INPUT_ROW_BYTES)
  {
	img_crop(nn_in_u8,
			 nn_crop_u8,
			 ctx->pitch_nn,
			 STAI_NETWORK_IN_1_WIDTH,
			 STAI_NETWORK_IN_1_HEIGHT,
			 STAI_NETWORK_IN_1_CHANNEL);

	SCB_CleanDCache_by_Addr((void *)nn_crop_u8, NN_U8_SIZE_PADDED);
	ctx->nn_src_u8 = nn_crop_u8;
  }

  memcpy((void *)nn_in, ctx->nn_src_u8, NN_U8_SIZE);

  if (ctx->nn_in_len > NN_U8_SIZE)
  {
	memset((uint8_t *)nn_in + NN_U8_SIZE,
		   0,
		   ctx->nn_in_len - NN_U8_SIZE);
  }

  SCB_CleanDCache_by_Addr((void *)nn_in, ctx->nn_in_len);
  PipelineDebug_CaptureInput(ctx);
}

	/* Process 2 helper: clear cached recognition/depth results when the scene is not stable. */
	static void ModelScheduler_ClearMatch(void)
	{
	  last_recog_idx = -1;
	  last_recog_score = 0.0f;
	  last_depth_match_score = 0.0f;
	  last_depth_template_seen = 0U;
	  g_depth_live_score = 0.0f;
	  s_last_recognition_ms = 0U;
	  s_last_depth_ms = 0U;
	}

	#if PIPELINE_DEBUG_ENABLE
	static void PipelineDebug_CaptureInput(const AppProcessContext_t *ctx)
	{
	  const uint8_t *p = (ctx != NULL) ? (const uint8_t *)ctx->nn_src_u8 : NULL;
	  uint32_t min_v = 255U;
	  uint32_t max_v = 0U;
	  uint32_t sum = 0U;
	  uint32_t checksum = 2166136261UL;

	  if (p == NULL)
	  {
		return;
	  }

	  g_pipe_dbg.frame_id++;
	  g_pipe_dbg.input_len = (ctx != NULL) ? ctx->nn_in_len : 0U;
	  g_pipe_dbg.input_expected_len = NN_U8_SIZE;
	  g_pipe_dbg.pitch_nn = (ctx != NULL) ? ctx->pitch_nn : 0U;

	  for (uint32_t i = 0U; i < NN_U8_SIZE; i++)
	  {
		uint32_t v = p[i];
		if (v < min_v) min_v = v;
		if (v > max_v) max_v = v;
		sum += v;
		checksum ^= v;
		checksum *= 16777619UL;
	  }

	  g_pipe_dbg.input_min = min_v;
	  g_pipe_dbg.input_max = max_v;
	  g_pipe_dbg.input_mean = sum / NN_U8_SIZE;
	  g_pipe_dbg.input_checksum = checksum;
	}

	static void PipelineDebug_CaptureOutputs(const AppProcessContext_t *ctx)
	{
	  if (ctx == NULL)
	  {
		return;
	  }

	  g_pipe_dbg.output_count = (uint32_t)ctx->number_output;
	  g_pipe_dbg.npu_ms = g_npu_infer_ms;

	  for (uint32_t out_idx = 0U; out_idx < (uint32_t)ctx->number_output; out_idx++)
	  {
		const float *p = (const float *)ctx->nn_out[out_idx];
		uint32_t count = (uint32_t)ctx->nn_out_len[out_idx] / (uint32_t)sizeof(float);
		float min_v = 0.0f;
		float max_v = 0.0f;
		float sum = 0.0f;
		uint32_t valid_count = 0U;
		uint32_t nan_count = 0U;

		if ((p == NULL) || (count == 0U))
		{
		  g_pipe_dbg.output_min[out_idx] = 0.0f;
		  g_pipe_dbg.output_max[out_idx] = 0.0f;
		  g_pipe_dbg.output_mean[out_idx] = 0.0f;
		  g_pipe_dbg.output_nan_count[out_idx] = count;
		  continue;
		}

		for (uint32_t i = 0U; i < count; i++)
		{
		  float v = p[i];
		  if (v != v)
		  {
			nan_count++;
			continue;
		  }
		  if (valid_count == 0U)
		  {
			min_v = v;
			max_v = v;
		  }
		  else
		  {
			if (v < min_v) min_v = v;
			if (v > max_v) max_v = v;
		  }
		  sum += v;
		  valid_count++;
		}

		g_pipe_dbg.output_min[out_idx] = min_v;
		g_pipe_dbg.output_max[out_idx] = max_v;
		g_pipe_dbg.output_mean[out_idx] = (valid_count > 0U) ? (sum / (float)valid_count) : 0.0f;
		g_pipe_dbg.output_nan_count[out_idx] = nan_count;
	  }
	}

	static void PipelineDebug_CapturePostprocess(const AppProcessContext_t *ctx,
												 int postprocess_ret)
	{
	  uint32_t count = (pp_output.nb_detect > 0) ? (uint32_t)pp_output.nb_detect : 0U;
	  uint32_t drawable = 0U;
	  uint32_t invalid = 0U;
	  uint32_t best = 0U;
	  float best_conf = -1.0f;

	  (void)ctx;
	  g_pipe_dbg.postprocess_ret = postprocess_ret;
	  g_pipe_dbg.detections = count;

	  if ((pp_output.pOutBuff == NULL) || (count == 0U))
	  {
		g_pipe_dbg.drawable_detections = 0U;
		g_pipe_dbg.invalid_detections = count;
		g_pipe_dbg.best_conf = 0.0f;
		g_pipe_dbg.best_x = 0.0f;
		g_pipe_dbg.best_y = 0.0f;
		g_pipe_dbg.best_w = 0.0f;
		g_pipe_dbg.best_h = 0.0f;
		return;
	  }

	  if (count > AI_FD_BLAZEFACE_PP_MAX_BOXES_LIMIT)
	  {
		count = AI_FD_BLAZEFACE_PP_MAX_BOXES_LIMIT;
	  }

	  for (uint32_t i = 0U; i < count; i++)
	  {
		float x = pp_output.pOutBuff[i].x_center;
		float y = pp_output.pOutBuff[i].y_center;
		float w = pp_output.pOutBuff[i].width;
		float h = pp_output.pOutBuff[i].height;
		float c = pp_output.pOutBuff[i].conf;
		float left = x - (w * 0.5f);
		float top = y - (h * 0.5f);
		float right = x + (w * 0.5f);
		float bottom = y + (h * 0.5f);
		bool finite = (x == x) && (y == y) && (w == w) && (h == h) && (c == c);

		if (finite &&
			(w > 0.0f) &&
			(h > 0.0f) &&
			(right > 0.0f) &&
			(bottom > 0.0f) &&
			(left < 1.0f) &&
			(top < 1.0f))
		{
		  drawable++;
		}
		else
		{
		  invalid++;
		}

		if (c > best_conf)
		{
		  best_conf = c;
		  best = i;
		}
	  }

	  g_pipe_dbg.drawable_detections = drawable;
	  g_pipe_dbg.invalid_detections = invalid;
	  g_pipe_dbg.best_conf = (best_conf > 0.0f) ? best_conf : 0.0f;
	  g_pipe_dbg.best_x = pp_output.pOutBuff[best].x_center;
	  g_pipe_dbg.best_y = pp_output.pOutBuff[best].y_center;
	  g_pipe_dbg.best_w = pp_output.pOutBuff[best].width;
	  g_pipe_dbg.best_h = pp_output.pOutBuff[best].height;
	}

	static void PipelineDebug_PrintIfDue(void)
	{
	  uint32_t frame_id = g_pipe_dbg.frame_id;
	  if ((frame_id <= 5U) ||
		  ((APP_PIPE_DEBUG_PRINT_PERIOD != 0U) &&
		   ((frame_id % APP_PIPE_DEBUG_PRINT_PERIOD) == 0U)))
	  {
		printf("[PIPE] f=%lu in=%lu/%lu p=%lu min/max/mean=%lu/%lu/%lu crc=%08lx run=%ld rst=%ld pp=%ld nb=%lu draw=%lu inv=%lu best=%.3f box=%.2f,%.2f,%.2f,%.2f flags=%08lx rec=%lu depth=%lu\r\n",
			   (unsigned long)frame_id,
			   (unsigned long)g_pipe_dbg.input_len,
			   (unsigned long)g_pipe_dbg.input_expected_len,
			   (unsigned long)g_pipe_dbg.pitch_nn,
			   (unsigned long)g_pipe_dbg.input_min,
			   (unsigned long)g_pipe_dbg.input_max,
			   (unsigned long)g_pipe_dbg.input_mean,
			   (unsigned long)g_pipe_dbg.input_checksum,
			   (long)g_pipe_dbg.run_ret,
			   (long)g_pipe_dbg.reset_ret,
			   (long)g_pipe_dbg.postprocess_ret,
			   (unsigned long)g_pipe_dbg.detections,
			   (unsigned long)g_pipe_dbg.drawable_detections,
			   (unsigned long)g_pipe_dbg.invalid_detections,
			   g_pipe_dbg.best_conf,
			   g_pipe_dbg.best_x,
			   g_pipe_dbg.best_y,
			   g_pipe_dbg.best_w,
			   g_pipe_dbg.best_h,
			   (unsigned long)g_pipe_dbg.schedule_flags,
			   (unsigned long)g_pipe_dbg.recog_attempted,
			   (unsigned long)g_pipe_dbg.depth_attempted);

		for (uint32_t i = 0U; i < g_pipe_dbg.output_count; i++)
		{
		  printf("[PIPE] out%lu min/max/mean=%.3f/%.3f/%.3f nan=%lu\r\n",
				 (unsigned long)(i + 1U),
				 g_pipe_dbg.output_min[i],
				 g_pipe_dbg.output_max[i],
				 g_pipe_dbg.output_mean[i],
				 (unsigned long)g_pipe_dbg.output_nan_count[i]);
		}
	  }
	}
	#endif

	static void Enroll_SetFailReason(const char *reason)
	{
	  memset(g_enroll_fail_reason, 0, 64U);
	  if (reason != NULL)
	  {
		strncpy(g_enroll_fail_reason, reason, 63U);
	  }
	}

	/* Process 2 scheduler: choose which expensive models should run for this frame. */
	static uint32_t ModelScheduler_SelectRecognition(uint32_t now,
													 const od_pp_out_t *pp,
													 int32_t best_idx,
													 float best_conf)
	{
	  uint32_t flags = MODEL_SCHED_RUN_DETECT;
	  bool stable_single_face =
		  ((pp != NULL) &&
		   (pp->nb_detect == 1U) &&
		   (best_idx >= 0) &&
		   (best_conf >= FACE_RECOG_MIN_DET_CONF));

	  if (stable_single_face)
	  {
		if (s_model_stable_face_frames < MODEL_SCHED_STABLE_FACE_FRAMES)
		{
		  s_model_stable_face_frames++;
		}
	  }
	  else
	  {
		s_model_stable_face_frames = 0U;
	  }

	  if ((s_model_stable_face_frames >= MODEL_SCHED_STABLE_FACE_FRAMES) &&
		  FaceRecog_IsReady())
	  {
		bool has_enrolled_faces = (FaceStore_Count() > 0U);
		bool auth_recheck_due =
			(has_enrolled_faces &&
			 (app_state == APP_STATE_AUTH) &&
			 ((s_last_recognition_ms == 0U) ||
			  ((now - s_last_recognition_ms) >= MODEL_SCHED_AUTH_RECHECK_MS)));
		bool enroll_needs_identity = (g_enroll_requested != 0U);
		bool main_recheck_due =
			(has_enrolled_faces &&
			 (app_state == APP_STATE_MAIN) &&
			 ((now - s_last_recognition_ms) >= MODEL_SCHED_MAIN_RECHECK_MS));

		if (auth_recheck_due || enroll_needs_identity || main_recheck_due)
		{
		  flags |= MODEL_SCHED_RUN_RECOG;
		}
	  }

	  return flags;
	}

	/* Process 2 scheduler: run depth only when it adds security value. */
	static bool ModelScheduler_ShouldRunDepth(const AppProcessContext_t *ctx,
											  uint32_t now)
	{
	  if ((ctx == NULL) ||
		  !Depth_IsReady() ||
		  (ctx->best_idx < 0) ||
		  (pp_output.nb_detect != 1U) ||
		  (s_model_stable_face_frames < MODEL_SCHED_STABLE_FACE_FRAMES))
	  {
		return false;
	  }

	  if (g_enroll_requested != 0U)
	  {
		return true;
	  }

	  if ((last_recog_idx >= 0) &&
		  ((s_last_depth_ms == 0U) ||
		   ((now - s_last_depth_ms) >= MODEL_SCHED_DEPTH_RECHECK_MS)))
	  {
		return true;
	  }

	  return false;
	}

	/* Process 2: run BlazeFace, then schedule recognition/depth adaptively. */
	void ModelScheduler_Process(void)
	{
	  AppProcessContext_t *ctx = &s_proc;
	  int ret;
	  stai_return_code run_ret;
	  stai_return_code reset_ret;
	  uint32_t now;
	  uint32_t npu_start_us;

	  if (ctx->nn_src_u8 == NULL)
	  {
		return;
	  }

	  npu_start_us = AppProcesses_MetricsNowUs();
	  run_ret = Run_Inference(network_context);
	  g_npu_infer_us = AppProcesses_MetricsElapsedUs(npu_start_us);
	  g_npu_infer_ms = (g_npu_infer_us + 500U) / 1000U;
	  #if PIPELINE_DEBUG_ENABLE
	  g_pipe_dbg.run_ret = run_ret;
	  g_pipe_dbg.recog_attempted = 0U;
	  g_pipe_dbg.depth_attempted = 0U;
	  #else
	  (void)run_ret;
	  #endif

	  for (int i = 0; i < (int)ctx->number_output; i++)
	  {
		SCB_InvalidateDCache_by_Addr((void *)ctx->nn_out[i],
									 ctx->nn_out_len[i]);
	  }
	  PipelineDebug_CaptureOutputs(ctx);

	  pp_output.nb_detect = 0;
	  pp_output.pOutBuff = NULL;
	  ret = app_postprocess_run((void **)ctx->nn_out,
								ctx->number_output,
								&pp_output,
								&pp_params);
	  PipelineDebug_CapturePostprocess(ctx, ret);
	  assert(ret == 0);

	  reset_ret = stai_ext_network_new_inference(network_context);
	  #if PIPELINE_DEBUG_ENABLE
	  g_pipe_dbg.reset_ret = reset_ret;
	  #endif
	  assert(reset_ret == STAI_SUCCESS);

	  last_det_conf = 0.0f;
	  g_embed_npu_us = 0U;
	  g_embed_npu_ms = 0U;
	  g_depth_npu_us = 0U;
	  g_depth_npu_ms = 0U;
	  ctx->best_idx = -1;

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

		ctx->best_idx = (int32_t)best;
		last_det_conf = pp_output.pOutBuff[best].conf;
	  }

	  now = HAL_GetTick();
	  g_model_schedule_flags =
		  ModelScheduler_SelectRecognition(now,
										   &pp_output,
										   ctx->best_idx,
										   last_det_conf);
	  #if PIPELINE_DEBUG_ENABLE
	  g_pipe_dbg.schedule_flags = g_model_schedule_flags;
	  #endif

	  if ((s_model_stable_face_frames < MODEL_SCHED_STABLE_FACE_FRAMES) ||
		  (pp_output.nb_detect != 1U) ||
		  (ctx->best_idx < 0) ||
		  (last_det_conf < FACE_RECOG_MIN_DET_CONF))
	  {
		ModelScheduler_ClearMatch();
	  }

	  if ((g_model_schedule_flags & MODEL_SCHED_RUN_RECOG) != 0U)
	  {
		#if PIPELINE_DEBUG_ENABLE
		g_pipe_dbg.recog_attempted = 1U;
		#endif
		(void)FaceRecog_Identify((const uint8_t *)ctx->nn_src_u8,
								 STAI_NETWORK_IN_1_WIDTH,
								 STAI_NETWORK_IN_1_HEIGHT,
								 &pp_output.pOutBuff[ctx->best_idx],
								 &last_recog_idx,
								 &last_recog_score);
		s_last_recognition_ms = now;
		if (last_recog_idx < 0)
		{
		  last_depth_match_score = 0.0f;
		  last_depth_template_seen = 0U;
		  g_depth_live_score = 0.0f;
		}
	  }

	  if (ModelScheduler_ShouldRunDepth(ctx, now))
	  {
		const od_pp_outBuffer_t *depth_box = &pp_output.pOutBuff[ctx->best_idx];
		g_model_schedule_flags |= MODEL_SCHED_RUN_DEPTH;
		#if PIPELINE_DEBUG_ENABLE
		g_pipe_dbg.depth_attempted = 1U;
		g_pipe_dbg.schedule_flags = g_model_schedule_flags;
		#endif

		if (Depth_RunFrame((const uint8_t *)ctx->nn_src_u8,
						   STAI_NETWORK_IN_1_WIDTH,
						   STAI_NETWORK_IN_1_HEIGHT,
						   depth_box) &&
			(last_recog_idx >= 0))
		{
		  float depth_score = 0.0f;
		  if (FaceStore_DepthScore((uint32_t)last_recog_idx,
								   g_depth_preview,
								   &depth_score))
		  {
			last_depth_template_seen = 1U;
			last_depth_match_score = depth_score;
			if (depth_score < FACE_DEPTH_MATCH_THRESHOLD)
			{
			  last_recog_idx = -1;
			}
		  }
		}
		s_last_depth_ms = now;
	  }

	  PipelineDebug_PrintIfDue();
	}

	/* Process 3: turn model outputs into enrollment/authentication state changes. */
	void AuthDecision_Process(void)
	{
	  AppProcessContext_t *ctx = &s_proc;

	  if (ctx->nn_src_u8 == NULL)
	  {
		return;
	  }

	  if (g_enroll_requested)
	  {
		g_enroll_requested = 0U;
		g_enroll_done_flag = 0U;
		g_enroll_fail_flag = 0U;
		g_enroll_duplicate_flag = 0U;
		Enroll_SetFailReason("");

		if ((ctx->best_idx >= 0) &&
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
										  (const uint8_t *)ctx->nn_src_u8,
										  STAI_NETWORK_IN_1_WIDTH,
										  STAI_NETWORK_IN_1_HEIGHT,
										  &pp_output.pOutBuff[ctx->best_idx],
										  &matched_index,
										  &matched_score);

		  if (enroll_status == FACE_ENROLL_OK)
		  {
			g_enroll_done_flag = 1U;
			Enroll_SetFailReason("");
			enroll_toast_ts = HAL_GetTick();
			printf("[Enroll] saved '%s' (total=%lu)\r\n",
				   default_name,
				   (unsigned long)FaceStore_Count());
		  }
		  else if (enroll_status == FACE_ENROLL_ERR_DUPLICATE)
		  {
			g_enroll_duplicate_flag = 1U;
			Enroll_SetFailReason("This face is already enrolled");
			enroll_toast_ts = HAL_GetTick();
			printf("[Enroll] duplicate face, matched index=%lu score=%.3f\r\n",
				   (unsigned long)matched_index,
				   matched_score);
		  }
		  else
		  {
			g_enroll_fail_flag = 1U;
			if (enroll_status == FACE_ENROLL_ERR_STORE)
			{
			  Enroll_SetFailReason("Face store is full");
			}
			else if (enroll_status == FACE_ENROLL_ERR_COMMIT)
			{
			  Enroll_SetFailReason("Flash save failed");
			}
			else
			{
			  Enroll_SetFailReason("Embedding extraction failed");
			}
			enroll_toast_ts = HAL_GetTick();
			printf("[Enroll] failed status=%ld count=%lu\r\n",
				   (long)enroll_status,
				   (unsigned long)FaceStore_Count());
		  }
		}
		else
		{
		  g_enroll_name[0] = '\0';
		  g_enroll_fail_flag = 1U;
		  if (ctx->best_idx < 0)
		  {
			Enroll_SetFailReason("No face ready for enrollment");
		  }
		  else if (last_det_conf < FACE_RECOG_MIN_DET_CONF)
		  {
			Enroll_SetFailReason("Face confidence too low");
		  }
		  else
		  {
			Enroll_SetFailReason("Recognizer not ready");
		  }
		  enroll_toast_ts = HAL_GetTick();
		  printf("[Enroll] failed precheck best=%ld conf=%.3f ready=%u count=%lu/%u\r\n",
				 (long)ctx->best_idx,
				 last_det_conf,
				 FaceRecog_IsReady() ? 1U : 0U,
				 (unsigned long)FaceStore_Count(),
				 (unsigned)FACE_STORE_MAX_RECORDS);
		}
	  }

	  if ((g_enroll_done_flag || g_enroll_fail_flag || g_enroll_duplicate_flag) &&
		  ((HAL_GetTick() - enroll_toast_ts) > 2000U))
	  {
		g_enroll_done_flag = 0U;
		g_enroll_fail_flag = 0U;
		g_enroll_duplicate_flag = 0U;
		Enroll_SetFailReason("");
	  }

	  {
		uint32_t frame_elapsed = HAL_GetTick() - ctx->frame_cpu_start;
		uint32_t npu_total_us = g_npu_infer_us + g_embed_npu_us + g_depth_npu_us;
		uint32_t epoch_us = HwMetrics_UsDelta(ctx->frame_epoch_start_us,
											  HwMetrics_ReadUs());
		uint32_t sleep_us = ctx->frame_sleep_us;
		uint32_t active_us;

		if (sleep_us > epoch_us)
		{
		  sleep_us = epoch_us;
		}
		active_us = epoch_us - sleep_us;

		g_frame_output_ms = frame_elapsed;
		g_cpu_frame_ms = (active_us + 999U) / 1000U;
		g_hw_cpu_active_us = active_us;
		g_hw_cpu_sleep_us = sleep_us;
		g_hw_cpu_pct = (epoch_us > 0U) ?
					   (uint32_t)(((uint64_t)active_us * 100ULL) / epoch_us) : 0U;
		g_hw_npu_run_pct = (epoch_us > 0U) ?
						   (uint32_t)(((uint64_t)npu_total_us * 100ULL) / epoch_us) : 0U;
		if (g_hw_npu_run_pct > 100U)
		{
		  g_hw_npu_run_pct = 100U;
		}
		Telemetry_RecordFrame(npu_total_us,
							  g_npu_infer_us,
							  g_embed_npu_us,
							  g_depth_npu_us,
							  active_us,
							  sleep_us,
							  epoch_us);
		s_hw_metrics_epoch_open = 0U;
	  }
	}

	/* Process 4: poll services and render UI. */
	void UiSystem_Process(void)
	{
	  TIM_AppPoll();
	  AppProcesses_RenderOutput(&pp_output, g_npu_infer_ms);
	}

	static void HwMetrics_Init(void)
	{
	  uint32_t timer_clk;
	  uint32_t prescaler;

	  __HAL_RCC_TIM2_CLK_ENABLE();
	  __HAL_RCC_TIM2_CLK_SLEEP_ENABLE();

	  timer_clk = HAL_RCC_GetPCLK1Freq();
	  if (timer_clk == 0U)
	  {
		timer_clk = HAL_RCC_GetHCLKFreq();
	  }

	  prescaler = (timer_clk > HW_METRICS_TIMER_HZ) ?
				  ((timer_clk / HW_METRICS_TIMER_HZ) - 1U) : 0U;

	  TIM2->CR1 = 0U;
	  TIM2->PSC = prescaler;
	  TIM2->ARR = 0xFFFFFFFFU;
	  TIM2->EGR = TIM_EGR_UG;
	  TIM2->CNT = 0U;
	  TIM2->CR1 = TIM_CR1_CEN;
	  s_hw_metrics_ready = 1U;
	}

	static uint32_t HwMetrics_ReadUs(void)
	{
	  return (s_hw_metrics_ready != 0U) ? TIM2->CNT : 0U;
	}

	static uint32_t HwMetrics_UsDelta(uint32_t start, uint32_t end)
	{
	  return (s_hw_metrics_ready != 0U) ? (end - start) : 0U;
	}

	void AppProcesses_MetricsSleepBegin(uint32_t *stamp_us)
	{
	  if (stamp_us != NULL)
	  {
		*stamp_us = HwMetrics_ReadUs();
	  }
	}

	void AppProcesses_MetricsSleepEnd(uint32_t stamp_us)
	{
	  if ((s_hw_metrics_epoch_open != 0U) && (s_hw_metrics_ready != 0U))
	  {
		s_proc.frame_sleep_us += HwMetrics_UsDelta(stamp_us, HwMetrics_ReadUs());
	  }
	}

	uint32_t AppProcesses_MetricsNowUs(void)
	{
	  return HwMetrics_ReadUs();
	}

	uint32_t AppProcesses_MetricsElapsedUs(uint32_t start_us)
	{
	  return HwMetrics_UsDelta(start_us, HwMetrics_ReadUs());
	}

	static void Telemetry_RecordFrame(uint32_t npu_total_us,
									  uint32_t detect_npu_us,
									  uint32_t embed_npu_us,
									  uint32_t depth_npu_us,
									  uint32_t cpu_active_us,
									  uint32_t cpu_sleep_us,
									  uint32_t total_us)
	{
	  uint32_t count;
	  uint32_t avg_npu_us;
	  uint32_t avg_detect_npu_us;
	  uint32_t avg_embed_npu_us;
	  uint32_t avg_depth_npu_us;
	  uint32_t avg_cpu_active_us;
	  uint32_t avg_cpu_sleep_us;
	  uint32_t avg_total_us;
	  uint32_t avg_total_ms;
	  uint32_t cpu_util_pct;
	  uint32_t npu_util_pct;
	  uint32_t nn_output_bytes = 0U;

	  if (TELEMETRY_PRINT_PERIOD_FRAMES == 0U)
	  {
		return;
	  }

	  s_telemetry_frame_count++;
	  s_telemetry_npu_us_sum += npu_total_us;
	  s_telemetry_detect_npu_us_sum += detect_npu_us;
	  s_telemetry_embed_npu_us_sum += embed_npu_us;
	  s_telemetry_depth_npu_us_sum += depth_npu_us;
	  s_telemetry_cpu_active_us_sum += cpu_active_us;
	  s_telemetry_cpu_sleep_us_sum += cpu_sleep_us;
	  s_telemetry_total_us_sum += total_us;

	  if (s_telemetry_frame_count < TELEMETRY_PRINT_PERIOD_FRAMES)
	  {
		return;
	  }

	  count = s_telemetry_frame_count;
	  avg_npu_us = (uint32_t)((s_telemetry_npu_us_sum + (count / 2U)) / count);
	  avg_detect_npu_us = (uint32_t)((s_telemetry_detect_npu_us_sum + (count / 2U)) / count);
	  avg_embed_npu_us = (uint32_t)((s_telemetry_embed_npu_us_sum + (count / 2U)) / count);
	  avg_depth_npu_us = (uint32_t)((s_telemetry_depth_npu_us_sum + (count / 2U)) / count);
	  avg_cpu_active_us = (uint32_t)((s_telemetry_cpu_active_us_sum + (count / 2U)) / count);
	  avg_cpu_sleep_us = (uint32_t)((s_telemetry_cpu_sleep_us_sum + (count / 2U)) / count);
	  avg_total_us = (uint32_t)((s_telemetry_total_us_sum + (count / 2U)) / count);
	  avg_total_ms = (avg_total_us + 500U) / 1000U;
	  cpu_util_pct = (avg_total_us > 0U) ?
					 (uint32_t)(((uint64_t)avg_cpu_active_us * 100ULL) / avg_total_us) : 0U;
	  npu_util_pct = (avg_total_us > 0U) ?
					 (uint32_t)(((uint64_t)avg_npu_us * 100ULL) / avg_total_us) : 0U;

	  if (npu_util_pct > 100U)
	  {
		npu_util_pct = 100U;
	  }

	  for (uint32_t i = 0U; i < s_proc.number_output; i++)
	  {
		if (s_proc.nn_out_len[i] > 0)
		{
		  nn_output_bytes += (uint32_t)s_proc.nn_out_len[i];
		}
	  }

	  printf("TELEMETRY,frames=%lu,avg_npu_us=%lu,avg_detect_npu_us=%lu,avg_embed_npu_us=%lu,avg_depth_npu_us=%lu,avg_cpu_active_us=%lu,avg_cpu_sleep_us=%lu,avg_total_us=%lu,avg_total_ms=%lu,cpu_util_pct=%lu,npu_util_pct=%lu,hclk_hz=%lu,pclk1_hz=%lu,pclk2_hz=%lu,nn_input_bytes=%lu,nn_output_bytes=%lu,nn_context_bytes=%lu,camera_buffer_bytes=%lu,crop_buffer_bytes=%lu,schedule_flags=%lu\r\n",
			 (unsigned long)count,
			 (unsigned long)avg_npu_us,
			 (unsigned long)avg_detect_npu_us,
			 (unsigned long)avg_embed_npu_us,
			 (unsigned long)avg_depth_npu_us,
			 (unsigned long)avg_cpu_active_us,
			 (unsigned long)avg_cpu_sleep_us,
			 (unsigned long)avg_total_us,
			 (unsigned long)avg_total_ms,
			 (unsigned long)cpu_util_pct,
			 (unsigned long)npu_util_pct,
			 (unsigned long)HAL_RCC_GetHCLKFreq(),
			 (unsigned long)HAL_RCC_GetPCLK1Freq(),
			 (unsigned long)HAL_RCC_GetPCLK2Freq(),
			 (unsigned long)s_proc.nn_in_len,
			 (unsigned long)nn_output_bytes,
			 (unsigned long)STAI_NETWORK_CONTEXT_SIZE,
			 (unsigned long)DCMIPP_OUT_NN_BUFF_LEN,
			 (unsigned long)NN_U8_SIZE_PADDED,
			 (unsigned long)g_model_schedule_flags);

	  s_telemetry_frame_count = 0U;
	  s_telemetry_npu_us_sum = 0ULL;
	  s_telemetry_detect_npu_us_sum = 0ULL;
	  s_telemetry_embed_npu_us_sum = 0ULL;
	  s_telemetry_depth_npu_us_sum = 0ULL;
	  s_telemetry_cpu_active_us_sum = 0ULL;
	  s_telemetry_cpu_sleep_us_sum = 0ULL;
	  s_telemetry_total_us_sum = 0ULL;
	}

	static stai_return_code Run_Inference(stai_network *network_instance)
	{
	  stai_return_code ret;

	  do
	  {
		ret = stai_network_run(network_instance, STAI_MODE_ASYNC);
		if (ret == STAI_RUNNING_WFE)
		{
		  uint32_t sleep_start;
		  AppProcesses_MetricsSleepBegin(&sleep_start);
		  LL_ATON_OSAL_WFE();
		  AppProcesses_MetricsSleepEnd(sleep_start);
		}
	  } while ((ret == STAI_RUNNING_WFE) || (ret == STAI_RUNNING_NO_WFE));

	  return ret;
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
	  stai_ptr input_buffers[STAI_NETWORK_IN_NUM] = {0};
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
	  if ((ret != STAI_SUCCESS) || (n_inputs == 0U))
	  {
		while (1)
		{
		}
	  }

	  if (nn_in == NULL)
	  {
		s_nn_capture_buf_idx = 0U;
		s_nn_infer_buf_idx = 0U;
		s_nn_capture_inflight = 0U;
		s_nn_capture_inflight_u8 = NULL;
		nn_in = nn_user_input_u8[s_nn_infer_buf_idx];
		input_buffers[0] = nn_in;

		g_nn_step = 52U;
		printf("NN52: external input buffer selected nn_in=%p bytes=%lu\r\n",
			   nn_in,
			   (unsigned long)*nn_in_length);
		ret = stai_network_set_inputs(network_context, input_buffers, n_inputs);
		g_nn_ret = ret;
		printf("NN53: stai_network_set_inputs ret=%d\r\n", ret);
		if (ret != STAI_SUCCESS)
		{
		  while (1)
		  {
		  }
		}

		g_nn_step = 54U;
		nn_in = NULL;
		ret = stai_network_get_inputs(network_context, &nn_in, &n_inputs);
		g_nn_ret = ret;
		g_nn_input_addr = (uintptr_t)nn_in;
		g_nn_input_count = (uint32_t)n_inputs;
		printf("NN55: stai_network_get_inputs ret=%d n_inputs=%lu nn_in=%p\r\n",
			   ret,
			   (uint32_t)n_inputs,
			   nn_in);
		if ((ret != STAI_SUCCESS) || (n_inputs == 0U) || (nn_in == NULL))
		{
		  while (1)
		  {
		  }
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
