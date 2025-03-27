/**
 * @file key.c
 * @brief 六面顶按键驱动实现
 */
#include "key.h"

/* 消抖时间定义 */
#define KEY_DEBOUNCE_TIME  10  // 20ms消抖(10个2ms周期)

// 按键状态记录(1表示未按下,0表示按下)
static uint8_t key_prev_state[KEY_NUM] = {1, 1, 1, 1, 1};  // 初始为释放状态(高电平)
uint8_t key_num = 0;  // 当前按键编号

/* 按键GPIO定义(按实际硬件连接配置) */
static const Key_GPIO KEY_GPIO[KEY_NUM] = {
    {Key1_GPIO_Port, Key1_Pin},  // Key1 - PB7
    {Key2_GPIO_Port, Key2_Pin},  // Key2 - PB8
    {Key3_GPIO_Port, Key3_Pin},  // Key3 - PB9
    {Key4_GPIO_Port, Key4_Pin},  // Key4 - PB12
    {Key5_GPIO_Port, Key5_Pin}   // Key5 - PB13
};

/**
 * @brief  按键初始化
 * @note   GPIO已在CubeMX中配置为上拉输入
 * @retval None
 */
void Key_Init(void) {
    // GPIO已在CubeMX中完成配置
    // 1. 使能GPIOB时钟
    // 2. 配置为上拉输入模式
}

/**
 * @brief  获取当前触发的按键编号
 * @note   非阻塞式读取,读取后自动清除按键状态
 * @retval uint8_t 按键编号(1-5),0表示无按键
 */
uint8_t Key_GetNum(void) {
    uint8_t temp = key_num;
    key_num = 0;  // 清除按键状态
    return temp;
}

/**
 * @brief  按键扫描处理函数
 * @note   需要在2ms定时器中断中调用
 * @retval None
 */
void Key_Tick(void) {
    static uint8_t count = 0;
    if(++count < KEY_DEBOUNCE_TIME) return;
    
    count = 0;
    for(uint8_t i = 0; i < KEY_NUM; i++) {
        GPIO_PinState current = HAL_GPIO_ReadPin(KEY_GPIO[i].GPIO_Port, 
                                               KEY_GPIO[i].GPIO_Pin);
        // 检测上升沿并更新状态
        if(!key_prev_state[i] && current) key_num = i + 1;
        key_prev_state[i] = current;
    }
}

