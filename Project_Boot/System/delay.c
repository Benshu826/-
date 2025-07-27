// 基于SysTick的精确延时函数

#include "delay.h"

// 延时功能初始化
void delay_init(void)
{
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
}

// 微秒级延时
void delay_us(uint16_t us)
{
	SysTick_Config(SystemCoreClock / 1000000);
	
	while(us--)
	{
		while(!((SysTick->CTRL) & (SysTick_CTRL_COUNTFLAG)));
	}
	
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

// 毫秒级延时
void delay_ms(uint16_t ms)
{
	SysTick_Config(SystemCoreClock / 1000);
	
	while(ms--)
	{
		while(!((SysTick->CTRL) & (SysTick_CTRL_COUNTFLAG)));
	}
	
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}









