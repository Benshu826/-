/**
 * @file usart.h
 * @brief USART串口通信驱动头文件
 * @author Jiahuan Ni
 * @version 1.0
 * @date 2025
 * 
 * @brief 该头文件定义了USART串口通信相关的数据结构和函数声明：
 * - 串口缓冲区大小定义
 * - 串口数据管理结构体
 * - 串口初始化和打印函数声明
 * 
 * 硬件配置：
 * - 使用USART1，引脚重映射到PB6(TX)和PB7(RX)
 * - 支持DMA接收，中断处理
 * - 环形缓冲区管理机制
 */

#include "stm32f10x.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"

#ifndef USART_H
#define USART_H

// 缓冲区大小定义
#define URx_SIZE 2048           // 接收缓冲区总大小
#define UTx_SIZE 2048           // 发送缓冲区大小
#define URx_MAX 256             // 单次DMA接收最大字节数

#define NUM 10                  // 接收数据包队列深度

typedef struct
{
	uint8_t *start;             // 数据包起始地址
	uint8_t *end;               // 数据包结束地址
}UCB_URxBuff;


typedef struct
{
	uint16_t URxCounter;        // 接收计数器，记录当前接收位置
	UCB_URxBuff URxDataPtr[NUM]; // 数据包指针数组（环形队列）
	UCB_URxBuff *URxDataIN;     // 输入指针（生产者）
	UCB_URxBuff *URxDataOUT;    // 输出指针（消费者）
	UCB_URxBuff *URxDataEND;    // 队列末尾指针
}UCB_CB;

extern UCB_CB UCB;

extern uint8_t URx_Buff[URx_SIZE];


void USART1_Init(uint32_t band);
void uprintf(char *format, ...);

#endif

