/**
 * @file motor.c
 * @brief 六面顶步进电机驱动实现
 */
#include "motor.h"
#include <string.h>

/* 计算DMA传输完成标志 */
// 根据DMA实例和流号判断标志位位置
#define MOTOR_DMA_TC_FLAG(hdma)  __HAL_DMA_GET_TC_FLAG_INDEX(hdma)

/* 静态函数声明 */
static void gpio_reset_pins(Motor *m);
static Motor_Status validate_motor_id(uint8_t motor_id);
static Motor_Status motor_dma_abort(Motor *m);
static Motor_Status motor_dma_start(Motor *m);

/* 相位表定义 - 使用BSRR寄存器实现原子操作
 * 高16位写1清零对应引脚,低16位写1置位对应引脚
 */
static const uint32_t Motor_PhaseTables[3][2][4] = {
    [0] = { // Motor1
        { // 顺时针相序
            Motor1_A_Pin | ((uint32_t)Motor1_B_Pin|Motor1_C_Pin|Motor1_D_Pin)<<16,
            Motor1_B_Pin | ((uint32_t)Motor1_A_Pin|Motor1_C_Pin|Motor1_D_Pin)<<16,
            Motor1_C_Pin | ((uint32_t)Motor1_A_Pin|Motor1_B_Pin|Motor1_D_Pin)<<16,
            Motor1_D_Pin | ((uint32_t)Motor1_A_Pin|Motor1_B_Pin|Motor1_C_Pin)<<16
        },
        { // 逆时针相序
            Motor1_D_Pin | ((uint32_t)Motor1_A_Pin|Motor1_B_Pin|Motor1_C_Pin)<<16,
            Motor1_C_Pin | ((uint32_t)Motor1_A_Pin|Motor1_B_Pin|Motor1_D_Pin)<<16,
            Motor1_B_Pin | ((uint32_t)Motor1_A_Pin|Motor1_C_Pin|Motor1_D_Pin)<<16,
            Motor1_A_Pin | ((uint32_t)Motor1_B_Pin|Motor1_C_Pin|Motor1_D_Pin)<<16
        }
    },
    [1] = { // Motor2
        { // 顺时针相序
            Motor2_A_Pin | ((uint32_t)Motor2_B_Pin|Motor2_C_Pin|Motor2_D_Pin)<<16,
            Motor2_B_Pin | ((uint32_t)Motor2_A_Pin|Motor2_C_Pin|Motor2_D_Pin)<<16,
            Motor2_C_Pin | ((uint32_t)Motor2_A_Pin|Motor2_B_Pin|Motor2_D_Pin)<<16,
            Motor2_D_Pin | ((uint32_t)Motor2_A_Pin|Motor2_B_Pin|Motor2_C_Pin)<<16
        },                                                          
        { // 逆时针相序                                            
            Motor2_D_Pin | ((uint32_t)Motor2_A_Pin|Motor2_B_Pin|Motor2_C_Pin)<<16,
            Motor2_C_Pin | ((uint32_t)Motor2_A_Pin|Motor2_B_Pin|Motor2_D_Pin)<<16,
            Motor2_B_Pin | ((uint32_t)Motor2_A_Pin|Motor2_C_Pin|Motor2_D_Pin)<<16,
            Motor2_A_Pin | ((uint32_t)Motor2_B_Pin|Motor2_C_Pin|Motor2_D_Pin)<<16
        }
    },
    [2] = { // Motor3
        { // 顺时针相序
            Motor3_A_Pin | ((uint32_t)Motor3_B_Pin|Motor3_C_Pin|Motor3_D_Pin)<<16,
            Motor3_B_Pin | ((uint32_t)Motor3_A_Pin|Motor3_C_Pin|Motor3_D_Pin)<<16,
            Motor3_C_Pin | ((uint32_t)Motor3_A_Pin|Motor3_B_Pin|Motor3_D_Pin)<<16,
            Motor3_D_Pin | ((uint32_t)Motor3_A_Pin|Motor3_B_Pin|Motor3_C_Pin)<<16
        },                                 
        { // 逆时针相序                  
            Motor3_D_Pin | ((uint32_t)Motor3_A_Pin|Motor3_B_Pin|Motor3_C_Pin)<<16,
            Motor3_C_Pin | ((uint32_t)Motor3_A_Pin|Motor3_B_Pin|Motor3_D_Pin)<<16,
            Motor3_B_Pin | ((uint32_t)Motor3_A_Pin|Motor3_C_Pin|Motor3_D_Pin)<<16,
            Motor3_A_Pin | ((uint32_t)Motor3_B_Pin|Motor3_C_Pin|Motor3_D_Pin)<<16
        }
    }
};

