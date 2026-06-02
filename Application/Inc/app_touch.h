

#ifndef APP_TOUCH_H
#define APP_TOUCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "app_ui.h"


typedef enum {
    TOUCH_BTN_NONE       = 0,
    TOUCH_BTN_ADD_PERSON,
    TOUCH_BTN_SETTINGS,
    TOUCH_BTN_LOGOUT,
    TOUCH_BTN_BACK,
    TOUCH_BTN_NAME_OK,
    TOUCH_BTN_NAME_CANCEL,
    TOUCH_BTN_NAME_BACKSPACE,
    TOUCH_BTN_NAME_SPACE,
    TOUCH_BTN_ADMIN_TOGGLE,
    TOUCH_BTN_CHAT_INPUT = 70,
    TOUCH_BTN_PERSON_0 = 40,
    TOUCH_BTN_PERSON_1,
    TOUCH_BTN_PERSON_2,
    TOUCH_BTN_PERSON_3,
    TOUCH_BTN_PERSON_4,
    TOUCH_BTN_PERSON_5,
    TOUCH_BTN_PERSON_6,
    TOUCH_BTN_PERSON_7,
    TOUCH_BTN_DELETE_0 = 50,
    TOUCH_BTN_DELETE_1,
    TOUCH_BTN_DELETE_2,
    TOUCH_BTN_DELETE_3,
    TOUCH_BTN_DELETE_4,
    TOUCH_BTN_DELETE_5,
    TOUCH_BTN_DELETE_6,
    TOUCH_BTN_DELETE_7,
    TOUCH_BTN_CONFIRM_DELETE,
    TOUCH_BTN_CANCEL_DELETE,
    TOUCH_BTN_NAME_A = 10,
    TOUCH_BTN_NAME_B,
    TOUCH_BTN_NAME_C,
    TOUCH_BTN_NAME_D,
    TOUCH_BTN_NAME_E,
    TOUCH_BTN_NAME_F,
    TOUCH_BTN_NAME_G,
    TOUCH_BTN_NAME_H,
    TOUCH_BTN_NAME_I,
    TOUCH_BTN_NAME_J,
    TOUCH_BTN_NAME_K,
    TOUCH_BTN_NAME_L,
    TOUCH_BTN_NAME_M,
    TOUCH_BTN_NAME_N,
    TOUCH_BTN_NAME_O,
    TOUCH_BTN_NAME_P,
    TOUCH_BTN_NAME_Q,
    TOUCH_BTN_NAME_R,
    TOUCH_BTN_NAME_S,
    TOUCH_BTN_NAME_T,
    TOUCH_BTN_NAME_U,
    TOUCH_BTN_NAME_V,
    TOUCH_BTN_NAME_W,
    TOUCH_BTN_NAME_X,
    TOUCH_BTN_NAME_Y,
    TOUCH_BTN_NAME_Z,
} TouchButton_t;


typedef struct {
    uint16_t      x;
    uint16_t      y;
    uint16_t      w;
    uint16_t      h;
    TouchButton_t btn;
} ButtonMap_t;


TouchButton_t ButtonMap_Check(uint16_t tx, uint16_t ty,
                               const ButtonMap_t *map, uint8_t count);


void Touch_Init(void);


TouchButton_t Touch_GetButton(AppState_t state);

#ifdef __cplusplus
}
#endif

#endif
