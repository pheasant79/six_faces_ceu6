/**
 * @file key.h
 * @brief 六面顶按键驱动头文件
 */
#ifndef __KEY_H
#define __KEY_H

#include "main.h"

/* 按键数量定义 */
#define KEY_NUM     5   // 总按键数量

/* 按键GPIO定义结构体 */
typedef struct {
    GPIO_TypeDef* GPIO_Port;
    uint16_t GPIO_Pin;
} Key_GPIO;

/**
 * @brief  按键初始化
 * @retval None
 */
void Key_Init(void);              // 按键初始化
uint8_t Key_GetNum(void);         // 获取按键编号
void Key_Tick(void);              // 2ms定时器回调

#endif /* __KEY_H */
