#include "stm32f10x.h"

#ifndef I2C_H
#define I2C_H

// 使用PB6和PB7作为I2C引脚（与Boot_Learn项目一致）
#define SCL_H GPIO_SetBits(GPIOB, GPIO_Pin_6)
#define SCL_L GPIO_ResetBits(GPIOB, GPIO_Pin_6)

#define SDA_H GPIO_SetBits(GPIOB, GPIO_Pin_7)
#define SDA_L GPIO_ResetBits(GPIOB, GPIO_Pin_7)

#define Read_SDA GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)

void I2C1_Init(void);
void I2C_Start(void);
void I2C_Stop(void);
void I2C_SendByte(uint8_t tx);
uint8_t I2C_Wait_ACK(int16_t timeout);
uint8_t I2C_ReadByte(uint8_t ack);

#endif
