#include "app_ui.h"
#include "app_touch.h"
#include "stm32_lcd.h"
#include "stm32n6570_discovery.h"
#include "stm32n6xx_hal.h"
#include "app_config.h"
#include "face_store.h"
#include "face_recog.h"
#include "app_depth.h"
#include "tim_app.h"
#include "tim_inference.h"
#include <string.h>
#include <stdio.h>


AppState_t app_state = APP_STATE_SPLASH;
uint32_t state_entry_time = 0;

static int32_t session_user_idx = -1;
static uint8_t session_active = 0U;

extern const char *classes_table[];


extern int32_t last_recog_idx;
extern float   last_recog_score;
extern float   last_det_conf;
extern float   last_depth_match_score;
extern uint8_t last_depth_template_seen;
extern uint32_t g_frame_output_ms;
extern uint32_t g_cpu_frame_ms;
extern uint32_t g_npu_infer_ms;
extern uint32_t g_embed_npu_ms;
extern uint32_t g_depth_npu_ms;
extern uint32_t g_hw_cpu_pct;
extern uint32_t g_hw_npu_run_pct;

#define PERF_OVERLAY_UPDATE_PERIOD_MS  500U


volatile uint8_t g_enroll_requested = 0;
volatile uint8_t g_enroll_done_flag = 0;
volatile uint8_t g_enroll_fail_flag = 0;
volatile uint8_t g_enroll_duplicate_flag = 0;
char g_enroll_name[FACE_STORE_NAME_LEN] = {0};
char g_enroll_fail_reason[64] = {0};

UI_Person_t ui_persons[UI_MAX_PERSONS];
uint8_t ui_person_count = 0;

static uint32_t prev_btn = 0;
static uint32_t idle_ts = 0;
static uint32_t backoff_ts = 0;
static uint32_t multi_person_ts = 0;
static char pending_enroll_name[FACE_STORE_NAME_LEN];
static int32_t pending_rename_idx = -1;
static int32_t pending_delete_idx = -1;
static uint8_t pending_admin_value = 0U;
static uint32_t settings_denied_ts = 0U;
static uint32_t tim_handled_turn_count = 0U;

void DrawText(uint32_t x, uint32_t y, uint8_t *pText,
              Text_AlignModeTypdef Mode, sFONT *fonts,
              uint32_t text_color, uint32_t back_color);


static const uint32_t bbox_colors[6] = {
    0xFF00FF00, 0xFFFF0000, 0xFF0000FF,
    0xFFFFFF00, 0xFFFF00FF, 0xFF00FFFF
};


static inline void Panel(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                         uint32_t fill, uint32_t border)
{
    UTIL_LCD_FillRect(x, y, w, h, fill);
    UTIL_LCD_DrawRect(x, y, w, h, border);
}

static inline uint8_t ButtonPressed(void)
{
    uint32_t now = HAL_GPIO_ReadPin(BUTTON_USER1_GPIO_PORT,
                                    BUTTON_USER1_PIN);

    uint8_t pressed = (now && !prev_btn);
    prev_btn = now;
    return pressed;
}

static void DrawDetectionBoxes(const od_pp_outBuffer_t *boxes,
                               uint32_t count,
                               uint32_t area_x,
                               uint32_t area_y,
                               uint32_t area_w,
                               uint32_t area_h)
{
    if ((boxes == NULL) || (count == 0U) || (area_w == 0U) || (area_h == 0U))
    {
        return;
    }

    for (uint32_t i = 0U; i < count; i++)
    {
        float left = (boxes[i].x_center - (boxes[i].width * 0.5f)) * (float)area_w;
        float top = (boxes[i].y_center - (boxes[i].height * 0.5f)) * (float)area_h;
        float right = left + (boxes[i].width * (float)area_w);
        float bottom = top + (boxes[i].height * (float)area_h);

        if (left < 0.0f) left = 0.0f;
        if (top < 0.0f) top = 0.0f;
        if (right > (float)area_w) right = (float)area_w;
        if (bottom > (float)area_h) bottom = (float)area_h;

        if ((right <= left) || (bottom <= top))
        {
            continue;
        }

        uint32_t x = area_x + (uint32_t)left;
        uint32_t y = area_y + (uint32_t)top;
        uint32_t w = (uint32_t)(right - left);
        uint32_t h = (uint32_t)(bottom - top);
        uint32_t col = bbox_colors[i % 6U];

        UTIL_LCD_DrawRect(x, y, w, h, col);
        if ((w > 4U) && (h > 4U))
        {
            UTIL_LCD_DrawRect(x + 1U, y + 1U, w - 2U, h - 2U, col);
        }
    }
}

static uint32_t DepthHeatColor(uint8_t v)
{
    uint32_t r = (uint32_t)v;
    uint32_t g = (uint32_t)((v > 96U) ? 220U : (v * 220U) / 96U);
    uint32_t b = (uint32_t)(255U - v);
    return 0xFF000000U | (r << 16) | (g << 8) | b;
}

static void DrawDepthPreviewMap(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    UTIL_LCD_FillRect(x, y, w, h, 0xFF101820U);
    UTIL_LCD_DrawRect(x, y, w, h, 0xFF405060U);

    if (g_depth_preview_ready == 0U)
    {
        DrawText(x + 14U, y + (h / 2U) - 6U,
                 (uint8_t*)"Waiting for depth",
                 LEFT_MODE, &Font12, 0xFF8090A0U, 0xFF101820U);
        return;
    }

    for (uint32_t py = 0U; py < h; py += 2U)
    {
        uint32_t sy = (py * DEPTH_PREVIEW_H) / h;
        for (uint32_t px = 0U; px < w; px += 2U)
        {
            uint32_t sx = (px * DEPTH_PREVIEW_W) / w;
            uint8_t v = g_depth_preview[sy * DEPTH_PREVIEW_W + sx];
            UTIL_LCD_FillRect(x + px, y + py, 2U, 2U, DepthHeatColor(v));
        }
    }
    UTIL_LCD_DrawRect(x, y, w, h, 0xFF8DEBFFU);
}

static void DrawLiveCheckRow(uint32_t y,
                             const char *name,
                             const char *detail,
                             uint8_t ok,
                             uint8_t active)
{
    const uint32_t fill = ok ? 0xDD103020U :
                          (active ? 0xDD302400U : 0xDD181820U);
    const uint32_t border = ok ? 0xFF00FF88U :
                            (active ? 0xFFFFCC00U : 0xFF405060U);
    const uint32_t state_col = ok ? 0xFF00FF88U :
                              (active ? 0xFFFFCC00U : 0xFF8090A0U);
    const char *state = ok ? "[OK]" : (active ? "[RUN]" : "[WAIT]");

    Panel(170U, y, 460U, 42U, fill, border);
    DrawText(190U, y + 12U, (uint8_t*)state,
             LEFT_MODE, &Font16, state_col, fill);
    DrawText(260U, y + 8U, (uint8_t*)name,
             LEFT_MODE, &Font16, 0xFFFFFFFFU, fill);
    DrawText(260U, y + 26U, (uint8_t*)detail,
             LEFT_MODE, &Font12, 0xFFB8E8FFU, fill);
}

