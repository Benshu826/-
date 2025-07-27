#include "stm32f10x.h"                  // Device header
#include "FreeRTOS.h"
#include "task.h"
#include "APP.h"
#include <string.h>
#include <stdio.h>

int main(void)
{
    // 配置中断优先级分组
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    
    // 初始化应用程序
    APP_Init();
    
    // 启动FreeRTOS调度器
    vTaskStartScheduler();
    
    // 正常情况下不会执行到这里
    while (1);
}




