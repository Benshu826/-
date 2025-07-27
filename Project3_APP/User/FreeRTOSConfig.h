#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f10x.h"

// 针对不同的编译器，调用不同的stdint.h文件
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
#include <stdint.h>
extern uint32_t SystemCoreClock;
#endif

// 断言配置 - 用于调试，当条件为假时停止程序
#define configASSERT(x)           \
    if ((x) == 0)                 \
    {                             \
        taskDISABLE_INTERRUPTS(); \
        while (1)                 \
            ;                     \
    }

/************************************************************************
 *               FreeRTOS基础配置选项
 *********************************************************************/

/**
 * 调度器类型选择
 * 1: 抢占式调度器 - 高优先级任务可以抢占低优先级任务
 * 0: 协作式调度器 - 任务必须主动释放CPU才能切换
 * 
 * 抢占式：就像急诊病人可以插队，高优先级任务立即执行
 * 协作式：就像排队，必须等前面的人办完事才能轮到下一个
 */
#define configUSE_PREEMPTION 1

/**
 * 时间片调度
 * 1: 启用时间片 - 同优先级任务轮流执行
 * 0: 禁用时间片 - 同优先级任务需要主动让出CPU
 */
#define configUSE_TIME_SLICING 1

/**
 * 任务选择优化
 * 1: 使用硬件优化 - 更快找到最高优先级任务
 * 0: 使用软件查找 - 兼容性更好但速度较慢
 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1

/**
 * 无滴答空闲模式
 * 1: 启用 - 空闲时停止系统滴答，省电
 * 0: 禁用 - 始终保持系统滴答运行
 */
#define configUSE_TICKLESS_IDLE 0

/**
 * CPU时钟频率
 * 写入实际的CPU主频，用于计算延时时间
 * 例如：72MHz = 72000000
 */
#define configCPU_CLOCK_HZ (SystemCoreClock)

/**
 * 系统滴答频率
 * 每秒中断次数，决定任务调度的精度
 * 1000 = 1ms一次中断，精度较高
 * 100 = 10ms一次中断，精度较低但省电
 */
#define configTICK_RATE_HZ ((TickType_t)1000)

/**
 * 最大优先级数量
 * 优先级范围：0 ~ (configMAX_PRIORITIES-1)
 * 数字越小优先级越高，0为最高优先级
 * 建议：根据实际任务数量设置，不要设置过大
 */
#define configMAX_PRIORITIES (16)

/**
 * 空闲任务栈大小
 * 空闲任务用于系统空闲时的处理
 * 单位：字（4字节）
 */
#define configMINIMAL_STACK_SIZE ((unsigned short)64)

/**
 * 任务名称最大长度
 * 用于调试时显示任务名称
 * 建议：8-16个字符
 */
#define configMAX_TASK_NAME_LEN (12)

/**
 * 滴答计数器数据类型
 * 1: 16位 - 最大65535个滴答（约65秒）
 * 0: 32位 - 最大4294967295个滴答（约49天）
 */
#define configUSE_16_BIT_TICKS 0

/**
 * 空闲任务让出CPU
 * 1: 启用 - 空闲时让出CPU给同优先级任务
 * 0: 禁用 - 空闲任务一直运行
 */
#define configIDLE_SHOULD_YIELD 1

/**
 * 队列集功能
 * 1: 启用 - 可以等待多个队列
 * 0: 禁用 - 只能等待单个队列
 */
#define configUSE_QUEUE_SETS 0

/**
 * 任务通知功能
 * 1: 启用 - 轻量级任务间通信
 * 0: 禁用 - 只能使用队列和信号量
 */
#define configUSE_TASK_NOTIFICATIONS 1

/**
 * 互斥量功能
 * 1: 启用 - 用于保护共享资源
 * 0: 禁用 - 无法使用互斥量
 */
#define configUSE_MUTEXES 1

/**
 * 递归互斥量功能
 * 1: 启用 - 同一任务可以多次获取同一个互斥量
 * 0: 禁用 - 只能获取一次
 */
#define configUSE_RECURSIVE_MUTEXES 1

/**
 * 计数信号量功能
 * 1: 启用 - 用于资源计数
 * 0: 禁用 - 无法使用计数信号量
 */
#define configUSE_COUNTING_SEMAPHORES 1

