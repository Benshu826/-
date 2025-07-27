// W25Q64 SPI Flash存储器驱动程序

#include "W25Q64.h"
#include "SPI.h"

// W25Q64初始化
void W25Q64_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	CS_DISABLE;
	SPI1_Init();
}

// 等待设备忙状态结束
void W25Q64_WaitBusy(void)
{
	uint8_t res;
	do
	{
		CS_ENABLE;
		SPI1_ReadWrite_Byte(0x05);
		res = SPI1_ReadWrite_Byte(0xFF);
		CS_DISABLE;
	} while((res & 0x01) == 0x01);
}

// 写使能操作
void W25Q64_Enable(void)
{
	W25Q64_WaitBusy();
	CS_ENABLE;
	SPI1_ReadWrite_Byte(0x06);
	CS_DISABLE;
}

// 64KB块擦除
void W25Q64_Erase_64k(uint8_t block)
{
	uint8_t wdata[4];
	
	wdata[0] = 0xD8;
	wdata[1] = (block * 64 * 1024) >> 16;
	wdata[2] = (block * 64 * 1024) >> 8;
	wdata[3] = (block * 64 * 1024) >> 0;
	
	W25Q64_WaitBusy();
	W25Q64_Enable();
	CS_ENABLE;
	SPI1_Write(wdata, 4);
	CS_DISABLE;
	W25Q64_WaitBusy();
}

// 页写入（256字节）
void W25Q64_PageWrite(uint8_t *wbuff, uint16_t page)
{
	uint8_t wdata[4];
	
	wdata[0] = 0x02;
	wdata[1] = (page * 256) >> 16;
	wdata[2] = (page * 256) >> 8;
	wdata[3] = (page * 256) >> 0;
	
	W25Q64_WaitBusy();
	W25Q64_Enable();
	CS_ENABLE;
	SPI1_Write(wdata, 4);
	SPI1_Write(wbuff, 256);
	CS_DISABLE;
}

// 数据读取
void W25Q64_Read(uint8_t *rbuff, uint32_t addr, uint32_t datalen)
{
	W25Q64_WaitBusy();
	CS_ENABLE;
	SPI1_ReadWrite_Byte(0x03);
	SPI1_ReadWrite_Byte((uint8_t)(addr >> 16));
	SPI1_ReadWrite_Byte((uint8_t)(addr >> 8));
	SPI1_ReadWrite_Byte((uint8_t)(addr >> 0));
	SPI1_Read(rbuff, datalen);
	CS_DISABLE;
}





