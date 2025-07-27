#include "stm32f10x.h"

#ifndef BOOT_H
#define BOOT_H

#pragma diag_suppress 870

typedef void (*load_a)(void);

void Bootloader_Jump(void);
void Bootloader_Info(void);
void Bootloader_Event(uint8_t *data, uint16_t datalen);
uint16_t Xmodem_CRC16(uint8_t *data, uint16_t datalen);

#endif

