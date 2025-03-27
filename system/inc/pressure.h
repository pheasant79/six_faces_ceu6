/**
 * @file pressure.h
 * @brief 六面顶压力控制模块头文件
 */
#ifndef __PRESSURE_H
#define __PRESSURE_H

#include "main.h"

/**
 * @brief 压力系统状态枚举
 */
typedef enum {
    PRESSURE_OK = 0,        // 正常
    PRESSURE_ERROR,         // 错误
    PRESSURE_OVERRANGE,     // 超出范围
    PRESSURE_SENSOR_ERROR   // 传感器错误
} Pressure_Status;

/**
 * @brief 压力参数结构体
 */
typedef struct {
    float target_pressure;   // 目标压力(GPa)
    float current_pressure;  // 当前压力
    float max_pressure;      // 最大压力
    float min_pressure;      // 最小压力
    uint8_t is_stable;      // 压力稳定标志
} Pressure_Params;

/**
 * @brief  压力系统初始化
 * @retval None
 */
void Pressure_Init(void);

/**
 * @brief  获取当前压力值
 * @retval float 当前压力值(GPa)
 */
float Get_Pressure(void);

/**
 * @brief  设置目标压力
 * @param  target 目标压力值(GPa)
 * @retval Pressure_Status 设置结果
 */
Pressure_Status Set_Pressure(float target);

/**
 * @brief  获取压力系统状态
 * @retval Pressure_Status 当前状态
 */
Pressure_Status Get_PressureStatus(void);

/**
 * @brief  压力控制处理函数
 * @note   需要在定时器中定期调用
 * @retval None
 */
void Pressure_Handler(void);

#endif /* __PRESSURE_H */ 
