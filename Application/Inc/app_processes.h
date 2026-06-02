#ifndef APP_PROCESSES_H
#define APP_PROCESSES_H

#include <stdint.h>

void AppProcesses_Init(uint32_t *lcd_bg_width, uint32_t *lcd_bg_height);
uint32_t AppProcesses_GetNnPitch(void);

void CameraFrame_Process(void);
void ModelScheduler_Process(void);
void AuthDecision_Process(void);
void UiSystem_Process(void);

void AppProcesses_MetricsSleepBegin(uint32_t *stamp_us);
void AppProcesses_MetricsSleepEnd(uint32_t stamp_us);
uint32_t AppProcesses_MetricsNowUs(void);
uint32_t AppProcesses_MetricsElapsedUs(uint32_t start_us);

#endif /* APP_PROCESSES_H */
