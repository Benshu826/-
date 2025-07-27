#include "delay.h"
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

static uint8_t fac_us = 0;  // us延时倍乘数
static uint16_t fac_ms = 0; // ms延时倍乘数,在RTOS下,代表每个节拍的ms数

extern void xPortSysTickHandler(void);

/**
 * @brief 时钟中断
 */
void SysTick_Handler(void)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) // 系统已经运行
    {
        xPortSysTickHandler();
    }
}

/**
 * @brief 时钟初始化 - 参考例程版本
 */
void Delay_init(void)
{
    uint32_t reload;
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK); // 选择外部时钟  HCLK
    fac_us = SystemCoreClock / 1000000;              // 不论是否使用OS,fac_us都需要使用
    reload = SystemCoreClock / 1000000;              // 每秒钟的计数次数 单位为M
    reload *= 1000000 / configTICK_RATE_HZ;          // 根据configTICK_RATE_HZ设定溢出时间
                                                     // reload为24位寄存器,最大值:16777216,在72M下,约合0.233s左右
    fac_ms = 1000 / configTICK_RATE_HZ;              // 代表OS可以延时的最少单位

    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk; // 开启SYSTICK中断
    SysTick->LOAD = reload;                    // 每1/configTICK_RATE_HZ秒中断一次
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;  // 开启SYSTICK
}

/**
 * @brief 延时微妙
 * @param us 延时0~65536微秒
 */
void Delay_us(uint32_t us)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD; // LOAD的值
    ticks = us * fac_us;            // 需要的节拍数
    told = SysTick->VAL;            // 刚进入时的计数器值
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
                tcnt += told - tnow; // 这里注意一下SYSTICK是一个递减的计数器就可以了.
            else
                tcnt += reload - tnow + told;
            told = tnow;
            if (tcnt >= ticks)
                break; // 时间超过/等于要延迟的时间,则退出.
        }
    };
}

/**
 * @brief 延时毫秒
 * @param ms 延时0~65536毫秒
 */
void Delay_ms(uint32_t ms)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) // 系统已经运行
    {
        if (ms >= fac_ms) // 延时的时间大于OS的最少时间周期
        {
            vTaskDelay(ms / fac_ms); // FreeRTOS延时
        }
        ms %= fac_ms; // OS已经无法提供这么小的延时了,采用普通方式延时
    }
    Delay_us((uint32_t)(ms * 1000)); // 普通方式延时
}

void Delay_s(uint32_t s)
{
    Delay_ms(s * 1000);
}

/**
 * @brief 延时ms,不会引起任务调度
 * @param ms 要延时的ms数
 */
void Delay_xms(uint16_t ms)
{
    uint32_t i;
    for (i = 0; i < ms; i++)
        Delay_us(1000);
}
