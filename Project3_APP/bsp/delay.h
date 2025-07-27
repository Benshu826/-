#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"

void Delay_init(void);
void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);
void Delay_s(uint32_t s);
void Delay_xms(uint16_t ms);  // 延时ms,不会引起任务调度

#endif

