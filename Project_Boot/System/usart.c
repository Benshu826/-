// USART串口通信驱动程序

#include "usart.h"

uint8_t URx_Buff[URx_SIZE];
uint8_t UTx_Buff[UTx_SIZE];



// USART1初始化
void USART1_Init(uint32_t band)
{
	// 使能时钟
	// RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);
	
	// 配置PA9为TX（复用推挽输出）
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	// 配置PA10为RX（浮空输入）
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	// 配置USART1参数
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = band;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1, &USART_InitStructure);
	
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
	
	// 配置中断
	NVIC_SetPriorityGrouping(NVIC_PriorityGroup_2);
	
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStructure);
	
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
	
	// 配置DMA
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
	DMA_InitTypeDef DMA_InitStructure;
	DMA_InitStructure.DMA_BufferSize = URx_MAX + 1;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)URx_Buff;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);
	
	DMA_Cmd(DMA1_Channel5, ENABLE);
	
	// 初始化串口控制块
	UCB.URxCounter = 0;
	UCB.URxDataIN = &UCB.URxDataPtr[0];
	UCB.URxDataOUT = &UCB.URxDataPtr[0];
	UCB.URxDataEND = &UCB.URxDataPtr[NUM - 1];
	UCB.URxDataIN->start = URx_Buff;
	
	USART_Cmd(USART1, ENABLE);
}

// 格式化打印函数
void uprintf(char *format, ...)
{
    va_list list_data;
    va_start(list_data, format);

    vsnprintf((char *)UTx_Buff, sizeof(UTx_Buff), format, list_data);
    va_end(list_data);

    uint16_t len = strlen((const char *)UTx_Buff);
    
    for (uint16_t i = 0; i < len; i++)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) != 1);
        USART_SendData(USART1, UTx_Buff[i]);
    }

    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) != 1);
}


