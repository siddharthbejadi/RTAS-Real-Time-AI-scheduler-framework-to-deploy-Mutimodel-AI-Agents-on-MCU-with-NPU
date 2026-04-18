#include "app_ui.h"
#include "stm32_lcd.h"
#include "stm32n6570_discovery.h"
#include "stm32n6xx_hal.h"
#include "face_store.h"
#include "face_recog.h"
#include <string.h>
#include <stdio.h>

/* ───────────────────────────────────────────── */
/* GLOBAL STATE                                 */
/* ───────────────────────────────────────────── */

AppState_t app_state = APP_STATE_SPLASH;
uint32_t state_entry_time = 0;

static int32_t session_user_idx = -1;
static uint8_t session_active = 0U;

extern const char *classes_table[];

/* Recognition results published by main.c each inference cycle */
extern int32_t last_recog_idx;
extern float   last_recog_score;
extern float   last_det_conf;

/* Enrollment request flag — UI raises it; main.c actions it at end of frame. */
volatile uint8_t g_enroll_requested = 0;
volatile uint8_t g_enroll_done_flag = 0;
volatile uint8_t g_enroll_fail_flag = 0;

UI_Person_t ui_persons[UI_MAX_PERSONS];
uint8_t ui_person_count = 0;

static uint32_t prev_btn = 0;
static uint32_t idle_ts = 0;
static uint32_t backoff_ts = 0;
static uint32_t multi_person_ts = 0;

/* ───────────────────────────────────────────── */
/* COLORS                                       */
/* ───────────────────────────────────────────── */

static const uint32_t bbox_colors[6] = {
    0xFF00FF00, 0xFFFF0000, 0xFF0000FF,
    0xFFFFFF00, 0xFFFF00FF, 0xFF00FFFF
};

/* ───────────────────────────────────────────── */
/* HELPERS                                      */
/* ───────────────────────────────────────────── */

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

/* ───────────────────────────────────────────── */
/* INIT                                         */
/* ───────────────────────────────────────────── */

void UI_Init(void)
{
    app_state = APP_STATE_SPLASH;
    state_entry_time = HAL_GetTick();

    idle_ts = 0U;
    backoff_ts = 0U;
    multi_person_ts = 0U;
    prev_btn = 0U;

    session_user_idx = -1;
    session_active = 0U;

    memset(ui_persons, 0, sizeof(ui_persons));
    strcpy(ui_persons[0].name, "Admin");
    ui_persons[0].active = 1;
    ui_person_count = 1;
}

/* ───────────────────────────────────────────── */
/* TEXT DRAW                                    */
/* ───────────────────────────────────────────── */

void DrawText(uint32_t x, uint32_t y, uint8_t *pText,
              Text_AlignModeTypdef Mode, sFONT *fonts,
              uint32_t text_color, uint32_t back_color)
{
    UTIL_LCD_SetTextColor(text_color);
    UTIL_LCD_SetBackColor(back_color);
    UTIL_LCD_SetFont(fonts);
    UTIL_LCD_DisplayStringAt(x, y, pText, Mode);
}

/* ───────────────────────────────────────────── */
/* SPLASH                                       */
/* ───────────────────────────────────────────── */

static void DrawSplash(void)
{
    const uint32_t bg = 0xFF0000FFU;

    UTIL_LCD_FillRect(0U, 0U, UI_SCREEN_W, UI_SCREEN_H, bg);

    DrawText(0U, 150U, (uint8_t*)"Hello! There :)",
             CENTER_MODE, &Font24, 0xFFFFFFFFU, bg);

    /* Progress */
    uint32_t elapsed = HAL_GetTick() - state_entry_time;
    uint32_t w = (elapsed * 400U) / 3000U;
    if (w > 400U) w = 400U;

    UTIL_LCD_FillRect(200U, 250U, 400U, 10U, 0xFF333333U);
    UTIL_LCD_FillRect(200U, 250U, w,    10U, 0xFF00FFFFU);
}

/* ───────────────────────────────────────────── */
/* AUTH SUCCESS TRANSITION                      */
/* ───────────────────────────────────────────── */

static void DrawAuthSuccess(void)
{
    const uint32_t bg = 0xEE0A0A12U;
    const char *who = "User";

    if (session_active && (session_user_idx >= 0))
    {
        const FaceStore_Record_t *rec = FaceStore_Get((uint32_t)session_user_idx);
        if (rec != NULL)
        {
            who = rec->name;
        }
    }

    UTIL_LCD_FillRect(0U, 0U, UI_SCREEN_W, UI_SCREEN_H, bg);

    DrawText(0U, 120U, (uint8_t*)"Access granted",
             CENTER_MODE, &Font24, 0xFF00FF88U, bg);

    char hello_buf[64];
    snprintf(hello_buf, sizeof(hello_buf), "Hello, %s", who);
    DrawText(0U, 170U, (uint8_t*)hello_buf,
             CENTER_MODE, &Font20, 0xFFFFFFFFU, bg);

    DrawText(0U, 210U, (uint8_t*)"Loading secure workspace...",
             CENTER_MODE, &Font20, 0xFFFFFFFFU, bg);

    uint32_t elapsed = HAL_GetTick() - state_entry_time;
    uint32_t w = (elapsed * 400U) / 1500U;
    if (w > 400U) w = 400U;

    UTIL_LCD_FillRect(200U, 250U, 400U, 12U, 0xFF333333U);
    UTIL_LCD_FillRect(200U, 250U, w,    12U, 0xFF00FFFFU);
    UTIL_LCD_DrawRect(200U, 250U, 400U, 12U, 0xFF00FFFFU);
}

