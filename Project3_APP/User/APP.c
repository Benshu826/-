#include "APP.h"
#include "delay.h"
#include "GPIO.h"
#include "OLED.h"
#include "DHT11.h"
#include "MPU6050.h"
#include "AT24C02.h"
#include "I2C.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// 编译时模式选择
// 0: 仅简易模式，1: 简易+多功能模式
#define MODE_SELECT 1

// 全局变量定义
// 应用程序控制块，存储系统状态信息
AppControlBlock_t g_app_ctrl = {
    .current_mode = SYSTEM_SLEEP_MODE,    // 初始为休眠模式
    .current_city = CITY_SUZHOU,          // 初始城市为苏州
    .last_vibration_time = 0,             // 上次震动时间
    .ota_flag = 0,                        // OTA标志
    .led_state = 0,                       // LED状态
    .time_hours = 0,                      // 小时
    .time_minutes = 0,                    // 分钟
    .time_seconds = 0,                    // 秒
    .system_uptime = 0                    // 系统运行时间
};

// FreeRTOS同步对象
SemaphoreHandle_t g_oled_mutex = NULL;    // OLED显示互斥量
QueueHandle_t g_oled_queue = NULL;        // OLED消息队列

// 任务句柄
static TaskHandle_t g_led_task_handle = NULL;      // LED任务句柄
static TaskHandle_t g_oled_task_handle = NULL;     // OLED任务句柄
static TaskHandle_t g_sensor_task_handle = NULL;   // 传感器任务句柄
static TaskHandle_t g_time_task_handle = NULL;     // 时间任务句柄

// 按键状态标志，防止重复触发
static uint8_t g_pa2_pressed = 0;     // PA2按键状态
static uint8_t g_pa0_pressed = 0;     // PA0按键状态
static uint8_t g_pc13_pressed = 0;    // PC13按键状态
static uint8_t g_pb10_pressed = 0;    // PB10按键状态

// 系统常量定义
#define VIBRATION_THRESHOLD 6000       // 震动检测阈值
#define AUTO_SLEEP_TIME 10000          // 自动休眠时间：10秒
#define SENSOR_READ_INTERVAL 1000      // 传感器读取间隔：1秒
#define BUTTON_DEBOUNCE_TIME 50        // 按键消抖时间：50ms
#define PC13_DEBOUNCE_TIME 50          // PC13按键消抖时间：50ms

/**
 * @brief AT24C02 EEPROM初始化函数
 * @note 防止重复初始化，从EEPROM读取OTA标志
 */
void AT24C02_Init(void)
{
    static uint8_t initialized = 0;
    if(initialized) return;  // 防止重复初始化
    
    // 从AT24C02读取OTA标志
    g_app_ctrl.ota_flag = AT24C02_ReadOTAFlag();
    
    // 如果读取的值无效，设置为默认值0
    if(g_app_ctrl.ota_flag != 0x00000001 && g_app_ctrl.ota_flag != 0x00000000)
    {
        g_app_ctrl.ota_flag = 0x00000000;
        AT24C02_WriteOTAFlag(g_app_ctrl.ota_flag);
    }
    
    initialized = 1;  // 标记已初始化
}

/**
 * @brief 切换OTA标志函数
 * @note 在0和1之间切换，并写入EEPROM
 */
void Toggle_OTA_Flag(void)
{
    // 简单的0和1切换
    g_app_ctrl.ota_flag = (g_app_ctrl.ota_flag == 0x00000001) ? 0x00000000 : 0x00000001;
    AT24C02_WriteOTAFlag(g_app_ctrl.ota_flag);  // 写入EEPROM
    Delay_ms(10);  // 等待写入完成
}

/**
 * @brief OLED异步显示字符串
 * @param line 行号
 * @param column 列号
 * @param str 要显示的字符串
 * @note 通过队列发送消息到OLED任务
 */
void OLED_ShowString_Async(uint8_t line, uint8_t column, const char *str)
{
    OLED_Message_t msg = {
        .type = OLED_MSG_SHOW_STRING,
        .x = column,
        .y = line
    };
    strncpy(msg.str, str, sizeof(msg.str)-1);
    msg.str[sizeof(msg.str)-1] = '\0';  // 确保字符串结束
    xQueueSend(g_oled_queue, &msg, pdMS_TO_TICKS(10));
}

/**
 * @brief OLED异步显示大字体字符串
 * @param line 行号
 * @param column 列号
 * @param str 要显示的字符串
 */
