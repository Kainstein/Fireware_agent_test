/**
 * @file led_driver.h
 * @brief LED Driver for STM32H743 project with individual LED functions
 * @author Generated based on gpio.c analysis
 */

#ifndef MLX90614_WRAPPER_H
#define MLX90614_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "main.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define MLX90614_PRINT_TUNIT    1u   // Set to 1 to enable printing temperature units in test function
/* Exported functions prototypes ---------------------------------------------*/
void mlx90614_wrapper_init(void);
void mlx90614_wrapper_sample_polling(void);
float read_mlx90614_obj_temp(void);
float read_mlx90614_amb_temp(void);
#ifdef __cplusplus
}
#endif

#endif /* MLX90614_WRAPPER_H */
