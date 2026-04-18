/**
 ******************************************************************************
 * @file    app_buzzer.c
 * @brief   Non-blocking GPIO buzzer driver.
 *
 *  Each BeepType has a pattern defined as a small table of (frequency, duration)
 *  steps.  The driver steps through the table using HAL_GetTick() without ever
 *  calling HAL_Delay(), so the main inference loop never stalls.
 *
 *  Frequency is approximated by toggling the GPIO every half-period (ms).
 *  e.g. 1 kHz tone → toggle every 0.5 ms → use ~1 ms tick granularity
 *       500 Hz tone → toggle every 1 ms
 *       2 kHz tone → toggle every 0.25 ms (rounds to 1 ms tick = ~1 kHz)
 *
 *  Because HAL_GetTick() has 1 ms resolution, the minimum useful frequency
 *  is ~500 Hz.  For a passive buzzer this is perfectly audible.
 *
 ******************************************************************************
 */

#include "app_buzzer.h"
#include "stm32n6xx_hal.h"

/* ── Pattern definition ──────────────────────────────────────────────────── */
/* Each row: { half_period_ms, tone_duration_ms }
   half_period_ms = 0 means silence (GPIO held LOW) for tone_duration_ms.
   The last row must have both fields = 0 (end-of-pattern sentinel).        */

typedef struct { uint32_t half_ms; uint32_t dur_ms; } Step_t;

/* ~1 kHz  = toggle every 1 ms  (close enough for a passive buzzer)         */
/* ~500 Hz = toggle every 2 ms                                               */

static const Step_t PAT_AUTH_OK[] = {
    { 1U,  120U },   /* 1 kHz  — 120 ms  */
    { 0U,   30U },   /* silence 30 ms    */
    { 1U,   60U },   /* 1 kHz  —  60 ms  */
    { 0U,    0U }    /* END               */
};

static const Step_t PAT_DENIED[] = {
    { 2U,  100U },   /* 500 Hz — 100 ms  */
    { 0U,   50U },   /* silence           */
    { 2U,  100U },   /* 500 Hz — 100 ms  */
    { 0U,    0U }    /* END               */
};

static const Step_t PAT_BACKING_OFF[] = {
    { 1U,  200U },   /* 1 kHz  — 200 ms  */
    { 0U,  200U },   /* silence 200 ms   */
    { 1U,  200U },
    { 0U,  200U },
    { 1U,  200U },
    { 0U,    0U }    /* END               */
};

static const Step_t PAT_SETTINGS[] = {
    { 1U,   40U },   /* quick soft click  */
    { 0U,    0U }    /* END               */
};

static const Step_t PAT_LOGOUT[] = {
    { 1U,   80U },   /* high               */
    { 0U,   20U },
    { 2U,   80U },   /* low                */
    { 0U,    0U }    /* END                */
};

/* ── Pattern lookup table ────────────────────────────────────────────────── */
static const Step_t * const s_patterns[] = {
    NULL,              /* BEEP_NONE         */
    PAT_AUTH_OK,       /* BEEP_AUTH_OK      */
    PAT_DENIED,        /* BEEP_DENIED       */
    PAT_BACKING_OFF,   /* BEEP_BACKING_OFF  */
    PAT_SETTINGS,      /* BEEP_SETTINGS_OPEN*/
    PAT_LOGOUT,        /* BEEP_LOGOUT       */
};

/* ── Module state ────────────────────────────────────────────────────────── */
static const Step_t *s_pat        = NULL;  /* current pattern               */
static uint8_t       s_step       = 0U;    /* current step index in pattern */
static uint32_t      s_step_start = 0U;    /* tick when current step began  */
static uint32_t      s_last_tog   = 0U;    /* tick of last GPIO toggle      */

/* ── Private helpers ─────────────────────────────────────────────────────── */
static inline void _gpio_high(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, GPIO_PIN_SET);
}
static inline void _gpio_low(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, GPIO_PIN_RESET);
}
static inline void _gpio_toggle(void)
{
    HAL_GPIO_TogglePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN);
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void Buzzer_Init(void)
{
    BUZZER_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = BUZZER_GPIO_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(BUZZER_GPIO_PORT, &gpio);

    _gpio_low();

    s_pat  = NULL;
    s_step = 0U;
}

void Buzzer_Play(BeepType_t beep)
{
    if ((uint32_t)beep >= (sizeof(s_patterns) / sizeof(s_patterns[0]))) return;

    s_pat        = s_patterns[(uint32_t)beep];
    s_step       = 0U;
    s_step_start = HAL_GetTick();
    s_last_tog   = HAL_GetTick();
    _gpio_low();
}

uint8_t Buzzer_IsPlaying(void)
{
    return (s_pat != NULL) ? 1U : 0U;
}

void Buzzer_Update(void)
{
    if (s_pat == NULL) return;

    const Step_t *step = &s_pat[s_step];

    /* End-of-pattern sentinel */
    if (step->half_ms == 0U && step->dur_ms == 0U) {
        _gpio_low();
        s_pat = NULL;
        return;
    }

    uint32_t now     = HAL_GetTick();
    uint32_t elapsed = now - s_step_start;

    /* Move to next step when this step's duration has expired */
    if (elapsed >= step->dur_ms) {
        s_step++;
        s_step_start = now;
        s_last_tog   = now;
        _gpio_low();
        return;
    }

    /* Silence step — keep GPIO low, nothing to toggle */
    if (step->half_ms == 0U) {
        _gpio_low();
        return;
    }

    /* Tone step — toggle GPIO at half_ms interval */
    if ((now - s_last_tog) >= step->half_ms) {
        _gpio_toggle();
        s_last_tog = now;
    }
}
