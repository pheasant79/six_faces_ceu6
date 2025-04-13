/**
  ******************************************************************************
  * @file    led.h
  * @brief   LED控制模块头文件
  * @details 控制PA7引脚作为开漏输出实现LED控制
  ******************************************************************************
  */

#ifndef __LED_H
#define __LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* 包含头文件 ------------------------------------------------------------------*/
#include "main.h"

/* 宏定义 ----------------------------------------------------------------------*/
#define LED_PORT                 lcd_GPIO_Port    // LED控制端口
#define LED_PIN                  lcd_Pin          // LED控制引脚

/* LED控制宏 */
#define LED_ON()                 HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET)   // LED打开
#define LED_OFF()                HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET)     // LED关闭

/* 导出函数声明 ----------------------------------------------------------------*/

//==== 基础初始化函数 ====//
// LED初始化函数
void LED_Init(void);

//==== LED控制函数 ====//
// 打开LED
void LED_TurnOn(void);

// 关闭LED
void LED_TurnOff(void);

// 切换LED状态
void LED_Toggle(void);

//==== LED状态获取 ====//
// 获取LED当前状态 - 返回值: 0-关闭, 1-打开
uint8_t LED_GetState(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_H */ 
