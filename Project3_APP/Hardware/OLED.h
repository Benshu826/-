#ifndef __OLED_H
#define __OLED_H
#include "stm32f10x.h"                  // Device header

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowChar_Small(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString_Small(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_SetCursor(uint8_t Y, uint8_t X);
void OLED_WriteData(uint8_t Data);

// 新增大号字体显示函数
void OLED_ShowChar_Large(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString_Large(uint8_t Line, uint8_t Column, char *String);

#endif
