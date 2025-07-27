#include "stm32f10x.h"
#include "GPIO.h"

void LED_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;  // PA8
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_SetBits(GPIOA, GPIO_Pin_8);
}

// LED控制函数
void LED_OFF(void)
{
    GPIO_ResetBits(GPIOA, GPIO_Pin_8);  // 低电平点亮LED
}

void LED_ON(void)
{
    GPIO_SetBits(GPIOA, GPIO_Pin_8);    // 高电平熄灭LED
}

void GPIO_Init_All(void)
{
    LED_Init();
} 

