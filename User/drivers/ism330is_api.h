/**
 * @file ism330is_api.h  
 * @brief I2C2 driver for ISM330IS sensor on STM32H743
 * @author Your Name
 * @version 1.0
 * @date 2025
 */

#ifndef ISM330IS_API_H
#define ISM330IS_API_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "ism330is_driver.h"
#include "stdint.h"
#include "stdbool.h"
#include <math.h>

/* Exported constants --------------------------------------------------------*/
#define I2C2_TIMEOUT            1000    // I2C timeout in ms

/* Mathematical constants */
#ifndef M_PI
#define M_PI                    3.14159265358979323846f
#endif

/* ISM330IS I2C Addresses */
#define ISM330IS_I2C_ADDR_LOW   0xD4    // 0xD5 >> 1 (7-bit address)
#define ISM330IS_I2C_ADDR_HIGH  0xD6    // 0xD7 >> 1 (7-bit address)

/* Default I2C address (depends on SA0 pin) */
#define ISM330IS_I2C_ADDR_DEFAULT  0xD5

/* Exported types ------------------------------------------------------------*/
typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint16_t device_address;
    uint32_t timeout;
} I2C2_ISM330IS_t;

/* Angle State Structure */
typedef struct {
    float pitch;
    float roll;
    float yaw;
    uint32_t last_timestamp;
    uint8_t initialized;
} ISM330IS_AngleState_t;

/* Exported variables --------------------------------------------------------*/
extern I2C_HandleTypeDef hi2c2;

/* Exported functions prototypes ---------------------------------------------*/

/* Initialization Functions */
int32_t I2C2_ISM330IS_HW_Init(void);
int32_t I2C2_ISM330IS_DeInit(void);

/* Low-level I2C Functions */
int32_t I2C2_ISM330IS_ReadReg(uint16_t DevAddr, uint8_t Reg, uint8_t *pData, uint16_t Length);
int32_t I2C2_ISM330IS_WriteReg(uint16_t DevAddr, uint8_t Reg, uint8_t *pData, uint16_t Length);

/* Utility Functions */
int32_t I2C2_ISM330IS_IsDeviceReady(uint16_t DevAddr);
uint32_t I2C2_GetTick(void);

/* ISM330IS Integration Functions */
int32_t I2C2_ISM330IS_ConfigureIO(ISM330IS_IO_t *pIO);
int32_t I2C2_ISM330IS_SensorInit(ISM330IS_Object_t *pObj);

/* Configuration Functions */
void I2C2_ISM330IS_SetAddress(uint16_t addr);
uint16_t I2C2_ISM330IS_GetAddress(void);
void I2C2_ISM330IS_SetTimeout(uint32_t timeout);

/* Temperature Functions */
int32_t I2C2_ISM330IS_GetTemperature(ISM330IS_Object_t *pObj, float *Temperature);
int32_t I2C2_ISM330IS_GetTemperatureRaw(ISM330IS_Object_t *pObj, int16_t *TempRaw);

/* Angle Calculation Functions */
int32_t I2C2_ISM330IS_GetAccelAngles(ISM330IS_Object_t *pObj, float *pitch, float *roll);
int32_t I2C2_ISM330IS_GetTiltAngle(ISM330IS_Object_t *pObj, float *tilt_angle);
int32_t I2C2_ISM330IS_AngleInit(ISM330IS_Object_t *pObj);
int32_t I2C2_ISM330IS_GetComplementaryAngles(ISM330IS_Object_t *pObj, float *pitch, float *roll, float *yaw);

/* Complete Sensor Data Function */
int32_t I2C2_ISM330IS_GetAllSensorData(ISM330IS_Object_t *pObj, ISM330IS_Axes_t *acceleration, 
                                       ISM330IS_Axes_t *angular_rate, float *temperature, 
                                       float *pitch, float *roll, float *yaw);

/* Test Demo Function */
void I2C2_ISM330IS_TestDemo(void);

#ifdef __cplusplus
}
#endif

#endif /* ISM330IS_API_H */
