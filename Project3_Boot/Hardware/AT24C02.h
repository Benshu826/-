#include "stm32f10x.h"

#ifndef AT24C02_H
#define AT24C02_H

#define AT24C02_WADDR 0xA0
#define AT24C02_RADDR 0xA1

uint8_t AT24C02_WriteByte(uint8_t addr, uint8_t wdata);
uint8_t AT24C02_WritePage(uint8_t addr, uint8_t *wdata);
uint8_t AT24C02_ReadData(uint8_t addr, uint8_t *rdata, uint16_t datalen);
void AT24C02_ReadOTA(void);
void AT24C02_WriteOTA(void);

#endif

