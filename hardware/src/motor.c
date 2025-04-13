/**
 * @file motor.c
 * @brief 六面顶步进电机驱动实现
 */
#include "motor.h"
#include <string.h>

/* 虚拟DMA句柄 - 用于编译通过，但实际不使用 */
DMA_HandleTypeDef hdma_tim2_ch3_up;
DMA_HandleTypeDef hdma_tim3_ch4_up;
DMA_HandleTypeDef hdma_tim4_up;

/* 静态函数声明 */
static void gpio_reset_pins(Motor *m);
static Motor_Status validate_motor_id(uint8_t motor_id);
static Motor_Status motor_start(Motor *m);

/* 相位表定义 - 使用BSRR寄存器实现原子操作
 * 高16位写1清零对应引脚,低16位写1置位对应引脚
 */
static const uint32_t Motor_PhaseTables[3][4] = {
    [0] = { // Motor1相序
        Motor1_A_Pin | ((uint32_t)Motor1_B_Pin|Motor1_C_Pin|Motor1_D_Pin)<<16,
        Motor1_B_Pin | ((uint32_t)Motor1_A_Pin|Motor1_C_Pin|Motor1_D_Pin)<<16,
        Motor1_C_Pin | ((uint32_t)Motor1_A_Pin|Motor1_B_Pin|Motor1_D_Pin)<<16,
        Motor1_D_Pin | ((uint32_t)Motor1_A_Pin|Motor1_B_Pin|Motor1_C_Pin)<<16
    },
    [1] = { // Motor2相序                                             
        Motor2_A_Pin | ((uint32_t)Motor2_B_Pin|Motor2_C_Pin|Motor2_D_Pin)<<16,
        Motor2_B_Pin | ((uint32_t)Motor2_A_Pin|Motor2_C_Pin|Motor2_D_Pin)<<16,
        Motor2_C_Pin | ((uint32_t)Motor2_A_Pin|Motor2_B_Pin|Motor2_D_Pin)<<16,
        Motor2_D_Pin | ((uint32_t)Motor2_A_Pin|Motor2_B_Pin|Motor2_C_Pin)<<16
    },
    [2] = { // Motor3相序
        Motor3_A_Pin | ((uint32_t)Motor3_B_Pin|Motor3_C_Pin|Motor3_D_Pin)<<16,
        Motor3_B_Pin | ((uint32_t)Motor3_A_Pin|Motor3_C_Pin|Motor3_D_Pin)<<16,
        Motor3_C_Pin | ((uint32_t)Motor3_A_Pin|Motor3_B_Pin|Motor3_D_Pin)<<16,
        Motor3_D_Pin | ((uint32_t)Motor3_A_Pin|Motor3_B_Pin|Motor3_C_Pin)<<16
    }
};

/* 电机实例定义 */
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

/**
 * @brief  GPIO复位函数
 * @param  m 电机实例指针
 * @retval None
 */
static void gpio_reset_pins(Motor *m) {
    m->gpio_port->BSRR = m->pins << 16; // 原子操作置低
}

/**
 * @brief  电机ID验证
 * @param  motor_id 电机ID
 * @retval Motor_Status 验证结果
 */
static Motor_Status validate_motor_id(uint8_t motor_id) {
    return (motor_id < MOTOR_NUM) ? MOTOR_OK : MOTOR_ERR_INVALID_ID;
}

/**
 * @brief  启动电机
 * @param  m 电机实例指针
 * @retval Motor_Status 状态码
 */
static Motor_Status motor_start(Motor *m) {
    if(m->state == MOTOR_RUNNING) return MOTOR_ERR_ALREADY_RUNNING;

    // 1. 配置定时器
    __HAL_TIM_SET_COUNTER(m->htim, 0);
    
    // 2. 启动定时器中断
    if(HAL_TIM_Base_Start_IT(m->htim) != HAL_OK) {
        return MOTOR_ERR_TIM_START;
    }

    m->state = MOTOR_RUNNING; // 设置为运行状态
    return MOTOR_OK;
}

/**
 * @brief  软件触发相位更新
 * @param  m 电机实例指针
 * @retval None
 */
void motor_update_phase(Motor *m) {
    if(m->state != MOTOR_RUNNING) return;
    
    // 直接将当前相位写入BSRR寄存器
    m->gpio_port->BSRR = m->phases[m->phase_index];
    
    // 根据方向更新相位索引和步数计数器
    if (m->dir == DIR_CW) {
        // 正转：索引递增
        m->phase_index = (m->phase_index + 1) % 4;
        // 每次相位变化都更新步数，增加1步
        m->step_counter++;
    } else {
        // 反转：索引递减（+3 等价于 -1，但避免负数）
        m->phase_index = (m->phase_index + 3) % 4;
        // 每次相位变化都更新步数，减少1步
        m->step_counter--;
    }
}

