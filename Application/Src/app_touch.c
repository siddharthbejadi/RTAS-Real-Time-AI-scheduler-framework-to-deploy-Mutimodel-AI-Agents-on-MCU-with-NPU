/**
 ******************************************************************************
 * @file    app_touch.c
 * @brief   Minimal GT911 driver — no external component library needed.
 *
 *  The GT911 exposes a simple I2C register map:
 *    0x814E  — status  (bit7 = buffer ready, bits[3:0] = touch count)
 *    0x8150  — touch point 0  (8 bytes: trackID, xl, xh, yl, yh, sl, sh, rsv)
 *  After reading, write 0x00 to 0x814E to clear the buffer.
 *
 *  Screen button regions (must match what app_ui.c draws):
 *
 *   Main screen — bottom bar  (y = 424..480)
 *     ADD PERSON : x = 20..220
 *     SETTINGS   : x = 260..460
 *     LOGOUT     : x = 500..660
 *
 *   Settings screen — BACK button  (top-left)
 *     BACK       : x = 16..126,  y = 16..48
 *
 ******************************************************************************
 */

#include "app_touch.h"
#include "stm32n6570_discovery_bus.h"
#include "stm32n6xx_hal.h"
#include <stdio.h>

/* ── GT911 constants ─────────────────────────────────────────────────────── */
/*
 * GT911 I2C address depends on INT level during reset:
 * - 0x5D (7-bit) => 0xBA (8-bit, HAL expects 7-bit<<1)
 * - 0x14 (7-bit) => 0x28 (8-bit)
 *
 * If INT is left floating/board-default, either can happen; probe both.
 */
#define GT911_ADDR_0x5D     0xBAU
#define GT911_ADDR_0x14     0x28U
#define GT911_REG_PRODUCT   0x8140U /* Product ID — should read "911"        */
#define GT911_REG_STATUS    0x814EU /* Buffer status + touch count           */
#define GT911_REG_TP0       0x814FU /* Track ID + touch point 0 data        */

/* ── Button maps — one table per screen ─────────────────────────────────── */
/*
 * Format: { x, y, w, h, button_id }
 *
 * To adjust a button's hit area just change its row here.
 * To add a new button add a new row.
 * The logic (ButtonMap_Check) never needs to change.
 *
 * Bottom toolbar is drawn at y=440, height=40  → y=440..480
 *   ADD PERSON  : drawn at x=30,  text ~120px wide  → hit x=0..240
 *   SETTINGS    : drawn centred (x≈340 on 800px)    → hit x=250..550
 *   LOGOUT      : drawn at x=660                    → hit x=560..800
 */
static const ButtonMap_t s_main_map[] = {
    {  36U, 358U, 398U, 38U, TOUCH_BTN_CHAT_INPUT },
    {   0U, 430U, 266U, 50U, TOUCH_BTN_ADD_PERSON },
    { 267U, 430U, 266U, 50U, TOUCH_BTN_SETTINGS   },
    { 534U, 430U, 266U, 50U, TOUCH_BTN_LOGOUT     },
};
#define MAIN_MAP_COUNT  (sizeof(s_main_map) / sizeof(s_main_map[0]))

/*
 * Settings screen — BACK bar is drawn full-width at y=430, height=50
 */
