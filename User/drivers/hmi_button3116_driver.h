/**
 * @file button3116_driver.h
 * @brief CY8CMBR3116 Capacitive Touch Controller Driver Header
 * @author Your Name
 * @date December 7, 2025
 */

#ifndef __BUTTON3116_DRIVER_H
#define __BUTTON3116_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief CY8CMBR3116 Error Codes
 */
typedef enum {
    CY8CMBR3116_OK       = 0x00,
    CY8CMBR3116_ERROR    = 0x01,
    CY8CMBR3116_BUSY     = 0x02,
    CY8CMBR3116_TIMEOUT  = 0x03
} CY8CMBR3116_Status;

/**
 * @brief Touch button state
 */
typedef enum {
    TOUCH_RELEASED = 0,
    TOUCH_PRESSED  = 1
} CY8CMBR3116_TouchState;

/**
 * @brief CY8CMBR3116 Device Handle Structure
 */
typedef struct {
    I2C_HandleTypeDef *hi2c;        // I2C handle
    uint8_t device_address;          // I2C device address (7-bit)
    uint16_t button_status;          // Current button status (16 bits for 16 channels)
    uint16_t button_status_prev;     // Previous button status
    uint8_t proximity_status;        // Proximity detection status
} CY8CMBR3116_Handle;

/* Exported constants --------------------------------------------------------*/

// I2C Address (7-bit format)
#define CY8CMBR3116_I2C_ADDR_DEFAULT    0x37    // Default I2C address
#define CY8CMBR3116_I2C_ADDR_ALT        0x08    // Alternative I2C address

// Register Map
#define CY8CMBR3116_REG_SENSOR_EN       0x00    // Sensor enable (2 bytes)
#define CY8CMBR3116_REG_FSS_EN          0x02    // Fast Scan Sensor Enable
#define CY8CMBR3116_REG_TOGGLE_EN       0x04    // Toggle Enable
#define CY8CMBR3116_REG_LED_ON_EN       0x06    // LED ON Enable
#define CY8CMBR3116_REG_SENSITIVITY     0x08    // Sensitivity (3 bytes)
#define CY8CMBR3116_REG_BASE_THRESHOLD  0x0B    // Base Threshold
#define CY8CMBR3116_REG_FINGER_THRESHOLD 0x0C   // Finger Threshold (16 bytes)
#define CY8CMBR3116_REG_SENSOR_DEBOUNCE 0x1C    // Sensor Debounce
#define CY8CMBR3116_REG_BUTTON_HYS      0x1D    // Button Hysteresis
#define CY8CMBR3116_REG_BUTTON_LBR      0x1F    // Button Low Baseline Reset
#define CY8CMBR3116_REG_BUTTON_NNT      0x20    // Button Negative Noise Threshold
#define CY8CMBR3116_REG_BUTTON_PNT      0x21    // Button Positive Noise Threshold
#define CY8CMBR3116_REG_PROX_EN         0x26    // Proximity Enable
#define CY8CMBR3116_REG_PROX_CFG        0x27    // Proximity Configuration
#define CY8CMBR3116_REG_PROX_CFG2       0x28    // Proximity Configuration 2
#define CY8CMBR3116_REG_PROX_TOUCH_TH0  0x2A    // Proximity Touch Threshold 0
#define CY8CMBR3116_REG_PROX_TOUCH_TH1  0x2C    // Proximity Touch Threshold 1
#define CY8CMBR3116_REG_PROX_RESOLUTION 0x2E    // Proximity Resolution
#define CY8CMBR3116_REG_CONFIG_CRC      0x7E    // Configuration CRC

// Command Register
#define CY8CMBR3116_REG_CTRL_CMD        0x86    // Control Command
#define CY8CMBR3116_REG_CTRL_CMD_STATUS 0x88    // Control Command Status
#define CY8CMBR3116_REG_CTRL_CMD_ERR    0x89    // Control Command Error

// Status Registers
#define CY8CMBR3116_REG_SYSTEM_STATUS   0x8A    // System Status
#define CY8CMBR3116_REG_BUTTON_STAT     0xAA    // Button Status (2 bytes)
#define CY8CMBR3116_REG_LATCHED_BUTTON  0xAC    // Latched Button Status (2 bytes)
#define CY8CMBR3116_REG_PROX_STAT       0xAE    // Proximity Status
#define CY8CMBR3116_REG_TOTAL_WORKING_SNS 0xB6  // Total Working Sensors

// Device ID Registers
#define CY8CMBR3116_REG_FAMILY_ID       0x8F    // Family ID
#define CY8CMBR3116_REG_DEVICE_ID       0x90    // Device ID (2 bytes)
#define CY8CMBR3116_REG_DEVICE_REV      0x92    // Device Revision

// Commands
#define CY8CMBR3116_CMD_SAVE_CHECK_CRC  0x02    // Save configuration and check CRC
#define CY8CMBR3116_CMD_CALC_CRC        0x03    // Calculate CRC
#define CY8CMBR3116_CMD_LOAD_FACTORY    0x04    // Load factory configuration
#define CY8CMBR3116_CMD_SLEEP           0x05    // Enter sleep mode
#define CY8CMBR3116_CMD_CLEAR_LATCHED   0x06    // Clear latched buttons
#define CY8CMBR3116_CMD_RESET_BASELINE  0x07    // Reset baseline
#define CY8CMBR3116_CMD_SW_RESET        0xFF    // Software reset

// System Status Bits
#define CY8CMBR3116_SYS_STAT_ACTIVE     0x01    // Device is active
#define CY8CMBR3116_SYS_STAT_WDT        0x04    // Watchdog reset occurred
#define CY8CMBR3116_SYS_STAT_CONFIG_ERR 0x80    // Configuration error

