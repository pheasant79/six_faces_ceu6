/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tim.h"
#include "dma.h"
#include "gpio.h"
#include "motor.h"
#include "Oled.h"
#include "Oled_Data.h"
#include "menu.h"
#include "mymath.h"
#include "Key.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Motor2_A_Pin GPIO_PIN_0
#define Motor2_A_GPIO_Port GPIOA
#define Motor2_B_Pin GPIO_PIN_1
#define Motor2_B_GPIO_Port GPIOA
#define Motor2_C_Pin GPIO_PIN_2
#define Motor2_C_GPIO_Port GPIOA
#define Motor2_D_Pin GPIO_PIN_3
#define Motor2_D_GPIO_Port GPIOA
#define lcd_Pin GPIO_PIN_7
#define lcd_GPIO_Port GPIOA
#define Key1_Pin GPIO_PIN_0
#define Key1_GPIO_Port GPIOB
#define Key2_Pin GPIO_PIN_1
#define Key2_GPIO_Port GPIOB
#define OLED_SCL_Pin GPIO_PIN_8
#define OLED_SCL_GPIO_Port GPIOA
#define OLED_SDA_Pin GPIO_PIN_9
#define OLED_SDA_GPIO_Port GPIOA
#define Motor1_A_Pin GPIO_PIN_10
#define Motor1_A_GPIO_Port GPIOA
#define Motor1_B_Pin GPIO_PIN_11
#define Motor1_B_GPIO_Port GPIOA
#define Motor1_C_Pin GPIO_PIN_12
#define Motor1_C_GPIO_Port GPIOA
#define Motor1_D_Pin GPIO_PIN_15
#define Motor1_D_GPIO_Port GPIOA
#define Motor3_A_Pin GPIO_PIN_3
#define Motor3_A_GPIO_Port GPIOB
#define Motor3_B_Pin GPIO_PIN_4
#define Motor3_B_GPIO_Port GPIOB
#define Motor3_C_Pin GPIO_PIN_5
#define Motor3_C_GPIO_Port GPIOB
#define Motor3_D_Pin GPIO_PIN_6
#define Motor3_D_GPIO_Port GPIOB
#define Key3_Pin GPIO_PIN_7
#define Key3_GPIO_Port GPIOB
#define Key4_Pin GPIO_PIN_8
#define Key4_GPIO_Port GPIOB
#define Key5_Pin GPIO_PIN_9
#define Key5_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
