/**
 * @file hmi_led_driver.c
 * @brief LED Driver implementation with individual LED functions
 */
#include "hmi_led_driver.h"
#include "driver_printf_config.h"
#include <stdio.h>

/* Conditional printf for LED module */
#if (ENABLE_DRV_PRINTF && ENABLE_DRV_PRINTF_LED_DRIVER)
    #define LED_DRIVER_PRINTF(...)    printf(__VA_ARGS__)
#else
    #define LED_DRIVER_PRINTF(...)    ((void)0)
#endif

/* Private types -------------------------------------------------------------*/
typedef struct {
    GPIO_TypeDef* gpio_port;
    uint16_t gpio_pin;
    GPIO_PinState active_state;
} LED_ConfigTypeDef;

/* Private variables ---------------------------------------------------------*/
static const LED_ConfigTypeDef led_config[LED_COUNT] = {
    // LED_STATUS
    {
        .gpio_port = LED_STATUS_GPIO_Port,
        .gpio_pin = LED_STATUS_Pin,
        .active_state = GPIO_PIN_SET
    },
    // LED_TRACKER_MOTOR_5130
    {
        .gpio_port = LED_TRACKER_MOTOR_5130_GPIO_Port,
        .gpio_pin = LED_TRACKER_MOTOR_5130_Pin,
        .active_state = GPIO_PIN_SET
    },
    // LED_TILT_MOTOR_5130
    {
        .gpio_port = LED_TILT_MOTOR_5130_GPIO_Port,
        .gpio_pin = LED_TILT_MOTOR_5130_Pin,
        .active_state = GPIO_PIN_SET
    },
    // LED_SPINNING_BLUE
    {
        .gpio_port = SPINNING_BLUE_LED_ENABLE_OUT_GPIO_Port,
        .gpio_pin = SPINNING_BLUE_LED_ENABLE_OUT_Pin,
        .active_state = GPIO_PIN_SET
    },
    // LED_HIGH_TEMP_RED
    {
        .gpio_port = HIGH_TEMP_RED_LED_ENABLE_OUT_GPIO_Port,
        .gpio_pin = HIGH_TEMP_RED_LED_ENABLE_OUT_Pin,
        .active_state = GPIO_PIN_SET
    }
};

/* Private function prototypes -----------------------------------------------*/
static void LED_WritePin(LED_TypeDef led, GPIO_PinState pin_state);
static GPIO_PinState LED_ReadPin(LED_TypeDef led);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief Initialize LED driver
 */
void LED_Init(void)
{
    LED_AllOff();
}

/*******************************************************************************
 *                        Generic LED Control Functions                        *
 *******************************************************************************/

void LED_SetHigh(LED_TypeDef led)
{
    if (led < LED_COUNT) {
        LED_WritePin(led, led_config[led].active_state);
    }
}

void LED_SetLow(LED_TypeDef led)
{
    if (led < LED_COUNT) {
        GPIO_PinState inactive_state = (led_config[led].active_state == GPIO_PIN_SET) ? 
                                       GPIO_PIN_RESET : GPIO_PIN_SET;
        LED_WritePin(led, inactive_state);
    }
}

void LED_Toggle(LED_TypeDef led)
{
    if (led < LED_COUNT) {
        HAL_GPIO_TogglePin(led_config[led].gpio_port, led_config[led].gpio_pin);
    }
}

void LED_SetState(LED_TypeDef led, LED_StateTypeDef state)
{
    if (state == LED_ON) {
        LED_SetHigh(led);
    } else {
        LED_SetLow(led);
    }
}

LED_StateTypeDef LED_GetState(LED_TypeDef led)
{
    if (led < LED_COUNT) {
        GPIO_PinState pin_state = LED_ReadPin(led);
        return (pin_state == led_config[led].active_state) ? LED_ON : LED_OFF;
    }
    return LED_OFF;
}

/*******************************************************************************
 *                        LED_STATUS Individual Functions                      *
 *******************************************************************************/

void LED_STATUS_SetHigh(void)
{
    HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, GPIO_PIN_SET);
}

void LED_STATUS_SetLow(void)
{
    HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, GPIO_PIN_RESET);
}

void LED_STATUS_Toggle(void)
{
    HAL_GPIO_TogglePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin);
}

void LED_STATUS_SetState(LED_StateTypeDef state)
{
    if (state == LED_ON) {
        LED_STATUS_SetHigh();
    } else {
        LED_STATUS_SetLow();
    }
}

