#ifndef APP_H
#define APP_H

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// 系统配置
#define MODE_SELECT 1

// 系统状态定义
typedef enum {
    SYSTEM_SLEEP_MODE = 0,
    SYSTEM_ACTIVE_MODE = 1
} SystemMode_t;

// 城市定义
typedef enum {
    CITY_SUZHOU = 0,
    CITY_NANJING = 1
} City_t;

// 应用程序控制块
typedef struct {
    SystemMode_t current_mode;
    City_t current_city;
    uint32_t last_vibration_time;
    uint32_t ota_flag;
    uint8_t led_state;
    uint8_t time_hours;
    uint8_t time_minutes;
    uint8_t time_seconds;
    uint32_t system_uptime;
} AppControlBlock_t;

// OLED消息类型枚举
typedef enum {
    OLED_MSG_SHOW_STRING,    // 显示字符串
    OLED_MSG_SHOW_LARGE,     // 显示大字体
    OLED_MSG_CLEAR,          // 清屏
    OLED_MSG_SET_MODE        // 设置显示模式
} OLED_MessageType_t;

// OLED队列消息结构体
typedef struct {
    OLED_MessageType_t type;
    uint8_t x;
    uint8_t y;
    char str[32];
    uint8_t mode;            // 用于模式切换
    uint8_t city_index;      // 城市索引
} OLED_Message_t;

// 全局变量声明
extern AppControlBlock_t g_app_ctrl;
extern SemaphoreHandle_t g_oled_mutex;
extern QueueHandle_t g_oled_queue;

// OLED异步显示接口函数声明
void OLED_ShowString_Async(uint8_t line, uint8_t column, const char *str);
void OLED_ShowLarge_Async(uint8_t line, uint8_t column, const char *str);
void OLED_Clear_Async(void);
void OLED_SetMode_Async(uint8_t mode, uint8_t city_idx);

// 函数声明
void APP_Init(void);
void APP_SystemInit(void);
void APP_TaskInit(void);
void APP_GPIOInit(void);
void APP_UpdateTime(void);
void APP_UpdateMode(SystemMode_t new_mode);
void APP_UpdateCity(City_t new_city);
void APP_HandleButtonEvents(void);
void APP_HandleSensorEvents(void);

// 任务函数声明
void APP_LEDTask(void *pvParameters);
void APP_OLEDTask(void *pvParameters);
void APP_SensorTask(void *pvParameters);
void APP_TimeTask(void *pvParameters);

#endif // APP_H
