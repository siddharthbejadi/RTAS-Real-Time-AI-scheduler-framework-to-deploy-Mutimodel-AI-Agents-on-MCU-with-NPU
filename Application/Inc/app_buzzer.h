/**
 ******************************************************************************
 * @file    app_buzzer.h
 * @brief   Non-blocking GPIO buzzer driver for STM32N6570-DK.
 *
 *  Connect a passive piezo buzzer (or small speaker) between the buzzer pin
 *  and GND.  The driver toggles the GPIO at audio frequency to create tones.
 *
 *  Default pin: PF14  (Arduino connector CN11 pin D2)
 *  Change BUZZER_GPIO_PORT / BUZZER_GPIO_PIN below if needed.
 *
 *  Usage:
 *    1. Call Buzzer_Init() once at startup.
 *    2. Call Buzzer_Play(BEEP_xxx) to trigger a sound pattern.
 *    3. Call Buzzer_Update() every main-loop iteration (once per frame).
 *       It is non-blocking — it uses HAL_GetTick() internally.
 *
 ******************************************************************************
 */

#ifndef APP_BUZZER_H
#define APP_BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ── Hardware pin — change these to match your wiring ───────────────────── */
#define BUZZER_GPIO_PORT    GPIOF
#define BUZZER_GPIO_PIN     GPIO_PIN_14
#define BUZZER_CLK_ENABLE() __HAL_RCC_GPIOF_CLK_ENABLE()

/* ── Tone patterns ───────────────────────────────────────────────────────── */
typedef enum {
    BEEP_NONE        = 0,
    BEEP_AUTH_OK,        /* Single short high beep  — authorised            */
    BEEP_DENIED,         /* Two short low beeps     — access denied          */
    BEEP_BACKING_OFF,    /* Slow repeating warn tone — too many people       */
    BEEP_SETTINGS_OPEN,  /* Single soft click       — entering settings      */
    BEEP_LOGOUT,         /* Descending two-tone     — logged out             */
} BeepType_t;

/* ── API ─────────────────────────────────────────────────────────────────── */

/** @brief  Configure the buzzer GPIO.  Call once at startup. */
void Buzzer_Init(void);

/**
 * @brief  Queue a sound pattern to play.
 *         If a pattern is already playing it is replaced immediately.
 * @param  beep  Pattern to play (BEEP_NONE = silence).
 */
void Buzzer_Play(BeepType_t beep);

/**
 * @brief  Drive the buzzer state machine.
 *         Must be called once per main-loop iteration.
 *         Execution time: < 5 µs (no blocking delays).
 */
void Buzzer_Update(void);

/** @brief  Return 1 if a pattern is currently playing, 0 if silent. */
uint8_t Buzzer_IsPlaying(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_BUZZER_H */
