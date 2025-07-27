#ifndef _MOTOR_H
#define _MOTOR_H

#include <stm32f1xx_hal.h>

void Load(int motorL, int motorR);
void Limit(int *Motor1, int* Motor2);
void Stop(float *Med_Jiaodu,float *Jiaodu);

#endif
