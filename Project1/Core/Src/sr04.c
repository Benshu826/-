#include "sr04.h"
#include "gpio.h"
#include "pid.h"
uint16_t count;
float distance;

extern TIM_HandleTypeDef htim3;

void RCCdelay_us(uint32_t udelay)
{
	__IO uint32_t Delay = udelay * 72 / 8;
	
	do
	{
		__NOP();
	}
	while(Delay--);
}

void Get_Dist(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
	RCCdelay_us(12);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == GPIO_PIN_5)
	{
		Control();
	}
	if(GPIO_Pin == GPIO_PIN_2)
	{
		if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2)==GPIO_PIN_SET)
		{
			__HAL_TIM_SetCounter(&htim3, 0);
			HAL_TIM_Base_Start(&htim3);
		}
		else
		{
			HAL_TIM_Base_Stop(&htim3);
			count = __HAL_TIM_GetCounter(&htim3);
			distance = count * 0.017;
		}
	}
}