static void ResetPendingEnrollName(void)
{
    snprintf(pending_enroll_name,
             sizeof(pending_enroll_name),
             "Person %lu",
             (unsigned long)(FaceStore_Count() + 1U));
}

static void SetPendingName(const char *name)
{
    memset(pending_enroll_name, 0, sizeof(pending_enroll_name));
    if ((name != NULL) && (name[0] != '\0'))
    {
        strncpy(pending_enroll_name, name, sizeof(pending_enroll_name) - 1U);
    }
    else
    {
        ResetPendingEnrollName();
    }
}

static void AddPendingNameChar(char c)
{
    size_t len = strlen(pending_enroll_name);
    if (len < (sizeof(pending_enroll_name) - 1U))
    {
        pending_enroll_name[len] = c;
        pending_enroll_name[len + 1U] = '\0';
    }
}

static void BackspacePendingName(void)
{
    size_t len = strlen(pending_enroll_name);
    if (len > 0U)
    {
        pending_enroll_name[len - 1U] = '\0';
    }
}

static uint8_t SessionIsAdmin(void)
{
    return session_active &&
           (session_user_idx >= 0) &&
           FaceStore_IsAdmin((uint32_t)session_user_idx);
}

static int32_t FirstDeletableUserIndex(void)
{
    int32_t found = -1;

    for (uint32_t i = 0U; i < FACE_STORE_MAX_RECORDS; i++)
    {
        const FaceStore_Record_t *rec = FaceStore_Get(i);
        if ((rec == NULL) || (rec->active == 0U) || FaceStore_IsAdmin(i))
        {
            continue;
        }

        if (found >= 0)
        {
            return -1;
        }
        found = (int32_t)i;
    }

    return found;
}

static void LockSession(uint32_t now)
{
    session_active = 0U;
    session_user_idx = -1;
    TIM_AppClearUser();
    tim_handled_turn_count = TIM_AppGetState()->turn_count;
    idle_ts = 0U;
    multi_person_ts = 0U;
    app_state = APP_STATE_AUTH;
    state_entry_time = now;
}

static void StartSession(int32_t user_idx)
{
    session_user_idx = user_idx;
    session_active = 1U;
    TIM_AppSetUser(user_idx);
    tim_handled_turn_count = TIM_AppGetState()->turn_count;
}

static void ExecuteTimIntent(const TIM_ChatTurn_t *turn, uint32_t now)
{
    if ((turn == NULL) || (turn->valid == 0U) || (turn->confidence < 0.70f))
    {
        return;
    }

    switch ((TimIntent_t)turn->intent_id)
    {
    case TIM_INTENT_HELP:
    case TIM_INTENT_SYSTEM_STATUS:
    case TIM_INTENT_SHOW_RUNTIME:
    case TIM_INTENT_LIST_USERS:
        app_state = APP_STATE_SETTINGS;
        state_entry_time = now;
        break;

    case TIM_INTENT_LOCK:
    case TIM_INTENT_LOGOUT:
        LockSession(now);
        break;

    case TIM_INTENT_UNLOCK:
        if ((last_recog_idx >= 0) && (FaceStore_Get((uint32_t)last_recog_idx) != NULL))
        {
            StartSession(last_recog_idx);
            app_state = APP_STATE_MAIN;
            state_entry_time = now;
        }
        else
        {
            app_state = APP_STATE_AUTH;
            state_entry_time = now;
        }
        break;

    case TIM_INTENT_ENROLL_FACE:
        if (((session_active != 0U) || (FaceStore_Count() == 0U)) &&
            (turn->confidence >= 0.80f))
        {
            pending_rename_idx = -1;
            ResetPendingEnrollName();
            app_state = APP_STATE_ENROLL_NAME;
            state_entry_time = now;
        }
        break;

    case TIM_INTENT_DELETE_USER:
        if (SessionIsAdmin() && (turn->confidence >= 0.85f))
        {
            pending_delete_idx = FirstDeletableUserIndex();
            app_state = APP_STATE_SETTINGS;
            state_entry_time = now;
        }
        break;

    case TIM_INTENT_RESET_SYSTEM:
        if (SessionIsAdmin() && (turn->confidence >= 0.90f))
        {
            NVIC_SystemReset();
        }
        break;

    case TIM_INTENT_VISION_QUERY:
        app_state = session_active ? APP_STATE_MAIN : APP_STATE_AUTH;
        state_entry_time = now;
        break;

    default:
        break;
    }
}

static void ProcessTimTriggers(uint32_t now)
{
    const TIM_ChatState_t *chat = TIM_AppGetState();
    if ((chat == NULL) || (chat->turn_count == tim_handled_turn_count))
    {
        return;
    }

    const TIM_ChatTurn_t *turn = &chat->history[TIM_CHAT_HISTORY_COUNT - 1U];
    tim_handled_turn_count = chat->turn_count;
    ExecuteTimIntent(turn, now);
}


void UI_Init(void)
{
    app_state = APP_STATE_SPLASH;
    state_entry_time = HAL_GetTick();

    idle_ts = 0U;
    backoff_ts = 0U;
    multi_person_ts = 0U;
    tim_handled_turn_count = 0U;

    BUTTON_USER1_GPIO_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = BUTTON_USER1_PIN;
    gpio.Mode  = GPIO_MODE_INPUT;
    gpio.Pull  = GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUTTON_USER1_GPIO_PORT, &gpio);

    prev_btn = HAL_GPIO_ReadPin(BUTTON_USER1_GPIO_PORT, BUTTON_USER1_PIN);

    session_user_idx = -1;
    session_active = 0U;
    ResetPendingEnrollName();

    memset(ui_persons, 0, sizeof(ui_persons));
    strcpy(ui_persons[0].name, "Admin");
    ui_persons[0].active = 1;
    ui_person_count = 1;
}


void DrawText(uint32_t x, uint32_t y, uint8_t *pText,
              Text_AlignModeTypdef Mode, sFONT *fonts,
              uint32_t text_color, uint32_t back_color)
{
    UTIL_LCD_SetTextColor(text_color);
    UTIL_LCD_SetBackColor(back_color);
    UTIL_LCD_SetFont(fonts);
    UTIL_LCD_DisplayStringAt(x, y, pText, Mode);
}


static void DrawSplash(void)
{
    const uint32_t bg = 0xFF0000FFU;

    UTIL_LCD_FillRect(0U, 0U, UI_SCREEN_W, UI_SCREEN_H, bg);

    DrawText(0U, 150U, (uint8_t*)"Hello! There :)",
             CENTER_MODE, &Font24, 0xFFFFFFFFU, bg);


    uint32_t elapsed = HAL_GetTick() - state_entry_time;
    uint32_t w = (elapsed * 400U) / 3000U;
    if (w > 400U) w = 400U;

    UTIL_LCD_FillRect(200U, 250U, 400U, 10U, 0xFF333333U);
    UTIL_LCD_FillRect(200U, 250U, w,    10U, 0xFF00FFFFU);
}


