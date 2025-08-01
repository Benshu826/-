# FreeRTOS智能桌面时钟项目

## 项目概述

这是一个基于STM32F103和FreeRTOS的智能桌面时钟项目，支持简易模式和多功能模式。

## 硬件配置

- **主控芯片**: STM32F103
- **显示**: OLED显示屏
- **传感器**: MPU6050（加速度计）、DHT11（温湿度）
- **存储**: AT24C02（EEPROM）
- **按键**: PA2、PA0、PC13、PB10

## 系统模式

### 简易模式 (SYSTEM_SLEEP_MODE)
- 显示时间、日期、城市、OTA状态
- 支持城市切换（苏州/南京）
- 支持OTA标志切换

### 多功能模式 (SYSTEM_ACTIVE_MODE)
- 显示时间、温湿度、加速度、OTA状态
- 支持震动唤醒
- 10秒无操作自动休眠

## FreeRTOS任务配置

### 任务优先级 (1-5，数字越大优先级越高)
- **OLED任务**: 优先级5 - 负责显示更新
- **LED任务**: 优先级4 - 处理按键和传感器事件
- **传感器任务**: 优先级2 - 读取传感器数据
- **时间任务**: 优先级1 - 更新时间计数

### 任务栈大小
- OLED任务: 512字节
- LED任务: 128字节
- 传感器任务: 512字节
- 时间任务: 256字节

## 关键参数说明

### 时间参数
```c
#define AUTO_SLEEP_TIME 10000      // 自动休眠时间：10秒
#define SENSOR_READ_INTERVAL 1000  // 传感器读取间隔：1秒
#define BUTTON_DEBOUNCE_TIME 50    // 按键消抖时间：50ms
#define PC13_DEBOUNCE_TIME 20      // PC13按键消抖时间：20ms
```

### 传感器参数
```c
#define VIBRATION_THRESHOLD 6000   // 震动检测阈值
```

## 按键功能

| 按键 | 功能 |
|------|------|
| PA2 | 简易模式：无功能<br>多功能模式：切换到多功能模式 |
| PA0 | 简易模式：切换城市<br>多功能模式：休眠模式切换城市，多功能模式切换到休眠 |
| PC13 | 切换OTA标志 |
| PB10 | 简易模式：重启系统<br>多功能模式：休眠模式下重启系统 |

## 通信机制

### 队列 (Queue)
- **OLED队列**: 10个消息容量，用于OLED显示任务通信
- **消息类型**: 显示字符串、大字体、清屏、模式切换

### 互斥量 (Mutex)
- **OLED互斥量**: 保护OLED显示资源，防止并发访问

## 数据存储

### AT24C02 EEPROM
- 存储OTA标志状态
- 掉电后保持设置

## 传感器功能

### MPU6050
- 检测震动唤醒
- 计算加速度变化
- 震动阈值：6000

### DHT11
- 读取温湿度数据
- 多功能模式下显示

## 编译配置

```c
#define MODE_SELECT 1  // 0: 仅简易模式, 1: 简易+多功能模式
```

## 任务调度

- **时间任务**: 每秒更新系统时间
- **LED任务**: 每50ms检测按键和传感器事件
- **传感器任务**: 每100ms读取传感器数据
- **OLED任务**: 每秒更新显示，处理队列消息

## 内存使用

- 总栈大小: 约1408字节
- 队列内存: 约320字节
- 互斥量内存: 约80字节

## 功耗管理

- 简易模式：低功耗运行
- 多功能模式：正常功耗，10秒无操作自动休眠
- LED指示：多功能模式点亮，简易模式关闭 