void OLED_ShowLarge_Async(uint8_t line, uint8_t column, const char *str)
{
    OLED_Message_t msg = {
        .type = OLED_MSG_SHOW_LARGE,
        .x = column,
        .y = line
    };
    strncpy(msg.str, str, sizeof(msg.str)-1);
    msg.str[sizeof(msg.str)-1] = '\0';
    xQueueSend(g_oled_queue, &msg, pdMS_TO_TICKS(10));
}

/**
 * @brief OLED异步清屏
 */
void OLED_Clear_Async(void)
{
    OLED_Message_t msg = {.type = OLED_MSG_CLEAR};
    xQueueSend(g_oled_queue, &msg, pdMS_TO_TICKS(10));
}

/**
 * @brief OLED异步设置显示模式
 * @param mode 模式：0-休眠模式，1-多功能模式
 * @param city_idx 城市索引
 */
void OLED_SetMode_Async(uint8_t mode, uint8_t city_idx)
{
    OLED_Message_t msg = {
        .type = OLED_MSG_SET_MODE,
        .mode = mode,
        .city_index = city_idx
    };
    xQueueSend(g_oled_queue, &msg, pdMS_TO_TICKS(10));
}

/**
 * @brief 应用程序初始化
 * @note 创建FreeRTOS对象，初始化硬件，创建任务
 */
void APP_Init(void)
{
    // 创建OLED队列和互斥量
    g_oled_queue = xQueueCreate(10, sizeof(OLED_Message_t));  // 10个消息容量
    g_oled_mutex = xSemaphoreCreateMutex();  // 创建互斥量
    
    // 初始化硬件
    APP_SystemInit();
    
    // 创建任务
    APP_TaskInit();
}

/**
 * @brief 系统硬件初始化
 * @note 初始化时钟、GPIO、外设等
 */
void APP_SystemInit(void)
{
    // 关闭JTAG，保留SWD，释放PB3/PB4/PB15为普通GPIO
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    // 初始化延时函数
    Delay_init();
    
    // 初始化LED
    LED_Init();
    
    // 初始化GPIO按键
    APP_GPIOInit();
    
    // 初始化I2C和AT24C02
    I2C1_Init();
    AT24C02_Init();
}

/**
 * @brief GPIO初始化
 * @note 配置所有按键引脚为浮空输入
 */
void APP_GPIOInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIO时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOB, ENABLE);
    
    // 配置GPIO为浮空输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    // 配置PA2和PA0按键
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_0;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置PC13按键
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // 配置PB10按键
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/**
 * @brief 任务初始化
 * @note 创建所有FreeRTOS任务，设置优先级和栈大小
 */
void APP_TaskInit(void)
{
    // 创建OLED任务（最高优先级）
    xTaskCreate(APP_OLEDTask, "OLED", 512, NULL, 4, &g_oled_task_handle);
    
    // 创建LED任务（处理按键和传感器事件）
    xTaskCreate(APP_LEDTask, "LED_Task", 80, NULL, 3, &g_led_task_handle);
    
    // 创建时间任务（最低优先级）
    xTaskCreate(APP_TimeTask, "Time", 256, NULL, 1, &g_time_task_handle);
    
#if MODE_SELECT == 1
    // 创建传感器任务（仅多功能模式）
    xTaskCreate(APP_SensorTask, "Sensor", 512, NULL, 2, &g_sensor_task_handle);
#endif
}

/**
 * @brief 更新时间计数
 * @note 每秒调用一次，更新时分秒和系统运行时间
 */
void APP_UpdateTime(void)
{
    g_app_ctrl.time_seconds++;
    if(g_app_ctrl.time_seconds >= 60)
    {
        g_app_ctrl.time_seconds = 0;
        g_app_ctrl.time_minutes++;
        if(g_app_ctrl.time_minutes >= 60)
        {
            g_app_ctrl.time_minutes = 0;
            g_app_ctrl.time_hours++;
            if(g_app_ctrl.time_hours >= 24)
            {
                g_app_ctrl.time_hours = 0;
            }
        }
    }
    g_app_ctrl.system_uptime++;  // 系统运行时间递增
}

/**
 * @brief 更新系统模式
 * @param new_mode 新的系统模式
 * @note 切换模式时更新LED状态和OLED显示
 */
void APP_UpdateMode(SystemMode_t new_mode)
{
    if(g_app_ctrl.current_mode != new_mode)
    {
        g_app_ctrl.current_mode = new_mode;
        
        // 根据模式更新LED状态
        if(new_mode == SYSTEM_ACTIVE_MODE)
        {
            LED_ON();
            g_app_ctrl.led_state = 1;
        }
        else
        {
            LED_OFF();
            g_app_ctrl.led_state = 0;
        }
        
        // 发送模式切换消息到OLED任务
        OLED_SetMode_Async((new_mode == SYSTEM_ACTIVE_MODE) ? 1 : 0, g_app_ctrl.current_city);
    }
}

