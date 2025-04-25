/**
 * @file motor.h
 * @brief 步进电机驱动模块接口函数
 */
#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "main.h"
#include "stm32f4xx_hal.h"  // 添加此头文件以解决GPIO_TypeDef未定义的问题

/*****************************************************
 * 电机引脚定义(采用平行四线驱动)
 * motor1: TIM2
 * motor1_A----PA10  motor1_B----PA11
 * motor1_C----PA12  motor1_D----PA15
 *
 * motor2: TIM3
 * motor2_A----PA0   motor2_B----PA1
 * motor2_C----PA2   motor2_D----PA3
 *
 * motor3: TIM4
 * motor3_A----PB3   motor3_B----PB4
 * motor3_C----PB5   motor3_D----PB6
*****************************************************/

/* 常量定义 */
#define MOTOR_NUM           3     // 电机数量

/**
 * 电机状态返回枚举类型
 * 用于各接口函数返回操作执行的状态码
 */
typedef enum {
    MOTOR_OK = 0,                 // 正常
    MOTOR_ERR_INVALID_ID,         // 无效电机ID
    MOTOR_ERR_SPEED_RANGE,        // 速度超出范围
    MOTOR_ERR_TIM_START,          // 定时器启动失败
    MOTOR_ERR_ALREADY_RUNNING,    // 电机已经运行
    MOTOR_ERR_NOT_RUNNING         // 电机未运行
}Motor_Status;

/**
 * 电机工作状态枚举类型
 * 表示电机的当前工作状态
 */
typedef enum {
    MOTOR_STOP = 0,               // 停止
    MOTOR_RUNNING,                // 运行中
    MOTOR_ERROR                   // 错误
}Motor_State;

/**
 * 电机转动方向枚举类型
 * 表示电机的转动方向
 */
typedef enum {
    DIR_NONE = 0,                 // 无方向
    DIR_CW,                       // 顺时针
    DIR_CCW                       // 逆时针
} Motor_Dir;

/**
 * 电机控制结构体
 * 保存电机的相关参数及工作状态信息
 */
typedef struct {
    /* 基本信息 */
    uint8_t id;                   // 电机ID (0-2)
    
    /* 硬件资源 */
    TIM_HandleTypeDef* htim;      // 定时器句柄
    GPIO_TypeDef* gpio_port;      // GPIO端口
    uint16_t pins;                // GPIO引脚掩码

    /* 控制参数 */
    Motor_State state;            // 电机状态
    Motor_Dir dir;                // 转动方向
    uint8_t phase_index;          // 当前相位索引 (0-3)
    const uint32_t *phases;       // 相位表

    /* 速度参数 */
    uint32_t max_speed;           // 最大速度(步/秒)
    uint32_t min_speed;           // 最小速度(步/秒)
    
    /* 状态参数 */
    int32_t step_counter;        // 电机行走步数(有符号)
    uint32_t actual_speed;        // 实际速度
    
    /* 步数控制参数 */
    int32_t target_steps;        // 目标步数，0表示不限制(有符号)
    uint8_t step_control_enabled; // 步数监控启用标志，1表示启用
} Motor;

extern Motor motors[MOTOR_NUM];

/*====================== 接口函数 ======================*/

/*--------------------- 初始化函数 ---------------------*/
Motor_Status Motor_InitAll(void); // 初始化所有电机

/*--------------------- 控制函数 ---------------------*/
// 启动指定ID电机
Motor_Status Motor_Run(uint8_t id);

// 停止指定ID电机
Motor_Status Motor_Stop(uint8_t id);

// 紧急停止所有电机
Motor_Status Motor_EmergencyStop(void);

// 设置指定电机转动方向(DIR_CW/DIR_CCW)
Motor_Status Motor_SetDir(uint8_t id, Motor_Dir dir);

// 设置指定电机速度(步/秒)
Motor_Status Motor_SetSpeed(uint8_t id, uint32_t steps_per_sec);

/*--------------------- 状态查询函数 ---------------------*/

// 获取指定电机当前步数
int32_t Motor_GetStepCount(uint8_t id);

// 设置指定电机当前步数
Motor_Status Motor_SetStepCount(uint8_t id, int32_t steps);

/*--------------------- 内部函数 ---------------------*/
// 相位更新(定时器中断调用)
void motor_update_phase(Motor *m);

#endif /* __MOTOR_H__ */
