/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "usart.h"

UCB_CB UCB;

/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief USART1中断处理函数
 * @param 无
 * @return 无
 * 
 * 处理USART1的IDLE（空闲）中断：
 * 1. 检测到串口空闲时，表示一个数据包接收完成
 * 2. 计算实际接收的数据长度
 * 3. 更新环形缓冲区指针
 * 4. 重新配置DMA为下一次接收做准备
 * 
 * 工作流程：
 * - IDLE中断触发 -> 数据包接收完成
 * - 计算接收长度 -> 更新数据包结束指针
 * - 移动输入指针 -> 处理环形缓冲区绕回
 * - 重启DMA接收 -> 准备接收下一个数据包
 */
void USART1_IRQHandler(void)
{
	// 检查IDLE中断标志位
	if(USART_GetFlagStatus(USART1, USART_FLAG_IDLE) != 0)
	{
		// 清除IDLE中断标志位
		USART_ClearFlag(USART1, USART_FLAG_IDLE);
		// 读取数据寄存器以清除IDLE标志（必须操作）
		USART_ReceiveData(USART1);
		
		// 计算本次接收的数据长度并更新接收计数器
		UCB.URxCounter += (URx_MAX + 1) - DMA_GetCurrDataCounter(DMA1_Channel5);
		// 设置当前数据包的结束地址
		UCB.URxDataIN->end = &URx_Buff[UCB.URxCounter - 1];
		
		// 移动输入指针到下一个数据包位置
		UCB.URxDataIN++;
		// 检查是否到达队列末尾，进行环形处理
		if(UCB.URxDataIN == UCB.URxDataEND)
		{
			UCB.URxDataIN = &UCB.URxDataPtr[0];     // 回到队列开头
		}
		
		// 检查剩余缓冲区空间，决定下一次DMA接收的起始地址
		if(URx_SIZE - UCB.URxCounter >= URx_MAX)
		{
			// 剩余空间足够，继续在当前位置接收
			UCB.URxDataIN->start = &URx_Buff[UCB.URxCounter];
		}
		else
		{
			// 剩余空间不足，回到缓冲区开头
			UCB.URxDataIN->start = URx_Buff;
			UCB.URxCounter = 0;                     // 重置接收计数器
		}
		
		// 重新配置DMA为下一次接收
		DMA_Cmd(DMA1_Channel5, DISABLE);                           // 禁用DMA
		DMA_SetCurrDataCounter(DMA1_Channel5, URx_MAX + 1);        // 设置传输计数
		DMA1_Channel5->CMAR = (uint32_t)UCB.URxDataIN->start;      // 设置内存地址
		DMA_Cmd(DMA1_Channel5, ENABLE);                            // 重新启用DMA
	}
}

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