static void DrawAuthSuccess(void)
{
    const uint32_t bg = 0xEE0A0A12U;
    const char *who = "User";
    uint32_t elapsed = HAL_GetTick() - state_entry_time;
    uint8_t face_ok = (last_det_conf >= FACE_RECOG_MIN_DET_CONF) ? 1U : 0U;
    uint8_t identity_ok = ((last_recog_idx >= 0) && session_active) ? 1U : 0U;
    uint8_t depth_active = (g_depth_npu_ms > 0U) ? 1U : 0U;
    float depth_display_score = (last_depth_template_seen != 0U) ?
                                last_depth_match_score :
                                g_depth_live_score;
    uint8_t depth_ok = ((g_depth_preview_ready != 0U) &&
                        ((last_depth_template_seen != 0U) ?
                         (last_depth_match_score >= FACE_DEPTH_MATCH_THRESHOLD) :
                         (g_depth_live_score >= 0.20f))) ? 1U : 0U;

    if (session_active && (session_user_idx >= 0))
    {
        const FaceStore_Record_t *rec = FaceStore_Get((uint32_t)session_user_idx);
        if (rec != NULL)
        {
            who = rec->name;
        }
    }

    UTIL_LCD_FillRect(0U, 0U, UI_SCREEN_W, UI_SCREEN_H, bg);

    DrawText(0U, 72U, (uint8_t*)"Access granted",
             CENTER_MODE, &Font24, 0xFF00FF88U, bg);

    char hello_buf[64];
    snprintf(hello_buf, sizeof(hello_buf), "Hello, %s", who);
    DrawText(0U, 116U, (uint8_t*)hello_buf,
             CENTER_MODE, &Font20, 0xFFFFFFFFU, bg);

    char face_detail[64];
    char identity_detail[64];
    char depth_detail[64];

    snprintf(face_detail, sizeof(face_detail),
             "BlazeFace confidence %.0f%%",
             last_det_conf * 100.0f);
    snprintf(identity_detail, sizeof(identity_detail),
             "Face embedding similarity %.0f%%",
             last_recog_score * 100.0f);
    snprintf(depth_detail, sizeof(depth_detail),
             "%s %.0f%%, %lums",
             (last_depth_template_seen != 0U) ? "Stored depth match" : "FastDepth shape",
             depth_display_score * 100.0f,
             (unsigned long)g_depth_npu_ms);

    DrawLiveCheckRow(164U,
                     "Face localization",
                     face_detail,
                     face_ok,
                     1U);
    DrawLiveCheckRow(216U,
                     "Identity embedding match",
                     identity_detail,
                     identity_ok,
                     face_ok);
    DrawLiveCheckRow(268U,
                     "Monocular depth consistency",
                     depth_detail,
                     depth_ok,
                     depth_active);

    DrawText(0U, 332U, (uint8_t*)"Live security handoff",
             CENTER_MODE, &Font16, 0xFF8DEBFFU, bg);

    uint32_t w = (elapsed * 400U) / 3000U;
    if (w > 400U) w = 400U;

    UTIL_LCD_FillRect(200U, 365U, 400U, 12U, 0xFF333333U);
    UTIL_LCD_FillRect(200U, 365U, w,    12U, 0xFF00FFFFU);
    UTIL_LCD_DrawRect(200U, 365U, 400U, 12U, 0xFF00FFFFU);
}


static void DrawAuth(od_pp_out_t *pp)
{
    const uint32_t bg = 0x00000000U;
    uint32_t nb = pp->nb_detect;
    od_pp_outBuffer_t *r = pp->pOutBuff;

    UTIL_LCD_FillRect(0U, 0U, UI_SCREEN_W, UI_SCREEN_H, bg);

    if ((r != NULL) && (nb > 0U))
    {
        DrawDetectionBoxes(r, nb, 0U, 0U, UI_SCREEN_W, UI_SCREEN_H);
    }
    else
    {

        UTIL_LCD_DrawRect(270U, 100U, 260U, 260U, 0xFF00FFFFU);
    }

    DrawText(0U, 40U, (uint8_t*)"FACE AUTHENTICATION",
             CENTER_MODE, &Font20, 0xFF00FFFFU, bg);

    DrawText(0U, 380U, (uint8_t*)"Scan your face or ID key",
             CENTER_MODE, &Font20, 0xFFFFFFFFU, bg);

    const char *who = "Unknown";
    uint32_t status_col = 0xFF888888U;
    char line1[60];
    char line2[65];
    char line3[70];

    if (nb == 0U)
    {
        snprintf(line1, sizeof(line1), "Status: Waiting for face...");
        snprintf(line2, sizeof(line2), "Place one face inside box");
        snprintf(line3, sizeof(line3), " ");
        status_col = 0xFF888888U;
    }
    else if (nb >= 2U)
    {
        snprintf(line1, sizeof(line1), "Status: Multiple objects detected");
        snprintf(line2, sizeof(line2), "Only one face should be visible");
        snprintf(line3, sizeof(line3), " ");
         	 status_col = 0xFFFF6600U;
    }
    else
    {
        if (FaceStore_Count() == 0U)
        {
            snprintf(line1, sizeof(line1), "Status: No enrolled faces");
            snprintf(line2, sizeof(line2), "Press USER to enroll first face");
            snprintf(line3, sizeof(line3), "Detection %.0f%% is working",
                     ((r != NULL) && (nb > 0U)) ? (r[0].conf * 100.0f) : 0.0f);
            status_col = 0xFFFFCC00U;
        }
        else if (!FaceRecog_IsReady())
        {
            snprintf(line1, sizeof(line1), "Status: Recognition disabled");
            snprintf(line2, sizeof(line2), "Face engine not ready");
            snprintf(line3, sizeof(line3), " ");
            status_col = 0xFFFFCC00U;
        }
        else if (last_recog_idx >= 0)
        {
            const FaceStore_Record_t *rec = FaceStore_Get((uint32_t)last_recog_idx);
            if (rec != NULL)
            {
                who = rec->name;
                snprintf(line1, sizeof(line1), "Status: Access granted");
                snprintf(line2, sizeof(line2), "Welcome, %s", who);
                snprintf(line3, sizeof(line3), "Identity match %.0f%%  Depth shape %.0f%%",
                         last_recog_score * 100.0f,
                         ((last_depth_template_seen != 0U) ?
                          last_depth_match_score :
                          g_depth_live_score) * 100.0f);
                status_col = 0xFF00FF88U;
            }
            else
            {
                snprintf(line1, sizeof(line1), "Status: Authenticating...");
                snprintf(line2, sizeof(line2), "Face matched index invalid");
                snprintf(line3, sizeof(line3), " ");
                status_col = 0xFFFFCC00U;
            }
        }
        else
        {
            float det_conf = 0.0f;
            if ((r != NULL) && (nb > 0U))
            {
                det_conf = r[0].conf * 100.0f;
            }

            snprintf(line1, sizeof(line1), "Status: Face detected");
            snprintf(line2, sizeof(line2), "Authenticating...");
            snprintf(line3, sizeof(line3), "Det %.0f%%  ID %.0f%%  Depth %.0f%%",
                     det_conf,
                     last_recog_score * 100.0f,
                     ((last_depth_template_seen != 0U) ?
                      last_depth_match_score :
                      g_depth_live_score) * 100.0f);
            status_col = 0xFF00CFFFU;
        }
    }

    DrawText(0U, 420U, (uint8_t*)line1,
             CENTER_MODE, &Font20, status_col, bg);
    DrawText(0U, 450U, (uint8_t*)line2,
             CENTER_MODE, &Font16, 0xFFFFFFFFU, bg);
    DrawText(0U, 475U, (uint8_t*)line3,
             CENTER_MODE, &Font16, 0xFF8090A0U, bg);
}


