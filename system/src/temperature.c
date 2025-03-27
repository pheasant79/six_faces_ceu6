/**
 * @file temperature.c
 * @brief 六面顶温度控制模块实现
 */
#include "temperature.h"
#include <math.h>
#include <stdlib.h>

/* 温度控制参数 */
static Temperature_Params temp = {
    .target_temp = 25.0f,
    .current_temp = 25.0f,
    .max_temp = 2000.0f,
    .min_temp = 0.0f,
    .is_heating = 0
};

static Temperature_Status status = TEMP_OK;

/**
 * @brief  模拟温度变化
 * @note   内部函数,用于模拟温度渐变和随机波动
 * @retval None
 */
static void Simulate_TempChange(void) {
    static uint32_t tick = 0;
    float diff = temp.target_temp - temp.current_temp;
    
    // 模拟温度渐变
    if(fabs(diff) > 0.5f) {
        temp.current_temp += (diff > 0) ? 0.5f : -0.3f;
        temp.is_heating = (diff > 0);
    } else {
        temp.is_heating = 0;
    }
    
    // 添加随机波动
    if(++tick % 10 == 0) {
        temp.current_temp += ((float)rand()/RAND_MAX - 0.5f) * 0.1f;
    }
}

/**
 * @brief  温度系统初始化
 * @retval None
 */
void Temperature_Init(void) {
    temp.current_temp = 25.0f;
    temp.target_temp = 25.0f;
    temp.is_heating = 0;
    status = TEMP_OK;
}

/**
 * @brief  获取当前温度
 * @retval float 当前温度值(°C)
 */
float Get_Temperature(void) {
    return temp.current_temp;
}

/**
 * @brief  设置目标温度
 * @param  target 目标温度值(°C)
 * @retval Temperature_Status 设置结果
 */
Temperature_Status Set_Temperature(float target) {
    if(target > temp.max_temp || target < temp.min_temp) {
        return TEMP_OVERRANGE;
    }
    
    temp.target_temp = target;
    return TEMP_OK;
}

/**
 * @brief  停止加热
 * @retval None
 */
void Stop_Heating(void) {
    temp.target_temp = temp.current_temp;
    temp.is_heating = 0;
}

/**
 * @brief  获取温度系统状态
 * @retval Temperature_Status 当前状态
 */
Temperature_Status Get_TempStatus(void) {
    return status;
}

/**
 * @brief  温度控制处理函数
 * @note   需要在定时器中定期调用
 * @retval None
 */
void Temperature_Handler(void) {
    Simulate_TempChange();
} 
