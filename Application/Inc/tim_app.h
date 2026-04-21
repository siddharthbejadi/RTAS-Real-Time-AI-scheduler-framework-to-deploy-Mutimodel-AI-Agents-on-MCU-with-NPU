#ifndef TIM_APP_H
#define TIM_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define TIM_CHAT_TEXT_MAX 96U
#define TIM_CHAT_HISTORY_COUNT 4U

typedef struct {
    char input[TIM_CHAT_TEXT_MAX];
    char response[TIM_CHAT_TEXT_MAX];
    int32_t intent_id;
    float confidence;
    uint8_t valid;
} TIM_ChatTurn_t;

typedef struct {
    char input[TIM_CHAT_TEXT_MAX];
    char response[TIM_CHAT_TEXT_MAX];
    char current_input[TIM_CHAT_TEXT_MAX];
    TIM_ChatTurn_t history[TIM_CHAT_HISTORY_COUNT];
    int32_t intent_id;
    float confidence;
    uint32_t turn_count;
    uint8_t ready;
    uint8_t has_message;
} TIM_ChatState_t;

void TIM_AppInit(void);
void TIM_AppSetUser(int32_t user_idx);
void TIM_AppClearUser(void);
void TIM_AppPoll(void);
const TIM_ChatState_t *TIM_AppGetState(void);
void TIM_AppSubmitText(const char *text);
void TIM_AppInputChar(char c);
void TIM_AppBackspace(void);
void TIM_AppClearInput(void);
void TIM_AppSubmitCurrentInput(void);

#ifdef __cplusplus
}
#endif

#endif /* TIM_APP_H */
