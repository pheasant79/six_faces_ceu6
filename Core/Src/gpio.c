/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, Motor2_A_Pin|Motor2_B_Pin|Motor2_C_Pin|Motor2_D_Pin
                          |OLED_SCL_Pin|OLED_SDA_Pin|Motor1_A_Pin|Motor1_B_Pin
                          |Motor1_C_Pin|Motor1_D_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(lcd_GPIO_Port, lcd_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Motor3_A_Pin|Motor3_B_Pin|Motor3_C_Pin|Motor3_D_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : Motor2_A_Pin Motor2_B_Pin Motor2_C_Pin Motor2_D_Pin
                           Motor1_A_Pin Motor1_B_Pin Motor1_C_Pin Motor1_D_Pin */
  GPIO_InitStruct.Pin = Motor2_A_Pin|Motor2_B_Pin|Motor2_C_Pin|Motor2_D_Pin
                          |Motor1_A_Pin|Motor1_B_Pin|Motor1_C_Pin|Motor1_D_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : lcd_Pin OLED_SCL_Pin OLED_SDA_Pin */
  GPIO_InitStruct.Pin = lcd_Pin|OLED_SCL_Pin|OLED_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : Key1_Pin Key2_Pin Key3_Pin Key4_Pin
                           Key5_Pin */
  GPIO_InitStruct.Pin = Key1_Pin|Key2_Pin|Key3_Pin|Key4_Pin
                          |Key5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : Motor3_A_Pin Motor3_B_Pin Motor3_C_Pin Motor3_D_Pin */
  GPIO_InitStruct.Pin = Motor3_A_Pin|Motor3_B_Pin|Motor3_C_Pin|Motor3_D_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
