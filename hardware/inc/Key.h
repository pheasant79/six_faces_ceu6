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

/*====================== 函数声明 ======================*/

/*--------------------- 初始化函数 ---------------------*/
// 按键初始化
void Key_Init(void);

/*--------------------- 状态查询函数 ---------------------*/
// 获取当前按下的按键编号
uint8_t Key_GetNum(void);

/*--------------------- 内部函数 ---------------------*/
// 2ms定时器中断回调处理函数
void Key_Tick(void);

#endif /* __KEY_H */
