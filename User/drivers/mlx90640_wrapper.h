/**
 * @file led_driver.h
 * @brief LED Driver for STM32H743 project with individual LED functions
 * @author Generated based on gpio.c analysis
 */

#ifndef MLX90640_WRAPPER_H
#define MLX90640_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "main.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define MLX90640_PRINT_TUNIT    0u   // Set to 1 to enable printing temperature units in test function
/* Exported functions prototypes ---------------------------------------------*/
void mlx90640_wrapper_init(void);
void mlx90640_wrapper_sample_polling(void);
float read_mlx90640_obj_min_temp(void);
float read_mlx90640_obj_max_temp(void);
float read_mlx90640_obj_avg_temp(void);
void MLX90640_I2CInit(void);
int  MLX90640_I2CRead(uint8_t slaveAddr,uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data);
int  MLX90640_I2CWrite(uint8_t slaveAddr,uint16_t writeAddress, uint16_t data);
void MLX90640_I2CFreqSet(int freq);
#ifdef __cplusplus
}
#endif

#endif /* MLX90640_WRAPPER_H */
