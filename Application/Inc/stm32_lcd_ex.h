

#ifndef STM32_LCD_EX_H
#define STM32_LCD_EX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32_lcd.h"

#define COL(x)  ((x) * (((sFONT *)UTIL_LCD_GetFont())->Width))


void UTIL_LCDEx_PrintfAtLine(uint16_t line, const char * format, ...);
void UTIL_LCDEx_PrintfAt(uint32_t x_pos, uint32_t y_pos, Text_AlignModeTypdef mode, const char * format, ...);

#ifdef __cplusplus
}
#endif

#endif
