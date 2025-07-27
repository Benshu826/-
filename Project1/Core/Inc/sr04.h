#ifndef __SR04_H__
#define __SR04_H__

#include "stm32f1xx_hal.h"


void RCCdelay_us(uint32_t udelay);
void Get_Dist(void);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif
