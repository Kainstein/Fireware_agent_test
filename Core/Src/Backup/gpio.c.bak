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

/** Configure pins
     PC14-OSC32_IN (OSC32_IN)   ------> RCC_OSC32_IN
     PC15-OSC32_OUT (OSC32_OUT)   ------> RCC_OSC32_OUT
     PH0-OSC_IN (PH0)   ------> RCC_OSC_IN
     PH1-OSC_OUT (PH1)   ------> RCC_OSC_OUT
     PB12   ------> SPI2_NSS
     PB13   ------> SPI2_SCK
     PB14   ------> SPI2_MISO
     PB15   ------> SPI2_MOSI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, LED_ADD3_OUT_Pin|LED_ADD2_OUT_Pin|LED_ADD1_OUT_Pin|LED_ADD0_OUT_Pin
                          |FT232_RESET__OUT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, LED_STATUS_Pin|LED_TRACKER_MOTOR_5130_Pin|LED_TILT_MOTOR_5130_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(HIGH_TEMP_RED_LED_ENABLE_OUT_GPIO_Port, HIGH_TEMP_RED_LED_ENABLE_OUT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, IMAGE_SEARCHING_5130_Enable_Driver_Stage_Pin|HEATING_PWM_OUT_Pin|PELTIER_PWM_OUT_Pin|BLDC_ENA_OUT_Pin
                          |BLDC_PULSES_SELECTION_OUT_Pin|D_Triggers_Reset_OUT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SPINNING_BLUE_LED_ENABLE_OUT_Pin|HIGH_TEMP_RED_LED_ENABLE_OUTB9_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : DS18B20_DQ_Pin */
  GPIO_InitStruct.Pin = DS18B20_DQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DS18B20_DQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : KEY_INT_IN_Pin */
  GPIO_InitStruct.Pin = KEY_INT_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY_INT_IN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_ADD3_OUT_Pin LED_ADD2_OUT_Pin LED_ADD1_OUT_Pin LED_ADD0_OUT_Pin */
  GPIO_InitStruct.Pin = LED_ADD3_OUT_Pin|LED_ADD2_OUT_Pin|LED_ADD1_OUT_Pin|LED_ADD0_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : RFU_GPIO2_Pin RFT_GPIO1_Pin */
  GPIO_InitStruct.Pin = RFU_GPIO2_Pin|RFT_GPIO1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : FT232_RESET__OUT_Pin */
  GPIO_InitStruct.Pin = FT232_RESET__OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(FT232_RESET__OUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 PB14 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_STATUS_Pin LED_TRACKER_MOTOR_5130_Pin LED_TILT_MOTOR_5130_Pin */
  GPIO_InitStruct.Pin = LED_STATUS_Pin|LED_TRACKER_MOTOR_5130_Pin|LED_TILT_MOTOR_5130_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : HIGH_TEMP_RED_LED_ENABLE_OUT_Pin */
  GPIO_InitStruct.Pin = HIGH_TEMP_RED_LED_ENABLE_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(HIGH_TEMP_RED_LED_ENABLE_OUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TILT_5130_DIAG1_INT_Pin */
  GPIO_InitStruct.Pin = TILT_5130_DIAG1_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(TILT_5130_DIAG1_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TILT_5130_DIAG0_INT_Pin */
  GPIO_InitStruct.Pin = TILT_5130_DIAG0_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(TILT_5130_DIAG0_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : IMAGE_SEARCHING_5130_Enable_Driver_Stage_Pin BLDC_ENA_OUT_Pin */
  GPIO_InitStruct.Pin = IMAGE_SEARCHING_5130_Enable_Driver_Stage_Pin|BLDC_ENA_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : IMAGE_SEARCHING_5130_DIAG1_INT_Pin IMAGE_SEARCHING_5130_DIAG0_INT_Pin */
  GPIO_InitStruct.Pin = IMAGE_SEARCHING_5130_DIAG1_INT_Pin|IMAGE_SEARCHING_5130_DIAG0_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : HEATING_PWM_OUT_Pin PELTIER_PWM_OUT_Pin BLDC_PULSES_SELECTION_OUT_Pin D_Triggers_Reset_OUT_Pin */
  GPIO_InitStruct.Pin = HEATING_PWM_OUT_Pin|PELTIER_PWM_OUT_Pin|BLDC_PULSES_SELECTION_OUT_Pin|D_Triggers_Reset_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : SPINNING_BLUE_LED_ENABLE_OUT_Pin HIGH_TEMP_RED_LED_ENABLE_OUTB9_Pin */
  GPIO_InitStruct.Pin = SPINNING_BLUE_LED_ENABLE_OUT_Pin|HIGH_TEMP_RED_LED_ENABLE_OUTB9_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
