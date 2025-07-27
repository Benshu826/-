#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

// DHT11数据线定义 - PB11
#define dht11_high GPIO_SetBits(GPIOB, GPIO_Pin_11)
#define dht11_low GPIO_ResetBits(GPIOB, GPIO_Pin_11)
#define Read_Data GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11)

// 温湿度数据结构体
typedef struct {
    uint8_t humidity_int;      // 湿度整数部分
    uint8_t humidity_dec;      // 湿度小数部分
    uint8_t temperature_int;   // 温度整数部分
    uint8_t temperature_dec;   // 温度小数部分
    uint8_t check_sum;         // 校验和
} DHT11_Data_TypeDef;

// 函数声明
void DHT11_Init(void);
void DHT11_GPIO_Init_OUT(void);
void DHT11_GPIO_Init_IN(void);
void DHT11_Start(void);
char DHT11_Rec_Byte(void);
void DHT11_REC_Data(void);
uint8_t DHT11_Read_Data(DHT11_Data_TypeDef *data);
uint8_t DHT11_Read_Temperature(void);
uint8_t DHT11_Read_Humidity(void);

// 全局数据数组
extern unsigned int rec_data[4];

#endif
