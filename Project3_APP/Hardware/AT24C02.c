// AT24C02 EEPROM驱动程序 - 简化版本，只在最后一页存储OTA_flag

#include "AT24C02.h"
#include "delay.h"
#include "I2C.h"

// AT24C02单字节写入
uint8_t AT24C02_WriteByte(uint8_t addr, uint8_t wdata)
{
	I2C_Start();
	I2C_SendByte(AT24C02_WADDR);
	if(I2C_Wait_ACK(100) != 0)
	{
		return 1;
	}
	I2C_SendByte(addr);
	if(I2C_Wait_ACK(100) != 0)
	{
		return 2;
	}
	I2C_SendByte(wdata);
	if(I2C_Wait_ACK(100) != 0)
	{
		return 3;
	}
	I2C_Stop();
	return 0;
}

// AT24C02单字节读取
uint8_t AT24C02_ReadByte(uint8_t addr)
{
	uint8_t data;
	
	I2C_Start();
	I2C_SendByte(AT24C02_WADDR);
	if(I2C_Wait_ACK(100) != 0)
	{
		return 0xFF;
	}
	I2C_SendByte(addr);
	if(I2C_Wait_ACK(100) != 0)
	{
		return 0xFF;
	}
	I2C_Start();
	I2C_SendByte(AT24C02_RADDR);
	if(I2C_Wait_ACK(100) != 0)
	{
		return 0xFF;
	}
	data = I2C_ReadByte(0);
	I2C_Stop();
	return data;
}

// 在AT24C02最后一页写入OTA_flag（4字节）
void AT24C02_WriteOTAFlag(uint32_t flag)
{
	uint8_t addr = 0xF0; // 最后一页开始地址（256-16=240=0xF0）
	uint8_t *pflag = (uint8_t *)&flag;
	
	// 写入4字节的OTA_flag
	for(int i = 0; i < 4; i++)
	{
		AT24C02_WriteByte(addr + i, pflag[i]);
		Delay_ms(5); // 等待写入完成
	}
}

// 从AT24C02最后一页读取OTA_flag（4字节）
uint32_t AT24C02_ReadOTAFlag(void)
{
	uint8_t addr = 0xF0; // 最后一页开始地址
	uint32_t flag = 0;
	uint8_t *pflag = (uint8_t *)&flag;
	
	// 读取4字节的OTA_flag
	for(int i = 0; i < 4; i++)
	{
		uint8_t data = AT24C02_ReadByte(addr + i);
		if(data == 0xFF) // 读取失败
		{
			return 0; // 返回0表示读取失败
		}
		pflag[i] = data;
	}
	
	return flag;
}