/* ───────────────────────────────────────────── */
/* AUTH                                         */
/* ───────────────────────────────────────────── */

static void DrawAuth(od_pp_out_t *pp)
{
    const uint32_t bg = 0x00000000U;
    uint32_t nb = pp->nb_detect;
    od_pp_outBuffer_t *r = pp->pOutBuff;

    UTIL_LCD_FillRect(0U, 0U, UI_SCREEN_W, UI_SCREEN_H, bg);

    /* Guide box */
    UTIL_LCD_DrawRect(270U, 100U, 260U, 260U, 0xFF00FFFFU);

    DrawText(0U, 40U, (uint8_t*)"FACE AUTHENTICATION",
             CENTER_MODE, &Font20, 0xFF00FFFFU, bg);

    DrawText(0U, 380U, (uint8_t*)"Scan your face or ID key",
             CENTER_MODE, &Font20, 0xFFFFFFFFU, bg);

    const char *who = "Unknown";
    uint32_t status_col = 0xFF888888U;
    char line1[64];
    char line2[64];
    char line3[64];

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
        if (!FaceRecog_IsReady())
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
                snprintf(line3, sizeof(line3), "Match %.0f%%", last_recog_score * 100.0f);
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
            snprintf(line3, sizeof(line3), "Detection %.0f%%", det_conf);
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

/* ───────────────────────────────────────────── */
/* MAIN                                         */
/* ───────────────────────────────────────────── */

static void DrawMain(od_pp_out_t *pp, UI_BgArea_t *bg)
{
    const uint32_t screen_bg = 0xEE0A0A12U;
    uint32_t nb = pp->nb_detect;
    od_pp_outBuffer_t *r = pp->pOutBuff;

    UTIL_LCD_FillRect(0U, 0U, UI_SCREEN_W, UI_SCREEN_H, screen_bg);

    /* Camera PiP window — tuned for 800x480 landscape */
    uint32_t cam_x = 500U;
    uint32_t cam_y = 120U;
    uint32_t cam_w = 250U;
    uint32_t cam_h = 200U;

    /* If caller provides a valid bg area, prefer that */
    if (bg != NULL)
    {
        cam_x = bg->X0 + bg->XSize / 2U;
        cam_y = 10U;
        cam_w = bg->XSize / 2U;
        cam_h = 200U;
    }

    UTIL_LCD_FillRect(cam_x, cam_y, cam_w, cam_h, 0x00000000U);
    UTIL_LCD_DrawRect(cam_x - 2U, cam_y - 2U, cam_w + 4U, cam_h + 4U, 0xFF00FFFFU);

    if ((r != NULL) && (nb > 0U))
    {
        float sx = (float)cam_w;
        float sy = (float)cam_h;

        for (uint32_t i = 0U; i < nb; i++)
        {
            uint32_t col = bbox_colors[i % 6U];
            uint32_t x = (uint32_t)((r[i].x_center - r[i].width * 0.5f) * sx) + cam_x;
            uint32_t y = (uint32_t)((r[i].y_center - r[i].height * 0.5f) * sy) + cam_y;
            uint32_t w = (uint32_t)(r[i].width * sx);
            uint32_t h = (uint32_t)(r[i].height * sy);

            UTIL_LCD_DrawRect(x, y, w, h, col);
        }
    }

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

    DrawText(30U, 40U, (uint8_t*)"MAIN ACCESS PANEL",
             LEFT_MODE, &Font20, 0xFF00FFFFU, screen_bg);

    DrawText(30U, 80U, (uint8_t*)"Access granted.",
             LEFT_MODE, &Font20, 0xFF00FF88U, screen_bg);

    char namebuf[48];
    snprintf(namebuf, sizeof(namebuf), "Welcome: %s", who);
    DrawText(30U, 110U, (uint8_t*)namebuf,
             LEFT_MODE, &Font20, name_col, screen_bg);

    if ((r != NULL) && (nb > 0U))
    {
        snprintf(line2, sizeof(line2), "det %.0f%% / match %.0f%%",
                 r[0].conf * 100.0f, last_recog_score * 100.0f);
    }
    else
    {
        snprintf(line2, sizeof(line2), "Camera active / session unlocked");
    }

    DrawText(30U, 140U, (uint8_t*)line2,
             LEFT_MODE, &Font16, 0xFF8090A0U, screen_bg);


    /* Bottom toolbar — corrected for 800x480 landscape */
    Panel(0U, 440U, 800U, 40U, 0xEE111111U, 0xFF00FFFFU);

    DrawText(20U, 452U, (uint8_t*)"+ ADD",
             LEFT_MODE, &Font16, 0xFF00FFFFU, 0xEE111111U);

    DrawText(320U, 452U, (uint8_t*)"SETTINGS",
             LEFT_MODE, &Font16, 0xFF00FFFFU, 0xEE111111U);

    DrawText(650U, 452U, (uint8_t*)"LOGOUT",
             LEFT_MODE, &Font16, 0xFF00FFFFU, 0xEE111111U);

    /* Enrollment toast */
    if (g_enroll_done_flag)
    {
        Panel(120U, 200U, 560U, 80U, 0xDD003020U, 0xFF00FF88U);
        DrawText(0U, 228U, (uint8_t*)"Face enrolled & saved to flash",
                 CENTER_MODE, &Font20, 0xFF00FF88U, 0xDD003020U);
    }
    else if (g_enroll_fail_flag)
    {
        Panel(120U, 200U, 560U, 80U, 0xDD200000U, 0xFFFF4444U);
        DrawText(0U, 228U, (uint8_t*)"Enroll failed (no face / store full)",
                 CENTER_MODE, &Font20, 0xFFFF4444U, 0xDD200000U);
    }
    else if (g_enroll_requested)
    {
        Panel(120U, 200U, 560U, 80U, 0xDD001830U, 0xFF00CFFFU);
        DrawText(0U, 228U, (uint8_t*)"Enrolling... hold still",
                 CENTER_MODE, &Font20, 0xFF00CFFFU, 0xDD001830U);
    }
}

/* ───────────────────────────────────────────── */
/* BACKING OFF                                  */
/* ───────────────────────────────────────────── */

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

/* ───────────────────────────────────────────── */
/* SETTINGS                                     */
/* ───────────────────────────────────────────── */

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

    Panel(0U, 430U, UI_SCREEN_W, 50U, 0xFF111122U, 0xFF00FFFFU);
    DrawText(0U, 445U, (uint8_t*)"< BACK (tap here or press USER button)",
             CENTER_MODE, &Font16, 0xFF00FFFFU, 0xFF111122U);
}

