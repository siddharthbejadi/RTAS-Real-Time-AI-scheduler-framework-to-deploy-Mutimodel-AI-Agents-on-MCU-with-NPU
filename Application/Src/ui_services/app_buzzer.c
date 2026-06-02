

#include "app_buzzer.h"
#include "stm32n6xx_hal.h"


typedef struct { uint32_t half_ms; uint32_t dur_ms; } Step_t;


static const Step_t PAT_AUTH_OK[] = {
    { 1U,  120U },
    { 0U,   30U },
    { 1U,   60U },
    { 0U,    0U }
};

static const Step_t PAT_DENIED[] = {
    { 2U,  100U },
    { 0U,   50U },
    { 2U,  100U },
    { 0U,    0U }
};

static const Step_t PAT_BACKING_OFF[] = {
    { 1U,  200U },
    { 0U,  200U },
    { 1U,  200U },
    { 0U,  200U },
    { 1U,  200U },
    { 0U,    0U }
};

static const Step_t PAT_SETTINGS[] = {
    { 1U,   40U },
    { 0U,    0U }
};

static const Step_t PAT_LOGOUT[] = {
    { 1U,   80U },
    { 0U,   20U },
    { 2U,   80U },
    { 0U,    0U }
};


static const Step_t * const s_patterns[] = {
    NULL,
    PAT_AUTH_OK,
    PAT_DENIED,
    PAT_BACKING_OFF,
    PAT_SETTINGS,
    PAT_LOGOUT,
};


static const Step_t *s_pat        = NULL;
static uint8_t       s_step       = 0U;
static uint32_t      s_step_start = 0U;
static uint32_t      s_last_tog   = 0U;


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


    if (step->half_ms == 0U && step->dur_ms == 0U) {
        _gpio_low();
        s_pat = NULL;
        return;
    }

    uint32_t now     = HAL_GetTick();
    uint32_t elapsed = now - s_step_start;


    if (elapsed >= step->dur_ms) {
        s_step++;
        s_step_start = now;
        s_last_tog   = now;
        _gpio_low();
        return;
    }


    if (step->half_ms == 0U) {
        _gpio_low();
        return;
    }


    if ((now - s_last_tog) >= step->half_ms) {
        _gpio_toggle();
        s_last_tog = now;
    }
}