/* 电机实例定义 */
Motor motors[MOTOR_NUM] = {
    [0] = {
        .htim = &htim2,
        .hdma = &hdma_tim2_ch3_up,
        .gpio_port = Motor1_A_GPIO_Port,
        .pins = Motor1_A_Pin | Motor1_B_Pin | Motor1_C_Pin | Motor1_D_Pin,

        .state = MOTOR_STOP,
        .dir = DIR_NONE,
        .phase_buffer = {0},
        .forward_phases = Motor_PhaseTables[0][0],
        .reverse_phases = Motor_PhaseTables[0][1],

        .max_speed = 1000,
        .min_speed = 10
    },
    [1] = {
        .htim = &htim3,
        .hdma = &hdma_tim3_ch4_up,
        .gpio_port = Motor2_A_GPIO_Port,
        .pins = Motor2_A_Pin | Motor2_B_Pin | Motor2_C_Pin | Motor2_D_Pin,
        
        .state = MOTOR_STOP,
        .dir = DIR_NONE,
        .phase_buffer = {0},
        .forward_phases = Motor_PhaseTables[1][0],
        .reverse_phases = Motor_PhaseTables[1][1],
        
        .max_speed = 1000,
        .min_speed = 10
    },
    [2] = {
        .htim = &htim4,
        .hdma = &hdma_tim4_up,
        .gpio_port = Motor3_A_GPIO_Port,
        .pins = Motor3_A_Pin | Motor3_B_Pin | Motor3_C_Pin | Motor3_D_Pin,
        
        .state = MOTOR_STOP,
        .dir = DIR_NONE,
        .phase_buffer = {0},
        .forward_phases = Motor_PhaseTables[2][0], 
        .reverse_phases = Motor_PhaseTables[2][1],
        
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
 * @brief  安全中止DMA传输
 * @param  m 电机实例指针
 * @retval Motor_Status 状态码
 * @note   中止DMA传输但保留当前相位位置,以便重新启动时能够从正确的相位继续
 */
static Motor_Status motor_dma_abort(Motor *m) {
    if(m->state != MOTOR_RUNNING) return MOTOR_OK;

    // 1. 获取当前相位位置
    uint32_t transferred = 4 - __HAL_DMA_GET_COUNTER(m->hdma);
    uint8_t current_phase = transferred % 4;

    // 2. 禁用TIM的DMA请求
    __HAL_TIM_DISABLE_DMA(m->htim, TIM_DMA_UPDATE);

    // 3. 等待当前传输完成
    uint32_t tc_flag = MOTOR_DMA_TC_FLAG(m->hdma);
    uint32_t start = HAL_GetTick();
    while(!__HAL_DMA_GET_FLAG(m->hdma, tc_flag)) {
        if(HAL_GetTick() - start > DMA_ABORT_TIMEOUT) {
            m->state = MOTOR_ERROR;
            __HAL_DMA_CLEAR_FLAG(m->hdma, tc_flag);
            HAL_DMA_Abort(m->hdma);
            return MOTOR_ERR_DMA_ABORT_TIMEOUT;
        }
    }

    // 4. 清除标志位
    __HAL_DMA_CLEAR_FLAG(m->hdma, tc_flag);

    // 5. 中止DMA传输
    if(HAL_DMA_Abort(m->hdma) != HAL_OK) {
        m->state = MOTOR_ERROR;
        return MOTOR_ERR_DMA_BUSY;
    }

    // 6. 保持最后的有效相位
    uint32_t last_phase = m->phase_buffer[current_phase];
    m->gpio_port->BSRR = last_phase;

    // 7. 重新排列相位缓冲区,使下次启动时从当前相位继续
    uint32_t temp[4];
    for(int i=0; i<4; i++) {
        temp[i] = m->phase_buffer[(current_phase + i) % 4];
    }
    memcpy(m->phase_buffer, temp, sizeof(temp));

    // 注意:不改变电机状态,由调用者决定是否设置为MOTOR_STOP
    return MOTOR_OK;
}

/**
 * @brief  启动DMA传输
 * @param  m 电机实例指针
 * @retval Motor_Status 状态码
 */
static Motor_Status motor_dma_start(Motor *m) {
    if(m->state == MOTOR_RUNNING) return MOTOR_ERR_ALREADY_RUNNING;

    // 1. 配置DMA
    m->hdma->Instance->CR &= ~DMA_SxCR_EN;
    m->hdma->Init.Mode = DMA_CIRCULAR;
    m->hdma->Init.Direction = DMA_MEMORY_TO_PERIPH;
    m->hdma->Init.PeriphInc = DMA_PINC_DISABLE;
    m->hdma->Init.MemInc = DMA_MINC_ENABLE;
    m->hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    m->hdma->Init.MemDataAlignment = DMA_MDATAALIGN_WORD;

    if(HAL_DMA_Init(m->hdma) != HAL_OK) {
        m->state = MOTOR_ERROR;
        return MOTOR_ERR_DMA_CONFIG;
    }

    // 2. 启动DMA传输
    if(HAL_DMA_Start_IT(m->hdma, (uint32_t)m->phase_buffer, 
        (uint32_t)&m->gpio_port->BSRR, 4) != HAL_OK) {
        m->state = MOTOR_ERROR;
        return MOTOR_ERR_DMA_BUSY;
    }

    // 3. 配置定时器
    __HAL_TIM_SET_COUNTER(m->htim, 0);
    __HAL_TIM_ENABLE_DMA(m->htim, TIM_DMA_UPDATE);

    // 4. 启动定时器
    if(HAL_TIM_Base_Start(m->htim) != HAL_OK) {
        motor_dma_abort(m);
        return MOTOR_ERR_TIM_START;
    }

    m->state = MOTOR_RUNNING; // 直接设置为运行状态
    return MOTOR_OK;
}

/**
 * @brief  初始化所有电机
 * @retval Motor_Status 初始化结果
 */
Motor_Status Motor_InitAll(void) {
    for(int i=0; i<MOTOR_NUM; i++) {
        Motor *m = &motors[i];
        
        // 初始化GPIO
        gpio_reset_pins(m);
        memcpy(m->phase_buffer, m->forward_phases, sizeof(m->phase_buffer));
        m->state = MOTOR_STOP;
        m->dir = DIR_NONE;
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
        memcpy(m->phase_buffer, m->forward_phases, sizeof(m->phase_buffer));
        m->dir = DIR_CW;
    }
    
    // 启动DMA传输,motor_dma_start内部会设置状态为MOTOR_RUNNING
    return motor_dma_start(m);
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
    
    // 中止DMA传输并保留当前相位位置
    status = motor_dma_abort(m);
    if(status != MOTOR_OK) {
        return status;
    }
    
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
    
    if(m->state == MOTOR_RUNNING) {
        // 运行状态下改变方向
        if((status = motor_dma_abort(m)) != MOTOR_OK) 
            return status;
        
        // 获取当前相位位置,确保平滑过渡
        // 注意:motor_dma_abort已经重新排列了相位缓冲区,
        // 所以这里直接使用新的相位表即可
        
        // 根据新方向重新设置相位,保持当前位置不变
        if(dir == DIR_CCW) {
            uint32_t temp[4];
            const uint32_t *src = m->reverse_phases;
            for(int i=0; i<4; i++) {
                temp[i] = src[i];
            }
            memcpy(m->phase_buffer, temp, sizeof(temp));
        } else {
            uint32_t temp[4];
            const uint32_t *src = m->forward_phases;
            for(int i=0; i<4; i++) {
                temp[i] = src[i];
            }
            memcpy(m->phase_buffer, temp, sizeof(temp));
        }
        
        m->dir = dir;
        
        // 重新启动DMA传输
        return motor_dma_start(m);
    } else {
        // 停止状态下直接设置方向
        m->dir = dir;
        const uint32_t *src = (dir == DIR_CW) ? m->forward_phases : m->reverse_phases;
        memcpy(m->phase_buffer, src, sizeof(m->phase_buffer));
        return MOTOR_OK;
    }
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
