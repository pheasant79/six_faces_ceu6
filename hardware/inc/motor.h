/**
 * @file motor.h
 * @brief 六面顶步进电机驱动头文件
 */
#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "main.h"

/*****************************************************
 * 步进电机引脚定义(低电平有效)
 * motor1: TIM2 + DMA1_Stream6
 * motor1_A----PA10  motor1_B----PA11
 * motor1_C----PA12  motor1_D----PA15
 *
 * motor2: TIM3 + DMA1_Stream2 
 * motor2_A----PA0   motor2_B----PA1
 * motor2_C----PA2   motor2_D----PA3
 *
 * motor3: TIM4 + DMA1_Stream7
 * motor3_A----PB3   motor3_B----PB4
 * motor3_C----PB5   motor3_D----PB6
*****************************************************/

/* 基本参数定义 */
#define MOTOR_NUM           3     // 电机数量
#define MOTOR_TIMEOUT     1000    // 超时时间(ms)
#define DMA_ABORT_TIMEOUT  50     // DMA中止超时(ms)
#define DMA_START_TIMEOUT 100     // DMA启动超时(ms)

/**
 * @brief 电机控制状态枚举
 */
typedef enum {
    MOTOR_OK = 0,                 // 正常
    MOTOR_ERR_INVALID_ID,         // 无效电机ID
    MOTOR_ERR_DMA_BUSY,           // DMA忙
    MOTOR_ERR_SPEED_RANGE,        // 速度超出范围
    MOTOR_ERR_DMA_ABORT_TIMEOUT,  // DMA中止超时
    MOTOR_ERR_DMA_CONFIG,         // DMA配置错误
    MOTOR_ERR_DMA_START_TIMEOUT,  // DMA启动超时
    MOTOR_ERR_TIM_START,          // 定时器启动错误
    MOTOR_ERR_ALREADY_RUNNING,    // 电机已在运行
    MOTOR_ERR_NOT_RUNNING         // 电机未运行
} Motor_Status;

/**
 * @brief 电机运行状态
 */
typedef enum {
    MOTOR_STOP = 0,               // 停止
    MOTOR_RUNNING,                // 运行中
    MOTOR_ERROR                   // 错误
} Motor_State;

/**
 * @brief 电机旋转方向
 */
typedef enum {
    DIR_NONE = 0,                 // 无方向
    DIR_CW,                       // 顺时针
    DIR_CCW                       // 逆时针
} Motor_Dir;

/**
 * @brief 电机控制结构体
 */
typedef struct {
    /* 硬件资源 */
    TIM_HandleTypeDef* htim;      // 定时器句柄
    DMA_HandleTypeDef* hdma;      // DMA句柄
    GPIO_TypeDef* gpio_port;      // GPIO端口
    uint16_t pins;                // GPIO引脚掩码

    /* 运动参数 */
    Motor_State state;            // 运行状态
    Motor_Dir dir;                // 旋转方向
    uint32_t phase_buffer[4];     // 相位缓冲区
    const uint32_t *forward_phases;  // 正向相序表
    const uint32_t *reverse_phases;  // 反向相序表

    /* 速度参数 */
    uint32_t max_speed;           // 最大速度(步/秒)
    uint32_t min_speed;           // 最小速度(步/秒)
    
    /* 状态监控 */
    uint32_t step_counter;        // 步数计数器
    uint32_t actual_speed;        // 实际速度
} Motor;

extern Motor motors[MOTOR_NUM];

/**
 * 函数声明
 */
// 初始化
Motor_Status Motor_InitAll(void);

/**
 * @brief  启动电机
 * @param  id 电机ID(0-2)
 * @retval Motor_Status 操作结果
 */
Motor_Status Motor_Run(uint8_t id);

/**
 * @brief  停止电机
 * @param  id 电机ID(0-2)
 * @retval Motor_Status 操作结果
 */
Motor_Status Motor_Stop(uint8_t id);

/**
 * @brief  紧急停止所有电机
 * @retval Motor_Status 操作结果
 */
Motor_Status Motor_EmergencyStop(void);

/**
 * @brief  设置电机旋转方向
 * @param  id 电机ID(0-2)
 * @param  dir 旋转方向(DIR_CW/DIR_CCW)
 * @retval Motor_Status 操作结果
 */
Motor_Status Motor_SetDir(uint8_t id, Motor_Dir dir);

/**
 * @brief  设置电机速度
 * @param  id 电机ID(0-2)
 * @param  steps_per_sec 速度(步/秒)
 * @retval Motor_Status 操作结果
 */
Motor_Status Motor_SetSpeed(uint8_t id, uint32_t steps_per_sec);

/**
 * @brief  获取电机状态
 * @param  id 电机ID(0-2)
 * @retval Motor_State 电机状态
 */
Motor_State Motor_GetState(uint8_t id);

/**
 * @brief  获取电机步数
 * @param  id 电机ID(0-2)
 * @retval uint32_t 步数
 */
uint32_t Motor_GetStepCount(uint8_t id);

#endif /* __MOTOR_H__ */