/**
 * @brief  初始化所有电机
 * @retval Motor_Status 初始化结果
 */
Motor_Status Motor_InitAll(void) {
    for(int i=0; i<MOTOR_NUM; i++) {
        Motor *m = &motors[i];
        
        // 确保ID和数组索引一致
        m->id = i;
        
        // 初始化GPIO
        gpio_reset_pins(m);
        m->state = MOTOR_STOP;
        m->dir = DIR_NONE;
        m->phase_index = 0;
        m->step_counter = 0;
    }
    return MOTOR_OK;
}

/**
 * @brief  启动电机
 * @param  id 电机ID(0-2)
 * @retval Motor_Status 操作结果
 */
Motor_Status Motor_Run(uint8_t id) {
    Motor_Status status;
    if((status = validate_motor_id(id)) != MOTOR_OK) 
        return status;
        
    Motor *m = &motors[id];
    
    // 如果电机已经在运行,直接返回成功
    if(m->state == MOTOR_RUNNING) 
        return MOTOR_OK;
    
    // 如果没有设置方向,默认使用顺时针
    if(m->dir == DIR_NONE) {
        m->dir = DIR_CW;
    }
    
    // 启动电机,motor_start内部会设置状态为MOTOR_RUNNING
    return motor_start(m);
}

/**
 * @brief  停止电机
 * @param  id 电机ID(0-2)
 * @retval Motor_Status 操作结果
 */
Motor_Status Motor_Stop(uint8_t id) {
    Motor_Status status;
    if((status = validate_motor_id(id)) != MOTOR_OK) 
        return status;
    
    Motor *m = &motors[id];
    
    // 如果电机已经停止,直接返回成功
    if(m->state == MOTOR_STOP)
        return MOTOR_OK;
    
    // 如果电机不在运行状态,返回错误
    if(m->state != MOTOR_RUNNING) 
        return MOTOR_OK;
    
    // 停止定时器中断
    HAL_TIM_Base_Stop_IT(m->htim);
    
    // 重置引脚状态
    gpio_reset_pins(m);
    
    // 设置电机状态为停止
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
 * @brief  设置电机旋转方向
 * @param  id 电机ID(0-2)
 * @param  dir 旋转方向(DIR_CW/DIR_CCW)
 * @retval Motor_Status 操作结果
 */
Motor_Status Motor_SetDir(uint8_t id, Motor_Dir dir) {
    Motor_Status status;
    if((status = validate_motor_id(id)) != MOTOR_OK) 
        return status;
    
    // 忽略无效方向设置
    if(dir == DIR_NONE) 
        return MOTOR_OK; // 忽略无效方向
    
    Motor *m = &motors[id];
    if(m->dir == dir) 
        return MOTOR_OK; // 方向相同,无需改变
    
    // 设置新方向
    m->dir = dir;
    
    return MOTOR_OK;
}

/**
 * @brief  设置电机速度
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
    
    // 二分查找最佳分频系数
    while(min_psc <= max_psc) {
        uint32_t mid_psc = (min_psc + max_psc) / 2;
        uint32_t calc_arr = (timer_clk / (steps_per_sec * (mid_psc + 1))) - 1;
        
        if(calc_arr <= 0xFFFF) {
            best_psc = mid_psc;
            best_arr = calc_arr;
            max_psc = mid_psc - 1; // 尽量小PSC
        } else {
            min_psc = mid_psc + 1;
        }
    }
    
    if(best_psc == 0 && best_arr == 0xFFFF) {
        return MOTOR_ERR_SPEED_RANGE;
    }
    
    // 设置定时器参数
    __HAL_TIM_SET_PRESCALER(m->htim, best_psc);
    __HAL_TIM_SET_AUTORELOAD(m->htim, best_arr);
    
    // 如果电机正在运行,更新计数器
    if(m->state == MOTOR_RUNNING) {
        __HAL_TIM_SET_COUNTER(m->htim, 0);
        m->htim->Instance->EGR = TIM_EGR_UG;
    }
    
    // 更新实际速度
    m->actual_speed = steps_per_sec;
    
    return MOTOR_OK;
}


/**
 * @brief  获取电机状态
 * @param  id 电机ID(0-2)
 * @retval Motor_State 电机状态
 */
Motor_State Motor_GetState(uint8_t id) {
    return (id < MOTOR_NUM) ? motors[id].state : MOTOR_ERROR;
}

/**
 * @brief  获取电机步数
 * @param  id 电机ID(0-2)
 * @retval uint32_t 步数
 */
uint32_t Motor_GetStepCount(uint8_t id) {
    return (id < MOTOR_NUM) ? motors[id].step_counter : 0;
}
