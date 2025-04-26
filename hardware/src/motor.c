/**
 * @file motor.c
 * @brief 电机驱动模块实现函数
 */
#include "motor.h"
#include <string.h>
#include "main.h" // 添加main.h以包含所有GPIO定义
#include "stm32f4xx_hal.h" // 添加HAL库头文件
#include "stm32f4xx_hal_tim.h" // 添加TIM相关头文件
#include "stm32f4xx_hal_gpio.h" // 添加GPIO相关头文件
#include "tim.h" // 添加定时器相关头文件
#include "stm32f4xx.h" // 添加STM32F4通用头文件
#include "stm32f411xe.h" // 添加STM32F411特定头文件

/* 添加外部变量声明 */
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

/* 静态函数声明 */
static void gpio_reset_pins(Motor *m);
static Motor_Status validate_motor_id(uint8_t motor_id);
static Motor_Status motor_start(Motor *m);

/* 电机相位表 - 使用BSRR寄存器直接设置GPIO
 * 低16位为1代表设置高电平，高16位为1代表设置低电平
 * 8相半步进控制模式：A, AB, B, BC, C, CD, D, DA
 */
static const uint32_t Motor_PhaseTables[3][8] = {
    [0] = { // Motor1相位表 - 8相半步进
        // A相
        Motor1_A_Pin | ((uint32_t)Motor1_B_Pin|Motor1_C_Pin|Motor1_D_Pin)<<16,
        // A+B相
        (Motor1_A_Pin|Motor1_B_Pin) | ((uint32_t)Motor1_C_Pin|Motor1_D_Pin)<<16,
        // B相
        Motor1_B_Pin | ((uint32_t)Motor1_A_Pin|Motor1_C_Pin|Motor1_D_Pin)<<16,
        // B+C相
        (Motor1_B_Pin|Motor1_C_Pin) | ((uint32_t)Motor1_A_Pin|Motor1_D_Pin)<<16,
        // C相
        Motor1_C_Pin | ((uint32_t)Motor1_A_Pin|Motor1_B_Pin|Motor1_D_Pin)<<16,
        // C+D相
        (Motor1_C_Pin|Motor1_D_Pin) | ((uint32_t)Motor1_A_Pin|Motor1_B_Pin)<<16,
        // D相
        Motor1_D_Pin | ((uint32_t)Motor1_A_Pin|Motor1_B_Pin|Motor1_C_Pin)<<16,
        // D+A相
        (Motor1_D_Pin|Motor1_A_Pin) | ((uint32_t)Motor1_B_Pin|Motor1_C_Pin)<<16
    },
    [1] = { // Motor2相位表 - 8相半步进                                             
        // A相
        Motor2_A_Pin | ((uint32_t)Motor2_B_Pin|Motor2_C_Pin|Motor2_D_Pin)<<16,
        // A+B相
        (Motor2_A_Pin|Motor2_B_Pin) | ((uint32_t)Motor2_C_Pin|Motor2_D_Pin)<<16,
        // B相
        Motor2_B_Pin | ((uint32_t)Motor2_A_Pin|Motor2_C_Pin|Motor2_D_Pin)<<16,
        // B+C相
        (Motor2_B_Pin|Motor2_C_Pin) | ((uint32_t)Motor2_A_Pin|Motor2_D_Pin)<<16,
        // C相
        Motor2_C_Pin | ((uint32_t)Motor2_A_Pin|Motor2_B_Pin|Motor2_D_Pin)<<16,
        // C+D相
        (Motor2_C_Pin|Motor2_D_Pin) | ((uint32_t)Motor2_A_Pin|Motor2_B_Pin)<<16,
        // D相
        Motor2_D_Pin | ((uint32_t)Motor2_A_Pin|Motor2_B_Pin|Motor2_C_Pin)<<16,
        // D+A相
        (Motor2_D_Pin|Motor2_A_Pin) | ((uint32_t)Motor2_B_Pin|Motor2_C_Pin)<<16
    },
    [2] = { // Motor3相位表 - 8相半步进
        // A相
        Motor3_A_Pin | ((uint32_t)Motor3_B_Pin|Motor3_C_Pin|Motor3_D_Pin)<<16,
        // A+B相
        (Motor3_A_Pin|Motor3_B_Pin) | ((uint32_t)Motor3_C_Pin|Motor3_D_Pin)<<16,
        // B相
        Motor3_B_Pin | ((uint32_t)Motor3_A_Pin|Motor3_C_Pin|Motor3_D_Pin)<<16,
        // B+C相
        (Motor3_B_Pin|Motor3_C_Pin) | ((uint32_t)Motor3_A_Pin|Motor3_D_Pin)<<16,
        // C相
        Motor3_C_Pin | ((uint32_t)Motor3_A_Pin|Motor3_B_Pin|Motor3_D_Pin)<<16,
        // C+D相
        (Motor3_C_Pin|Motor3_D_Pin) | ((uint32_t)Motor3_A_Pin|Motor3_B_Pin)<<16,
        // D相
        Motor3_D_Pin | ((uint32_t)Motor3_A_Pin|Motor3_B_Pin|Motor3_C_Pin)<<16,
        // D+A相
        (Motor3_D_Pin|Motor3_A_Pin) | ((uint32_t)Motor3_B_Pin|Motor3_C_Pin)<<16
    }
};

