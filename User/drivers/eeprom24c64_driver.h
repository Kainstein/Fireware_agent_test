/**
 * @file eeprom24c64_driver.h
 * @brief Header for M24C64 EEPROM driver using I2C3 on STM32H743
 */
#ifndef EEPROM24C64_DRIVER_H
#define EEPROM24C64_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
/*******************************************************************************
 *                                  MACROS                                     *
 *******************************************************************************/

/* Address validation macros */
#define IS_M24C64_MAIN_ADDRESS(addr)    ((addr) < M24C64_TOTAL_SIZE)
#define IS_M24C64_ID_ADDRESS(addr)      ((addr) <= M24C64_MAX_ID_ADDR)

/* Page alignment helpers */
#define M24C64_PAGE_ALIGN(addr)         ((addr) & ~(M24C64_PAGE_SIZE - 1))
#define M24C64_PAGE_OFFSET(addr)        ((addr) & (M24C64_PAGE_SIZE - 1))

/* Exported constants --------------------------------------------------------*/


/* I2C3 Pin Definitions */
#define I2C3_SDA_Pin              GPIO_PIN_9   // Update with your actual pin
#define I2C3_SCL_Pin              GPIO_PIN_8   // Update with your actual pin
#define I2C3_SDA_GPIO_Port        GPIOC       // Update with your actual port
#define I2C3_SCL_GPIO_Port        GPIOA       // Update with your actual port
/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern uint8_t g_m24c64_main_eeprom_addr;    // Main EEPROM memory address
extern uint8_t g_m24c64_id_page_addr;        // Identification page address

/* Exported functions prototypes ---------------------------------------------*/

/* Initialization */
HAL_StatusTypeDef M24C64_Init(void);  // Updated to remove parameter
HAL_StatusTypeDef M24C64_IsMainDeviceReady(void);
/* Helper Functions */
HAL_StatusTypeDef M24C64_WaitForOperation(uint32_t timeout);
HAL_StatusTypeDef M24C64_WaitForWriteCompletion(void);
// Added bus recovery function
void I2C_BusRecovery(void);  
void M24C64_TestI2CLines(void);
void M24C64_Diagnostics(void);

/* Identification Page Functions */
HAL_StatusTypeDef M24C64_WriteIdPage_Polling(uint8_t id_address, uint8_t data);
HAL_StatusTypeDef M24C64_ReadIdPage_Polling(uint8_t id_address, uint8_t *pData);

/* Polling Mode Functions */
HAL_StatusTypeDef M24C64_WriteByte_Polling(uint16_t memory_address, uint8_t data);
HAL_StatusTypeDef M24C64_ReadByte_Polling(uint16_t memory_address, uint8_t *pData);
HAL_StatusTypeDef M24C64_WriteBytes_Polling(uint16_t memory_address, uint8_t *pData, uint16_t size);
HAL_StatusTypeDef M24C64_ReadBytes_Polling(uint16_t memory_address, uint8_t *pData, uint16_t size);

/* Interrupt Mode Functions */
HAL_StatusTypeDef M24C64_WriteByte_IT(uint16_t memory_address, uint8_t data);
HAL_StatusTypeDef M24C64_ReadByte_IT(uint16_t memory_address, uint8_t *pData);
HAL_StatusTypeDef M24C64_WritePage_IT(uint16_t memory_address, uint8_t *pData, uint16_t size);
HAL_StatusTypeDef M24C64_ReadBytes_IT(uint16_t memory_address, uint8_t *pData, uint16_t size);

/* Convenience wrappers for simplified API */
HAL_StatusTypeDef EEPROM_WriteBytes(uint16_t address, uint8_t *pData, uint16_t size);
HAL_StatusTypeDef EEPROM_ReadBytes(uint16_t address, uint8_t *pData, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* EEPROM24C64_DRIVER_H */
