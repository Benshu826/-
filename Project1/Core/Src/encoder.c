#include "encoder.h"


int Read_Speed(TIM_HandleTypeDef* htim)
{
	int tmp;
	// 
	tmp = (short)__HAL_TIM_GetCounter(htim);
	__HAL_TIM_SetCounter(htim, 0);
	return tmp;
}