static const ButtonMap_t s_settings_map[] = {
    { 380U, 105U,  80U, 30U, TOUCH_BTN_DELETE_0 },
    { 380U, 141U,  80U, 30U, TOUCH_BTN_DELETE_1 },
    { 380U, 177U,  80U, 30U, TOUCH_BTN_DELETE_2 },
    { 380U, 213U,  80U, 30U, TOUCH_BTN_DELETE_3 },
    { 380U, 249U,  80U, 30U, TOUCH_BTN_DELETE_4 },
    { 380U, 285U,  80U, 30U, TOUCH_BTN_DELETE_5 },
    { 380U, 321U,  80U, 30U, TOUCH_BTN_DELETE_6 },
    { 380U, 357U,  80U, 30U, TOUCH_BTN_DELETE_7 },
    { 170U, 275U, 180U, 50U, TOUCH_BTN_CANCEL_DELETE },
    { 450U, 275U, 180U, 50U, TOUCH_BTN_CONFIRM_DELETE },
    { 20U, 105U, 440U, 30U, TOUCH_BTN_PERSON_0 },
    { 20U, 141U, 440U, 30U, TOUCH_BTN_PERSON_1 },
    { 20U, 177U, 440U, 30U, TOUCH_BTN_PERSON_2 },
    { 20U, 213U, 440U, 30U, TOUCH_BTN_PERSON_3 },
    { 20U, 249U, 440U, 30U, TOUCH_BTN_PERSON_4 },
    { 20U, 285U, 440U, 30U, TOUCH_BTN_PERSON_5 },
    { 20U, 321U, 440U, 30U, TOUCH_BTN_PERSON_6 },
    { 20U, 357U, 440U, 30U, TOUCH_BTN_PERSON_7 },
    { 0U, 430U, 800U, 50U, TOUCH_BTN_BACK },
};
#define SETTINGS_MAP_COUNT  (sizeof(s_settings_map) / sizeof(s_settings_map[0]))

static const ButtonMap_t s_name_map[] = {
    {  80U, 160U, 50U, 38U, TOUCH_BTN_NAME_A },
    { 135U, 160U, 50U, 38U, TOUCH_BTN_NAME_B },
    { 190U, 160U, 50U, 38U, TOUCH_BTN_NAME_C },
    { 245U, 160U, 50U, 38U, TOUCH_BTN_NAME_D },
    { 300U, 160U, 50U, 38U, TOUCH_BTN_NAME_E },
    { 355U, 160U, 50U, 38U, TOUCH_BTN_NAME_F },
    { 410U, 160U, 50U, 38U, TOUCH_BTN_NAME_G },
    { 465U, 160U, 50U, 38U, TOUCH_BTN_NAME_H },
    { 520U, 160U, 50U, 38U, TOUCH_BTN_NAME_I },
    { 575U, 160U, 50U, 38U, TOUCH_BTN_NAME_J },
    { 630U, 160U, 50U, 38U, TOUCH_BTN_NAME_K },
    { 685U, 160U, 50U, 38U, TOUCH_BTN_NAME_L },
    {  80U, 205U, 50U, 38U, TOUCH_BTN_NAME_M },
    { 135U, 205U, 50U, 38U, TOUCH_BTN_NAME_N },
    { 190U, 205U, 50U, 38U, TOUCH_BTN_NAME_O },
    { 245U, 205U, 50U, 38U, TOUCH_BTN_NAME_P },
    { 300U, 205U, 50U, 38U, TOUCH_BTN_NAME_Q },
    { 355U, 205U, 50U, 38U, TOUCH_BTN_NAME_R },
    { 410U, 205U, 50U, 38U, TOUCH_BTN_NAME_S },
    { 465U, 205U, 50U, 38U, TOUCH_BTN_NAME_T },
    { 520U, 205U, 50U, 38U, TOUCH_BTN_NAME_U },
    { 575U, 205U, 50U, 38U, TOUCH_BTN_NAME_V },
    { 630U, 205U, 50U, 38U, TOUCH_BTN_NAME_W },
    { 685U, 205U, 50U, 38U, TOUCH_BTN_NAME_X },
    { 245U, 250U, 50U, 38U, TOUCH_BTN_NAME_Y },
    { 300U, 250U, 50U, 38U, TOUCH_BTN_NAME_Z },
    { 355U, 250U, 105U, 38U, TOUCH_BTN_NAME_SPACE },
    { 465U, 250U, 105U, 38U, TOUCH_BTN_NAME_BACKSPACE },
    { 580U, 92U, 100U, 48U, TOUCH_BTN_ADMIN_TOGGLE },
    { 190U, 370U, 180U, 48U, TOUCH_BTN_NAME_CANCEL },
    { 430U, 370U, 180U, 48U, TOUCH_BTN_NAME_OK },
};
#define NAME_MAP_COUNT  (sizeof(s_name_map) / sizeof(s_name_map[0]))

