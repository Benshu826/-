/**
 * @file delay.h
 * @brief 基于SysTick的精确延时函数头文件
 * @author Jiahuan Ni
 * @version 1.0
 * @date 2025
 * 
 * @brief 该头文件声明了基于ARM Cortex-M3 SysTick定时器的精确延时函数：
 * - 延时功能初始化
 * - 微秒级精确延时
 * - 毫秒级精确延时
 * 
 * 特点：
 * - 使用硬件定时器，延时精确
 * - 支持微秒和毫秒两种时间单位
 * - 基于系统时钟，自动适应不同频率
 */

#include "stm32f10x.h"

#ifndef DELAY_H
#define DELAY_H

/**
 * @brief 延时功能初始化函数
 * @param 无
 * @return 无
 */
void delay_init(void);

/**
 * @brief 微秒级精确延时函数
 * @param us 延时时间（微秒，1-65535）
 * @return 无
 */
void delay_us(uint16_t us);

/**
 * @brief 毫秒级精确延时函数
 * @param ms 延时时间（毫秒，1-65535）
 * @return 无
 */
void delay_ms(uint16_t ms);

#endif