// Expected Device IDs (Cypress CapSense Family)
#define CY8CMBR3116_FAMILY_ID           0x9A    // Family ID (Common for all CY8CMBR31xx)

// Supported Device IDs
#define CY8CMBR3116_DEVICE_ID_H         0x03    // CY8CMBR3116 Device ID high byte
#define CY8CMBR3116_DEVICE_ID_L         0x16    // CY8CMBR3116 Device ID low byte
#define CY8CMBR3108_DEVICE_ID_H         0x05    // CY8CMBR3108 Device ID high byte  
#define CY8CMBR3108_DEVICE_ID_L         0x0A    // CY8CMBR3108 Device ID low byte
#define CY8CMBR3110_DEVICE_ID_H         0x05    // CY8CMBR3110 Device ID high byte
#define CY8CMBR3110_DEVICE_ID_L         0x0A    // CY8CMBR3110 Device ID low byte

// Timeout
#define CY8CMBR3116_I2C_TIMEOUT         500     // 500ms timeout
#define CY8CMBR3116_CMD_TIMEOUT         100     // 100ms command timeout

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize CY8CMBR3116 device
 * @param handle: Pointer to CY8CMBR3116 handle structure
 * @param hi2c: Pointer to I2C handle
 * @param device_addr: I2C device address (7-bit)
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_Init(CY8CMBR3116_Handle *handle, I2C_HandleTypeDef *hi2c, uint8_t device_addr);

/**
 * @brief Check if device is present and read device ID
 * @param handle: Pointer to CY8CMBR3116 handle
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_CheckDevice(CY8CMBR3116_Handle *handle);

/**
 * @brief Software reset the device
 * @param handle: Pointer to CY8CMBR3116 handle
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_SoftwareReset(CY8CMBR3116_Handle *handle);

/**
 * @brief Read button status (16 buttons)
 * @param handle: Pointer to CY8CMBR3116 handle
 * @param status: Pointer to store button status (16 bits)
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_ReadButtonStatus(CY8CMBR3116_Handle *handle, uint16_t *status);

/**
 * @brief Check if specific button is pressed
 * @param handle: Pointer to CY8CMBR3116 handle
 * @param button: Button number (0-15)
 * @retval CY8CMBR3116_TouchState
 */
CY8CMBR3116_TouchState CY8CMBR3116_IsButtonPressed(CY8CMBR3116_Handle *handle, uint8_t button);

/**
 * @brief Check if any button state changed
 * @param handle: Pointer to CY8CMBR3116 handle
 * @retval true if any button state changed
 */
bool CY8CMBR3116_ButtonChanged(CY8CMBR3116_Handle *handle);

/**
 * @brief Get which button was just pressed (edge detection)
 * @param handle: Pointer to CY8CMBR3116 handle
 * @param button: Pointer to store button number (0-15, 0xFF if none)
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_GetPressedButton(CY8CMBR3116_Handle *handle, uint8_t *button);

/**
 * @brief Get which button was just released (edge detection)
 * @param handle: Pointer to CY8CMBR3116 handle
 * @param button: Pointer to store button number (0-15, 0xFF if none)
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_GetReleasedButton(CY8CMBR3116_Handle *handle, uint8_t *button);

/**
 * @brief Read proximity status
 * @param handle: Pointer to CY8CMBR3116 handle
 * @param proximity: Pointer to store proximity status
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_ReadProximityStatus(CY8CMBR3116_Handle *handle, uint8_t *proximity);

/**
 * @brief Enable/disable specific sensors
 * @param handle: Pointer to CY8CMBR3116 handle
 * @param sensor_mask: Sensor enable mask (16 bits, 1=enable, 0=disable)
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_SetSensorEnable(CY8CMBR3116_Handle *handle, uint16_t sensor_mask);

/**
 * @brief Set sensitivity for all sensors
 * @param handle: Pointer to CY8CMBR3116 handle
 * @param sensitivity: Sensitivity value (0-3, higher = more sensitive)
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_SetSensitivity(CY8CMBR3116_Handle *handle, uint8_t sensitivity);

/**
 * @brief Clear latched button status
 * @param handle: Pointer to CY8CMBR3116 handle
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_ClearLatchedButtons(CY8CMBR3116_Handle *handle);

/**
 * @brief Reset sensor baseline
 * @param handle: Pointer to CY8CMBR3116 handle
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_ResetBaseline(CY8CMBR3116_Handle *handle);

/**
 * @brief Read system status register
 * @param handle: Pointer to CY8CMBR3116 handle
 * @param status: Pointer to store system status
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_ReadSystemStatus(CY8CMBR3116_Handle *handle, uint8_t *status);

/**
 * @brief Send command to device
 * @param handle: Pointer to CY8CMBR3116 handle
 * @param command: Command byte
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_SendCommand(CY8CMBR3116_Handle *handle, uint8_t command);

/**
 * @brief Write data to CY8CMBR3116 register
 * @param handle: Pointer to CY8CMBR3116 handle
 * @param reg_addr: Register address
 * @param data: Pointer to data to write
 * @param length: Number of bytes to write
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_WriteRegister(CY8CMBR3116_Handle *handle, uint8_t reg_addr, uint8_t *data, uint16_t length);

/**
 * @brief Read data from CY8CMBR3116 register
 * @param handle: Pointer to CY8CMBR3116 handle
 * @param reg_addr: Register address
 * @param data: Pointer to store read data
 * @param length: Number of bytes to read
 * @retval CY8CMBR3116_Status
 */
CY8CMBR3116_Status CY8CMBR3116_ReadRegister(CY8CMBR3116_Handle *handle, uint8_t reg_addr, uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* __BUTTON3116_DRIVER_H */