/*
 * GT911 physical sensor range on the STM32N6570-DK:
 *   X: 0 .. 51200  (= 64 × 800)
 *   Y: 0 .. 7680   (= 16 × 480)
 *
 * The config registers return 800×480 (the logical target), but the chip
 * still outputs raw sensor coordinates — so we hardcode the physical range.
 */
#define GT911_RAW_X_MAX       51200U
#define GT911_RAW_Y_MAX       7680U
#define GT911_SCREEN_X_MAX    800U
#define GT911_SCREEN_Y_MAX    480U

/* ── Module state ────────────────────────────────────────────────────────── */
static uint8_t s_finger_down = 0U;
static uint16_t s_gt911_addr = GT911_ADDR_0x5D;

/* ── Private helpers ─────────────────────────────────────────────────────── */

/**
 * @brief  Read 'len' bytes from a 16-bit GT911 register.
 */
static inline int32_t _gt_read(uint16_t reg, uint8_t *buf, uint16_t len)
{
    return BSP_I2C2_ReadReg16(s_gt911_addr, reg, buf, len);
}

/**
 * @brief  Write 'len' bytes to a 16-bit GT911 register.
 */
static inline int32_t _gt_write(uint16_t reg, uint8_t *buf, uint16_t len)
{
    return BSP_I2C2_WriteReg16(s_gt911_addr, reg, buf, len);
}

/**
 * @brief  Clear the GT911 data-ready flag so new events can be received.
 */
static void _gt_clear(void)
{
    uint8_t zero = 0x00U;
    _gt_write(GT911_REG_STATUS, &zero, 1U);
}

/* ── ButtonMap_Check ─────────────────────────────────────────────────────── */

TouchButton_t ButtonMap_Check(uint16_t tx, uint16_t ty,
                               const ButtonMap_t *map, uint8_t count)
{
    for (uint8_t i = 0U; i < count; i++)
    {
        if (tx >= map[i].x              &&
            tx <  map[i].x + map[i].w  &&
            ty >= map[i].y              &&
            ty <  map[i].y + map[i].h)
        {
            return map[i].btn;
        }
    }
    return TOUCH_BTN_NONE;
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void Touch_Init(void)
{
    s_finger_down = 0U;

    /*
     * Step 1 — Release GT911 hardware reset.
     *
     * The GT911 NRST pin is PE1.  It must be driven HIGH before the chip
     * will respond on I2C.  Without this the chip stays in reset and every
     * I2C transaction silently fails.
     *
     * Sequence: pull NRST low briefly, then release (high) and wait 100 ms.
     */
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = GPIO_PIN_1;            /* PE1 = TS_NRST */
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &gpio);

    /* Pulse reset: low → high */
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);
    HAL_Delay(20U);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET);
    HAL_Delay(100U);    /* GT911 needs ~100 ms to boot after reset release */

    /* Step 2 — Initialise I2C2 bus */
    BSP_I2C2_Init();

    /* Step 3 — Verify GT911 is alive (product ID = "911") */
    uint8_t pid[4] = {0};
    int32_t rc = _gt_read(GT911_REG_PRODUCT, pid, 4U);
    if (rc != BSP_ERROR_NONE)
    {
        s_gt911_addr = GT911_ADDR_0x14;
        rc = _gt_read(GT911_REG_PRODUCT, pid, 4U);
    }
    if (rc == BSP_ERROR_NONE) {
        printf("Touch: GT911 I2C addr=0x%02X\n", (unsigned)(s_gt911_addr & 0xFFU));
        printf("Touch: GT911 found — ID=%c%c%c\n", pid[0], pid[1], pid[2]);
    } else {
        printf("Touch: GT911 not responding (I2C err %ld)\n", (long)rc);
    }

    /*
     * Step 4 — The GT911 config registers return 800×480 (logical target)
     * but the chip outputs raw physical sensor coordinates up to 51200×7680.
     * We hardcode the physical range and scale down to screen pixels.
     */
    printf("Touch: scaling raw range %u x %u to screen %u x %u\n",
           (unsigned)GT911_RAW_X_MAX, (unsigned)GT911_RAW_Y_MAX,
           (unsigned)GT911_SCREEN_X_MAX, (unsigned)GT911_SCREEN_Y_MAX);

    /* Step 5 — Clear any stale touch buffer */
    _gt_clear();
}

