#include "stm32f10x.h"

#ifndef AT24C02_H
#define AT24C02_H

#define AT24C02_WADDR 0xA0
#define AT24C02_RADDR 0xA1

// AT24C02基本读写函数
uint8_t AT24C02_WriteByte(uint8_t addr, uint8_t wdata);
uint8_t AT24C02_ReadByte(uint8_t addr);

// OTA_flag专用函数
void AT24C02_WriteOTAFlag(uint32_t flag);
uint32_t AT24C02_ReadOTAFlag(void);

#endif