/* 电机结构体数组 */
Motor motors[MOTOR_NUM] = {
    [0] = {
        .id = 0,
        .htim = &htim2,
        .gpio_port = Motor1_A_GPIO_Port,
        .pins = Motor1_A_Pin | Motor1_B_Pin | Motor1_C_Pin | Motor1_D_Pin,

        .state = MOTOR_STOP,
        .dir = DIR_NONE,
        .phase_index = 0,
        .phases = Motor_PhaseTables[0],

        .max_speed = 1000,
        .min_speed = 10
    },
    [1] = {
        .id = 1,
        .htim = &htim3,
        .gpio_port = Motor2_A_GPIO_Port,
        .pins = Motor2_A_Pin | Motor2_B_Pin | Motor2_C_Pin | Motor2_D_Pin,
        
        .state = MOTOR_STOP,
        .dir = DIR_NONE,
        .phase_index = 0,
        .phases = Motor_PhaseTables[1],
        
        .max_speed = 1000,
        .min_speed = 10
    },
    [2] = {
        .id = 2,
        .htim = &htim4,
        .gpio_port = Motor3_A_GPIO_Port,
        .pins = Motor3_A_Pin | Motor3_B_Pin | Motor3_C_Pin | Motor3_D_Pin,
        
        .state = MOTOR_STOP,
        .dir = DIR_NONE,
        .phase_index = 0,
        .phases = Motor_PhaseTables[2],
        
        .max_speed = 1000,
        .min_speed = 10
    }
};


/**
 * @brief  GPIO引脚复位函数
 * @param  m 电机结构体指针
 * @retval None
 */
static void gpio_reset_pins(Motor *m) {
    m->gpio_port->BSRR = m->pins << 16; // 将所有引脚复位
}

/**
 * @brief  电机ID验证函数
 * @param  motor_id 电机ID
 * @retval Motor_Status 操作结果
 */
static Motor_Status validate_motor_id(uint8_t motor_id) {
    return (motor_id < MOTOR_NUM) ? MOTOR_OK : MOTOR_ERR_INVALID_ID;
}

/**
 * @brief  电机启动函数
 * @param  m 电机结构体指针
 * @retval Motor_Status 结果
 */
static Motor_Status motor_start(Motor *m) {
    if(m->state == MOTOR_RUNNING) return MOTOR_ERR_ALREADY_RUNNING;

    // 1. 清零计数器
    __HAL_TIM_SET_COUNTER(m->htim, 0);
    
    // 2. 启动定时器中断
    if(HAL_TIM_Base_Start_IT(m->htim) != HAL_OK) {
        return MOTOR_ERR_TIM_START;
    }

    m->state = MOTOR_RUNNING; // 更新电机状态为运行中
    return MOTOR_OK;
}

