/**
 ******************************************************************************
 * @file    app.h
 * @brief   High-Level Application Configuration Header
 * @details Central configuration file for application-level settings,
 *          macro definitions, constants, and parameters
 * @date    January 4, 2026
 * @note    Duplicate definitions removed - refer to hmi_handler.h and main.h
 *          for operational configuration values
 ******************************************************************************
 */

#ifndef __APP_H
#define __APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "hmi_handler.h"
#include "hal_callbacks.h"
#include "uart_protocol.h"
#include "app_printf_config.h"

/* ============================================================================
   APPLICATION VERSION
   ============================================================================ */
#define APP_VERSION                 2.34
#define APP_BUILD_DATE              "2026-01-04"
#define APP_NAME                    "HPHT_H743ZIT6_FW"

/* ============================================================================
   SYSTEM CONFIGURATION
   ============================================================================ */
#define SYSTEM_CORE_CLOCK_HZ        480000000UL     // 480 MHz
#define SYSTEM_TICK_FREQ_HZ         1000            // 1ms tick (1 kHz)

/* ============================================================================
   TEMPERATURE CONTROL CONFIGURATION
   ============================================================================ */
// Temperature setpoint parameters (ranges defined in hmi_handler.h)
#define TEMP_DEFAULT                25.0f           // Default setpoint
#define TEMP_STEP_SMALL             0.5f            // Small adjustment step
#define TEMP_STEP_LARGE             5.0f            // Large adjustment step

// Temperature sensor selection
#define TEMP_SENSOR_MLX90614        0               // Use MLX90614 (single point)
#define TEMP_SENSOR_MLX90640        1               // Use MLX90640 (thermal array)
#define TEMP_SENSOR_SELECT          TEMP_SENSOR_MLX90640

// Temperature measurement mode (for MLX90640)
#define TEMP_MEASURE_MIN            0               // Use minimum temperature
#define TEMP_MEASURE_MAX            1               // Use maximum temperature
#define TEMP_MEASURE_AVG            2               // Use average temperature
#define TEMP_MEASURE_CENTER         3               // Use center pixel
#define TEMP_MEASURE_MODE           TEMP_MEASURE_AVG

/* ============================================================================
   BLDC MOTOR CONTROL CONFIGURATION
   ============================================================================ */
// BLDC setpoint parameters (ranges defined in hmi_handler.h)
#define BLDC_DEFAULT                5000            // Default setpoint
#define BLDC_STEP_SMALL             10              // Small adjustment step
#define BLDC_STEP_LARGE             100             // Large adjustment step

// BLDC control modes
#define BLDC_MODE_OFF               0               // Motor OFF
#define BLDC_MODE_SPEED             1               // Speed control mode
#define BLDC_MODE_TORQUE            2               // Torque control mode (future)
#define BLDC_MODE_DEFAULT           BLDC_MODE_SPEED

/* ============================================================================
   DISPLAY CONFIGURATION
   ============================================================================ */
// Display controller (ZLG72128) - operational values in hmi_handler.h
#define DISPLAY_I2C_ADDRESS_GROUP1  0x60            // Group1 address (5-digit)
#define DISPLAY_I2C_ADDRESS_GROUP2  0x61            // Group2 address (4-digit)
#define DISPLAY_I2C_TIMEOUT_MS      100             // I2C timeout

// Display groups
#define DISPLAY_GROUP1_DIGITS       5               // BLDC display (5 digits)
#define DISPLAY_GROUP2_DIGITS       4               // Temperature display (4 digits)

/* ============================================================================
   BUTTON CONFIGURATION
   ============================================================================ */
// Button controller (CY8CMBR3116)
#define BUTTON_I2C_ADDRESS          0x37            // Button controller address
#define BUTTON_COUNT                10              // Total number of buttons

// Button timing parameters
#define BUTTON_DEBOUNCE_TIME_MS     30              // Debounce period
#define BUTTON_LONG_PRESS_TIME_MS   1000            // Long press threshold
#define BUTTON_DOUBLE_CLICK_TIME_MS 400             // Double-click window
#define BUTTON_POLL_PERIOD_MS       30              // Polling interval

// Continuous adjustment (when button held)
#define BUTTON_CONT_ADJ_DELAY_MS    500             // Initial delay before continuous
#define BUTTON_CONT_ADJ_PERIOD_MS   200             // Continuous adjustment rate

/* ============================================================================
   SENSOR CONFIGURATION
   ============================================================================ */
// MLX90614 - Non-contact IR Temperature Sensor
#define MLX90614_I2C_ADDRESS        0x5A            // Default I2C address
#define MLX90614_SAMPLE_PERIOD_MS   1000            // Sampling period

// MLX90640 - Thermal Imaging Array (32x24 pixels = 768 points)
#define MLX90640_I2C_ADDRESS        0x33            // Default I2C address
#define MLX90640_SAMPLE_PERIOD_MS   1000            // Sampling period
#define MLX90640_PIXEL_COUNT        768             // 32x24 pixels
#define MLX90640_REFRESH_RATE       16              // 16 Hz refresh rate
#define MLX90640_EMISSIVITY         0.95f           // Default emissivity
#define MLX90640_TA_SHIFT           8.0f            // Temperature ambient shift

// ISM330IS - 6-axis IMU (Accelerometer + Gyroscope)
#define ISM330IS_I2C_ADDRESS        0x6A            // Default I2C address
#define ISM330IS_SAMPLE_PERIOD_MS   100             // Sampling period (future use)

