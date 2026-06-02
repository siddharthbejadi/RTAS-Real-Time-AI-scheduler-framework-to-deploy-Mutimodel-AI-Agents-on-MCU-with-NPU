

#ifndef APP_BUZZER_H
#define APP_BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


#define BUZZER_GPIO_PORT    GPIOF
#define BUZZER_GPIO_PIN     GPIO_PIN_14
#define BUZZER_CLK_ENABLE() __HAL_RCC_GPIOF_CLK_ENABLE()


typedef enum {
    BEEP_NONE        = 0,
    BEEP_AUTH_OK,
    BEEP_DENIED,
    BEEP_BACKING_OFF,
    BEEP_SETTINGS_OPEN,
    BEEP_LOGOUT,
} BeepType_t;


void Buzzer_Init(void);


void Buzzer_Play(BeepType_t beep);


void Buzzer_Update(void);


uint8_t Buzzer_IsPlaying(void);

#ifdef __cplusplus
}
#endif

#endif
