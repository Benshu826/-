#include "stm32f10x.h"

#ifndef FLASH_H
#define FLASH_H

void FLASH_Erase(uint16_t start, uint16_t num);
void FLASH_Write(uint32_t saddr, uint32_t *wdata, uint32_t wnum);

#endif


