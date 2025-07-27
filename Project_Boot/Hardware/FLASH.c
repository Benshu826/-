// STM32内部Flash读写操作驱动程序

#include "FLASH.h"
#include "main.h"

/**
 * @brief Flash页擦除函数
 * @param start 起始页编号（相对于FLASH_SADDR的偏移页数）
 * @param num 要擦除的页数
 * @return 无
 * 
 * 擦除指定数量的Flash页，每页1KB
 * 擦除前会自动解锁Flash，擦除后自动上锁
 * 
 * 地址计算方式：
 * 实际擦除地址 = FLASH_SADDR + start * 1024 + i * 1024
 */
void FLASH_Erase(uint16_t start, uint16_t num)
{
    uint16_t i;
    FLASH_Unlock();                     // 解锁Flash以允许擦除操作
    
    // 循环擦除指定数量的页
    for (i = 0; i < num; i++)
    {
        // 计算每页的实际地址并执行擦除
        FLASH_ErasePage((FLASH_SADDR + start * 1024) + (1024 * i));
    }
    
    FLASH_Lock();                       // 擦除完成后锁定Flash
}

/**
 * @brief Flash字写入函数
 * @param saddr 起始写入地址（必须4字节对齐）
 * @param wdata 写入数据缓冲区指针
 * @param wnum 要写入的字节数（必须是4的倍数）
 * @return 无
 * 
 * 将数据按32位字的方式写入Flash
 * 写入前会自动解锁Flash，写入后自动上锁
 * 
 * 注意事项：
 * - 起始地址必须4字节对齐
 * - 写入字节数必须是4的倍数
 * - 写入前目标区域必须已擦除（为0xFFFFFFFF状态）
 */
void FLASH_Write(uint32_t saddr, uint32_t *wdata, uint32_t wnum)
{
    FLASH_Unlock();                     // 解锁Flash以允许写入操作
    
    // 按32位字循环写入数据
    while (wnum)
    {
        FLASH_ProgramWord(saddr, *wdata);   // 写入一个32位字
        wnum -= 4;                          // 减少剩余字节数
        saddr += 4;                         // 地址递增4字节
        wdata++;                            // 数据指针递增
    }
    
    FLASH_Lock();                       // 写入完成后锁定Flash
}