/* ============================================================================
   FREERTOS TASK CONFIGURATION
   ============================================================================ */
// Task stack sizes (in words, 1 word = 4 bytes on STM32H7)
#define TASK_STACK_DEFAULT          512             // 2KB
#define TASK_STACK_HMI              4096            // 16KB (Task03)
#define TASK_STACK_SENSOR           4096            // 16KB (Task02)
#define TASK_STACK_NORMAL           512             // 2KB

// Task priorities (osPriority_t values)
#define TASK_PRIORITY_REALTIME2     48              // Highest (HMI)
#define TASK_PRIORITY_REALTIME1     40              // High (Sensors)
#define TASK_PRIORITY_NORMAL2       24              // Normal
#define TASK_PRIORITY_NORMAL1       16              // Normal
#define TASK_PRIORITY_NORMAL        8               // Normal
#define TASK_PRIORITY_LOW           0               // Lowest

// Task loop delays
#define TASK_HMI_LOOP_DELAY_MS      30              // Task03: 33Hz (button/display)
#define TASK_SENSOR_LOOP_DELAY_MS   1000            // Task02: 1Hz (sensor sampling)
#define TASK_LED_LOOP_DELAY_MS      1000            // defaultTask: 1Hz heartbeat

/* ============================================================================
   EEPROM CONFIGURATION (M24C64)
   ============================================================================ */
#define EEPROM_I2C_ADDRESS          0x50            // EEPROM base address
#define EEPROM_SIZE_BYTES           8192            // 64Kbit = 8KB
#define EEPROM_PAGE_SIZE            32              // 32-byte page writes

// EEPROM memory map (address offsets)
#define EEPROM_ADDR_TEMP_SETPOINT   0x0000          // Temperature setpoint (4 bytes)
#define EEPROM_ADDR_BLDC_SETPOINT   0x0004          // BLDC setpoint (4 bytes)
#define EEPROM_ADDR_CALIBRATION     0x0100          // Calibration data (256 bytes)
#define EEPROM_ADDR_USER_DATA       0x0200          // User data area

/* ============================================================================
   TIMING PARAMETERS
   ============================================================================ */
#define TIMEOUT_I2C_MS              100             // I2C operation timeout
#define TIMEOUT_UART_MS             1000            // UART operation timeout
#define WATCHDOG_TIMEOUT_MS         5000            // Watchdog timeout (future)

/* ============================================================================
   ERROR CODES
   ============================================================================ */
typedef enum {
    APP_OK                      = 0x00,             // Success
    APP_ERROR                   = 0x01,             // Generic error
    APP_ERROR_INIT              = 0x02,             // Initialization error
    APP_ERROR_I2C               = 0x03,             // I2C communication error
    APP_ERROR_TIMEOUT           = 0x04,             // Timeout error
    APP_ERROR_PARAM             = 0x05,             // Invalid parameter
    APP_ERROR_SENSOR            = 0x06,             // Sensor error
    APP_ERROR_DISPLAY           = 0x07,             // Display error
    APP_ERROR_BUTTON            = 0x08,             // Button error
    APP_ERROR_EEPROM            = 0x09,             // EEPROM error
    APP_ERROR_OVERFLOW          = 0x0A,             // Buffer overflow
} AppError_t;

/* ============================================================================
   FEATURE FLAGS
   ============================================================================ */
#define FEATURE_DEBUG_PRINTF        1               // Enable debug printf
#define FEATURE_EEPROM_SAVE         0               // Enable EEPROM save (future)
#define FEATURE_IMU_MONITORING      0               // Enable IMU monitoring (future)
#define FEATURE_PID_CONTROL         0               // Enable PID control (future)
#define FEATURE_WATCHDOG            0               // Enable watchdog (future)

/* ============================================================================
   DEBUG CONFIGURATION
   ============================================================================ */
#if FEATURE_DEBUG_PRINTF
    #define DEBUG_UART              &huart1         // Debug UART handle
    #define DEBUG_BAUDRATE          115200          // Debug UART baud rate
    #define APP_DEBUG(fmt, ...)     printf("[APP] " fmt "\r\n", ##__VA_ARGS__)
    #define APP_INFO(fmt, ...)      printf("[INFO] " fmt "\r\n", ##__VA_ARGS__)
    #define APP_WARN(fmt, ...)      printf("[WARN] " fmt "\r\n", ##__VA_ARGS__)
    #define APP_ERROR_MSG(fmt, ...) printf("[ERROR] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define APP_DEBUG(fmt, ...)     ((void)0)
    #define APP_INFO(fmt, ...)      ((void)0)
    #define APP_WARN(fmt, ...)      ((void)0)
    #define APP_ERROR_MSG(fmt, ...) ((void)0)
#endif

/* ============================================================================
   EXPORTED FUNCTIONS (Declarations only - implementations needed)
   ============================================================================ */
/**
 * @brief Get application version string
 * @return Version string (e.g., "v0.10.0")
 */
const char* App_GetVersionString(void);

/**
 * @brief Get application build date
 * @return Build date string
 */
const char* App_GetBuildDate(void);

/**
 * @brief Convert error code to string
 * @param error Error code
 * @return Error description string
 */
const char* App_GetErrorString(AppError_t error);

#ifdef __cplusplus
}
#endif

#endif /* __APP_H */
