/**
  ******************************************************************************
  * @file    debug_printf_config.h
  * @brief   Centralized debug printf enable/disable switches for all modules
  * @date    December 12, 2025
  ******************************************************************************
  * @attention
  *
  * This file contains compile-time switches to enable or disable debug printf
  * output for each module independently.
  * 
  * Usage:
  *   - Set to 1u to enable debug printf for that module
  *   - Set to 0u to disable (default) - zero runtime cost when disabled
  *   - Disabled modules will not evaluate printf arguments (fully compiled out)
  *
  ******************************************************************************
  */

#ifndef DEBUG_PRINTF_CONFIG_H
#define DEBUG_PRINTF_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
   Per-Module Debug Printf Enable Switches
   Default: 0u (disabled) - set to 1u to enable debug output for that module
   ============================================================================= */

/* Global driver printf master switch */
#define ENABLE_DRV_PRINTF                          1u

/* Application Module */
#define ENABLE_DRV_PRINTF_HMI_APP_INIT             0u

/* Button and Display Modules */
#define ENABLE_DRV_PRINTF_BUTTON3116               0u
#define ENABLE_DRV_PRINTF_BUTTON_DISPLAY_WRAPPER   0u
#define ENABLE_DRV_PRINTF_DISPLAY72128             0u

/* EEPROM Module */
#define ENABLE_DRV_PRINTF_EEPROM24C64              0u

/* FreeRTOS Example */
#define ENABLE_DRV_PRINTF_FREERTOS_CLEAN_EXAMPLE   0u

/* ISM330IS Sensor Modules */
#define ENABLE_DRV_PRINTF_ISM330IS_API             0u
#define ENABLE_DRV_PRINTF_ISM330IS_DRIVER          0u
#define ENABLE_DRV_PRINTF_ISM330IS_I2C2_EXAMPLE    0u
#define ENABLE_DRV_PRINTF_ISM330IS_REG             0u

/* LED Module */
#define ENABLE_DRV_PRINTF_LED_DRIVER               0u

/* MLX90614 Temperature Sensor Modules */
#define ENABLE_DRV_PRINTF_MLX90614_API             0u
#define ENABLE_DRV_PRINTF_MLX90614_WRAPPER         0u

/* MLX90640 Thermal Camera Modules */
#define ENABLE_DRV_PRINTF_MLX90640_DRIVER          0u
#define ENABLE_DRV_PRINTF_MLX90640_WRAPPER         0u

/* =============================================================================
   Transport Configuration (Optional - for future use)
   ============================================================================= */
/* Uncomment the desired transport method */
/* #define DEBUG_PRINTF_USE_UART      1u */
/* #define DEBUG_PRINTF_USE_RTT       1u */
/* #define DEBUG_PRINTF_USE_SWO       1u */

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_PRINTF_CONFIG_H */
