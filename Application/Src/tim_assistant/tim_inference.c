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

static stai_ptr s_tim_inputs[STAI_TIM_NETWORK_IN_NUM] = {NULL};
static stai_ptr s_tim_outputs[STAI_TIM_NETWORK_OUT_NUM] = {NULL};
static stai_size s_tim_input_count = STAI_TIM_NETWORK_IN_NUM;
static stai_size s_tim_output_count = STAI_TIM_NETWORK_OUT_NUM;
#endif

static uint8_t s_tim_ready = 0U;

static const char *const s_intent_names[] = {
    "greeting",
    "help",
    "system_status",
    "show_runtime",
    "list_users",
    "lock",
    "unlock",
    "enroll_face",
    "delete_user",
    "logout",
    "reset_system",
    "vision_query",
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

    ret = stai_tim_network_get_inputs(s_tim_context, s_tim_inputs, &s_tim_input_count);
    if ((ret != STAI_SUCCESS) || (s_tim_inputs[0] == NULL) || (s_tim_input_count < 1U))
    {
        s_tim_ready = 0U;
        return -2;
    }

    ret = stai_tim_network_get_outputs(s_tim_context, s_tim_outputs, &s_tim_output_count);
    if ((ret != STAI_SUCCESS) || (s_tim_outputs[0] == NULL) || (s_tim_output_count == 0U))
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
    int32_t mask[TIM_TOKENIZER_MAX_LEN];
    stai_return_code ret;

    if ((s_tim_ready == 0U) || (s_tim_inputs[0] == NULL) || (s_tim_outputs[0] == NULL) || (intent_id == NULL))
    {
        return -1;
    }

    TIM_Tokenizer_EncodeWithMask(text, ids, mask);
    memcpy((void *)s_tim_inputs[0], ids, STAI_TIM_NETWORK_IN_1_SIZE_BYTES);
    SCB_CleanDCache_by_Addr((void *)s_tim_inputs[0], STAI_TIM_NETWORK_IN_1_SIZE_BYTES);

#if STAI_TIM_NETWORK_IN_NUM > 1
    if (s_tim_inputs[1] != NULL)
    {
        memcpy((void *)s_tim_inputs[1], mask, STAI_TIM_NETWORK_IN_2_SIZE_BYTES);
        SCB_CleanDCache_by_Addr((void *)s_tim_inputs[1], STAI_TIM_NETWORK_IN_2_SIZE_BYTES);
    }
#endif

    ret = stai_tim_network_run(s_tim_context, STAI_MODE_SYNC);
    if (ret != STAI_SUCCESS)
    {
        return -2;
    }

    SCB_InvalidateDCache_by_Addr((void *)s_tim_outputs[0], STAI_TIM_NETWORK_OUT_1_SIZE_BYTES);

    const float *logits = (const float *)s_tim_outputs[0];
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
    else if ((strstr(norm, "help") != NULL) || (strstr(norm, "command") != NULL) ||
             (strstr(norm, "what can you do") != NULL))
    {
        *intent_id = TIM_INTENT_HELP;
    }
    else if ((strstr(norm, "logout") != NULL) || (strstr(norm, "log out") != NULL) ||
             (strstr(norm, "sign out") != NULL))
    {
        *intent_id = TIM_INTENT_LOGOUT;
    }
    else if ((strstr(norm, "reset") != NULL) || (strstr(norm, "restart") != NULL) ||
             (strstr(norm, "reboot") != NULL))
    {
        *intent_id = TIM_INTENT_RESET_SYSTEM;
    }
    else if ((strstr(norm, "runtime") != NULL) || (strstr(norm, "uptime") != NULL) ||
             (strstr(norm, "how long") != NULL))
    {
        *intent_id = TIM_INTENT_SHOW_RUNTIME;
    }
    else if ((strstr(norm, "list") != NULL && strstr(norm, "user") != NULL) ||
             (strstr(norm, "show") != NULL && strstr(norm, "user") != NULL) ||
             (strstr(norm, "people") != NULL) || (strstr(norm, "profiles") != NULL))
    {
        *intent_id = TIM_INTENT_LIST_USERS;
    }
    else if ((strstr(norm, "enroll") != NULL) || (strstr(norm, "register") != NULL) ||
             (strstr(norm, "add my face") != NULL) || (strstr(norm, "add me") != NULL))
    {
        *intent_id = TIM_INTENT_ENROLL_FACE;
    }
    else if ((strstr(norm, "delete") != NULL) || (strstr(norm, "remove") != NULL) ||
             (strstr(norm, "erase") != NULL) || (strstr(norm, "forget") != NULL))
    {
        *intent_id = TIM_INTENT_DELETE_USER;
    }
    else if ((strstr(norm, "unlock") != NULL) || (strstr(norm, "open") != NULL) ||
             (strstr(norm, "let me in") != NULL) || (strstr(norm, "grant access") != NULL))
    {
        *intent_id = TIM_INTENT_UNLOCK;
    }
    else if ((strstr(norm, "lock") != NULL) || (strstr(norm, "secure") != NULL) ||
             (strstr(norm, "close access") != NULL))
    {
        *intent_id = TIM_INTENT_LOCK;
    }
    else if ((strstr(norm, "see") != NULL) || (strstr(norm, "camera") != NULL) ||
             (strstr(norm, "vision") != NULL) || (strstr(norm, "detect") != NULL))
    {
        *intent_id = TIM_INTENT_VISION_QUERY;
    }
    else if ((strstr(norm, "battery") != NULL) || (strstr(norm, "system") != NULL) ||
             (strstr(norm, "status") != NULL) || (strstr(norm, "device") != NULL))
    {
        *intent_id = TIM_INTENT_SYSTEM_STATUS;
    }
    else if ((strstr(norm, "how are you") != NULL) || (strstr(norm, "running") != NULL))
    {
        *intent_id = TIM_INTENT_SYSTEM_STATUS;
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
    case TIM_INTENT_HELP:
        return "I can run access and system commands.";
    case TIM_INTENT_SYSTEM_STATUS:
        return "Opening system status.";
    case TIM_INTENT_SHOW_RUNTIME:
        return "Opening runtime information.";
    case TIM_INTENT_LIST_USERS:
        return "Opening enrolled users.";
    case TIM_INTENT_LOCK:
        return "Locking session.";
    case TIM_INTENT_UNLOCK:
        return "Checking access.";
    case TIM_INTENT_ENROLL_FACE:
        return "Starting face enrollment.";
    case TIM_INTENT_DELETE_USER:
        return "Opening user delete flow.";
    case TIM_INTENT_LOGOUT:
        return "Logging out.";
    case TIM_INTENT_RESET_SYSTEM:
        return "Reset command detected.";
    case TIM_INTENT_VISION_QUERY:
        return "Vision is active. I can see the live camera feed.";
    default:
        return "I did not understand.";
    }
}
