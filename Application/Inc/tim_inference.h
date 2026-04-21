#ifndef TIM_INFERENCE_H
#define TIM_INFERENCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    TIM_INTENT_GREETING = 0,
    TIM_INTENT_HELP = 1,
    TIM_INTENT_SYSTEM_STATUS = 2,
    TIM_INTENT_SHOW_RUNTIME = 3,
    TIM_INTENT_LIST_USERS = 4,
    TIM_INTENT_LOCK = 5,
    TIM_INTENT_UNLOCK = 6,
    TIM_INTENT_ENROLL_FACE = 7,
    TIM_INTENT_DELETE_USER = 8,
    TIM_INTENT_LOGOUT = 9,
    TIM_INTENT_RESET_SYSTEM = 10,
    TIM_INTENT_VISION_QUERY = 11,
    TIM_INTENT_UNKNOWN = 12,
} TimIntent_t;

int TIM_Inference_Init(void);
int TIM_Predict(const char *text, int32_t *intent_id, float *confidence);
const char *TIM_IntentName(int32_t intent_id);
const char *TIM_ResponseForIntent(int32_t intent_id);

#ifdef __cplusplus
}
#endif

#endif /* TIM_INFERENCE_H */
