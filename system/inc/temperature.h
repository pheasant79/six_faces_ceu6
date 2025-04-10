/**
 * @file temperature.h
 * @brief 六面顶温度控制模块头文件
 */
#ifndef __TEMPERATURE_H
#define __TEMPERATURE_H

#include "main.h"

/* 常量定义 */
#define MAX_TEMPERATURE    2000.0f // 最大温度(℃)
#define MAX_SPEED          1000.0f // 最大速度(mm/s)
#define MAX_FEED           100.0f  // 最大进给量(mm)

/**
 * @brief 温度系统状态枚举
 */
typedef enum {
    TEMP_OK = 0,         // 正常
    TEMP_ERROR,          // 错误
    TEMP_OVERRANGE,      // 超出范围
    TEMP_SENSOR_ERROR    // 传感器错误
} Temperature_Status;

/**
 * @brief 温度参数结构体
 */
typedef struct {
    float target_temp;    // 目标温度(°C)
    float current_temp;   // 当前温度
    float max_temp;       // 最大温度
    float min_temp;       // 最小温度
    uint8_t is_heating;   // 加热状态标志
} Temperature_Params;

/**
 * @brief  温度系统初始化
 * @retval None
 */
void Temperature_Init(void);

/**
 * @brief  获取当前温度
 * @retval float 当前温度值(°C)
 */
float Get_Temperature(void);

/**
 * @brief  设置目标温度
 * @param  target 目标温度值(°C)
 * @retval Temperature_Status 设置结果
 */
Temperature_Status Set_Temperature(float target);

/**
 * @brief  停止加热
 * @retval None
 */
void Stop_Heating(void);

/**
 * @brief  获取温度系统状态
 * @retval Temperature_Status 当前状态
 */
Temperature_Status Get_TempStatus(void);

/**
 * @brief  温度控制处理函数
 * @note   需要在定时器中定期调用
 * @retval None
 */
void Temperature_Handler(void);

#endif /* __TEMPERATURE_H */ 
