/**
 * @file motor.h
 * @brief 六面顶步进电机驱动头文件
 */
#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "main.h"

/*****************************************************
 * 步进电机引脚定义(低电平有效)
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

/* 基本参数定义 */
#define MOTOR_NUM           3     // 电机数量
#define MOTOR_TIMEOUT     1000    // 超时时间(ms)

/**
 * 【状态码枚举】
 * 定义电机控制操作返回的状态码
 */
typedef enum {
    MOTOR_OK = 0,                 // 正常
    MOTOR_ERR_INVALID_ID,         // 无效电机ID
    MOTOR_ERR_SPEED_RANGE,        // 速度超出范围
    MOTOR_ERR_TIM_START,          // 定时器启动错误
    MOTOR_ERR_ALREADY_RUNNING,    // 电机已在运行
    MOTOR_ERR_NOT_RUNNING         // 电机未运行
} Motor_Status;

/**
 * 【运行状态枚举】
 * 定义电机的当前运行状态
 */
typedef enum {
    MOTOR_STOP = 0,               // 停止
    MOTOR_RUNNING,                // 运行中
    MOTOR_ERROR                   // 错误
} Motor_State;

/**
 * 【旋转方向枚举】
 * 定义电机的旋转方向
 */
typedef enum {
    DIR_NONE = 0,                 // 无方向
    DIR_CW,                       // 顺时针
    DIR_CCW                       // 逆时针
} Motor_Dir;

/**
 * 【电机结构体】
 * 保存电机的所有配置和状态信息
 */
typedef struct {
    /* 基本信息 */
    uint8_t id;                   // 电机ID (0-2)
    
    /* 硬件资源 */
    TIM_HandleTypeDef* htim;      // 定时器句柄
    DMA_HandleTypeDef* hdma;      // DMA句柄(保留用于兼容性)
    GPIO_TypeDef* gpio_port;      // GPIO端口
    uint16_t pins;                // GPIO引脚掩码

    /* 运动参数 */
    Motor_State state;            // 运行状态
    Motor_Dir dir;                // 旋转方向
    uint8_t phase_index;          // 当前相位索引 (0-3)
    const uint32_t *phases;       // 相位表

    /* 速度参数 */
    uint32_t max_speed;           // 最大速度(步/秒)
    uint32_t min_speed;           // 最小速度(步/秒)
    
    /* 状态监控 */
    uint32_t step_counter;        // 步数计数器
    uint32_t actual_speed;        // 实际速度
} Motor;

extern Motor motors[MOTOR_NUM];

/*====================== 函数声明 ======================*/

/*--------------------- 初始化函数 ---------------------*/
Motor_Status Motor_InitAll(void); // 初始化所有电机

/*--------------------- 控制函数 ---------------------*/
// 启动指定ID电机
Motor_Status Motor_Run(uint8_t id);

// 停止指定ID电机
Motor_Status Motor_Stop(uint8_t id);

// 紧急停止所有电机
Motor_Status Motor_EmergencyStop(void);

// 设置电机旋转方向(DIR_CW/DIR_CCW)
Motor_Status Motor_SetDir(uint8_t id, Motor_Dir dir);

// 设置电机速度(步/秒)
Motor_Status Motor_SetSpeed(uint8_t id, uint32_t steps_per_sec);

/*--------------------- 状态查询函数 ---------------------*/
// 获取电机运行状态(MOTOR_STOP/MOTOR_RUNNING/MOTOR_ERROR)
Motor_State Motor_GetState(uint8_t id);

// 获取电机当前步数
uint32_t Motor_GetStepCount(uint8_t id);

/*--------------------- 内部函数 ---------------------*/
// 软件触发相位更新(由定时器中断调用)
void motor_update_phase(Motor *m);

#endif /* __MOTOR_H__ */