static void DrawMain(od_pp_out_t *pp, UI_BgArea_t *bg)
{
    const uint32_t screen_bg = 0x00000000U;
    const uint32_t panel_bg = 0xDD0A0A12U;
    const uint32_t chat_bg = 0xEE101820U;
    uint32_t nb = pp->nb_detect;
    od_pp_outBuffer_t *r = pp->pOutBuff;

    UTIL_LCD_FillRect(0U, 0U, UI_SCREEN_W, UI_SCREEN_H, screen_bg);


    Panel(0U, 0U, 534U, 432U, panel_bg, 0xFF00FFFFU);

    uint32_t cam_x = 520U;
    uint32_t cam_y = 16U;
    uint32_t cam_w = 256U;
    uint32_t cam_h = 212U;


    if (bg != NULL)
    {
        cam_x = bg->X0;
        cam_y = bg->Y0;
        cam_w = bg->XSize;
        cam_h = bg->YSize;
    }

    UTIL_LCD_DrawRect(cam_x - 2U, cam_y - 2U, cam_w + 4U, cam_h + 4U, 0xFF00FFFFU);

    DrawDetectionBoxes(r, nb, cam_x, cam_y, cam_w, cam_h);

    const char *who = "User";
    uint32_t name_col = 0xFF00FF88U;
    char line2[64];

    if (session_active && (session_user_idx >= 0))
    {
        const FaceStore_Record_t *rec = FaceStore_Get((uint32_t)session_user_idx);
        if (rec != NULL)
        {
            who = rec->name;
        }
    }

    char namebuf[48];
    snprintf(namebuf, sizeof(namebuf), "Welcome: %s", who);
    if ((r != NULL) && (nb > 0U))
    {
        snprintf(line2, sizeof(line2),
                 "Face %.0f%%  Identity %.0f%%  Depth %.0f%%",
                 r[0].conf * 100.0f,
                 last_recog_score * 100.0f,
                 ((last_depth_template_seen != 0U) ?
                  last_depth_match_score :
                  g_depth_live_score) * 100.0f);
    }
    else
    {
        snprintf(line2, sizeof(line2),
                 "Face --  Identity %.0f%%  Depth %.0f%%",
                 last_recog_score * 100.0f,
                 ((last_depth_template_seen != 0U) ?
                  last_depth_match_score :
                  g_depth_live_score) * 100.0f);
    }

    uint32_t depth_y = cam_y + cam_h;
    uint32_t depth_h = cam_h;
    if ((depth_y + depth_h) <= 440U)
    {
        DrawDepthPreviewMap(cam_x, depth_y, cam_w, depth_h);
    }


    Panel(10U, 20U, 514U, 82U, panel_bg, 0xFF00FFFFU);
    DrawText(26U, 32U, (uint8_t*)"MAIN ACCESS PANEL",
             LEFT_MODE, &Font16, 0xFF00FFFFU, panel_bg);
    DrawText(26U, 56U, (uint8_t*)"Access granted.",
             LEFT_MODE, &Font16, 0xFF00FF88U, panel_bg);
    DrawText(208U, 56U, (uint8_t*)namebuf,
             LEFT_MODE, &Font16, name_col, panel_bg);
    DrawText(26U, 80U, (uint8_t*)line2,
             LEFT_MODE, &Font12, 0xFFFFFFFFU, panel_bg);

    const TIM_ChatState_t *chat = TIM_AppGetState();
    Panel(10U, 112U, 514U, 310U, chat_bg, 0xFF2FA8CCU);
    DrawText(36U, 126U, (uint8_t*)"Ask TIM",
             LEFT_MODE, &Font16, 0xFF8DEBFFU, chat_bg);
    DrawText(198U, 128U, (uint8_t*)"Secure workspace active",
             LEFT_MODE, &Font12, 0xFF00FF88U, chat_bg);

    if ((chat != NULL) && (chat->turn_count > TIM_CHAT_HISTORY_COUNT))
    {
        char scroll_line[32];
        snprintf(scroll_line, sizeof(scroll_line), "%lu older",
                 (unsigned long)(chat->turn_count - TIM_CHAT_HISTORY_COUNT));
        DrawText(390U, 128U, (uint8_t*)scroll_line,
                 LEFT_MODE, &Font12, 0xFF8090A0U, chat_bg);
    }

    uint32_t y = 156U;
    uint8_t drew_turn = 0U;

    if (chat != NULL)
    {
        uint32_t first_turn = (TIM_CHAT_HISTORY_COUNT > 2U) ?
                              (TIM_CHAT_HISTORY_COUNT - 2U) : 0U;
        for (uint32_t i = first_turn; i < TIM_CHAT_HISTORY_COUNT; i++)
        {
            const TIM_ChatTurn_t *turn = &chat->history[i];
            if (turn->valid == 0U)
            {
                continue;
            }

            char user_line[96];
            char meta_line[96];
            char response_line[96];

            snprintf(user_line, sizeof(user_line), "%.64s", turn->input);
            snprintf(meta_line, sizeof(meta_line), "TIM  %s %.0f%%",
                     TIM_IntentName(turn->intent_id),
                     (double)(turn->confidence * 100.0f));
            snprintf(response_line, sizeof(response_line), "%.64s", turn->response);

            DrawText(398U, y, (uint8_t*)"You",
                     LEFT_MODE, &Font12, 0xFF8DEBFFU, chat_bg);
            Panel(126U, y + 16U, 308U, 30U, 0xFF182028U, 0xFF405060U);
            DrawText(140U, y + 25U, (uint8_t*)user_line,
                     LEFT_MODE, &Font12, 0xFFFFFFFFU, 0xFF182028U);

            DrawText(36U, y + 50U, (uint8_t*)meta_line,
                     LEFT_MODE, &Font12, 0xFF00FF88U, chat_bg);
            Panel(36U, y + 66U, 350U, 30U, 0xFF102820U, 0xFF207050U);
            DrawText(50U, y + 75U, (uint8_t*)response_line,
                     LEFT_MODE, &Font12, 0xFFB8E8FFU, 0xFF102820U);
            y += 100U;
            drew_turn = 1U;
        }
    }

    if (drew_turn == 0U)
    {
        Panel(36U, 176U, 350U, 44U, 0xFF102820U, 0xFF207050U);
        DrawText(50U, 190U, (uint8_t*)"Tap below and type a message.",
                 LEFT_MODE, &Font12, 0xFFB8E8FFU, 0xFF102820U);
     }

    Panel(32U, 364U, 406U, 44U, 0xFF0B1118U, 0xFF405060U);
    char input_line[112];
    const char *typed = ((chat != NULL) && (chat->current_input[0] != '\0')) ?
                        chat->current_input : "Hey VIP! Type Here ^_^";
    snprintf(input_line, sizeof(input_line), "> %.82s", typed);
    DrawText(48U, 382U, (uint8_t*)input_line,
             LEFT_MODE, &Font12,
             ((chat != NULL) && (chat->current_input[0] != '\0')) ? 0xFFFFFFFFU : 0xFF8090A0U,
             0xFF0B1118U);

    Panel(0U, 440U, 800U, 40U, 0xEE111111U, 0xFF00FFFFU);

    DrawText(20U, 452U, (uint8_t*)"+ ADD",
             LEFT_MODE, &Font16, 0xFF00FFFFU, 0xEE111111U);

    DrawText(320U, 452U, (uint8_t*)"SETTINGS",
             LEFT_MODE, &Font16, 0xFF00FFFFU, 0xEE111111U);

    DrawText(650U, 452U, (uint8_t*)"LOGOUT",
             LEFT_MODE, &Font16, 0xFF00FFFFU, 0xEE111111U);


    if (g_enroll_done_flag)
    {
        Panel(120U, 200U, 560U, 80U, 0xDD003020U, 0xFF00FF88U);
        DrawText(0U, 228U, (uint8_t*)"Face enrolled & saved to flash",
                 CENTER_MODE, &Font20, 0xFF00FF88U, 0xDD003020U);
    }
    else if (g_enroll_duplicate_flag)
    {
        Panel(120U, 200U, 560U, 80U, 0xDD302000U, 0xFFFFCC00U);
        DrawText(0U, 228U, (uint8_t*)"This face is already enrolled",
                 CENTER_MODE, &Font20, 0xFFFFCC00U, 0xDD302000U);
    }
    else if (g_enroll_fail_flag)
    {
        const char *msg = (g_enroll_fail_reason[0] != '\0') ?
                          g_enroll_fail_reason :
                          "Enroll failed";
        Panel(120U, 200U, 560U, 80U, 0xDD200000U, 0xFFFF4444U);
        DrawText(0U, 228U, (uint8_t*)msg,
                CENTER_MODE, &Font20, 0xFFFF4444U, 0xDD200000U);
    }
    else if (g_enroll_requested)
    {
        Panel(120U, 200U, 560U, 80U, 0xDD001830U, 0xFF00CFFFU);
        DrawText(0U, 228U, (uint8_t*)"Enrolling... hold still",
                 CENTER_MODE, &Font20, 0xFF00CFFFU, 0xDD001830U);
    }
}


