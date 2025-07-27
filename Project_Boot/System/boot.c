// STM32F103 Bootloader核心功能实现

#include "boot.h"
#include "main.h"

#include "usart.h"
#include "delay.h"

#include "FLASH.h"
#include "AT24C02.h"
#include "W25Q64.h"

load_a load_A;

extern uint32_t BootState;

// 从AT24C02最后一页读取OTA_flag（4字节）
uint32_t AT24C02_ReadOTAFlag(void)
{
	uint8_t addr = 0xF0; // 最后一页开始地址
	uint32_t flag = 0;
	uint8_t *pflag = (uint8_t *)&flag;
	
	// 读取4字节的OTA_flag
	for(int i = 0; i < 4; i++)
	{
		uint8_t data = 0;
		AT24C02_ReadData(addr + i, &data, 1);
		if(data == 0xFF) // 读取失败
		{
			return 0; // 返回0表示读取失败
		}
		pflag[i] = data;
	}
	
	return flag;
}

// Bootloader进入检测函数
uint8_t Bootloader_Enter(uint8_t timeout)
{
	uprintf("Enter lowercase letter w to enter Bootloader command line \r\n");
	while(timeout--)
	{
		delay_ms(200);
		if(URx_Buff[0] == 'w')
		{
			return 1;
		}
	}
	return 0;
}

// 显示Bootloader命令菜单
void Bootloader_Info(void)
{
	uprintf(" \r\n");
	uprintf("[1] Erase Area A \r\n");
	uprintf("[2] Serial IAP download to Area A \r\n");
	uprintf("[3] Set OTA version number \r\n");
	uprintf("[4] Query OTA version number \r\n");
	uprintf("[5] Download program to external FLASH \r\n");
	uprintf("[6] Use program in external FLASH \r\n");
	uprintf("[7] Restart \r\n");
}

// 设置主栈指针
__ASM void MSR_SP(uint32_t addr)
{
	MSR MSP, r0;
	BX r14;
}

// 跳转到A区应用程序
void LOAD_A(uint32_t addr)
{
	if((*(uint32_t *)addr >= 0x20000000) && (*(uint32_t *)addr <= 0x20004FFF))
	{
		MSR_SP(*(uint32_t *)addr);
		load_A = (load_a)*(uint32_t *)(addr + 4);
		load_A();
	}
	else
	{
		uprintf("Failed to jump to Area A \r\n");
	}
}

// Bootloader跳转逻辑控制
void Bootloader_Jump(void)
{
	// 读取并打印OTA_flag
	uint32_t OTA_flag = AT24C02_ReadOTAFlag();
	uprintf("OTA_flag: %d \r\n", OTA_flag);

	// 如果OTA_flag == 1，直接跳转APP
	if(OTA_flag == 1) {
		uprintf("Direct jump to APP\r\n");
		LOAD_A(FLASH_A_SADDR);
		return;
	}
	
	// 继续原有Bootloader逻辑
	if(Bootloader_Enter(20) == 0)
	{
		if(OTA_flag == OTA_SET_FLAG)
		{
			uprintf("OTA update \r\n");
			BootState |= UPDATA_A_FLAG;
			Updata_A.W25Q64_BlockNM = 0;
		}
		else
		{
			uprintf("Jump to Area A \r\n");
			LOAD_A(FLASH_A_SADDR);
		}
	}
	uprintf("Enter Bootloader command line \r\n");
	Bootloader_Info();
}

