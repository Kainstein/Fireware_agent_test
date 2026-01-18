/*
 * mlx90614_api.h
 *
 *  Created on: Sep 13, 2023
 *      Author: fex95
 */

#ifndef SRC_MLX90614_API_H_
#define SRC_MLX90614_API_H_

#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
	void* interface;
	uint8_t address;
	uint8_t address_internal;

	GPIO_TypeDef * power_gpio;
	uint16_t power_gpio_pin;

} MLX90614;

enum MLX90614_RAM{
	MLX90614_RAM_RAW_IR_1 = 0x04,
	MLX90614_RAM_RAW_IR_2 = 0x05,
	MLX90614_RAM_T_AMBIENT = 0x06,
	MLX90614_RAM_T_OBJ_1	= 0x07,
	MLX90614_RAM_T_OBJ_2	= 0x08
};

enum MLX90614_EEPROM{
	MLX90614_EEPROM_TO_MAX = 0x00,
	MLX90614_EEPROM_TO_MIN = 0x01,
	MLX90614_EEPROM_PWM_CTRL = 0x02,
	MLX90614_EEPROM_TA_RANGE = 0x03,
	MLX90614_EEPROM_EMISSIVITY = 0x04,
	MLX90614_EEPROM_CONFIG_1 = 0x05,
	MLX90614_EEPROM_I2C_ADDRESS = 0x0E,
	MLX90614_EEPROM_ID_1 = 0x1C,
	MLX90614_EEPROM_ID_2 = 0x1D,
	MLX90614_EEPROM_ID_3 = 0x1E,
	MLX90614_EEPROM_ID_4 = 0x1F
};

enum MLC90614_OBJ{
	MLX90614_OBJ1,
	MLX90614_OBJ2
};

uint8_t MLX90614_init(MLX90614* sensor_obj);
void MLX90614_restart(MLX90614* sensor_obj);

uint16_t MLX90614_readEEPROM(MLX90614* sensor_obj, uint8_t reg);
uint8_t MLX90614_writeEEPROM(MLX90614* sensor_obj, uint8_t reg, uint16_t value);
void MLX90614_readRAM(MLX90614* sensor_obj, uint8_t reg, uint8_t* data);
void MLX90614_tunitToDegreeC(uint8_t* bytes, float* temperature);
void MLX90614_readAmbientTemperature(MLX90614* sensor_obj, float* data);
void MLX90614_readObjTemperature(MLX90614* sensor_obj, float* data, uint8_t obj_nr);

#ifdef __cplusplus
}
#endif

#endif /* SRC_MLX90614_API_H_ */
