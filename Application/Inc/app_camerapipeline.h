

#ifndef APP_CAMERAPIPELINE
#define APP_CAMERAPIPELINE

#define SCREEN_HEIGHT (480)
#define SCREEN_WIDTH  (800)

void CameraPipeline_Init(uint32_t *lcd_bg_width, uint32_t *lcd_bg_height, uint32_t *pitch_nn);
void CameraPipeline_DeInit(void);
void CameraPipeline_Start(void);
void CameraPipeline_DisplayPipe_Start(uint8_t *display_pipe_dst, uint32_t cam_mode);
void CameraPipeline_DisplayPipe_Stop(void);
void CameraPipeline_NNPipe_Start(uint8_t *nn_pipe_dst, uint32_t cam_mode);
void CameraPipeline_IspUpdate(void);

#endif
