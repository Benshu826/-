#include "stm32f10x.h"

#ifndef W25Q64_H
#define W25Q64_H

#define CS_ENABLE GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define CS_DISABLE GPIO_SetBits(GPIOA, GPIO_Pin_4)

void W25Q64_Init(void);
void W25Q64_WaitBusy(void);
void W25Q64_Enable(void);
void W25Q64_Erase_64k(uint8_t block);
void W25Q64_PageWrite(uint8_t *wbuff, uint16_t page);
void W25Q64_Read(uint8_t *rbuff, uint32_t addr, uint32_t datalen);

#endif

