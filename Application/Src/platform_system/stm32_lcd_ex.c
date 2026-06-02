

#include "stm32_lcd_ex.h"
#include <stdio.h>
#include <stdarg.h>


#define N_PRINTABLE_CHARS    47


void UTIL_LCDEx_PrintfAtLine(uint16_t line, const char * format, ...)
{
  static char buffer[N_PRINTABLE_CHARS + 1];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, N_PRINTABLE_CHARS + 1, format, args);
  UTIL_LCD_DisplayStringAtLine(line, (uint8_t *) buffer);
  va_end(args);
}

void UTIL_LCDEx_PrintfAt(uint32_t x_pos, uint32_t y_pos, Text_AlignModeTypdef mode, const char * format, ...)
{
  static char buffer[N_PRINTABLE_CHARS + 1];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, N_PRINTABLE_CHARS + 1, format, args);
  UTIL_LCD_DisplayStringAt(x_pos, y_pos, (uint8_t *) buffer, mode);
  va_end(args);
}
