/**
 * @file pressure.c
 * @brief 六面顶压力控制模块实现
 */
#include "pressure.h"
#include <math.h>
#include <stdlib.h>

/* 压力控制参数 */
static Pressure_Params pressure = {
    .target_pressure = 0.0f,
    .current_pressure = 0.0f,
    .max_pressure = 10.0f,
    .min_pressure = 0.0f,
    .is_stable = 1
};

static Pressure_Status status = PRESSURE_OK;

/**
 * @brief  模拟压力变化
 * @note   内部函数,用于模拟压力渐变和随机波动
 * @retval None
 */
static void Simulate_PressureChange(void) {
    static uint32_t tick = 0;
    float diff = pressure.target_pressure - pressure.current_pressure;
    
    // 模拟压力渐变
    if(fabs(diff) > 0.1f) {
        pressure.current_pressure += (diff > 0) ? 0.1f : -0.1f;
        pressure.is_stable = 0;
    } else {
        pressure.is_stable = 1;
    }
    
    // 添加随机波动
    if(++tick % 10 == 0) {
        pressure.current_pressure += ((float)rand()/RAND_MAX - 0.5f) * 0.02f;
    }
}

/**
 * @brief  压力系统初始化
 * @retval None
 */
void Pressure_Init(void) {
    pressure.current_pressure = 0.0f;
    pressure.target_pressure = 0.0f;
    status = PRESSURE_OK;
}

/**
 * @brief  获取当前压力值
 * @retval float 当前压力值(GPa)
 */
float Get_Pressure(void) {
    return pressure.current_pressure;
}

/**
 * @brief  设置目标压力
 * @param  target 目标压力值(GPa)
 * @retval Pressure_Status 设置结果
 */
Pressure_Status Set_Pressure(float target) {
    if(target > pressure.max_pressure || target < pressure.min_pressure) {
        return PRESSURE_OVERRANGE;
    }
    
    pressure.target_pressure = target;
    return PRESSURE_OK;
}

/**
 * @brief  获取压力系统状态
 * @retval Pressure_Status 当前状态
 */
Pressure_Status Get_PressureStatus(void) {
    return status;
}

/**
 * @brief  压力控制处理函数
 * @note   需要在定时器中定期调用
 * @retval None
 */
void Pressure_Handler(void) {
    Simulate_PressureChange();
} 
