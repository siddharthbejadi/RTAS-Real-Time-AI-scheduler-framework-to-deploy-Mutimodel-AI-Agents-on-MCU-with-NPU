

#ifndef APP_UI_H
#define APP_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "app_postprocess.h"


#define UI_SCREEN_W         800u
#define UI_SCREEN_H         480u

#define UI_TOP_BAR_H         52u
#define UI_BOTTOM_BAR_H      56u
#define UI_CONTENT_Y        UI_TOP_BAR_H
#define UI_CONTENT_H        (UI_SCREEN_H - UI_TOP_BAR_H - UI_BOTTOM_BAR_H)


#define UI_SPLASH_MS        3000u
#define UI_BACKOFF_COOL_MS  10000u


#define UI_AUTH_CONF_MIN    0.60f
#define UI_BACKOFF_COUNT    2u


#define UI_MAX_PERSONS      5u
#define UI_MAX_NAME_LEN     20u

typedef struct {
    char    name[UI_MAX_NAME_LEN];
    uint8_t active;
} UI_Person_t;


typedef struct {
    uint32_t X0;
    uint32_t Y0;
    uint32_t XSize;
    uint32_t YSize;
} UI_BgArea_t;


#define UI_COL_TRANSPARENT  0x00000000u
#define UI_COL_BG_FULL      0xFF101820u
#define UI_COL_BG_DARK      0xD0101820u
#define UI_COL_BG_MID       0x90101820u
#define UI_COL_BG_LIGHT     0x60101820u
#define UI_COL_BG_OVERLAY   0x40000000u

#define UI_COL_WHITE        0xFFFFFFFFu
#define UI_COL_GRAY         0xFF708090u
#define UI_COL_BLACK        0xFF000000u

#define UI_COL_TEAL         0xFF00BCD4u
#define UI_COL_GREEN        0xFF00C853u
#define UI_COL_GREEN_DARK   0xFF003810u
#define UI_COL_RED          0xFFFF1744u
#define UI_COL_RED_DARK     0xFF5A0010u
#define UI_COL_ORANGE       0xFFFF6D00u
#define UI_COL_BLUE         0xFF2979FFu
#define UI_COL_YELLOW       0xFFFFD600u


typedef enum {
    APP_STATE_SPLASH      = 0,
    APP_STATE_AUTH        = 1,
	APP_STATE_AUTH_SUCCESS = 2,
    APP_STATE_MAIN        = 3,
    APP_STATE_BACKING_OFF = 4,
    APP_STATE_SETTINGS    = 5,
    APP_STATE_ENROLL_NAME = 6,
    APP_STATE_CHAT_KEYBOARD = 7,
} AppState_t;


extern AppState_t   app_state;
extern uint32_t     state_entry_time;
extern UI_Person_t  ui_persons[UI_MAX_PERSONS];
extern uint8_t      ui_person_count;


void UI_Init(void);


AppState_t UI_UpdateState(od_pp_out_t *pp, uint32_t touch_btn);


void UI_Render(od_pp_out_t *pp, UI_BgArea_t *bg_area);


void UI_AddPerson(const char *name);


void UI_RemovePerson(uint8_t index);

#ifdef __cplusplus
}
#endif

#endif
