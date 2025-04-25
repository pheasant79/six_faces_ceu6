/**
 * @file motor.c
 * @brief 电机驱动模块实现函数
 */
#include "motor.h"
#include <string.h>

/* 定义DMA句柄 - 这些句柄在STM32主文件中初始化 */
DMA_HandleTypeDef hdma_tim2_ch3_up;
DMA_HandleTypeDef hdma_tim3_ch4_up;
DMA_HandleTypeDef hdma_tim4_up;

/* 静态函数声明 */
static void gpio_reset_pins(Motor *m);
static Motor_Status validate_motor_id(uint8_t motor_id);
static Motor_Status motor_start(Motor *m);

/* 电机相位表 - 使用BSRR寄存器直接设置GPIO
 * 低16位为1代表设置高电平，高16位为1代表设置低电平
 */
static const uint32_t Motor_PhaseTables[3][4] = {
    [0] = { // Motor1相位表
        Motor1_A_Pin | ((uint32_t)Motor1_B_Pin|Motor1_C_Pin|Motor1_D_Pin)<<16,
        Motor1_B_Pin | ((uint32_t)Motor1_A_Pin|Motor1_C_Pin|Motor1_D_Pin)<<16,
        Motor1_C_Pin | ((uint32_t)Motor1_A_Pin|Motor1_B_Pin|Motor1_D_Pin)<<16,
        Motor1_D_Pin | ((uint32_t)Motor1_A_Pin|Motor1_B_Pin|Motor1_C_Pin)<<16
    },
    [1] = { // Motor2相位表                                             
        Motor2_A_Pin | ((uint32_t)Motor2_B_Pin|Motor2_C_Pin|Motor2_D_Pin)<<16,
        Motor2_B_Pin | ((uint32_t)Motor2_A_Pin|Motor2_C_Pin|Motor2_D_Pin)<<16,
        Motor2_C_Pin | ((uint32_t)Motor2_A_Pin|Motor2_B_Pin|Motor2_D_Pin)<<16,
        Motor2_D_Pin | ((uint32_t)Motor2_A_Pin|Motor2_B_Pin|Motor2_C_Pin)<<16
    },
    [2] = { // Motor3相位表
        Motor3_A_Pin | ((uint32_t)Motor3_B_Pin|Motor3_C_Pin|Motor3_D_Pin)<<16,
        Motor3_B_Pin | ((uint32_t)Motor3_A_Pin|Motor3_C_Pin|Motor3_D_Pin)<<16,
        Motor3_C_Pin | ((uint32_t)Motor3_A_Pin|Motor3_B_Pin|Motor3_D_Pin)<<16,
        Motor3_D_Pin | ((uint32_t)Motor3_A_Pin|Motor3_B_Pin|Motor3_C_Pin)<<16
    }
};

