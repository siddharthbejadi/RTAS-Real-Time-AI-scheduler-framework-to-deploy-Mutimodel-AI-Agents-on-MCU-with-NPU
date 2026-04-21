/**
 ******************************************************************************
 * @file    app_ui.h
 * @brief   Application UI — state machine, screen layout definitions,
 *          and function declarations for the STM32N6570-DK secure-access demo.
 *
 *  Screen flow
 *  ───────────
 *   [SPLASH] ──(3 s)──► [AUTH] ──(1 detected, conf ≥ 0.60)──► [MAIN]
 *                          ▲                                      │  ▲
 *                          │          (0 detections)              │  │
 *                          └──────────────────────────────────────┘  │
 *                                                      (≥2 det.)  │  │
 *                                                    [BACKING OFF]◄──┘
 *                                                         │
 *                                               (< 2 det. + 2 s cool)
 *                                                         │
 *                                                      [MAIN]
 *
 *  USER button (B2 / BUTTON_USER1) pressed in MAIN  → opens SETTINGS
 *  USER button pressed again in SETTINGS            → returns to MAIN
 *
 ******************************************************************************
 * @attention
 * Copyright (c) 2024.  All rights reserved.
 ******************************************************************************
 */

#ifndef APP_UI_H
#define APP_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "app_postprocess.h"   /* od_pp_out_t, od_pp_outBuffer_t */

/* ─────────────────────────────────────────────────────────────────────────── */
/*  Screen & layout constants                                                  */
/* ─────────────────────────────────────────────────────────────────────────── */
#define UI_SCREEN_W         800u
#define UI_SCREEN_H         480u

#define UI_TOP_BAR_H         52u   /* height of top status bar              */
#define UI_BOTTOM_BAR_H      56u   /* height of bottom action bar           */
#define UI_CONTENT_Y        UI_TOP_BAR_H
#define UI_CONTENT_H        (UI_SCREEN_H - UI_TOP_BAR_H - UI_BOTTOM_BAR_H)

/* ─────────────────────────────────────────────────────────────────────────── */
/*  Timing constants                                                           */
/* ─────────────────────────────────────────────────────────────────────────── */
#define UI_SPLASH_MS        3000u  /* splash display duration (ms)          */
#define UI_BACKOFF_COOL_MS  10000u  /* grace period before leaving BACKOFF   */

/* ─────────────────────────────────────────────────────────────────────────── */
/*  Detection thresholds                                                       */
/* ─────────────────────────────────────────────────────────────────────────── */
#define UI_AUTH_CONF_MIN    0.60f  /* min confidence to grant access        */
#define UI_BACKOFF_COUNT    2u     /* number of detections that triggers    */
                                   /* "backing off" mode                    */

/* ─────────────────────────────────────────────────────────────────────────── */
/*  Authorized-persons registry                                                */
/* ─────────────────────────────────────────────────────────────────────────── */
#define UI_MAX_PERSONS      5u
#define UI_MAX_NAME_LEN     20u

typedef struct {
    char    name[UI_MAX_NAME_LEN]; /* display name                         */
    uint8_t active;                /* 1 = slot occupied, 0 = empty         */
} UI_Person_t;

/* ─────────────────────────────────────────────────────────────────────────── */
/*  Background-area descriptor (mirrors Rectangle_TypeDef in main.c)          */
/*  Pass a pointer to lcd_bg_area cast to this type.                           */
/* ─────────────────────────────────────────────────────────────────────────── */
typedef struct {
    uint32_t X0;
    uint32_t Y0;
    uint32_t XSize;
    uint32_t YSize;
} UI_BgArea_t;