/**
 * @brief 更新城市设置
 * @param new_city 新的城市
 */
void APP_UpdateCity(City_t new_city)
{
    if(g_app_ctrl.current_city != new_city)
    {
        g_app_ctrl.current_city = new_city;
    }
}

/**
 * @brief 通用按键检测函数
 * @param GPIOx GPIO端口
 * @param GPIO_Pin GPIO引脚
 * @param pressed_flag 按键状态标志指针
 * @param debounce_time 消抖时间
 * @return 1:按键按下，0:按键未按下
 * @note 实现按键消抖，防止重复触发
 */
static uint8_t CheckButton(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t* pressed_flag, uint16_t debounce_time)
{
    if(GPIO_ReadInputDataBit(GPIOx, GPIO_Pin) == 0)  // 按键按下（低电平）
    {
        if(*pressed_flag == 0)  // 第一次检测到按下
        {
            Delay_ms(debounce_time);  // 消抖延时
            if(GPIO_ReadInputDataBit(GPIOx, GPIO_Pin) == 0)  // 再次确认按下
            {
                *pressed_flag = 1;  // 设置按下标志
                return 1;  // 返回按键按下
            }
        }
    }
    else  // 按键释放
    {
        *pressed_flag = 0;  // 清除按下标志
    }
    return 0;  // 返回按键未按下
}

// 处理按键事件
void APP_HandleButtonEvents(void)
{
    // 检测PA2按键（切换到多功能模式）
#if MODE_SELECT == 1
    if(CheckButton(GPIOA, GPIO_Pin_2, &g_pa2_pressed, BUTTON_DEBOUNCE_TIME))
    {
        if(g_app_ctrl.current_mode == SYSTEM_SLEEP_MODE)
        {
            APP_UpdateMode(SYSTEM_ACTIVE_MODE);
            g_app_ctrl.last_vibration_time = xTaskGetTickCount();
        }
        else if(g_app_ctrl.current_mode == SYSTEM_ACTIVE_MODE)
        {
            g_app_ctrl.last_vibration_time = xTaskGetTickCount();
        }
    }
#endif

    // 检测PA0按键
    if(CheckButton(GPIOA, GPIO_Pin_0, &g_pa0_pressed, BUTTON_DEBOUNCE_TIME))
    {
#if MODE_SELECT == 0
        City_t new_city = (g_app_ctrl.current_city == CITY_SUZHOU) ? CITY_NANJING : CITY_SUZHOU;
        APP_UpdateCity(new_city);
#else
        if(g_app_ctrl.current_mode == SYSTEM_ACTIVE_MODE) {
            APP_UpdateMode(SYSTEM_SLEEP_MODE);
        } else {
            City_t new_city = (g_app_ctrl.current_city == CITY_SUZHOU) ? CITY_NANJING : CITY_SUZHOU;
            APP_UpdateCity(new_city);
        }
#endif
    }

    // 检测PC13按键（切换OTA_flag）
    if(CheckButton(GPIOC, GPIO_Pin_13, &g_pc13_pressed, PC13_DEBOUNCE_TIME))
    {
        Toggle_OTA_Flag();
        g_app_ctrl.last_vibration_time = xTaskGetTickCount();
    }

    // 检测PB10按键（重启系统）
    if(CheckButton(GPIOB, GPIO_Pin_10, &g_pb10_pressed, BUTTON_DEBOUNCE_TIME))
    {
#if MODE_SELECT == 0
        NVIC_SystemReset();
#else
        if(g_app_ctrl.current_mode == SYSTEM_SLEEP_MODE)
        {
            NVIC_SystemReset();
        }
#endif
    }
}

// 处理传感器事件
void APP_HandleSensorEvents(void)
{
#if MODE_SELECT == 1
    uint32_t current_time = xTaskGetTickCount();
    if(g_app_ctrl.current_mode == SYSTEM_ACTIVE_MODE && 
       (current_time - g_app_ctrl.last_vibration_time > pdMS_TO_TICKS(AUTO_SLEEP_TIME)))
    {
        APP_UpdateMode(SYSTEM_SLEEP_MODE);
    }
#endif
}

/**
 * @brief LED任务
 * @param pvParameters 任务参数（未使用）
 * @note 处理按键和传感器事件，每50ms执行一次
 */