// 处理串口命令和数据
void Bootloader_Event(uint8_t *data, uint16_t datalen)
{
	int temp, i;
	
	if(BootState == 0)
	{
		if(datalen == 1 && data[0] == '1')
		{
			uprintf("[1] Erase Area A \r\n");
			FLASH_Erase(FLASH_A_SPAGE, FLASH_A_NUM);
			delay_ms(100);
			Bootloader_Info();
		}
		else if(datalen == 1 && data[0] == '2')
		{
			uprintf("Download Area A program via Xmodem protocol, use bin file \r\n");
			FLASH_Erase(FLASH_A_SPAGE, FLASH_A_NUM);
			BootState |= (IAP_XMODEMD_FLAG | IAP_XMODEM_FLAG);
			Updata_A.XmodemTimer = 0;
			Updata_A.XmodemNB = 0;
		}
		else if(datalen == 1 && data[0] == '3')
		{
			uprintf("Set version number \r\n");
			BootState |= SET_VERSION_FLAG;
		}
		else if(datalen == 1 && data[0] == '4')
		{
			uprintf("Query version number \r\n");
			AT24C02_ReadOTA();
			uprintf("Version: %s \r\n", OTA.OTA_Ver);
		}
		else if(datalen == 1 && data[0] == '5')
		{
			uprintf("Download program to external FLASH, enter the block number to use (1 ~ 9) \r\n");
			BootState |= CMD_5_FLAG;
		}
		else if(datalen == 1 && data[0] == '6')
		{
			uprintf("Use external FLASH to download program, enter the block number to use (1 ~ 9) \r\n");
			BootState |= CMD_6_FLAG;
		}
		else if(datalen == 1 && data[0] == '7')
		{
			uprintf("Restarting... \r\n");
			__set_FAULTMASK(1);
			delay_ms(100);
			NVIC_SystemReset();
		}
	}
	else if(BootState & IAP_XMODEMD_FLAG)
	{
		// 接收数据包（133字节：1个字节+128字节+2CRC+2Byte序号）
		if(datalen == 133 && data[0] == 0x01)
		{
			// 清除Xmodem标志
			BootState &= ~IAP_XMODEM_FLAG;
			// 计算CRC16校验
			Updata_A.XmodemCRC = Xmodem_CRC16(&data[3], 128);
			// 校验CRC16
			if(Updata_A.XmodemCRC == data[131] * 256 + data[132])
			{
				// 更新数据包序号
				Updata_A.XmodemNB++;
				// 将数据包复制到缓冲区
				memcpy(&Updata_A.UpDataBuff[((Updata_A.XmodemNB - 1) % (FLASH_PAGE_SIZE / 128)) * 128], &data[3], 128);
				// 每8个数据包发送一次
				if(Updata_A.XmodemNB % (FLASH_PAGE_SIZE / 128) == 0)
				{
					// 如果使用外部FLASH
					if(BootState & CMD5_XMODEM_FLAG)
					{
						// 将数据包写入外部FLASH
						for(i = 0; i < 4; i++)
						{
							W25Q64_PageWrite(&Updata_A.UpDataBuff[i * 256], (Updata_A.XmodemNB / 8 - 1) * 4 + i + Updata_A.W25Q64_BlockNM * 64 * 4);
						}
					}
					// 如果使用内部FLASH --- A区
					else
					{
						// 将数据包写入内部FLASH --- A区
						FLASH_Write(FLASH_A_SADDR + ((Updata_A.XmodemNB / (FLASH_PAGE_SIZE / 128)) - 1) * FLASH_PAGE_SIZE, (uint32_t *)Updata_A.UpDataBuff, FLASH_PAGE_SIZE);
					}
				}
				// 发送成功字符
				uprintf("\x06");
			}
			else
			{
				// 发送失败字符
				uprintf("\x15");
			}
		}
		// 如果数据包还有剩余
		if(datalen == 1 && data[0] == 0x04)
		{
			uprintf("\x06");
			// 如果数据包不是最后一个
			if((Updata_A.XmodemNB % (FLASH_PAGE_SIZE / 128)) != 0)
			{
				// 如果使用外部FLASH
				if(BootState & CMD5_XMODEM_FLAG)
				{
					// 将数据包写入外部FLASH
					for(i = 0; i < 4; i++)
					{
						W25Q64_PageWrite(&Updata_A.UpDataBuff[i * 256], (Updata_A.XmodemNB / 8) * 4 + i + Updata_A.W25Q64_BlockNM * 64 * 4);
					}
				}
				else
				{
					// 将数据包写入内部FLASH --- A区
					FLASH_Write(FLASH_A_SADDR + ((Updata_A.XmodemNB / (FLASH_PAGE_SIZE / 128))) * FLASH_PAGE_SIZE, (uint32_t *)Updata_A.UpDataBuff, (Updata_A.XmodemNB % (FLASH_PAGE_SIZE / 128)) * 128);
				}
			}
			// 清除Xmodem标志
			BootState &= ~IAP_XMODEMD_FLAG;
			// 如果使用外部FLASH
			if(BootState & CMD5_XMODEM_FLAG)
			{
				// 清除CMD5_XMODEM_FLAG标志
				BootState &= ~CMD5_XMODEM_FLAG;
				// 更新文件长度
				OTA.FireLen[Updata_A.W25Q64_BlockNM] = Updata_A.XmodemNB * 128;
				// 写入版本号到AT24C02
				AT24C02_WriteOTA();
				// 延时100ms
				delay_ms(100);
				Bootloader_Info();
			}
			else
			{
				// 设置故障屏蔽
				__set_FAULTMASK(1);
				delay_ms(100);
				// 重启
				NVIC_SystemReset();
			}
		}
	}
	else if(BootState & SET_VERSION_FLAG)		// VER-1.2.3-4/5/6-7:8
	{
		if(datalen <= 32)
		{
			if(sscanf((const char *)data, "VER-%d.%d.%d-%d/%d/%d-%d:%d", &temp, &temp, &temp, &temp, &temp, &temp, &temp, &temp) == 8)
			{
				memset(OTA.OTA_Ver, 0, 32);
				memcpy(OTA.OTA_Ver, data, 32);
				AT24C02_WriteOTA();
				uprintf("Version number is correct \r\n");
				Bootloader_Info();
				BootState &= ~SET_VERSION_FLAG;
			}
			else
			{
				uprintf("Version number format error \r\n");
			}
		}
		else
		{
			uprintf("Version number length error \r\n");
		}
	}
	else if(BootState & CMD_5_FLAG)
	{
		if(datalen == 1)
		{
			if(data[0] >= 0x31 && data[0] <= 0x39)
			{
				// 设置块编号
				Updata_A.W25Q64_BlockNM = data[0] - 0x30;
				// 设置Xmodem标志
				BootState |= (IAP_XMODEM_FLAG | IAP_XMODEMD_FLAG | CMD5_XMODEM_FLAG);
				// 初始化Xmodem计时器和序号
				Updata_A.XmodemTimer = 0;
				Updata_A.XmodemNB = 0;
				// 初始化文件长度
				OTA.FireLen[Updata_A.W25Q64_BlockNM] = 0;
				// 擦除外部FLASH
				W25Q64_Erase_64k(Updata_A.W25Q64_BlockNM);
				// 打印信息
				uprintf("Download program to block %d of external FLASH via Xmodem protocol, use bin file \r\n", Updata_A.W25Q64_BlockNM);
				// 清除CMD_5_FLAG标志
				BootState &= ~CMD_5_FLAG;
			}
			else
			{
				uprintf("Block number error \r\n");
			}
		}
		else
		{
			uprintf("Data format error \r\n");
		}
	}
	else if(BootState & CMD_6_FLAG)
	{
		if(datalen == 1)
		{
			if(data[0] >= 0x31 && data[0] <= 0x39)
			{
				// 设置块编号
				Updata_A.W25Q64_BlockNM = data[0] - 0x30;
				// 设置UPDATA_A_FLAG标志
				BootState |= UPDATA_A_FLAG;
				// 清除CMD_6_FLAG标志
				BootState &= ~CMD_6_FLAG;
			}
			else
			{
				uprintf("Block number error \r\n");
			}
		}
		else
		{
			uprintf("Length error \r\n");
		}
	}
}

uint16_t Xmodem_CRC16(uint8_t *data, uint16_t datalen)
{
	uint8_t i;
	uint16_t Crcinit = 0x0000;
	uint16_t Poly = 0x1021;

	while (datalen--)
	{
		Crcinit = (*data << 8) ^ Crcinit;
		for (i = 0; i < 8; i++)
		{
			if (Crcinit & 0x8000)
				Crcinit = (Crcinit << 1) ^ Poly;
			else
				Crcinit = (Crcinit << 1);
		}
		data++;
	}
	return Crcinit;
}

