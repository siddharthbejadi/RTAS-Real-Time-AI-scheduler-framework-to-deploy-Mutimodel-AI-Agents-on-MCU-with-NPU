#include "tim_inference.h"
#include "tim_tokenizer.h"
#include <string.h>

#ifndef TIM_USE_STEDGEAI_MODEL
#define TIM_USE_STEDGEAI_MODEL 0
#endif

#if TIM_USE_STEDGEAI_MODEL
#include "tim_network.h"
#include "stai.h"
#include "stm32n6xx_hal.h"
#include <math.h>

STAI_NETWORK_CONTEXT_DECLARE(s_tim_context, STAI_TIM_NETWORK_CONTEXT_SIZE)

static stai_ptr s_tim_input = NULL;
static stai_ptr s_tim_output = NULL;
static stai_size s_tim_input_count = STAI_TIM_NETWORK_IN_NUM;
static stai_size s_tim_output_count = STAI_TIM_NETWORK_OUT_NUM;
#endif

static uint8_t s_tim_ready = 0U;

static const char *const s_intent_names[] = {
    "greeting",
    "status",
    "thanks",
    "goodbye",
    "help",
    "vision_query",
    "system_query",
    "command",
    "unknown",
};

int TIM_Inference_Init(void)
{
#if TIM_USE_STEDGEAI_MODEL
    stai_return_code ret;

    ret = stai_tim_network_init(s_tim_context);
    if (ret != STAI_SUCCESS)
    {
        s_tim_ready = 0U;
        return -1;
    }

    ret = stai_tim_network_get_inputs(s_tim_context, &s_tim_input, &s_tim_input_count);
    if ((ret != STAI_SUCCESS) || (s_tim_input == NULL) || (s_tim_input_count == 0U))
    {
        s_tim_ready = 0U;
        return -2;
    }

    ret = stai_tim_network_get_outputs(s_tim_context, &s_tim_output, &s_tim_output_count);
    if ((ret != STAI_SUCCESS) || (s_tim_output == NULL) || (s_tim_output_count == 0U))
    {
        s_tim_ready = 0U;
        return -3;
    }

    s_tim_ready = 1U;
    return 0;
#else
    s_tim_ready = 1U;
    return 0;
#endif
}