static void DrawBackoff(uint32_t nb)
{
    const uint32_t bg = 0xDD000000U;
    UTIL_LCD_FillRect(0U, 0U, UI_SCREEN_W, UI_SCREEN_H, bg);

    DrawText(0U, 150U, (uint8_t*)"!! WARNING !!",
             CENTER_MODE, &Font24, 0xFFFF0000U, bg);

    char buf[64];
    sprintf(buf, "%lu persons detected", (unsigned long)nb);

    DrawText(0U, 200U, (uint8_t*)buf,
             CENTER_MODE, &Font20, 0xFFFFFFFFU, bg);
}


static void DrawSettings(void)
{
    const uint32_t bg = 0xFF0A0A14U;

    UTIL_LCD_FillRect(0U, 0U, UI_SCREEN_W, UI_SCREEN_H, bg);

    Panel(0U, 0U, UI_SCREEN_W, 50U, 0xFF111122U, 0xFF00FFFFU);
    DrawText(0U, 14U, (uint8_t*)"SETTINGS", CENTER_MODE,
             &Font24, 0xFF00FFFFU, 0xFF111122U);

    Panel(10U, 60U, 460U, 360U, 0xFF111118U, 0xFF334455U);
    DrawText(20U, 70U, (uint8_t*)"Authorized Persons",
             LEFT_MODE, &Font20, 0xFF00FFFFU, 0xFF111118U);
    UTIL_LCD_DrawHLine(10U, 95U, 460U, 0xFF334455U);

    uint8_t shown = 0U;
    for (int i = 0; i < UI_MAX_PERSONS; i++)
    {
        uint32_t row_y = 105U + (uint32_t)i * 36U;

        const FaceStore_Record_t *rec = FaceStore_Get((uint32_t)i);

        if (rec && rec->active)
        {
            Panel(20U, row_y, 440U, 30U, 0xFF003315U, 0xFF00AA44U);
            DrawText(32U, row_y + 7U, (uint8_t*)rec->name,
                     LEFT_MODE, &Font16, 0xFF00FF88U, 0xFF003315U);
            if (FaceStore_IsAdmin((uint32_t)i))
            {
                DrawText(300U, row_y + 7U, (uint8_t*)"ADMIN",
                         LEFT_MODE, &Font16, 0xFFFFD600U, 0xFF003315U);
            }
            if (SessionIsAdmin())
            {
                Panel(380U, row_y, 80U, 30U, 0xFF401010U, 0xFFFF4444U);
                DrawText(390U, row_y + 7U, (uint8_t*)"DELETE",
                         LEFT_MODE, &Font16, 0xFFFFAAAAU, 0xFF401010U);
            }
            shown++;
        }
        else if (ui_persons[i].active)
        {
            Panel(20U, row_y, 440U, 30U, 0xFF003315U, 0xFF00AA44U);
            DrawText(32U, row_y + 7U, (uint8_t*)ui_persons[i].name,
                     LEFT_MODE, &Font16, 0xFF00FF88U, 0xFF003315U);
            shown++;
        }
        else
        {
            Panel(20U, row_y, 440U, 30U, 0xFF111118U, 0xFF223333U);
            DrawText(32U, row_y + 7U, (uint8_t*)"(empty slot)",
                     LEFT_MODE, &Font16, 0xFF334455U, 0xFF111118U);
        }
    }

    char cnt[64];
    sprintf(cnt, "%d / %d slots used (%lu persisted)",
            (int)shown, (int)UI_MAX_PERSONS,
            (unsigned long)FaceStore_Count());
    DrawText(20U, 395U, (uint8_t*)cnt, LEFT_MODE,
             &Font16, 0xFF607080U, 0xFF111118U);

    if ((settings_denied_ts != 0U) &&
        ((HAL_GetTick() - settings_denied_ts) < 2000U))
    {
        Panel(40U, 360U, 390U, 42U, 0xFF301010U, 0xFFFF4444U);
        DrawText(56U, 373U, (uint8_t*)"Only admin can edit other names",
                 LEFT_MODE, &Font16, 0xFFFFAAAAU, 0xFF301010U);
    }

    Panel(480U, 60U, 310U, 360U, 0xFF111118U, 0xFF334455U);
    DrawText(490U, 70U, (uint8_t*)"System Info",
             LEFT_MODE, &Font20, 0xFF00FFFFU, 0xFF111118U);
    UTIL_LCD_DrawHLine(480U, 95U, 310U, 0xFF334455U);

    uint32_t uptime_s = HAL_GetTick() / 1000U;
    char info[48];

    sprintf(info, "Uptime: %02lu:%02lu:%02lu",
            (unsigned long)(uptime_s / 3600U),
            (unsigned long)((uptime_s / 60U) % 60U),
            (unsigned long)(uptime_s % 60U));
    DrawText(490U, 110U, (uint8_t*)info, LEFT_MODE,
             &Font16, 0xFFCCCCCCU, 0xFF111118U);

    DrawText(490U, 150U, (uint8_t*)"Auth threshold: 30%",
             LEFT_MODE, &Font16, 0xFFCCCCCCU, 0xFF111118U);

    DrawText(490U, 190U, (uint8_t*)"Idle timeout: 5 s",
             LEFT_MODE, &Font16, 0xFFCCCCCCU, 0xFF111118U);

    DrawText(490U, 230U, (uint8_t*)"Backoff delay: 2 s",
             LEFT_MODE, &Font16, 0xFFCCCCCCU, 0xFF111118U);

    DrawText(490U, 270U, (uint8_t*)"Multi-person: 2+",
             LEFT_MODE, &Font16, 0xFFCCCCCCU, 0xFF111118U);

    sprintf(info, "NOR flash: 128 MB");
    DrawText(490U, 300U, (uint8_t*)info, LEFT_MODE,
             &Font16, 0xFFCCCCCCU, 0xFF111118U);

    sprintf(info, "Face store: %lu KB",
            (unsigned long)(FACE_STORE_FLASH_SIZE / 1024U));
    DrawText(490U, 325U, (uint8_t*)info, LEFT_MODE,
             &Font16, 0xFFCCCCCCU, 0xFF111118U);

    uint32_t used_records = FaceStore_Count();
    uint32_t used_bytes = 16U + (used_records * (uint32_t)sizeof(FaceStore_Record_t));
    uint32_t free_slots = (used_records < FACE_STORE_MAX_RECORDS) ?
                          (FACE_STORE_MAX_RECORDS - used_records) : 0U;

    sprintf(info, "Used: %lu B / %lu B",
            (unsigned long)used_bytes,
            (unsigned long)FACE_STORE_FLASH_SIZE);
    DrawText(490U, 350U, (uint8_t*)info, LEFT_MODE,
             &Font16, 0xFFCCCCCCU, 0xFF111118U);

    sprintf(info, "Free slots: %lu / %lu",
            (unsigned long)free_slots,
            (unsigned long)FACE_STORE_MAX_RECORDS);
    DrawText(490U, 375U, (uint8_t*)info, LEFT_MODE,
             &Font16, 0xFFCCCCCCU, 0xFF111118U);

    Panel(0U, 430U, UI_SCREEN_W, 50U, 0xFF111122U, 0xFF00FFFFU);
    DrawText(0U, 445U, (uint8_t*)"< BACK (tap here or press USER button)",
             CENTER_MODE, &Font16, 0xFF00FFFFU, 0xFF111122U);

    if (pending_delete_idx >= 0)
    {
        const FaceStore_Record_t *rec = FaceStore_Get((uint32_t)pending_delete_idx);
        const char *name = (rec != NULL) ? rec->name : "person";
        char msg[64];

        Panel(120U, 160U, 560U, 190U, 0xEE101018U, 0xFFFF4444U);
        snprintf(msg, sizeof(msg), "Delete %s?", name);
        DrawText(0U, 190U, (uint8_t*)msg,
                 CENTER_MODE, &Font20, 0xFFFFFFFFU, 0xEE101018U);
        DrawText(0U, 225U, (uint8_t*)"This removes the saved face from flash",
                 CENTER_MODE, &Font16, 0xFFFFAAAAU, 0xEE101018U);

        Panel(170U, 275U, 180U, 50U, 0xFF182028U, 0xFF00AACCU);
        DrawText(220U, 292U, (uint8_t*)"CANCEL",
                 LEFT_MODE, &Font16, 0xFFFFFFFFU, 0xFF182028U);

        Panel(450U, 275U, 180U, 50U, 0xFF401010U, 0xFFFF4444U);
        DrawText(504U, 292U, (uint8_t*)"DELETE",
                 LEFT_MODE, &Font16, 0xFFFFAAAAU, 0xFF401010U);
    }
}


