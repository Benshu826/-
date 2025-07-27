#include "motor.h"

#define PWM_MAX 7200
#define PWM_MIN -7200

extern TIM_HandleTypeDef htim1;
extern int stop;
int abs(int p)
{
	if(p>0)
	{
		return p;
	}
	else
	{
		return -p;
	}
}

void Load(int motorL, int motorR)			//-7200 ---- 7200
{
	if(motorL < 0)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
	}
	else
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
	}
	__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, abs(motorL));
	if(motorR < 0)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
	}
	else
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
	}
	__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, abs(motorL));
}
void Limit(int *Motor1, int* Motor2)
{
	if(*Motor1 > PWM_MAX) *Motor1 = PWM_MAX;
	if(*Motor1 < PWM_MIN) *Motor1 = PWM_MIN;
	if(*Motor2 > PWM_MAX) *Motor2 = PWM_MAX;
	if(*Motor2 < PWM_MIN) *Motor2 = PWM_MIN;
}

void Stop(float *Med_Jiaodu,float *Jiaodu)
{
	if(abs((int)(*Jiaodu-*Med_Jiaodu))>60)
	{
		Load(0,0);
		stop=1;
	}
}