/**
 * @brief  电机相位更新函数 - 中断中调用
 * @param  m 电机结构体指针
 * @retval None
 */
void motor_update_phase(Motor *m) {
    if(m->state != MOTOR_RUNNING) return;
    
    // 根据当前相位索引设置BSRR寄存器
    m->gpio_port->BSRR = m->phases[m->phase_index];
    
    // 根据电机旋转方向更新相位索引和步数计数器
    if (m->dir == DIR_CW) {
        // 顺时针旋转，相位索引+1
        m->phase_index = (m->phase_index + 1) % 8; // 修改为8相模式
        
        // 步数统计逻辑：每2个半步算作1个全步
        // 半步模式下，两个半步等于一个全步，所以每2个半步增加1个步数
        if (m->phase_index % 2 == 0) {
            m->step_counter++;
        }
    } else {
        // 逆时针旋转，相位索引+7（相当于-1，避免负数处理）
        m->phase_index = (m->phase_index + 7) % 8; // 修改为8相模式
        
        // 步数统计逻辑：每2个半步算作1个全步
        // 半步模式下，两个半步等于一个全步，所以每2个半步减少1个步数
        // 增加步数下限检查，确保步数不会小于0
        if (m->phase_index % 2 == 0 && m->step_counter > 0) {
            m->step_counter--;
        }
    }
}

/**
 * @brief  电机初始化函数
 * @retval Motor_Status 初始化结果
 */
Motor_Status Motor_InitAll(void) {
    for(int i=0; i<MOTOR_NUM; i++) {
        Motor *m = &motors[i];
        
        // 设置电机ID和其他参数
        m->id = i;
        
        // 复位所有GPIO
        gpio_reset_pins(m);
        m->state = MOTOR_STOP;
        m->dir = DIR_NONE;
        m->phase_index = 0;
        m->step_counter = 0;
    }
    return MOTOR_OK;
}

/**
 * @brief  启动指定电机
 * @param  id 电机ID(0-2)
 * @retval Motor_Status 操作结果
 */
Motor_Status Motor_Run(uint8_t id) {
    Motor_Status status;
    if((status = validate_motor_id(id)) != MOTOR_OK) 
        return status;
        
    Motor *m = &motors[id];
    
    // 如果电机已经处于运行状态，直接返回
    if(m->state == MOTOR_RUNNING) 
        return MOTOR_OK;
    
    // 如果没有设置旋转方向，默认设为顺时针
    if(m->dir == DIR_NONE) {
        m->dir = DIR_CW;
    }
    
    // 启动电机，motor_start内部会将电机状态更新为MOTOR_RUNNING
    return motor_start(m);
}

/**
 * @brief  停止指定电机
 * @param  id 电机ID(0-2)
 * @retval Motor_Status 操作结果
 */
Motor_Status Motor_Stop(uint8_t id) {
    Motor_Status status;
    if((status = validate_motor_id(id)) != MOTOR_OK) 
        return status;
    
    Motor *m = &motors[id];
    
    // 如果电机已经处于停止状态，直接返回
    if(m->state == MOTOR_STOP)
        return MOTOR_OK;
    
    // 如果电机不处于运行状态，也直接返回
    if(m->state != MOTOR_RUNNING) 
        return MOTOR_OK;
    
    // 停止定时器中断
    HAL_TIM_Base_Stop_IT(m->htim);
    
    // 将所有引脚复位
    gpio_reset_pins(m);
    
    // 更新电机状态为停止
    m->state = MOTOR_STOP;
    
    return MOTOR_OK;
}

/**
 * @brief  紧急停止所有电机
 * @retval Motor_Status 操作结果
 */
Motor_Status Motor_EmergencyStop(void) {
    Motor_Status final_status = MOTOR_OK;
    for(uint8_t i=0; i<MOTOR_NUM; i++) {
        Motor_Status s = Motor_Stop(i);
        if(s != MOTOR_OK) final_status = s;
    }
    return final_status;
}

/**
 * @brief  设置指定电机旋转方向
 * @param  id 电机ID(0-2)
 * @param  dir 旋转方向(DIR_CW/DIR_CCW)
 * @retval Motor_Status 操作结果
 */
