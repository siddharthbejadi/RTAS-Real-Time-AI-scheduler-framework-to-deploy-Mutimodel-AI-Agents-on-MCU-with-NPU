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