/* ───────────────────────────────────────────── */
/* STATE MACHINE                                */
/* ───────────────────────────────────────────── */

#define _TB_ADD_PERSON  1U
#define _TB_SETTINGS    2U
#define _TB_LOGOUT      3U
#define _TB_BACK        4U

AppState_t UI_UpdateState(od_pp_out_t *pp, uint32_t touch_btn)
{
    uint32_t now = HAL_GetTick();
    uint32_t nb = (uint32_t)pp->nb_detect;
    uint8_t btn = ButtonPressed();

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
        if ((nb == 1U) && FaceRecog_IsReady() && (last_recog_idx >= 0))
        {
            const FaceStore_Record_t *rec = FaceStore_Get((uint32_t)last_recog_idx);
            if (rec != NULL)
            {
                session_user_idx = last_recog_idx;
                session_active = 1U;
                multi_person_ts = 0U;

                app_state = APP_STATE_AUTH_SUCCESS;
                state_entry_time = now;
                idle_ts = 0U;
            }
        }
        break;

    case APP_STATE_AUTH_SUCCESS:
        if ((now - state_entry_time) > 1500U)
        {
            app_state = APP_STATE_MAIN;
            state_entry_time = now;
        }
        break;

    case APP_STATE_MAIN:
        /* 0 or 1 person -> normal
           2+ persons continuously for 5s -> logout to AUTH */

        if (nb >= 2U)
        {
            if (multi_person_ts == 0U)
            {
                multi_person_ts = now;
            }
            else if ((now - multi_person_ts) >= 5000U)
            {
                session_active = 0U;
                session_user_idx = -1;
                idle_ts = 0U;
                multi_person_ts = 0U;

                app_state = APP_STATE_AUTH;
                state_entry_time = now;
                break;
            }
        }
        else
        {
            multi_person_ts = 0U;
        }

        if (btn || touch_btn == _TB_SETTINGS)
        {
            app_state = APP_STATE_SETTINGS;
            state_entry_time = now;
        }
        else if (touch_btn == _TB_LOGOUT)
        {
            session_active = 0U;
            session_user_idx = -1;
            idle_ts = 0U;
            multi_person_ts = 0U;

            app_state = APP_STATE_AUTH;
            state_entry_time = now;
        }
        else if (touch_btn == _TB_ADD_PERSON)
        {
            g_enroll_requested = 1U;
            state_entry_time = now;
        }
        break;

    case APP_STATE_BACKING_OFF:
        app_state = session_active ? APP_STATE_MAIN : APP_STATE_AUTH;
        state_entry_time = now;
        break;

    case APP_STATE_SETTINGS:
        if (btn || touch_btn == _TB_BACK)
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

/* ───────────────────────────────────────────── */
/* RENDER ENTRY                                 */
/* ───────────────────────────────────────────── */

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

    case APP_STATE_BACKING_OFF:
        DrawBackoff(pp->nb_detect);
        break;

    case APP_STATE_SETTINGS:
        DrawSettings();
        break;

    default:
        break;
    }
}
