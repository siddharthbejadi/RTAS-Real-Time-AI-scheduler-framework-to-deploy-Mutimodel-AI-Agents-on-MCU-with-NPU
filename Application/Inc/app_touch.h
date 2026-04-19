/**
 ******************************************************************************
 * @file    app_touch.h
 * @brief   Minimal GT911 capacitive touch driver for STM32N6570-DK.
 *          Reads touch coordinates over I2C2 and maps them to the
 *          on-screen button regions drawn by app_ui.c.
 *
 *  Hardware:  GT911 touch controller, I2C2, address 0xBA (8-bit)
 *  Screen:    800 x 480 px (landscape)
 ******************************************************************************
 */

#ifndef APP_TOUCH_H
#define APP_TOUCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "app_ui.h"   /* AppState_t */

/* ── Button IDs ─────────────────────────────────────────────────────────── */
typedef enum {
    TOUCH_BTN_NONE       = 0,
    TOUCH_BTN_ADD_PERSON,   /* Main screen — bottom bar left  */
    TOUCH_BTN_SETTINGS,     /* Main screen — bottom bar mid   */
    TOUCH_BTN_LOGOUT,       /* Main screen — bottom bar right */
    TOUCH_BTN_BACK,         /* Settings screen — bottom bar   */
    TOUCH_BTN_NAME_OK,
    TOUCH_BTN_NAME_CANCEL,
    TOUCH_BTN_NAME_BACKSPACE,
    TOUCH_BTN_NAME_SPACE,
    TOUCH_BTN_ADMIN_TOGGLE,
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

/* ── ButtonMap ───────────────────────────────────────────────────────────── */
/**
 * One entry in a button map.
 * Define a table of these for each screen, then call ButtonMap_Check().
 *
 *   x, y  — top-left corner of the hit rectangle (screen pixels)
 *   w, h  — width and height of the hit rectangle
 *   btn   — value returned when a tap lands inside this rectangle
 */
typedef struct {
    uint16_t      x;
    uint16_t      y;
    uint16_t      w;
    uint16_t      h;
    TouchButton_t btn;
} ButtonMap_t;

/**
 * @brief  Check whether touch point (tx, ty) falls inside any entry of a
 *         button map.  Scans the table in order and returns the first match.
 *
 * @param  tx     Scaled screen X coordinate
 * @param  ty     Scaled screen Y coordinate
 * @param  map    Pointer to an array of ButtonMap_t entries
 * @param  count  Number of entries in the array
 * @return Matching TouchButton_t, or TOUCH_BTN_NONE if no match.
 */
TouchButton_t ButtonMap_Check(uint16_t tx, uint16_t ty,
                               const ButtonMap_t *map, uint8_t count);

/* ── API ─────────────────────────────────────────────────────────────────── */

/** @brief  Initialise I2C2 and verify GT911 is reachable.
 *          Call once after BSP_LCD_Init(). */
void Touch_Init(void);

/**
 * @brief  Poll GT911 for a new finger tap and return which button was hit.
 *         Uses ButtonMap_Check() internally for each screen's button table.
 *
 * @param  state  Current UI state — selects which button map is active.
 * @return The button that was tapped, or TOUCH_BTN_NONE.
 */
TouchButton_t Touch_GetButton(AppState_t state);

#ifdef __cplusplus
}
#endif

#endif /* APP_TOUCH_H */