static void DrawNameKey(uint32_t x, uint32_t y, uint32_t w, const char *label)
{
    Panel(x, y, w, 38U, 0xFF182028U, 0xFF00AACCU);
    uint32_t text_x = x + 8U;
    if (w == 50U)
    {
        text_x = x + 20U;
    }
    DrawText(text_x, y + 10U, (uint8_t*)label,
             LEFT_MODE, &Font16, 0xFFFFFFFFU, 0xFF182028U);
}

static void DrawEnrollName(void)
{
    const uint32_t bg = 0xEE0A0A12U;
    UTIL_LCD_FillRect(0U, 0U, UI_SCREEN_W, UI_SCREEN_H, bg);

    DrawText(0U, 40U,
             (uint8_t*)((pending_rename_idx >= 0) ? "Rename person" : "Name this face"),
             CENTER_MODE, &Font24, 0xFF00FFFFU, bg);

    Panel(120U, 92U, 560U, 48U, 0xFF101820U, 0xFF00FFFFU);
    DrawText(0U, 108U, (uint8_t*)pending_enroll_name,
             CENTER_MODE, &Font20, 0xFFFFFFFFU, 0xFF101820U);

    if ((pending_rename_idx > 0) && SessionIsAdmin())
    {
        uint32_t fill = pending_admin_value ? 0xFF103020U : 0xFF182028U;
        uint32_t border = pending_admin_value ? 0xFF00FF88U : 0xFF00AACCU;
        Panel(580U, 92U, 100U, 48U, fill, border);
        DrawText(598U, 108U,
                 (uint8_t*)(pending_admin_value ? "ADMIN" : "USER"),
                 LEFT_MODE, &Font16,
                 pending_admin_value ? 0xFF00FF88U : 0xFFFFFFFFU,
                 fill);
    }

    const char *keys1 = "ABCDEFGHIJKL";
    const char *keys2 = "MNOPQRSTUVWX";
    char label[2] = {0};
    for (uint32_t i = 0U; i < 12U; i++)
    {
        label[0] = keys1[i];
        DrawNameKey(80U + (i * 55U), 160U, 50U, label);
        label[0] = keys2[i];
        DrawNameKey(80U + (i * 55U), 205U, 50U, label);
    }

    DrawNameKey(245U, 250U, 50U, "Y");
    DrawNameKey(300U, 250U, 50U, "Z");
    DrawNameKey(355U, 250U, 105U, "SPACE");
    DrawNameKey(465U, 250U, 105U, "DEL");

    Panel(190U, 370U, 180U, 48U, 0xFF301010U, 0xFFFF4444U);
    DrawText(248U, 385U, (uint8_t*)"CANCEL",
             LEFT_MODE, &Font16, 0xFFFFAAAAU, 0xFF301010U);

    Panel(430U, 370U, 180U, 48U, 0xFF103020U, 0xFF00FF88U);
    if (pending_rename_idx >= 0)
    {
        DrawText(500U, 385U, (uint8_t*)"SAVE",
                 LEFT_MODE, &Font16, 0xFF00FF88U, 0xFF103020U);
    }
    else
    {
        DrawText(464U, 385U, (uint8_t*)"OK / DEFAULT",
                 LEFT_MODE, &Font16, 0xFF00FF88U, 0xFF103020U);
    }
}

