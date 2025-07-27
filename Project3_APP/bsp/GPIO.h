#ifndef __GPIO_H
#define __GPIO_H

#include "stm32f10x.h"

// LED函数声明
void LED_Init(void);
void GPIO_Init_All(void);

// LED控制函数
void LED_ON(void);
void LED_OFF(void);

#endif



