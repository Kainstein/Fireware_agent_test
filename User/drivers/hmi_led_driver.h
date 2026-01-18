/**
 * @file hmi_led_driver.h
 * @brief LED Driver for STM32H743 project with individual LED functions
 * @author Generated based on gpio.c analysis
 */

#ifndef HMI_LED_DRIVER_H
#define LED_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
    LED_STATUS = 0,                    // LED_STATUS_Pin on GPIOG
    LED_TRACKER_MOTOR_5130,           // LED_TRACKER_MOTOR_5130_Pin on GPIOG
    LED_TILT_MOTOR_5130,              // LED_TILT_MOTOR_5130_Pin on GPIOG
    LED_SPINNING_BLUE,                // SPINNING_BLUE_LED_ENABLE_OUT_Pin on GPIOB
    LED_HIGH_TEMP_RED,                // HIGH_TEMP_RED_LED_ENABLE_OUT_Pin on GPIOA
    LED_COUNT                         // Total number of LEDs
} LED_TypeDef;

typedef enum {
    LED_OFF = 0,                      // LED turned off (GPIO_PIN_RESET)
    LED_ON = 1                        // LED turned on (GPIO_PIN_SET)
} LED_StateTypeDef;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/

/* LED Initialization */
void LED_Init(void);

/* Generic LED Control Functions */
void LED_SetHigh(LED_TypeDef led);
void LED_SetLow(LED_TypeDef led);
void LED_Toggle(LED_TypeDef led);
void LED_SetState(LED_TypeDef led, LED_StateTypeDef state);
LED_StateTypeDef LED_GetState(LED_TypeDef led);

/* LED_STATUS Individual Functions */
void LED_STATUS_SetHigh(void);
void LED_STATUS_SetLow(void);
void LED_STATUS_Toggle(void);
void LED_STATUS_SetState(LED_StateTypeDef state);
LED_StateTypeDef LED_STATUS_GetState(void);

/* LED_TRACKER_MOTOR_5130 Individual Functions */
void LED_TRACKER_MOTOR_5130_SetHigh(void);
void LED_TRACKER_MOTOR_5130_SetLow(void);
void LED_TRACKER_MOTOR_5130_Toggle(void);
void LED_TRACKER_MOTOR_5130_SetState(LED_StateTypeDef state);
LED_StateTypeDef LED_TRACKER_MOTOR_5130_GetState(void);

/* LED_TILT_MOTOR_5130 Individual Functions */
void LED_TILT_MOTOR_5130_SetHigh(void);
void LED_TILT_MOTOR_5130_SetLow(void);
void LED_TILT_MOTOR_5130_Toggle(void);
void LED_TILT_MOTOR_5130_SetState(LED_StateTypeDef state);
LED_StateTypeDef LED_TILT_MOTOR_5130_GetState(void);

/* LED_SPINNING_BLUE Individual Functions */
void LED_SPINNING_BLUE_SetHigh(void);
void LED_SPINNING_BLUE_SetLow(void);
void LED_SPINNING_BLUE_Toggle(void);
void LED_SPINNING_BLUE_SetState(LED_StateTypeDef state);
LED_StateTypeDef LED_SPINNING_BLUE_GetState(void);

/* LED_HIGH_TEMP_RED Individual Functions */
void LED_HIGH_TEMP_RED_SetHigh(void);
void LED_HIGH_TEMP_RED_SetLow(void);
void LED_HIGH_TEMP_RED_Toggle(void);
void LED_HIGH_TEMP_RED_SetState(LED_StateTypeDef state);
LED_StateTypeDef LED_HIGH_TEMP_RED_GetState(void);

/* Bulk Operations */
void LED_AllOn(void);
void LED_AllOff(void);
void LED_AllToggle(void);

/* Pattern Functions */
void LED_BlinkPattern(LED_TypeDef led, uint32_t on_time_ms, uint32_t off_time_ms, uint8_t cycles);
void LED_RunningLights(uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif /* LED_DRIVER_H */