/* 电机结构体数组 */
Motor motors[MOTOR_NUM] = {
    [0] = {
        .id = 0,
        .htim = &htim2,
        .hdma = &hdma_tim2_ch3_up,
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
        .hdma = &hdma_tim3_ch4_up,
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
        .hdma = &hdma_tim4_up,
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

// 引用menu.c中定义的目标步数数组
extern uint32_t motor_target_steps[MOTOR_NUM];

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
        m->phase_index = (m->phase_index + 1) % 4;
        // 顺时针运转时，步数计数器增加1步
        m->step_counter++;
    } else {
        // 逆时针旋转，相位索引+3（相当于-1，避免负数处理）
        m->phase_index = (m->phase_index + 3) % 4;
        // 逆时针运转时，步数计数器减少1步
        m->step_counter--;
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
 */
Motor_Status Motor_SetSpeed(uint8_t id, uint32_t steps_per_sec) {
    Motor_Status status;
    if((status = validate_motor_id(id)) != MOTOR_OK)
        return status;
        
    Motor *m = &motors[id];
    
    if(steps_per_sec < m->min_speed || steps_per_sec > m->max_speed)
        return MOTOR_ERR_SPEED_RANGE;
    
    uint32_t timer_clk = HAL_RCC_GetPCLK1Freq() * 2;
    uint32_t min_psc = 0;
    uint32_t max_psc = 0xFFFF;
    uint32_t best_psc = 0;
    uint32_t best_arr = 0xFFFF;
    
    // 二分查找最佳的分频和重装载值
    while(min_psc <= max_psc) {
        uint32_t mid_psc = (min_psc + max_psc) / 2;
        uint32_t calc_arr = (timer_clk / (steps_per_sec * (mid_psc + 1))) - 1;
        
        if(calc_arr <= 0xFFFF) {
            best_psc = mid_psc;
            best_arr = calc_arr;
            max_psc = mid_psc - 1; // 尝试更小的PSC
        } else {
            min_psc = mid_psc + 1;
        }
    }
    
    if(best_psc == 0 && best_arr == 0xFFFF) {
        return MOTOR_ERR_SPEED_RANGE;
    }
    
    // 设置定时器的分频和重装载值
    __HAL_TIM_SET_PRESCALER(m->htim, best_psc);
    __HAL_TIM_SET_AUTORELOAD(m->htim, best_arr);
    
    // 如果电机已经在运行，重置计数器
    if(m->state == MOTOR_RUNNING) {
        __HAL_TIM_SET_COUNTER(m->htim, 0);
        m->htim->Instance->EGR = TIM_EGR_UG;
    }
    
    // 保存实际速度值
    m->actual_speed = steps_per_sec;
    
    return MOTOR_OK;
}

/**
 * @brief  获取指定电机状态
 * @param  id 电机ID(0-2)
 * @retval Motor_State 电机状态
 */
Motor_State Motor_GetState(uint8_t id) {
    return (id < MOTOR_NUM) ? motors[id].state : MOTOR_ERROR;
}

/**
 * @brief  获取指定电机步数
 * @param  id 电机ID(0-2)
 * @retval uint32_t 步数
 */
uint32_t Motor_GetStepCount(uint8_t id) {
    return (id < MOTOR_NUM) ? motors[id].step_counter : 0;
}

/**
 * @brief  设置电机目标步数
 * @param  id 电机ID(0-2)
 * @param  steps 目标步数，0表示不限制
 * @retval Motor_Status 操作状态
 */
Motor_Status Motor_SetTargetSteps(uint8_t id, uint32_t steps) {
    Motor_Status status;
    if((status = validate_motor_id(id)) != MOTOR_OK) 
        return status;
        
    motor_target_steps[id] = steps;
    return MOTOR_OK;
}

/**
 * @brief  电机精调控制函数 - 少量步数微调
 * @param  id 电机ID(0-2)
 * @param  steps 精调步数
 * @param  dir 调整方向
 * @retval Motor_Status 操作状态
 */
Motor_Status Motor_FineTune(uint8_t id, uint32_t steps, Motor_Dir dir) {
    Motor_Status status;
    
    // 参数检查
    if((status = validate_motor_id(id)) != MOTOR_OK) 
        return status;
    
    if(dir == DIR_NONE) 
        return MOTOR_OK; // 无方向则不进行操作
    
    // 设置方向
    if((status = Motor_SetDir(id, dir)) != MOTOR_OK)
        return status;
    
    // 精调采用较慢速度，确保准确性
    uint32_t fine_tune_speed = motors[id].min_speed + 20; // 稍快于最小速度
    if((status = Motor_SetSpeed(id, fine_tune_speed)) != MOTOR_OK)
        return status;
    
    // 设置目标步数
    Motor_SetTargetSteps(id, Motor_GetStepCount(id) + (dir == DIR_CW ? steps : -steps));
    
    // 启动电机
    return Motor_Run(id);
}

/**
 * @brief  电机加压控制函数 - 正向增加压力
 * @param  id 电机ID(0-2)
 * @param  steps 加压步数
 * @param  speed 加压速度，0表示使用默认速度
 * @retval Motor_Status 操作状态
 */
Motor_Status Motor_ApplyPressure(uint8_t id, uint32_t steps, uint32_t speed) {
    Motor_Status status;
    
    // 参数检查
    if((status = validate_motor_id(id)) != MOTOR_OK) 
        return status;
    
    // 加压方向默认为顺时针
    if((status = Motor_SetDir(id, DIR_CW)) != MOTOR_OK)
        return status;
    
    // 设置速度
    uint32_t apply_speed = (speed > 0) ? speed : (motors[id].max_speed / 2); // 默认使用中等速度
    if(apply_speed < motors[id].min_speed) apply_speed = motors[id].min_speed;
    if(apply_speed > motors[id].max_speed) apply_speed = motors[id].max_speed;
    
    if((status = Motor_SetSpeed(id, apply_speed)) != MOTOR_OK)
        return status;
    
    // 设置目标步数
    uint32_t current_steps = Motor_GetStepCount(id);
    Motor_SetTargetSteps(id, current_steps + steps);
    
    // 启动电机
    return Motor_Run(id);
}

/**
 * @brief  电机卸压控制函数 - 反向释放压力
 * @param  id 电机ID(0-2)
 * @param  steps 卸压步数
 * @param  speed 卸压速度，0表示使用默认速度
 * @retval Motor_Status 操作状态
 */
Motor_Status Motor_ReleasePressure(uint8_t id, uint32_t steps, uint32_t speed) {
    Motor_Status status;
    
    // 参数检查
    if((status = validate_motor_id(id)) != MOTOR_OK) 
        return status;
    
    // 卸压方向默认为逆时针
    if((status = Motor_SetDir(id, DIR_CCW)) != MOTOR_OK)
        return status;
    
    // 设置速度
    uint32_t release_speed = (speed > 0) ? speed : (motors[id].max_speed / 3); // 默认使用较慢速度
    if(release_speed < motors[id].min_speed) release_speed = motors[id].min_speed;
    if(release_speed > motors[id].max_speed) release_speed = motors[id].max_speed;
    
    if((status = Motor_SetSpeed(id, release_speed)) != MOTOR_OK)
        return status;
    
    // 设置目标步数
    uint32_t current_steps = Motor_GetStepCount(id);
    if(steps > current_steps) steps = current_steps; // 防止步数为负
    Motor_SetTargetSteps(id, current_steps - steps);
    
    // 启动电机
    return Motor_Run(id);
}