LED_StateTypeDef LED_STATUS_GetState(void)
{
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(LED_STATUS_GPIO_Port, LED_STATUS_Pin);
    return (pin_state == GPIO_PIN_SET) ? LED_ON : LED_OFF;
}

/*******************************************************************************
 *                   LED_TRACKER_MOTOR_5130 Individual Functions              *
 *******************************************************************************/

void LED_TRACKER_MOTOR_5130_SetHigh(void)
{
    HAL_GPIO_WritePin(LED_TRACKER_MOTOR_5130_GPIO_Port, LED_TRACKER_MOTOR_5130_Pin, GPIO_PIN_SET);
}

void LED_TRACKER_MOTOR_5130_SetLow(void)
{
    HAL_GPIO_WritePin(LED_TRACKER_MOTOR_5130_GPIO_Port, LED_TRACKER_MOTOR_5130_Pin, GPIO_PIN_RESET);
}

void LED_TRACKER_MOTOR_5130_Toggle(void)
{
    HAL_GPIO_TogglePin(LED_TRACKER_MOTOR_5130_GPIO_Port, LED_TRACKER_MOTOR_5130_Pin);
}

void LED_TRACKER_MOTOR_5130_SetState(LED_StateTypeDef state)
{
    if (state == LED_ON) {
        LED_TRACKER_MOTOR_5130_SetHigh();
    } else {
        LED_TRACKER_MOTOR_5130_SetLow();
    }
}

LED_StateTypeDef LED_TRACKER_MOTOR_5130_GetState(void)
{
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(LED_TRACKER_MOTOR_5130_GPIO_Port, LED_TRACKER_MOTOR_5130_Pin);
    return (pin_state == GPIO_PIN_SET) ? LED_ON : LED_OFF;
}

/*******************************************************************************
 *                     LED_TILT_MOTOR_5130 Individual Functions               *
 *******************************************************************************/

void LED_TILT_MOTOR_5130_SetHigh(void)
{
    HAL_GPIO_WritePin(LED_TILT_MOTOR_5130_GPIO_Port, LED_TILT_MOTOR_5130_Pin, GPIO_PIN_SET);
}

void LED_TILT_MOTOR_5130_SetLow(void)
{
    HAL_GPIO_WritePin(LED_TILT_MOTOR_5130_GPIO_Port, LED_TILT_MOTOR_5130_Pin, GPIO_PIN_RESET);
}

void LED_TILT_MOTOR_5130_Toggle(void)
{
    HAL_GPIO_TogglePin(LED_TILT_MOTOR_5130_GPIO_Port, LED_TILT_MOTOR_5130_Pin);
}

void LED_TILT_MOTOR_5130_SetState(LED_StateTypeDef state)
{
    if (state == LED_ON) {
        LED_TILT_MOTOR_5130_SetHigh();
    } else {
        LED_TILT_MOTOR_5130_SetLow();
    }
}

LED_StateTypeDef LED_TILT_MOTOR_5130_GetState(void)
{
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(LED_TILT_MOTOR_5130_GPIO_Port, LED_TILT_MOTOR_5130_Pin);
    return (pin_state == GPIO_PIN_SET) ? LED_ON : LED_OFF;
}

/*******************************************************************************
 *                     LED_SPINNING_BLUE Individual Functions                 *
 *******************************************************************************/

void LED_SPINNING_BLUE_SetHigh(void)
{
    HAL_GPIO_WritePin(SPINNING_BLUE_LED_ENABLE_OUT_GPIO_Port, SPINNING_BLUE_LED_ENABLE_OUT_Pin, GPIO_PIN_SET);
}

void LED_SPINNING_BLUE_SetLow(void)
{
    HAL_GPIO_WritePin(SPINNING_BLUE_LED_ENABLE_OUT_GPIO_Port, SPINNING_BLUE_LED_ENABLE_OUT_Pin, GPIO_PIN_RESET);
}

void LED_SPINNING_BLUE_Toggle(void)
{
    HAL_GPIO_TogglePin(SPINNING_BLUE_LED_ENABLE_OUT_GPIO_Port, SPINNING_BLUE_LED_ENABLE_OUT_Pin);
}

void LED_SPINNING_BLUE_SetState(LED_StateTypeDef state)
{
    if (state == LED_ON) {
        LED_SPINNING_BLUE_SetHigh();
    } else {
        LED_SPINNING_BLUE_SetLow();
    }
}

LED_StateTypeDef LED_SPINNING_BLUE_GetState(void)
{
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(SPINNING_BLUE_LED_ENABLE_OUT_GPIO_Port, SPINNING_BLUE_LED_ENABLE_OUT_Pin);
    return (pin_state == GPIO_PIN_SET) ? LED_ON : LED_OFF;
}

