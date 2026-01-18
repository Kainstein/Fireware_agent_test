/**
 * @file hal_callbacks.h
 * @brief UART Protocol HAL Callback Declarations
 * @date 2026-01-09
 * @details Contains HAL interrupt callbacks for UART frame processing
 */

#ifndef __HAL_CALLBACKS_H__
#define __HAL_CALLBACKS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "app_printf_config.h"

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_CALLBACKS_H__ */
