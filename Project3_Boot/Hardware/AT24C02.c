// AT24C02 EEPROM驱动程序 - Bootloader配置存储

#include "AT24C02.h"
#include "delay.h"
#include "I2C.h"
#include "main.h"
#include "string.h"



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

// AT24C02页写入（8字节）
uint8_t AT24C02_WritePage(uint8_t addr, uint8_t *wdata)
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
	for(int i = 0; i < 8; i++)
	{
		I2C_SendByte(wdata[i]);
		if(I2C_Wait_ACK(100) != 0)
		{
			return 3 + i;
		}
	}
	I2C_Stop();
	return 0;
}

// AT24C02数据读取
uint8_t AT24C02_ReadData(uint8_t addr, uint8_t *rdata, uint16_t datalen)
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
	I2C_Start();
	I2C_SendByte(AT24C02_RADDR);
	if(I2C_Wait_ACK(100) != 0)
	{
		return 3;
	}
	for(int i = 0; i < datalen - 1; i++)
	{
		rdata[i] = I2C_ReadByte(1);
	}
	rdata[datalen - 1] = I2C_ReadByte(0);
	I2C_Stop();
	return 0;
}

// 写入OTA配置到EEPROM
void AT24C02_WriteOTA(void)
{
	uint8_t i;
	uint8_t *pdata = (uint8_t *)&OTA;
	
	for(i = 0; i < (OTA_INFO_SIZE + 7) / 8; i++)
	{
		AT24C02_WritePage(i * 8, pdata + i * 8);
    	delay_ms(10);
	}
}

// 从EEPROM读取OTA配置
void AT24C02_ReadOTA(void)
{
	memset(&OTA, 0, OTA_INFO_SIZE);
	AT24C02_ReadData(0, (uint8_t *)&OTA, OTA_INFO_SIZE);
}





