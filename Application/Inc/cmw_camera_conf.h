

#ifndef CMW_CAMERA_CONF_H
#define CMW_CAMERA_CONF_H

#ifdef __cplusplus
extern "C" {
#endif


#if defined (STM32N657xx)
#include "stm32n6xx_hal.h"
#ifdef USE_STM32N6570_NUCLEO_REV_B01
#include "stm32n6xx_nucleo_bus.h"
#else
#include "stm32n6570_discovery_bus.h"
#endif
#else
#error Add header files for your specific board
#endif


#define USE_IMX335_SENSOR

#ifdef __cplusplus
}
#endif

#endif