void APP_LEDTask(void *pvParameters)
{
    LED_ON();  // 初始点亮LED
    
    while(1)
    {
        APP_HandleButtonEvents();    // 处理按键事件
        APP_HandleSensorEvents();    // 处理传感器事件
        vTaskDelay(pdMS_TO_TICKS(50));  // 50ms延时
    }
}

/**
 * @brief 时间任务
 * @param pvParameters 任务参数（未使用）
 * @note 每秒更新系统时间
 */
void APP_TimeTask(void *pvParameters)
{
    while(1)
    {
        APP_UpdateTime();  // 更新时间
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1秒延时
    }
}

/**
 * @brief 传感器任务
 * @param pvParameters 任务参数（未使用）
 * @note 读取MPU6050和DHT11传感器数据，检测震动
 */
void APP_SensorTask(void *pvParameters)
{
    int16_t AccX, AccY, AccZ, GyroX, GyroY, GyroZ;
    uint32_t last_read_time = 0;
    uint32_t current_time;
    int32_t accel_mag, accel_mag_prev;
    DHT11_Data_TypeDef dht11_data;
    uint8_t dht11_status;

    // 初始化传感器
    MPU6050_Init();
    DHT11_Init();
    Delay_ms(100);  // 等待传感器稳定

    // 初始化加速度值
    MPU6050_GetData(&AccX, &AccY, &AccZ, &GyroX, &GyroY, &GyroZ);
    accel_mag_prev = (AccX * AccX + AccY * AccY + AccZ * AccZ) / 100;

    while(1)
    {
        current_time = xTaskGetTickCount();

        // 每秒读取一次传感器数据
        if(current_time - last_read_time > pdMS_TO_TICKS(SENSOR_READ_INTERVAL))
        {
            // 读取MPU6050数据
            MPU6050_GetData(&AccX, &AccY, &AccZ, &GyroX, &GyroY, &GyroZ);
            accel_mag = (AccX * AccX + AccY * AccY + AccZ * AccZ) / 100;

            // 检测震动（加速度变化超过阈值）
            if(abs(accel_mag - accel_mag_prev) > VIBRATION_THRESHOLD)
            {
                g_app_ctrl.last_vibration_time = current_time;  // 更新震动时间
                // 震动唤醒：从休眠模式切换到多功能模式
                if(g_app_ctrl.current_mode == SYSTEM_SLEEP_MODE)
                {
                    APP_UpdateMode(SYSTEM_ACTIVE_MODE);
                }
            }

            // 只在多功能模式下更新传感器数据显示
            if(g_app_ctrl.current_mode == SYSTEM_ACTIVE_MODE)
            {
                // 读取DHT11温湿度数据
                dht11_status = DHT11_Read_Data(&dht11_data);
                
                if(dht11_status == 0)  // 读取成功
                {
                    // 显示温度数据
                    char temp_str[32];
                    sprintf(temp_str, "Temp: %d.%dC", dht11_data.temperature_int, dht11_data.temperature_dec);
                    OLED_ShowString_Async(3, 1, temp_str);
                    
                    // 显示湿度数据
                    char humi_str[32];
                    sprintf(humi_str, "Humi: %d.%d%%", dht11_data.humidity_int, dht11_data.humidity_dec);
                    OLED_ShowString_Async(4, 1, humi_str);
                }
                else  // 读取失败
                {
                    OLED_ShowString_Async(3, 1, "Temp: ERROR!");
                    OLED_ShowString_Async(4, 1, "Humi: ERROR!");
                }

                // 显示加速度数据
                char acc_str[32];
                sprintf(acc_str, "ACC: %ld", accel_mag);
                OLED_ShowString_Async(5, 1, acc_str);
            }

            accel_mag_prev = accel_mag;  // 保存当前加速度值
            last_read_time = current_time;  // 更新读取时间
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms延时
    }
}

/**
 * @brief 显示简易模式内容
 * @note 显示时间、日期、城市、OTA状态
 */
static void DisplaySimpleMode(void)
{
    // 显示大字体时间
    char time_big[12];
    sprintf(time_big, "%02d:%02d:%02d", g_app_ctrl.time_hours, g_app_ctrl.time_minutes, g_app_ctrl.time_seconds);
    OLED_ShowString_Large(1, 1, time_big);
    
    // 显示日期
    OLED_ShowString_Small(4, 7, "2025-07-26");
    
    // 显示城市
    if(g_app_ctrl.current_city == CITY_SUZHOU) {
        OLED_ShowString_Small(6, 9, "SUZHOU");
    } else {
        OLED_ShowString_Small(6, 9, "NANJING");
    }
    
    // 显示OTA状态
    char ota_str[4];
    sprintf(ota_str, "%d", (g_app_ctrl.ota_flag == 0x00000001) ? 1 : 0);
    OLED_ShowString_Small(7, 21, ota_str);
}

/**
 * @brief 显示多功能模式内容
 * @note 显示标题、时间、传感器数据、OTA状态
 */
static void DisplayMultiFunctionMode(void)
{
    // 显示标题
    OLED_ShowString_Async(1, 1, "Smart Desk Clock");
    
    // 显示时间
    OLED_ShowString_Async(2, 1, "Time: 00:00:00");
    
    // 显示传感器数据占位符
    OLED_ShowString_Async(3, 1, "Temp: --.-C");
    OLED_ShowString_Async(4, 1, "Humi: --.-%");
    OLED_ShowString_Async(5, 1, "ACC: --");
    
    // 显示OTA状态
    OLED_ShowString_Async(6, 1, "OTA: ");
    char ota_str[4];
    sprintf(ota_str, "%d", (g_app_ctrl.ota_flag == 0x00000001) ? 1 : 0);
    OLED_ShowString_Async(6, 8, ota_str);
}

/**
 * @brief OLED显示任务
 * @param pvParameters 任务参数（未使用）
 * @note 负责OLED显示更新，处理队列消息
 */
void APP_OLEDTask(void *pvParameters)
{
    OLED_Message_t msg;
    
    // 初始化OLED
    OLED_Init();
    OLED_Clear();
    
    // 根据初始模式显示内容
#if MODE_SELECT == 0
    DisplaySimpleMode();  // 简易模式
#else
    if(g_app_ctrl.current_mode == SYSTEM_ACTIVE_MODE)
    {
        DisplayMultiFunctionMode();  // 多功能模式
    }
    else
    {
        DisplaySimpleMode();  // 休眠模式
    }
#endif
    
    while(1)
    {
        // 定期更新时间显示（每秒更新一次）
        static uint32_t last_time_update = 0;
        uint32_t current_time = xTaskGetTickCount();
        
        if(current_time - last_time_update >= pdMS_TO_TICKS(1000))
        {
#if MODE_SELECT == 0
            // 简易模式：更新整个显示
            DisplaySimpleMode();
#else
            if(g_app_ctrl.current_mode == SYSTEM_ACTIVE_MODE)
            {
                // 多功能模式：只更新时间显示
                char time_str[24];
                sprintf(time_str, "Time: %02d:%02d:%02d", g_app_ctrl.time_hours, g_app_ctrl.time_minutes, g_app_ctrl.time_seconds);
                OLED_ShowString_Async(2, 1, time_str);
                
                // 更新OTA标志显示
                static uint32_t last_ota_display = 0;
                if(g_app_ctrl.ota_flag != last_ota_display)
                {
                    char ota_str[4];
                    sprintf(ota_str, "%d", (g_app_ctrl.ota_flag == 0x00000001) ? 1 : 0);
                    OLED_ShowString_Async(6, 8, ota_str);
                    last_ota_display = g_app_ctrl.ota_flag;
                }
            }
            else
            {
                // 休眠模式：更新整个显示
                DisplaySimpleMode();
            }
#endif
            last_time_update = current_time;
        }
        
        // 处理OLED队列消息
        if(xQueueReceive(g_oled_queue, &msg, pdMS_TO_TICKS(10)) == pdPASS)
        {
            // 获取OLED互斥量
            if(xSemaphoreTake(g_oled_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                switch(msg.type)
                {
                    case OLED_MSG_SHOW_STRING:
                        // 显示小字体字符串
                        OLED_ShowString_Small(msg.y, msg.x, msg.str);
                        break;
                        
                    case OLED_MSG_SHOW_LARGE:
                        // 显示大字体字符串
                        OLED_ShowString_Large(msg.y, msg.x, msg.str);
                        break;
                        
                    case OLED_MSG_CLEAR:
                        // 清屏
                        OLED_Clear();
                        break;
                        
                    case OLED_MSG_SET_MODE:
                        // 设置显示模式
                        OLED_Clear();
                        if(msg.mode == 0) // 休眠模式
                        {
                            DisplaySimpleMode();
                        }
                        else // 多功能模式
                        {
                            DisplayMultiFunctionMode();
                        }
                        break;
                }
                // 释放OLED互斥量
                xSemaphoreGive(g_oled_mutex);
            }
        }
    }
} 
