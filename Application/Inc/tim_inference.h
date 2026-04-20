#ifndef TIM_INFERENCE_H
#define TIM_INFERENCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    TIM_INTENT_GREETING = 0,
    TIM_INTENT_STATUS = 1,
    TIM_INTENT_THANKS = 2,
    TIM_INTENT_GOODBYE = 3,
    TIM_INTENT_HELP = 4,
    TIM_INTENT_VISION_QUERY = 5,
    TIM_INTENT_SYSTEM_QUERY = 6,
    TIM_INTENT_COMMAND = 7,
    TIM_INTENT_UNKNOWN = 8,
} TimIntent_t;

int TIM_Inference_Init(void);
int TIM_Predict(const char *text, int32_t *intent_id, float *confidence);
const char *TIM_IntentName(int32_t intent_id);
const char *TIM_ResponseForIntent(int32_t intent_id);

#ifdef __cplusplus
}
#endif

#endif /* TIM_INFERENCE_H */
