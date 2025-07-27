#include "stm32f10x.h"

#ifndef SPI_H
#define SPI_H

void SPI1_Init(void);
uint16_t SPI1_ReadWrite_Byte(uint16_t tx);
void SPI1_Write(uint8_t *wdata, uint16_t datalen);
void SPI1_Read(uint8_t *rdata, uint16_t datalen);

#endif

