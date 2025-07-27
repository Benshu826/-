/**
 * @file I2C.c
 * @brief 软件模拟I2C通信驱动程序
 * @version 1.0
 * @date 2025
 * 
 * @brief 该文件实现了软件模拟I2C通信协议的基本功能：
 * - I2C总线初始化
 * - I2C起始和停止条件生成
 * - I2C字节发送和接收
 * - I2C应答等待和生成
 * 
 * 引脚分配：
 * - SCL: PB6（时钟线）
 * - SDA: PB7（数据线）
 * 
 * 注意：使用开漏输出模式，需要外接上拉电阻
 */

#include "I2C.h"
#include "delay.h"

/**
 * @brief I2C1初始化函数
 * @param 无
 * @return 无
 * 
 * 初始化I2C1接口的硬件资源：
 * 1. 使能GPIOB的时钟
 * 2. 配置PB6(SCL)和PB7(SDA)为开漏输出
 * 3. 设置初始状态为高电平（总线空闲状态）
 */
void I2C1_Init(void)
{
	// 使能GPIOB时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// 配置SCL引脚（PB6）为开漏输出
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;   // 开漏输出模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// 配置SDA引脚（PB7）为开漏输出
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;   // 开漏输出模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// 设置总线初始状态为高电平（空闲状态）
	SCL_H;
	SDA_H;
}

/**
 * @brief I2C起始条件生成函数
 * @param 无
 * @return 无
 * 
 * I2C起始条件：当SCL为高电平时，SDA由高电平变为低电平
 * 时序：SDA_H -> SCL_H -> 延时 -> SDA_L -> SCL_L
 */
void I2C_Start(void)
{
	SCL_H;                              // 确保SCL为高电平
	SDA_H;                              // 确保SDA为高电平
	Delay_us(2);                        // 延时等待信号稳定
	SDA_L;                              // SDA变为低电平（起始条件）
	SCL_L;                              // SCL变为低电平，准备发送数据
}

/**
 * @brief I2C停止条件生成函数
 * @param 无
 * @return 无
 * 
 * I2C停止条件：当SCL为高电平时，SDA由低电平变为高电平
 * 时序：SCL_L -> SDA_L -> 延时 -> SCL_H -> SDA_H
 */
void I2C_Stop(void)
{
	SCL_L;                              // 确保SCL为低电平
	SDA_L;                              // 确保SDA为低电平
	Delay_us(2);                        // 延时等待信号稳定
	SCL_H;                              // SCL变为高电平
	SDA_H;                              // SDA变为高电平（停止条件）
}

/**
 * @brief I2C字节发送函数
 * @param tx 要发送的字节数据
 * @return 无
 * 
 * 按照I2C协议发送一个字节数据（8位）
 * 数据传输顺序：MSB先传输
 * 
 */
void I2C_SendByte(uint8_t tx)
{
	// 从最高位开始发送（MSB first）
	for(int8_t i = 7; i >= 0; i--)
	{
		SCL_L;                              // SCL置低，准备设置数据
		if(tx & (1 << i))                   // 检查当前位是否为1
		{
			SDA_H;                          // 发送逻辑1
		}
		else
		{
			SDA_L;                          // 发送逻辑0
		}
		Delay_us(2);                        // 延时等待数据稳定
		SCL_H;                              // SCL置高，让从机读取数据
		Delay_us(2);                        // 延时保持时钟高电平
	}
	SCL_L;                                  // 发送完成，SCL置低
	SDA_H;                                  // 释放SDA线，准备接收ACK
}

/**
 * @brief I2C等待应答函数
 * @param timeout 等待超时时间（单位：2us）
 * @return uint8_t 错误码（0=成功，其他值为错误）
 * 
 * 等待从机发送应答信号（ACK）
 * 应答信号：从机将SDA拉低表示应答
 * 
 * 返回值说明：
 * 0 - 成功接收到应答
 * 1 - 等待超时
 * 2 - 应答信号无效
 */
uint8_t I2C_Wait_ACK(int16_t timeout)
{
	// 等待SDA变为低电平（从机应答）
	do
	{
		timeout--;
		Delay_us(2);
	}while(Read_SDA && timeout >= 0);
	
	if(timeout < 0)
	{
		return 1;                           // 等待超时
	}
	
	SCL_H;                                  // SCL置高，读取应答信号
	Delay_us(2);
	if(Read_SDA != 0)
	{
		return 2;                           // 应答信号无效
	}
	SCL_L;                                  // SCL置低，应答读取完成
	Delay_us(2);
	return 0;                               // 成功接收应答
}

/**
 * @brief I2C字节接收函数
 * @param ack 是否发送应答（1=发送ACK，0=发送NACK）
 * @return uint8_t 接收到的字节数据
 * 
 * 接收一个字节数据（8位）
 * 数据接收顺序：MSB先接收
 * 接收完成后根据ack参数决定发送ACK还是NACK
 */
uint8_t I2C_ReadByte(uint8_t ack)
{
	uint8_t rx = 0;
	
	// 从最高位开始接收（MSB first）
	for(int8_t i = 7; i >= 0; i--)
	{
		SCL_L;                              // SCL置低，准备读取数据
		Delay_us(2);
		SCL_H;                              // SCL置高，读取数据
		if(Read_SDA)                        // 读取SDA上的数据
		{
			rx |= (1 << i);                 // 如果SDA为高，设置对应位
		}
		Delay_us(2);
	}
	
	SCL_L;                                  // 数据接收完成，SCL置低
	
	// 发送应答或非应答
	if(ack)
	{
		SDA_L;                              // 发送ACK（应答）
		SCL_H;
		Delay_us(2);
		SCL_L;
		SDA_H;                              // 释放SDA线
	}
	else
	{
		SDA_H;
		SCL_H;
		Delay_us(2);
		SCL_L;
	}
	
	return rx;
} 