static void DrawChatKeyboard(void)
{
    const TIM_ChatState_t *chat = TIM_AppGetState();
    const char *typed = ((chat != NULL) && (chat->current_input[0] != '\0')) ?
                        chat->current_input : "tap letters below";
    const uint32_t bg = 0xEE0A0A12U;

    Panel(60U, 66U, 680U, 368U, bg, 0xFF00FFFFU);
    DrawText(0U, 86U, (uint8_t*)"TIM chat input",
             CENTER_MODE, &Font24, 0xFF00FFFFU, bg);

    Panel(120U, 116U, 560U, 34U, 0xFF101820U, 0xFF00FFFFU);
    DrawText(136U, 126U, (uint8_t*)typed,
             LEFT_MODE, &Font16, 0xFFFFFFFFU, 0xFF101820U);

    const char *keys1 = "ABCDEFGHIJKL";
    const char *keys2 = "MNOPQRSTUVWX";
    char label[2] = {0};

    for (uint32_t i = 0U; i < 12U; i++)
    {
        label[0] = keys1[i];
        DrawNameKey(80U + (i * 55U), 160U, 50U, label);
        label[0] = keys2[i];
        DrawNameKey(80U + (i * 55U), 205U, 50U, label);
    }

    DrawNameKey(245U, 250U, 50U, "Y");
    DrawNameKey(300U, 250U, 50U, "Z");
    DrawNameKey(355U, 250U, 105U, "SPACE");
    DrawNameKey(465U, 250U, 105U, "DEL");

    Panel(190U, 370U, 180U, 48U, 0xFF301010U, 0xFFFF4444U);
    DrawText(248U, 385U, (uint8_t*)"CANCEL",
             LEFT_MODE, &Font16, 0xFFFFAAAAU, 0xFF301010U);

    Panel(430U, 370U, 180U, 48U, 0xFF103020U, 0xFF00FF88U);
    DrawText(498U, 385U, (uint8_t*)"SEND",
             LEFT_MODE, &Font16, 0xFF00FF88U, 0xFF103020U);
}

static void DrawPerfOverlay(od_pp_out_t *pp)
{
    char perf[96];
    uint32_t npu_total_ms = g_npu_infer_ms + g_embed_npu_ms + g_depth_npu_ms;
    uint32_t fol_ms = g_frame_output_ms;
    uint32_t pipe_fps = 0U;
    uint32_t hw_cpu_pct = g_hw_cpu_pct;
    uint32_t hw_npu_run_pct = g_hw_npu_run_pct;
    static uint32_t shown_fol_ms = 0U;
    static uint32_t shown_pipe_fps = 0U;
    static uint32_t shown_hw_cpu_pct = 0U;
    static uint32_t shown_hw_npu_run_pct = 0U;
    static uint32_t last_perf_update_ms = 0U;
    uint32_t now = HAL_GetTick();

    (void)pp;
    if (fol_ms < npu_total_ms)
    {
        fol_ms = npu_total_ms;
    }

    if (fol_ms > 0U)
    {
        pipe_fps = 1000U / fol_ms;
    }

    if ((last_perf_update_ms == 0U) ||
        ((now - last_perf_update_ms) >= PERF_OVERLAY_UPDATE_PERIOD_MS))
    {
        shown_fol_ms = fol_ms;
        shown_pipe_fps = pipe_fps;
        shown_hw_cpu_pct = hw_cpu_pct;
        shown_hw_npu_run_pct = hw_npu_run_pct;
        last_perf_update_ms = now;
    }

    uint32_t back_col = 0x00000000U;
    uint32_t text_col = 0xFFB0FFE8U;

    if (app_state == APP_STATE_SPLASH)
    {
        back_col = 0xFF0000FFU;
        text_col = 0xFFFFFFFFU;
    }
    else if (app_state == APP_STATE_SETTINGS)
    {
        back_col = 0xFF111122U;
        text_col = 0xFFB0FFE8U;
    }
    else if (app_state == APP_STATE_ENROLL_NAME)
    {
        back_col = 0xEE0A0A12U;
        text_col = 0xFFB0FFE8U;
    }
    else if (app_state == APP_STATE_CHAT_KEYBOARD)
    {
        back_col = 0xEE0A0A12U;
        text_col = 0xFFB0FFE8U;
    }
    else if (app_state == APP_STATE_AUTH_SUCCESS)
    {
        back_col = 0xEE0A0A12U;
        text_col = 0xFFB0FFE8U;
    }
    else if (app_state == APP_STATE_BACKING_OFF)
    {
        back_col = 0xDD000000U;
        text_col = 0xFFFFAAAAU;
    }



    snprintf(perf, sizeof(perf), "FOL %lums  PipeFPS %lu",
             (unsigned long)shown_fol_ms,
             (unsigned long)shown_pipe_fps);
    DrawText(4U, 3U, (uint8_t*)perf,
             LEFT_MODE, &Font12, text_col, back_col);

    snprintf(perf, sizeof(perf), "HW CPU %lu%%  NPUrun %lu%%",
             (unsigned long)shown_hw_cpu_pct,
             (unsigned long)shown_hw_npu_run_pct);
    DrawText(4U, 17U, (uint8_t*)perf,
             LEFT_MODE, &Font12, text_col, back_col);

}

#define _TB_ADD_PERSON  1U
#define _TB_SETTINGS    2U
#define _TB_LOGOUT      3U
#define _TB_BACK        4U