/* ─────────────────────────────────────────────────────────────────────────── */
/*  Color palette  (ARGB8888; hardware converts to ARGB4444 at write-time)    */
/* ─────────────────────────────────────────────────────────────────────────── */
#define UI_COL_TRANSPARENT  0x00000000u
#define UI_COL_BG_FULL      0xFF101820u  /* fully opaque dark-blue-grey      */
#define UI_COL_BG_DARK      0xD0101820u  /* ~82 % opaque                     */
#define UI_COL_BG_MID       0x90101820u  /* ~56 % opaque                     */
#define UI_COL_BG_LIGHT     0x60101820u  /* ~38 % opaque                     */
#define UI_COL_BG_OVERLAY   0x40000000u  /* 25 % transparent black           */

#define UI_COL_WHITE        0xFFFFFFFFu
#define UI_COL_GRAY         0xFF708090u
#define UI_COL_BLACK        0xFF000000u

#define UI_COL_TEAL         0xFF00BCD4u  /* primary accent                   */
#define UI_COL_GREEN        0xFF00C853u  /* success / authorized             */
#define UI_COL_GREEN_DARK   0xFF003810u  /* authorized panel fill            */
#define UI_COL_RED          0xFFFF1744u  /* danger / backing-off             */
#define UI_COL_RED_DARK     0xFF5A0010u  /* danger panel fill                */
#define UI_COL_ORANGE       0xFFFF6D00u  /* warning                          */
#define UI_COL_BLUE         0xFF2979FFu  /* action button                    */
#define UI_COL_YELLOW       0xFFFFD600u

/* ─────────────────────────────────────────────────────────────────────────── */
/*  Application state machine                                                  */
/* ─────────────────────────────────────────────────────────────────────────── */
typedef enum {
    APP_STATE_SPLASH      = 0,  /* welcome / loading animation              */
    APP_STATE_AUTH        = 1,  /* waiting for face / ID scan               */
	APP_STATE_AUTH_SUCCESS = 2,  /* welcome / loading animation              */
    APP_STATE_MAIN        = 3,  /* live detection + toolbar                 */
    APP_STATE_BACKING_OFF = 4,  /* ≥ UI_BACKOFF_COUNT detections            */
    APP_STATE_SETTINGS    = 5,  /* manage authorized persons                */
    APP_STATE_ENROLL_NAME = 6,  /* choose display name before enrolment     */
    APP_STATE_CHAT_KEYBOARD = 7, /* on-screen TIM chat keypad                */
} AppState_t;

/* ─────────────────────────────────────────────────────────────────────────── */
/*  Exported module state  (read-only from outside)                           */
/* ─────────────────────────────────────────────────────────────────────────── */
extern AppState_t   app_state;
extern uint32_t     state_entry_time;
extern UI_Person_t  ui_persons[UI_MAX_PERSONS];
extern uint8_t      ui_person_count;

/* ─────────────────────────────────────────────────────────────────────────── */
/*  Public API                                                                 */
/* ─────────────────────────────────────────────────────────────────────────── */

/**
 * @brief  Must be called once after LCD_init() and before the main loop.
 *         Initialises module state and pre-loads the default admin entry.
 */
void UI_Init(void);

/**
 * @brief  Evaluate detection results AND touch input, then advance the state machine.
 *         Call once per inference cycle, before UI_Render.
 * @param  pp         post-process output (detection results)
 * @param  touch_btn  button tapped this frame (TOUCH_BTN_NONE if none)
 * @return Current (possibly updated) AppState_t value.
 */
AppState_t UI_UpdateState(od_pp_out_t *pp, uint32_t touch_btn);

/**
 * @brief  Render the current screen based on app_state.
 *         Call once per inference cycle, after UI_UpdateState.
 * @param  pp       post-process output from the current inference frame
 * @param  bg_area  pointer to lcd_bg_area (cast to UI_BgArea_t*)
 */
void UI_Render(od_pp_out_t *pp, UI_BgArea_t *bg_area);

/** @brief  Add a person to the authorized registry (first free slot). */
void UI_AddPerson(const char *name);

/** @brief  Remove the person at the given index from the registry. */
void UI_RemovePerson(uint8_t index);

#ifdef __cplusplus
}
#endif

#endif /* APP_UI_H */