/**
 * 事件组功能
 * 1: 启用 - 用于多任务同步
 * 0: 禁用 - 无法使用事件组
 */
#define configUSE_EVENT_GROUPS 1

/**
 * 队列注册表大小
 * 用于调试时跟踪队列和信号量
 * 建议：根据实际使用的队列数量设置
 */
#define configQUEUE_REGISTRY_SIZE 6

/**
 * 任务标签功能
 * 1: 启用 - 可以为任务设置标签
 * 0: 禁用 - 无法使用任务标签
 */
#define configUSE_APPLICATION_TASK_TAG 0

/*****************************************************************
              FreeRTOS内存管理配置选项
*****************************************************************/

/**
 * 动态内存分配
 * 1: 启用 - 可以使用xTaskCreate等动态创建函数
 * 0: 禁用 - 只能使用静态创建函数
 */
#define configSUPPORT_DYNAMIC_ALLOCATION 1

/**
 * 静态内存分配
 * 1: 启用 - 可以使用xTaskCreateStatic等静态创建函数
 * 0: 禁用 - 只能使用动态创建函数
 */
#define configSUPPORT_STATIC_ALLOCATION 0

/**
 * 堆内存大小
 * 用于动态内存分配，包括任务栈、队列等
 * 单位：字节
 * 建议：根据实际内存使用情况设置
 */
#define configTOTAL_HEAP_SIZE ((size_t)(14 * 1024))

/***************************************************************
             FreeRTOS钩子函数配置选项
**************************************************************/

/**
 * 空闲钩子函数
 * 1: 启用 - 空闲时调用用户定义的函数
 * 0: 禁用 - 不调用钩子函数
 */
#define configUSE_IDLE_HOOK 0

/**
 * 滴答钩子函数
 * 1: 启用 - 每次系统滴答时调用用户定义的函数
 * 0: 禁用 - 不调用钩子函数
 */
#define configUSE_TICK_HOOK 0

/**
 * 内存分配失败钩子函数
 * 1: 启用 - 内存分配失败时调用用户定义的函数
 * 0: 禁用 - 不调用钩子函数
 */
#define configUSE_MALLOC_FAILED_HOOK 0

/**
 * 栈溢出检查
 * 0: 禁用 - 不检查栈溢出
 * 1: 方法1 - 检查栈顶是否被覆盖
 * 2: 方法2 - 检查栈底是否被覆盖（更严格）
 */
#define configCHECK_FOR_STACK_OVERFLOW 0

/********************************************************************
          FreeRTOS运行时间和任务状态收集配置选项
**********************************************************************/

/**
 * 运行时间统计
 * 1: 启用 - 可以统计任务运行时间
 * 0: 禁用 - 不统计运行时间
 */
#define configGENERATE_RUN_TIME_STATS 0

/**
 * 可视化跟踪调试
 * 1: 启用 - 支持FreeRTOS+Trace等调试工具
 * 0: 禁用 - 不支持跟踪调试
 */
#define configUSE_TRACE_FACILITY 1

/**
 * 统计格式化函数
 * 1: 启用 - 提供格式化统计信息的函数
 * 0: 禁用 - 不提供格式化函数
 */
#define configUSE_STATS_FORMATTING_FUNCTIONS 1

/********************************************************************
                FreeRTOS协程配置选项
*********************************************************************/

/**
 * 协程功能
 * 1: 启用 - 可以使用协程（轻量级任务）
 * 0: 禁用 - 无法使用协程
 * 注意：启用后必须添加croutine.c文件
 */
#define configUSE_CO_ROUTINES 0

/**
 * 协程优先级数量
 * 协程的优先级范围：0 ~ (configMAX_CO_ROUTINE_PRIORITIES-1)
 */
#define configMAX_CO_ROUTINE_PRIORITIES (2)

/***********************************************************************
                FreeRTOS软件定时器配置选项
**********************************************************************/

/**
 * 软件定时器功能
 * 1: 启用 - 可以使用软件定时器
 * 0: 禁用 - 无法使用软件定时器
 */
#define configUSE_TIMERS 1

/**
 * 软件定时器任务优先级
 * 建议：设置为较高优先级，确保定时器及时处理
 */
#define configTIMER_TASK_PRIORITY (configMAX_PRIORITIES - 1)

