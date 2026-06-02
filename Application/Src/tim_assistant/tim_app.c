#include "tim_app.h"
#include "tim_inference.h"
#include "face_store.h"
#include "stm32n6xx_hal.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;

static TIM_ChatState_t s_guest_chat;
static TIM_ChatState_t s_user_chats[FACE_STORE_MAX_RECORDS];
static TIM_ChatState_t *s_chat = &s_guest_chat;
static char s_line[TIM_CHAT_TEXT_MAX];
static uint32_t s_line_len = 0U;
static uint8_t s_tim_ready = 0U;

static void InitChatState(TIM_ChatState_t *chat)
{
    if (chat == NULL)
    {
        return;
    }

    memset(chat, 0, sizeof(*chat));
    chat->intent_id = TIM_INTENT_UNKNOWN;
    chat->ready = s_tim_ready;
    strncpy(chat->response,
            s_tim_ready ? "Hey VIP! Type Here ^_^" : "TIM init failed.",
            sizeof(chat->response) - 1U);
}

static void SyncCurrentLine(void)
{
    uint32_t n = s_line_len;
    if (n >= sizeof(s_chat->current_input))
    {
        n = sizeof(s_chat->current_input) - 1U;
    }
    memcpy(s_chat->current_input, s_line, n);
    s_chat->current_input[n] = '\0';
}

static void SubmitLine(const char *text)
{
    int32_t intent = TIM_INTENT_UNKNOWN;
    float conf = 0.0f;

    if ((text == NULL) || (text[0] == '\0'))
    {
        return;
    }

    for (uint32_t i = 0U; i < (TIM_CHAT_HISTORY_COUNT - 1U); i++)
    {
        s_chat->history[i] = s_chat->history[i + 1U];
    }
    memset(&s_chat->history[TIM_CHAT_HISTORY_COUNT - 1U], 0, sizeof(s_chat->history[0]));

    strncpy(s_chat->input, text, sizeof(s_chat->input) - 1U);
    s_chat->ready = s_tim_ready;
    s_chat->has_message = 1U;

    if (TIM_Predict(text, &intent, &conf) == 0)
    {
        s_chat->intent_id = intent;
        s_chat->confidence = conf;
        strncpy(s_chat->response, TIM_ResponseForIntent(intent), sizeof(s_chat->response) - 1U);
    }
    else
    {
        s_chat->intent_id = TIM_INTENT_UNKNOWN;
        s_chat->confidence = 0.0f;
        strncpy(s_chat->response, "TIM model is not ready.", sizeof(s_chat->response) - 1U);
    }

    TIM_ChatTurn_t *turn = &s_chat->history[TIM_CHAT_HISTORY_COUNT - 1U];
    strncpy(turn->input, s_chat->input, sizeof(turn->input) - 1U);
    strncpy(turn->response, s_chat->response, sizeof(turn->response) - 1U);
    turn->intent_id = s_chat->intent_id;
    turn->confidence = s_chat->confidence;
    turn->valid = 1U;
    s_chat->turn_count++;

    printf("[TIM] \"%s\" -> %ld %s (%.1f%%): %s\r\n",
           s_chat->input,
           (long)s_chat->intent_id,
           TIM_IntentName(s_chat->intent_id),
           (double)(s_chat->confidence * 100.0f),
           s_chat->response);
}

void TIM_AppInit(void)
{
    s_tim_ready = (TIM_Inference_Init() == 0) ? 1U : 0U;

    InitChatState(&s_guest_chat);
    for (uint32_t i = 0U; i < FACE_STORE_MAX_RECORDS; i++)
    {
        InitChatState(&s_user_chats[i]);
    }
    TIM_AppClearUser();

    printf(s_tim_ready ?
           "[TIM] ready. Type a message and press Enter.\r\n" :
           "[TIM] init failed.\r\n");
}

void TIM_AppSetUser(int32_t user_idx)
{
    if ((user_idx >= 0) && (user_idx < (int32_t)FACE_STORE_MAX_RECORDS))
    {
        s_chat = &s_user_chats[user_idx];
    }
    else
    {
        s_chat = &s_guest_chat;
    }

    s_line_len = 0U;
    SyncCurrentLine();
}

void TIM_AppClearUser(void)
{
    s_chat = &s_guest_chat;
    s_line_len = 0U;
    SyncCurrentLine();
}

void TIM_AppPoll(void)
{
    uint8_t ch = 0U;

    while (HAL_UART_Receive(&huart1, &ch, 1U, 0U) == HAL_OK)
    {
        if ((ch == '\r') || (ch == '\n'))
        {
            if (s_line_len > 0U)
            {
                s_line[s_line_len] = '\0';
                SubmitLine(s_line);
                s_line_len = 0U;
                SyncCurrentLine();
            }
        }
        else if ((ch == 0x08U) || (ch == 0x7FU))
        {
            if (s_line_len > 0U)
            {
                s_line_len--;
                SyncCurrentLine();
            }
        }
        else if ((ch >= 32U) && (ch <= 126U))
        {
            if (s_line_len < (sizeof(s_line) - 1U))
            {
                s_line[s_line_len++] = (char)ch;
                SyncCurrentLine();
            }
        }
    }
}

const TIM_ChatState_t *TIM_AppGetState(void)
{
    return s_chat;
}

void TIM_AppSubmitText(const char *text)
{
    SubmitLine(text);
}

void TIM_AppInputChar(char c)
{
    if (s_line_len < (sizeof(s_line) - 1U))
    {
        s_line[s_line_len++] = c;
        SyncCurrentLine();
    }
}

void TIM_AppBackspace(void)
{
    if (s_line_len > 0U)
    {
        s_line_len--;
        SyncCurrentLine();
    }
}

void TIM_AppClearInput(void)
{
    s_line_len = 0U;
    SyncCurrentLine();
}

void TIM_AppSubmitCurrentInput(void)
{
    if (s_line_len > 0U)
    {
        s_line[s_line_len] = '\0';
        SubmitLine(s_line);
        s_line_len = 0U;
        SyncCurrentLine();
    }
}
