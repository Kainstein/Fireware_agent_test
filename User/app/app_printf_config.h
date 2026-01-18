/**
 ******************************************************************************
 * @file    app_printf_config.h
 * @brief   Application Printf and Debug Configuration
 * @details Centralized configuration for debug output, logging levels,
 *          and printf redirection settings
 * @date    January 9, 2026
 ******************************************************************************
 */

#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "main.h"

/* ============================================================================
   PRINTF CONFIGURATION
   ============================================================================ */

/** @defgroup Printf_Enable_Switches Module-specific Printf Enable Switches
 * @brief Enable/disable printf output for individual modules
 * @{
 */
#define ENABLE_APP_PRINTF                   1               // Global printf master switch

/* Module-specific printf switches (set 0 to disable, 1 to enable) */
#define ENABLE_APP_PRINTF_UART_PROTOCOL     0               // uart_protocol.c debug output
#define ENABLE_APP_PRINTF_HMI               1               // HMI modules debug output
#define ENABLE_APP_PRINTF_TASK              1               // FreeRTOS task debug output

/**
 * @}
 */

/* ============================================================================
   DEBUG LOGGING LEVELS
   ============================================================================ */

/** @defgroup Debug_Levels Debug Logging Levels
 * @{
 */

/**
 * @brief Debug Level Configuration
 * 0 = NONE    - No debug output
 * 1 = ERROR   - Errors only
 * 2 = WARNING - Errors + Warnings
 * 3 = INFO    - Errors + Warnings + Info
 * 4 = DEBUG   - All messages including detailed debug
 */
#define DEBUG_LEVEL                 3               // Default: INFO level

#define DEBUG_LEVEL_NONE            0
#define DEBUG_LEVEL_ERROR           1
#define DEBUG_LEVEL_WARNING         2
#define DEBUG_LEVEL_INFO            3
#define DEBUG_LEVEL_DEBUG           4



#endif /* __APP_PRINTF_CONFIG_H */