/**
 * 软件定时器队列长度
 * 用于存储定时器命令的队列大小
 */
#define configTIMER_QUEUE_LENGTH 5

/**
 * 软件定时器任务栈大小
 * 软件定时器服务任务的栈大小
 */
#define configTIMER_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE)

/************************************************************
            FreeRTOS可选函数配置选项
************************************************************/

/**
 * 任务调度器状态查询函数
 * 1: 启用 - 可以使用xTaskGetSchedulerState()
 * 0: 禁用 - 无法查询调度器状态
 */
#define INCLUDE_xTaskGetSchedulerState 1

/**
 * 任务优先级设置函数
 * 1: 启用 - 可以使用vTaskPrioritySet()
 * 0: 禁用 - 无法动态修改任务优先级
 */
#define INCLUDE_vTaskPrioritySet 1

/**
 * 任务优先级获取函数
 * 1: 启用 - 可以使用uxTaskPriorityGet()
 * 0: 禁用 - 无法获取任务优先级
 */
#define INCLUDE_uxTaskPriorityGet 1

/**
 * 任务删除函数
 * 1: 启用 - 可以使用vTaskDelete()
 * 0: 禁用 - 无法删除任务
 */
#define INCLUDE_vTaskDelete 1

/**
 * 任务清理资源函数
 * 1: 启用 - 可以使用vTaskCleanUpResources()
 * 0: 禁用 - 无法清理任务资源
 */
#define INCLUDE_vTaskCleanUpResources 1

/**
 * 任务挂起函数
 * 1: 启用 - 可以使用vTaskSuspend()
 * 0: 禁用 - 无法挂起任务
 */
#define INCLUDE_vTaskSuspend 1

/**
 * 任务延时直到指定时间函数
 * 1: 启用 - 可以使用vTaskDelayUntil()
 * 0: 禁用 - 无法使用绝对延时
 */
#define INCLUDE_vTaskDelayUntil 1

/**
 * 任务延时函数
 * 1: 启用 - 可以使用vTaskDelay()
 * 0: 禁用 - 无法使用相对延时
 */
#define INCLUDE_vTaskDelay 1

/**
 * 任务状态获取函数
 * 1: 启用 - 可以使用eTaskGetState()
 * 0: 禁用 - 无法获取任务状态
 */
#define INCLUDE_eTaskGetState 1

/**
 * 定时器挂起函数调用
 * 1: 启用 - 可以使用xTimerPendFunctionCall()
 * 0: 禁用 - 无法使用定时器挂起函数调用
 */
#define INCLUDE_xTimerPendFunctionCall 0

/**
 * 任务栈高水位标记函数
 * 1: 启用 - 可以使用uxTaskGetStackHighWaterMark()
 * 0: 禁用 - 无法检查栈使用情况
 */
#define INCLUDE_uxTaskGetStackHighWaterMark 1

/******************************************************************
            FreeRTOS中断配置选项
******************************************************************/

/**
 * 中断优先级位数
 * STM32F103使用4位优先级
 */
#ifdef __NVIC_PRIO_BITS
#define configPRIO_BITS __NVIC_PRIO_BITS
#else
#define configPRIO_BITS 4
#endif

/**
 * 中断最低优先级
 * 数值越大优先级越低
 * 15 = 最低优先级
 */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15

/**
 * 系统可管理的最高中断优先级
 * 数值越小优先级越高
 * 1 = 较高优先级，可以被FreeRTOS管理
 * 0 = 最高优先级，不能被FreeRTOS管理
 */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 1

/**
 * 内核中断优先级
 * 用于FreeRTOS内核的中断优先级
 */
#define configKERNEL_INTERRUPT_PRIORITY (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS)) /* 240 */

/**
 * 系统调用最高中断优先级
 * 可以被FreeRTOS管理的中断最高优先级
 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/****************************************************************
            FreeRTOS中断服务函数配置选项
****************************************************************/

/**
 * PendSV中断处理函数映射
 * 用于任务切换的中断处理函数
 */
#define xPortPendSVHandler PendSV_Handler

/**
 * SVC中断处理函数映射
 * 用于启动第一个任务的中断处理函数
 */
#define vPortSVCHandler SVC_Handler

#endif /* FREERTOS_CONFIG_H */
