#include "stm32f10x.h"

#ifndef MAIN_H
#define MAIN_H

#define FLASH_SADDR 0x08000000
#define FLASH_PAGE_SIZE 1024
#define FLASH_NUM 64
#define FLASH_B_NUM 20
#define FLASH_A_NUM FLASH_NUM - FLASH_B_NUM
#define FLASH_A_SPAGE FLASH_B_NUM
#define FLASH_A_SADDR FLASH_SADDR + FLASH_A_SPAGE * FLASH_PAGE_SIZE

#define UPDATA_A_FLAG 0x00000001
#define IAP_XMODEM_FLAG 0x00000002
#define IAP_XMODEMD_FLAG 0x00000004
#define SET_VERSION_FLAG 0x00000008
#define CMD_5_FLAG 0x000000010
#define CMD5_XMODEM_FLAG 0x000000020
#define CMD_6_FLAG 0x000000040

#define OTA_SET_FLAG 0xAABBCCDD


typedef struct
{
    uint32_t OTA_flag;
    uint32_t FireLen[11];			// 0号成员固定对应OTA的大小
    uint8_t OTA_Ver[32];
} OTA_CB;

typedef struct
{
    uint8_t UpDataBuff[FLASH_PAGE_SIZE];
    uint32_t W25Q64_BlockNM;
    uint32_t XmodemTimer;
    uint32_t XmodemNB;
    uint32_t XmodemCRC;
} UpData;

extern OTA_CB OTA;
extern UpData Updata_A;

#define OTA_INFO_SIZE sizeof(OTA_CB)

#endif