TouchButton_t Touch_GetButton(AppState_t state)
{
    uint8_t status = 0U;

    if (_gt_read(GT911_REG_STATUS, &status, 1U) != BSP_ERROR_NONE) {
        return TOUCH_BTN_NONE;
    }

    uint8_t ready  = (status >> 7U) & 0x01U;
    uint8_t ntouches = status & 0x0FU;

    if (!ready || ntouches == 0U) {
        _gt_clear();
        s_finger_down = 0U;
        return TOUCH_BTN_NONE;
    }

    if (s_finger_down) {
        _gt_clear();
        return TOUCH_BTN_NONE;
    }
    s_finger_down = 1U;

    uint8_t tp[8] = {0};
    if (_gt_read(GT911_REG_TP0, tp, 8U) != BSP_ERROR_NONE) {
        _gt_clear();
        return TOUCH_BTN_NONE;
    }
    _gt_clear();

    uint16_t raw_x = (uint16_t)tp[1] | ((uint16_t)tp[2] << 8U);
    uint16_t raw_y = (uint16_t)tp[3] | ((uint16_t)tp[4] << 8U);

    uint32_t tx_val;
    uint32_t ty_val;

    if ((raw_x < GT911_SCREEN_X_MAX) && (raw_y < GT911_SCREEN_Y_MAX))
    {
        tx_val = raw_x;
        ty_val = raw_y;
    }
    else
    {
        tx_val = ((uint32_t)raw_x * GT911_SCREEN_X_MAX) / GT911_RAW_X_MAX;
        ty_val = ((uint32_t)raw_y * GT911_SCREEN_Y_MAX) / GT911_RAW_Y_MAX;
    }

    if (tx_val >= GT911_SCREEN_X_MAX) tx_val = GT911_SCREEN_X_MAX - 1U;
    if (ty_val >= GT911_SCREEN_Y_MAX) ty_val = GT911_SCREEN_Y_MAX - 1U;

    uint16_t tx = (uint16_t)tx_val;
    uint16_t ty = (uint16_t)ty_val;

    TouchButton_t result = TOUCH_BTN_NONE;

    switch (state)
    {
        case APP_STATE_MAIN:
            result = ButtonMap_Check(tx, ty, s_main_map, MAIN_MAP_COUNT);
            break;
        case APP_STATE_SETTINGS:
            result = ButtonMap_Check(tx, ty, s_settings_map, SETTINGS_MAP_COUNT);
            break;
        case APP_STATE_ENROLL_NAME:
            result = ButtonMap_Check(tx, ty, s_name_map, NAME_MAP_COUNT);
            break;
        case APP_STATE_CHAT_KEYBOARD:
            result = ButtonMap_Check(tx, ty, s_name_map, NAME_MAP_COUNT);
            break;
        default:
            break;
    }

    /* Print RAW and SCALED to help you calibrate */
    printf("Touch RAW:(%u,%u) -> SCREEN:(%u,%u)", raw_x, raw_y, tx, ty);
    if (result != TOUCH_BTN_NONE)
        printf(" HIT btn=%d\n", (int)result);
    else
        printf(" no match\n");

    return result;
}