Motor_Status Motor_SetDir(uint8_t id, Motor_Dir dir) {
    Motor_Status status;
    if((status = validate_motor_id(id)) != MOTOR_OK) 
        return status;
    
    // 如果是无方向，则不处理
    if(dir == DIR_NONE) 
        return MOTOR_OK; // 保持当前方向不变
    
    Motor *m = &motors[id];
    if(m->dir == dir) 
        return MOTOR_OK; // 方向相同，无需更改
    
    // 更新方向
    m->dir = dir;
    
    return MOTOR_OK;
}

/**
 * @brief  设置指定电机速度
 * @param  id 电机ID(0-2)
 * @param  steps_per_sec 速度(步/秒)
 * @retval Motor_Status 操作结果
 * @note   在半步进模式下，传入的steps_per_sec表示全步数/秒
 *         系统会自动转换为半步数/秒
 */
Motor_Status Motor_SetSpeed(uint8_t id, uint32_t steps_per_sec) {
	//展示先不用设置速度
//    Motor_Status status;
//    if((status = validate_motor_id(id)) != MOTOR_OK)
//        return status;
//        
//    Motor *m = &motors[id];
//    
//    if(steps_per_sec < m->min_speed || steps_per_sec > m->max_speed)
//        return MOTOR_ERR_SPEED_RANGE;
//    
//    // 半步进模式下，定时器频率需要翻倍，因为每个全步需要2个半步
//    uint32_t halfsteps_per_sec = steps_per_sec * 2;
//    
//    uint32_t timer_clk = HAL_RCC_GetPCLK1Freq() * 2;
//    uint32_t min_psc = 0;
//    uint32_t max_psc = 0xFFFF;
//    uint32_t best_psc = 0;
//    uint32_t best_arr = 0xFFFF;
//    
//    // 二分查找最佳的分频和重装载值
//    while(min_psc <= max_psc) {
//        uint32_t mid_psc = (min_psc + max_psc) / 2;
//        uint32_t calc_arr = (timer_clk / (halfsteps_per_sec * (mid_psc + 1))) - 1;
//        
//        if(calc_arr <= 0xFFFF) {
//            best_psc = mid_psc;
//            best_arr = calc_arr;
//            max_psc = mid_psc - 1; // 尝试更小的PSC
//        } else {
//            min_psc = mid_psc + 1;
//        }
//    }
//    
//    if(best_psc == 0 && best_arr == 0xFFFF) {
//        return MOTOR_ERR_SPEED_RANGE;
//    }
//    
//    // 设置定时器的分频和重装载值
//    __HAL_TIM_SET_PRESCALER(m->htim, best_psc);
//    __HAL_TIM_SET_AUTORELOAD(m->htim, best_arr);
//    
//    // 如果电机已经在运行，重置计数器
//    if(m->state == MOTOR_RUNNING) {
//        __HAL_TIM_SET_COUNTER(m->htim, 0);
//        m->htim->Instance->EGR = TIM_EGR_UG;
//    }
//    
//    // 保存实际速度值
//    m->actual_speed = steps_per_sec;
//    
    return MOTOR_OK;
}


/**
 * @brief  获取指定电机步数
 * @param  id 电机ID(0-2)
 * @retval int32_t 步数，使用有符号类型避免反转溢出
 * @note   此函数返回的是全步数，半步进内部计数时每2个半步算1个全步
 */
int32_t Motor_GetStepCount(uint8_t id) {
    return (id < MOTOR_NUM) ? motors[id].step_counter : 0;
}

/**
 * @brief  设置指定电机步数
 * @param  id 电机ID(0-2)
 * @param  steps 步数值
 * @retval Motor_Status 操作结果
 * @note   此函数设置的是全步数，半步进内部会自动处理
 */
Motor_Status Motor_SetStepCount(uint8_t id, int32_t steps) {
    Motor_Status status;
    if((status = validate_motor_id(id)) != MOTOR_OK) 
        return status;
    
    motors[id].step_counter = steps;
    return MOTOR_OK;
}