/*******************************************************************************
 *                     LED_HIGH_TEMP_RED Individual Functions                 *
 *******************************************************************************/

void LED_HIGH_TEMP_RED_SetHigh(void)
{
    HAL_GPIO_WritePin(HIGH_TEMP_RED_LED_ENABLE_OUT_GPIO_Port, HIGH_TEMP_RED_LED_ENABLE_OUT_Pin, GPIO_PIN_SET);
}

void LED_HIGH_TEMP_RED_SetLow(void)
{
    HAL_GPIO_WritePin(HIGH_TEMP_RED_LED_ENABLE_OUT_GPIO_Port, HIGH_TEMP_RED_LED_ENABLE_OUT_Pin, GPIO_PIN_RESET);
}

void LED_HIGH_TEMP_RED_Toggle(void)
{
    HAL_GPIO_TogglePin(HIGH_TEMP_RED_LED_ENABLE_OUT_GPIO_Port, HIGH_TEMP_RED_LED_ENABLE_OUT_Pin);
}

void LED_HIGH_TEMP_RED_SetState(LED_StateTypeDef state)
{
    if (state == LED_ON) {
        LED_HIGH_TEMP_RED_SetHigh();
    } else {
        LED_HIGH_TEMP_RED_SetLow();
    }
}

LED_StateTypeDef LED_HIGH_TEMP_RED_GetState(void)
{
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(HIGH_TEMP_RED_LED_ENABLE_OUT_GPIO_Port, HIGH_TEMP_RED_LED_ENABLE_OUT_Pin);
    return (pin_state == GPIO_PIN_SET) ? LED_ON : LED_OFF;
}

/*******************************************************************************
 *                           Bulk Operations                                   *
 *******************************************************************************/

void LED_AllOn(void)
{
    LED_STATUS_SetHigh();
    LED_TRACKER_MOTOR_5130_SetHigh();
    LED_TILT_MOTOR_5130_SetHigh();
    LED_SPINNING_BLUE_SetHigh();
    LED_HIGH_TEMP_RED_SetHigh();
}

void LED_AllOff(void)
{
    LED_STATUS_SetLow();
    LED_TRACKER_MOTOR_5130_SetLow();
    LED_TILT_MOTOR_5130_SetLow();
    LED_SPINNING_BLUE_SetLow();
    LED_HIGH_TEMP_RED_SetLow();
}

void LED_AllToggle(void)
{
    LED_STATUS_Toggle();
    LED_TRACKER_MOTOR_5130_Toggle();
    LED_TILT_MOTOR_5130_Toggle();
    LED_SPINNING_BLUE_Toggle();
    LED_HIGH_TEMP_RED_Toggle();
}

/*******************************************************************************
 *                           Pattern Functions                                 *
 *******************************************************************************/

void LED_BlinkPattern(LED_TypeDef led, uint32_t on_time_ms, uint32_t off_time_ms, uint8_t cycles)
{
    uint8_t count = 0;
    
    while ((cycles == 0) || (count < cycles)) {
        LED_SetHigh(led);
        HAL_Delay(on_time_ms);
        LED_SetLow(led);
        HAL_Delay(off_time_ms);
        
        if (cycles > 0) {
            count++;
        }
    }
}

void LED_RunningLights(uint32_t delay_ms)
{
    LED_AllOff();
    
    LED_STATUS_SetHigh();
    HAL_Delay(delay_ms);
    LED_STATUS_SetLow();
    
    LED_TRACKER_MOTOR_5130_SetHigh();
    HAL_Delay(delay_ms);
    LED_TRACKER_MOTOR_5130_SetLow();
    
    LED_TILT_MOTOR_5130_SetHigh();
    HAL_Delay(delay_ms);
    LED_TILT_MOTOR_5130_SetLow();
    
    LED_SPINNING_BLUE_SetHigh();
    HAL_Delay(delay_ms);
    LED_SPINNING_BLUE_SetLow();
    
    LED_HIGH_TEMP_RED_SetHigh();
    HAL_Delay(delay_ms);
    LED_HIGH_TEMP_RED_SetLow();
}

/* Private functions ---------------------------------------------------------*/

static void LED_WritePin(LED_TypeDef led, GPIO_PinState pin_state)
{
    HAL_GPIO_WritePin(led_config[led].gpio_port, led_config[led].gpio_pin, pin_state);
}

static GPIO_PinState LED_ReadPin(LED_TypeDef led)
{
    return HAL_GPIO_ReadPin(led_config[led].gpio_port, led_config[led].gpio_pin);
}