int TIM_Predict(const char *text, int32_t *intent_id, float *confidence)
{
#if TIM_USE_STEDGEAI_MODEL
    int32_t ids[TIM_TOKENIZER_MAX_LEN];
    stai_return_code ret;

    if ((s_tim_ready == 0U) || (s_tim_input == NULL) || (s_tim_output == NULL) || (intent_id == NULL))
    {
        return -1;
    }

    TIM_Tokenizer_Encode(text, ids);
    memcpy((void *)s_tim_input, ids, STAI_TIM_NETWORK_IN_1_SIZE_BYTES);
    SCB_CleanInvalidateDCache_by_Addr((void *)s_tim_input, STAI_TIM_NETWORK_IN_1_SIZE_BYTES);

    ret = stai_tim_network_run(s_tim_context, STAI_MODE_SYNC);
    if (ret != STAI_SUCCESS)
    {
        return -2;
    }

    SCB_InvalidateDCache_by_Addr((void *)s_tim_output, STAI_TIM_NETWORK_OUT_1_SIZE_BYTES);

    const float *logits = (const float *)s_tim_output;
    int32_t best = 0;
    float best_logit = logits[0];
    float sum = 0.0f;

    for (int32_t i = 1; i < (int32_t)STAI_TIM_NETWORK_OUT_1_SIZE; i++)
    {
        if (logits[i] > best_logit)
        {
            best_logit = logits[i];
            best = i;
        }
    }

    if (confidence != NULL)
    {
        for (int32_t i = 0; i < (int32_t)STAI_TIM_NETWORK_OUT_1_SIZE; i++)
        {
            sum += expf(logits[i] - best_logit);
        }
        *confidence = (sum > 0.0f) ? (1.0f / sum) : 0.0f;
    }

    *intent_id = best;
    return 0;
#else
    char norm[96];
    uint32_t n = 0U;

    if ((s_tim_ready == 0U) || (intent_id == NULL))
    {
        return -1;
    }

    if (text != NULL)
    {
        for (const char *p = text; (*p != '\0') && (n < (sizeof(norm) - 1U)); p++)
        {
            char c = *p;
            if ((c >= 'A') && (c <= 'Z'))
            {
                c = (char)(c + ('a' - 'A'));
            }
            if (((c >= 'a') && (c <= 'z')) || ((c >= '0') && (c <= '9')) || (c == ' '))
            {
                norm[n++] = c;
            }
            else
            {
                norm[n++] = ' ';
            }
        }
    }
    norm[n] = '\0';

    if ((strstr(norm, "hello") != NULL) || (strstr(norm, "hi") != NULL))
    {
        *intent_id = TIM_INTENT_GREETING;
    }
    else if ((strstr(norm, "thank") != NULL) || (strstr(norm, "thanks") != NULL))
    {
        *intent_id = TIM_INTENT_THANKS;
    }
    else if ((strstr(norm, "bye") != NULL) || (strstr(norm, "goodbye") != NULL))
    {
        *intent_id = TIM_INTENT_GOODBYE;
    }
    else if (strstr(norm, "help") != NULL)
    {
        *intent_id = TIM_INTENT_HELP;
    }
    else if ((strstr(norm, "see") != NULL) || (strstr(norm, "camera") != NULL) ||
             (strstr(norm, "vision") != NULL) || (strstr(norm, "detect") != NULL))
    {
        *intent_id = TIM_INTENT_VISION_QUERY;
    }
    else if ((strstr(norm, "battery") != NULL) || (strstr(norm, "system") != NULL) ||
             (strstr(norm, "status") != NULL) || (strstr(norm, "device") != NULL))
    {
        *intent_id = TIM_INTENT_SYSTEM_QUERY;
    }
    else if ((strstr(norm, "restart") != NULL) || (strstr(norm, "reset") != NULL) ||
             (strstr(norm, "open") != NULL) || (strstr(norm, "run") != NULL))
    {
        *intent_id = TIM_INTENT_COMMAND;
    }
    else if ((strstr(norm, "how are you") != NULL) || (strstr(norm, "running") != NULL))
    {
        *intent_id = TIM_INTENT_STATUS;
    }
    else
    {
        *intent_id = TIM_INTENT_UNKNOWN;
    }

    if (confidence != NULL)
    {
        *confidence = (*intent_id == TIM_INTENT_UNKNOWN) ? 0.50f : 0.90f;
    }

    return 0;
#endif
}

const char *TIM_IntentName(int32_t intent_id)
{
    if ((intent_id < 0) || (intent_id >= (int32_t)(sizeof(s_intent_names) / sizeof(s_intent_names[0]))))
    {
        return "unknown";
    }
    return s_intent_names[intent_id];
}

const char *TIM_ResponseForIntent(int32_t intent_id)
{
    switch (intent_id)
    {
    case TIM_INTENT_GREETING:
        return "Hello.";
    case TIM_INTENT_STATUS:
        return "I am running normally.";
    case TIM_INTENT_THANKS:
        return "You are welcome.";
    case TIM_INTENT_GOODBYE:
        return "Goodbye.";
    case TIM_INTENT_HELP:
        return "I can answer status, system and vision requests.";
    case TIM_INTENT_VISION_QUERY:
        return "Vision is active. I can see the live camera feed.";
    case TIM_INTENT_SYSTEM_QUERY:
        return "System is online. Camera and face access are running.";
    case TIM_INTENT_COMMAND:
        return "Command intent detected. No command has been executed yet.";
    default:
        return "I did not understand.";
    }
}
