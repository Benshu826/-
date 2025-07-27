#include "main.h"

#include "delay.h"
#include "usart.h"
#include "I2C.h"
#include "SPI.h"
#include "boot.h"

#include "AT24C02.h"
#include "FLASH.h"
#include "W25Q64.h"

uint32_t BootState;
OTA_CB OTA;
UpData Updata_A;

uint8_t wdata[2048];  // 写数据缓冲区
uint8_t rdata[2048];  // 读数据缓冲区


int main()
{
	uint8_t i = 0;
	
	// 系统初始化
	delay_init();
	USART1_Init(921600);
	I2C1_Init();
	W25Q64_Init();
	
	AT24C02_ReadOTA();
	
	Bootloader_Jump();
	
	while(1)
	{
		delay_ms(10);
		if(UCB.URxDataOUT != UCB.URxDataIN)
		{
			// 处理当前数据包
			Bootloader_Event(UCB.URxDataOUT->start, UCB.URxDataOUT->end - UCB.URxDataOUT->start + 1);
			
			// 更新队列指针
			UCB.URxDataOUT++;
			if(UCB.URxDataOUT == UCB.URxDataEND)
			{
				UCB.URxDataOUT = &UCB.URxDataPtr[0];
			}
		}
		
		// XMODEM协议处理
		if(BootState & IAP_XMODEMD_FLAG)
		{
			if(Updata_A.XmodemTimer >= 100)
			{
				uprintf("C");
				Updata_A.XmodemTimer = 0;
			}
			Updata_A.XmodemTimer++;
		}
		
		// 固件更新处理
		if(BootState & UPDATA_A_FLAG)
		{
			uprintf("Updating %d bytes of firmware \r\n", OTA.FireLen[Updata_A.W25Q64_BlockNM]);
			
			// 擦除目标Flash区域
			FLASH_Erase(FLASH_A_SPAGE, FLASH_A_NUM);
			uprintf("Flash in Area A has been erased successfully \r\n");
			
			// 检查文件长度（必须4字节对齐）
			if(OTA.FireLen[Updata_A.W25Q64_BlockNM] % 4 == 0)
			{
				// 按1KB页块传输固件
				for(i = 0; i < (OTA.FireLen[Updata_A.W25Q64_BlockNM] / 1024); i++)
				{
					W25Q64_Read(Updata_A.UpDataBuff, i * FLASH_PAGE_SIZE + 64 * 1024 * Updata_A.W25Q64_BlockNM, FLASH_PAGE_SIZE);
					FLASH_Write(FLASH_A_SADDR + i * FLASH_PAGE_SIZE, (uint32_t *)Updata_A.UpDataBuff, FLASH_PAGE_SIZE);
				}
				
				// 处理剩余数据
				if(OTA.FireLen[Updata_A.W25Q64_BlockNM] % 1024 != 0)
				{
					memset(Updata_A.UpDataBuff, 0, FLASH_PAGE_SIZE);
					
					W25Q64_Read(Updata_A.UpDataBuff, i * FLASH_PAGE_SIZE + 64 * 1024 * Updata_A.W25Q64_BlockNM, OTA.FireLen[Updata_A.W25Q64_BlockNM] % 1024);
					
					FLASH_Write(FLASH_A_SADDR + i * FLASH_PAGE_SIZE, (uint32_t *)Updata_A.UpDataBuff, OTA.FireLen[Updata_A.W25Q64_BlockNM] % 1024);
				}
				
				if(Updata_A.W25Q64_BlockNM == 0)
				{
					OTA.OTA_flag = 0;
					AT24C02_WriteOTA();
				}
				
				uprintf("\r\n Firmware update in Area A completed! System is restarting... \r\n");
				
				__set_FAULTMASK(1);
				delay_ms(100);
				NVIC_SystemReset();
			}
			else
			{
				uprintf("Error: Firmware length is not a multiple of 4 bytes, cannot write to Flash \r\n");
				BootState &= ~UPDATA_A_FLAG;
				Bootloader_Info();
			}
		}
	}
}