AppState_t UI_UpdateState(od_pp_out_t *pp, uint32_t touch_btn)
{
    uint32_t now = HAL_GetTick();
    uint32_t nb = (uint32_t)pp->nb_detect;
    uint8_t btn = ButtonPressed();

    ProcessTimTriggers(now);

    switch (app_state)
    {
    case APP_STATE_SPLASH:
        if ((now - state_entry_time) > 3000U)
        {
            app_state = APP_STATE_AUTH;
            state_entry_time = now;
        }
        break;

    case APP_STATE_AUTH:
        if ((FaceStore_Count() == 0U) && btn)
        {
            pending_rename_idx = -1;
            ResetPendingEnrollName();
            app_state = APP_STATE_ENROLL_NAME;
            state_entry_time = now;
        }
        else if ((nb == 1U) &&
            FaceRecog_IsReady() &&
            (last_recog_idx >= 0) &&
            (last_recog_score >= FACE_RECOG_MATCH_THRESHOLD))
        {
            const FaceStore_Record_t *rec = FaceStore_Get((uint32_t)last_recog_idx);
            if (rec != NULL)
            {
                StartSession(last_recog_idx);
                multi_person_ts = 0U;

                app_state = APP_STATE_AUTH_SUCCESS;
                state_entry_time = now;
                idle_ts = 0U;
            }
        }
        break;

    case APP_STATE_AUTH_SUCCESS:
        if ((now - state_entry_time) > 3000U)
        {
            app_state = APP_STATE_MAIN;
            state_entry_time = now;
        }
        break;

    case APP_STATE_MAIN:


        if (nb >= 2U)
        {
            if (multi_person_ts == 0U)
            {
                multi_person_ts = now;
            }
            else if ((now - multi_person_ts) >= 5000U)
            {
                LockSession(now);
                break;
            }
        }
        else
        {
            multi_person_ts = 0U;
        }

        if (touch_btn == TOUCH_BTN_CHAT_INPUT)
        {
            app_state = APP_STATE_CHAT_KEYBOARD;
            state_entry_time = now;
        }
        else if (btn || touch_btn == _TB_SETTINGS)
        {
            app_state = APP_STATE_SETTINGS;
            state_entry_time = now;
        }
        else if (touch_btn == _TB_LOGOUT)
        {
            LockSession(now);
        }
        else if (touch_btn == _TB_ADD_PERSON)
        {
            pending_rename_idx = -1;
            ResetPendingEnrollName();
            app_state = APP_STATE_ENROLL_NAME;
            state_entry_time = now;
        }
        break;

    case APP_STATE_BACKING_OFF:
        app_state = session_active ? APP_STATE_MAIN : APP_STATE_AUTH;
        state_entry_time = now;
        break;

    case APP_STATE_SETTINGS:
        if (pending_delete_idx >= 0)
        {
            if ((touch_btn == TOUCH_BTN_CANCEL_DELETE) || btn || (touch_btn == _TB_BACK))
            {
                pending_delete_idx = -1;
                state_entry_time = now;
            }
            else if (touch_btn == TOUCH_BTN_CONFIRM_DELETE)
            {
                if (FaceStore_Remove((uint32_t)pending_delete_idx))
                {
                    (void)FaceStore_Commit();
                }
                pending_delete_idx = -1;
                state_entry_time = now;
            }
        }
        else if (btn || touch_btn == _TB_BACK)
        {
            app_state = APP_STATE_MAIN;
            state_entry_time = now;
        }
        else if ((touch_btn >= TOUCH_BTN_DELETE_0) && (touch_btn <= TOUCH_BTN_DELETE_7))
        {
            uint32_t idx = (uint32_t)(touch_btn - TOUCH_BTN_DELETE_0);
            const FaceStore_Record_t *rec = FaceStore_Get(idx);
            if ((rec != NULL) && SessionIsAdmin())
            {
                pending_delete_idx = (int32_t)idx;
                state_entry_time = now;
            }
        }
        else if ((touch_btn >= TOUCH_BTN_PERSON_0) && (touch_btn <= TOUCH_BTN_PERSON_7))
        {
            uint32_t idx = (uint32_t)(touch_btn - TOUCH_BTN_PERSON_0);
            const FaceStore_Record_t *rec = FaceStore_Get(idx);
            if (rec != NULL)
            {
                uint8_t is_admin = SessionIsAdmin();
                uint8_t is_self = (session_active && (session_user_idx == (int32_t)idx));

                if (is_admin || is_self)
                {
                    pending_rename_idx = (int32_t)idx;
                    pending_admin_value = FaceStore_IsAdmin(idx) ? 1U : 0U;
                    SetPendingName(rec->name);
                    app_state = APP_STATE_ENROLL_NAME;
                    state_entry_time = now;
                }
                else
                {
                    settings_denied_ts = now;
                }
            }
        }
        break;

    case APP_STATE_ENROLL_NAME:
        if ((touch_btn >= TOUCH_BTN_NAME_A) && (touch_btn <= TOUCH_BTN_NAME_Z))
        {
            AddPendingNameChar((char)('A' + (touch_btn - TOUCH_BTN_NAME_A)));
        }
        else if (touch_btn == TOUCH_BTN_NAME_BACKSPACE)
        {
            BackspacePendingName();
        }
        else if (touch_btn == TOUCH_BTN_NAME_SPACE)
        {
            AddPendingNameChar(' ');
        }
        else if ((touch_btn == TOUCH_BTN_ADMIN_TOGGLE) &&
                 (pending_rename_idx > 0) &&
                 SessionIsAdmin())
        {
            pending_admin_value = pending_admin_value ? 0U : 1U;
        }
        else if (touch_btn == TOUCH_BTN_NAME_OK)
        {
            if (pending_rename_idx >= 0)
            {
                (void)FaceStore_Rename((uint32_t)pending_rename_idx, pending_enroll_name);
                if ((pending_rename_idx > 0) && SessionIsAdmin())
                {
                    (void)FaceStore_SetAdmin((uint32_t)pending_rename_idx,
                                             pending_admin_value != 0U);
                }
                pending_rename_idx = -1;
                app_state = APP_STATE_SETTINGS;
            }
            else
            {
                strncpy(g_enroll_name, pending_enroll_name, sizeof(g_enroll_name) - 1U);
                g_enroll_name[sizeof(g_enroll_name) - 1U] = '\0';
                g_enroll_requested = 1U;
                app_state = (session_active != 0U) ? APP_STATE_MAIN : APP_STATE_AUTH;
            }
            state_entry_time = now;
        }
        else if ((touch_btn == TOUCH_BTN_NAME_CANCEL) || btn)
        {
            app_state = (pending_rename_idx >= 0) ? APP_STATE_SETTINGS : APP_STATE_MAIN;
            pending_rename_idx = -1;
            state_entry_time = now;
        }
        break;

    case APP_STATE_CHAT_KEYBOARD:
        if ((touch_btn >= TOUCH_BTN_NAME_A) && (touch_btn <= TOUCH_BTN_NAME_Z))
        {
            TIM_AppInputChar((char)('a' + (touch_btn - TOUCH_BTN_NAME_A)));
        }
        else if (touch_btn == TOUCH_BTN_NAME_BACKSPACE)
        {
            TIM_AppBackspace();
        }
        else if (touch_btn == TOUCH_BTN_NAME_SPACE)
        {
            TIM_AppInputChar(' ');
        }
        else if (touch_btn == TOUCH_BTN_NAME_OK)
        {
            TIM_AppSubmitCurrentInput();
            app_state = APP_STATE_MAIN;
            state_entry_time = now;
        }
        else if ((touch_btn == TOUCH_BTN_NAME_CANCEL) || btn)
        {
            app_state = APP_STATE_MAIN;
            state_entry_time = now;
        }
        break;

    default:
        break;
    }

    return app_state;
}


void UI_Render(od_pp_out_t *pp, UI_BgArea_t *bg)
{
    switch (app_state)
    {
    case APP_STATE_SPLASH:
        DrawSplash();
        break;

    case APP_STATE_AUTH:
        DrawAuth(pp);
        break;

    case APP_STATE_AUTH_SUCCESS:
        DrawAuthSuccess();
        break;

    case APP_STATE_MAIN:
        DrawMain(pp, bg);
        break;

    case APP_STATE_CHAT_KEYBOARD:
        DrawMain(pp, bg);
        DrawChatKeyboard();
        break;

    case APP_STATE_BACKING_OFF:
        DrawBackoff(pp->nb_detect);
        break;

    case APP_STATE_SETTINGS:
        DrawSettings();
        break;

    case APP_STATE_ENROLL_NAME:
        DrawEnrollName();
        break;

    default:
        break;
    }

    DrawPerfOverlay(pp);
}
