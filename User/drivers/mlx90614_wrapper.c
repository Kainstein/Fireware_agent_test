/**
 * @file led_driver.c
 * @brief LED Driver implementation with individual LED functions
 */
#include <stdio.h>
#include "mlx90614_api.h"
#include "driver_printf_config.h"

/* Conditional printf for MLX90614 wrapper module */
#if (ENABLE_DRV_PRINTF && ENABLE_DRV_PRINTF_MLX90614_WRAPPER)
    #define MLX90614_WRAPPER_PRINTF(...)    printf(__VA_ARGS__)
#else
    #define MLX90614_WRAPPER_PRINTF(...)    ((void)0)
#endif

/* Private types -------------------------------------------------------------*/
extern I2C_HandleTypeDef hi2c1;
/* Private variables ---------------------------------------------------------*/
#define  MLX90614_ADDR 0x5A
MLX90614 mlx90614_sensor[1];
float mlx90614_t_a, mlx90614_t_obj;
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/

void mlx90614_wrapper_init(void)
{
	mlx90614_sensor->address = MLX90614_ADDR;
	mlx90614_sensor->interface = &hi2c1;
	mlx90614_sensor->power_gpio = GPIOB; // GPIOB
	mlx90614_sensor->power_gpio_pin = GPIO_PIN_9; // SPINNING_BLUE_LED_ENABLE_OUT_Pin

	if(MLX90614_init(mlx90614_sensor) != HAL_OK) 
	{
		MLX90614_WRAPPER_PRINTF("connect mlx90614 failure!\r\n");
	}
	else
	{
		MLX90614_WRAPPER_PRINTF("connect mlx90614 success!\r\n");
	}

	MLX90614_writeEEPROM(mlx90614_sensor, MLX90614_EEPROM_I2C_ADDRESS, 0x005A);

	int newAddress = MLX90614_readEEPROM(mlx90614_sensor, MLX90614_EEPROM_I2C_ADDRESS);
#if MLX90614_PRINT_TUNIT == 1u
	MLX90614_WRAPPER_PRINTF("mlx90614 New address: %X\r\n", newAddress);
#endif
}

static uint32_t mlx90614_test_cnt = 0;
void mlx90614_wrapper_sample_polling(void)
{
	mlx90614_test_cnt++;
#if MLX90614_PRINT_TUNIT == 1u
	MLX90614_WRAPPER_PRINTF("\r\n============%d==============MLX90614 WRAPPER TEST EXAMPLE ==========================\r\n", mlx90614_test_cnt);
#endif
	MLX90614_readAmbientTemperature(mlx90614_sensor, &mlx90614_t_a);
	MLX90614_readObjTemperature(mlx90614_sensor, &mlx90614_t_obj, MLX90614_OBJ1);
#if MLX90614_PRINT_TUNIT == 1u
	MLX90614_WRAPPER_PRINTF("t_a: %f\t t_obj: %f\r\n", mlx90614_t_a, mlx90614_t_obj);
#endif
}

float read_mlx90614_obj_temp(void)
{
	return mlx90614_t_obj;
}
float read_mlx90614_ambient_temp(void)
{
	return mlx90614_t_a;
}
