/**
 * @file display72128_driver.h
 * @brief ZLG72128 LED Display and Keyboard Driver Header
 * @author Your Name
 * @date December 6, 2025
 */

#ifndef __DISPLAY72128_DRIVER_H
#define __DISPLAY72128_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief ZLG72128 Error Codes
 */
typedef enum {
    ZLG72128_OK       = 0x00,
    ZLG72128_ERROR    = 0x01,
    ZLG72128_BUSY     = 0x02,
    ZLG72128_TIMEOUT  = 0x03
} ZLG72128_Status;

/**
 * @brief Display group selection
 */
typedef enum {
    ZLG72128_GROUP1_5BIT = 0,  // 5-digit group (COM0-COM4, positions 0-4)
    ZLG72128_GROUP2_4BIT = 1   // 4-digit group (COM5-COM8, positions 5-8)
} ZLG72128_DisplayGroup;

/**
 * @brief ZLG72128 Device Handle Structure
 */
typedef struct {
    I2C_HandleTypeDef *hi2c;        // I2C handle
    uint8_t device_address;          // I2C device address (7-bit)
    uint8_t display_buffer[9];       // Display buffer for 9 digits
    uint8_t decimal_point_pos;       // Decimal point position (0-8, 0xFF = none)
    uint8_t brightness;              // Brightness level (0-7)
    uint32_t flash_tick;             // Last flash tick for timing
    bool flash_state;                // Current flash state (on/off)
    bool group1_flash_enabled;       // Group 1 flash enable
    bool group2_flash_enabled;       // Group 2 flash enable
} ZLG72128_Handle;

/* Exported constants --------------------------------------------------------*/

// I2C Address (7-bit format)
#define ZLG72128_I2C_ADDR           0x30    // 0x60 >> 1

// Register/Command Addresses
#define ZLG72128_REG_SYSTEM_REG     0x00    // System register
#define ZLG72128_REG_KEY_DATA       0x01    // Key data register
#define ZLG72128_REG_REPEAT_CNT     0x02    // Repeat counter
#define ZLG72128_REG_FUNCTION_KEY   0x03    // Function key

// Display buffer addresses (COM pin mapping)
#define ZLG72128_DIG0_ADDR          0x10    // Digit 0 address (COM0) - Group1
#define ZLG72128_DIG1_ADDR          0x11    // Digit 1 address (COM1) - Group1
#define ZLG72128_DIG2_ADDR          0x12    // Digit 2 address (COM2) - Group1
#define ZLG72128_DIG3_ADDR          0x13    // Digit 3 address (COM3) - Group1
#define ZLG72128_DIG4_ADDR          0x14    // Digit 4 address (COM4) - Group1
#define ZLG72128_DIG5_ADDR          0x15    // Digit 5 address (COM5) - Group2
#define ZLG72128_DIG6_ADDR          0x16    // Digit 6 address (COM6) - Group2
#define ZLG72128_DIG7_ADDR          0x17    // Digit 7 address (COM7) - Group2
#define ZLG72128_DIG8_ADDR          0x18    // Digit 8 address (COM8) - Group2

// Control Commands
#define ZLG72128_CMD_RESET          0x70    // Software reset
#define ZLG72128_CMD_TEST           0x71    // Test mode
#define ZLG72128_CMD_BRK_ON         0x72    // Break on
#define ZLG72128_CMD_BRK_OFF        0x73    // Break off
#define ZLG72128_CMD_SYS_ON         0x74    // System on
#define ZLG72128_CMD_SYS_OFF        0x75    // System off
#define ZLG72128_CMD_READ_KEY       0x07    // Read key

// Brightness levels
#define ZLG72128_BRIGHTNESS_MIN     0x00
#define ZLG72128_BRIGHTNESS_MAX     0x07

// 7-Segment encoding for digits 0-9
#define ZLG72128_SEG_0              0x3F
#define ZLG72128_SEG_1              0x06
#define ZLG72128_SEG_2              0x5B
#define ZLG72128_SEG_3              0x4F
#define ZLG72128_SEG_4              0x66
#define ZLG72128_SEG_5              0x6D
#define ZLG72128_SEG_6              0x7D
#define ZLG72128_SEG_7              0x07
#define ZLG72128_SEG_8              0x7F
#define ZLG72128_SEG_9              0x6F

// Special characters
#define ZLG72128_SEG_MINUS          0x40    // '-'
#define ZLG72128_SEG_BLANK          0x00    // ' '
#define ZLG72128_SEG_DOT            0x80    // Decimal point bit

// Timeout
#define ZLG72128_I2C_TIMEOUT        500     // 500ms timeout

// Flash timing
#define ZLG72128_FLASH_INTERVAL     500     // 500ms flash interval

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize ZLG72128 device
 * @param handle: Pointer to ZLG72128 handle structure
 * @param hi2c: Pointer to I2C handle
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_Init(ZLG72128_Handle *handle, I2C_HandleTypeDef *hi2c);

/**
 * @brief Reset ZLG72128 device
 * @param handle: Pointer to ZLG72128 handle
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_Reset(ZLG72128_Handle *handle);

/**
 * @brief Set display brightness
 * @param handle: Pointer to ZLG72128 handle
 * @param brightness: Brightness level (0-7)
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_SetBrightness(ZLG72128_Handle *handle, uint8_t brightness);

/**
 * @brief Clear all display segments
 * @param handle: Pointer to ZLG72128 handle
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_ClearDisplay(ZLG72128_Handle *handle);

/**
 * @brief Write raw segment data to a specific digit position
 * @param handle: Pointer to ZLG72128 handle
 * @param position: Digit position (0-8)
 * @param segment_data: 7-segment data byte
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_WriteDigit(ZLG72128_Handle *handle, uint8_t position, uint8_t segment_data);

/**
 * @brief Display a number on specified digit position
 * @param handle: Pointer to ZLG72128 handle
 * @param position: Digit position (0-8)
 * @param number: Number to display (0-9)
 * @param show_dot: Show decimal point if true
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_DisplayNumber(ZLG72128_Handle *handle, uint8_t position, uint8_t number, bool show_dot);

/**
 * @brief Display integer value across multiple digits
 * @param handle: Pointer to ZLG72128 handle
 * @param value: Integer value to display
 * @param start_pos: Starting digit position
 * @param num_digits: Number of digits to use
 * @param leading_zero: Show leading zeros if true
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_DisplayInteger(ZLG72128_Handle *handle, int32_t value, uint8_t start_pos, uint8_t num_digits, bool leading_zero);

/**
 * @brief Display floating point value with decimal point
 * @param handle: Pointer to ZLG72128 handle
 * @param value: Float value to display
 * @param start_pos: Starting digit position
 * @param num_digits: Total number of digits to use
 * @param decimal_places: Number of decimal places
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_DisplayFloat(ZLG72128_Handle *handle, float value, uint8_t start_pos, uint8_t num_digits, uint8_t decimal_places);

/**
 * @brief Display integer value on Group 1 (COM0-COM4, 5 digits)
 * @param handle: Pointer to ZLG72128 handle
 * @param value: Integer value to display
 * @param leading_zero: Show leading zeros if true
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_DisplayGroup1_Integer(ZLG72128_Handle *handle, int32_t value, bool leading_zero);

/**
 * @brief Display float value on Group 1 (COM0-COM4, 5 digits)
 * @param handle: Pointer to ZLG72128 handle
 * @param value: Float value to display
 * @param decimal_places: Number of decimal places (1-4)
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_DisplayGroup1_Float(ZLG72128_Handle *handle, float value, uint8_t decimal_places);

/**
 * @brief Display integer value on Group 2 (COM5-COM8, 4 digits)
 * @param handle: Pointer to ZLG72128 handle
 * @param value: Integer value to display
 * @param leading_zero: Show leading zeros if true
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_DisplayGroup2_Integer(ZLG72128_Handle *handle, int32_t value, bool leading_zero);

/**
 * @brief Display float value on Group 2 (COM5-COM8, 4 digits)
 * @param handle: Pointer to ZLG72128 handle
 * @param value: Float value to display
 * @param decimal_places: Number of decimal places (1-3)
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_DisplayGroup2_Float(ZLG72128_Handle *handle, float value, uint8_t decimal_places);

/**
 * @brief Display integer value on Group 1 with flash (0.5s interval)
 * @param handle: Pointer to ZLG72128 handle
 * @param value: Integer value to display
 * @param leading_zero: Show leading zeros if true
 * @retval ZLG72128_Status
 * @note Call ZLG72128_UpdateFlash() periodically to maintain flash effect
 */
ZLG72128_Status ZLG72128_DisplayGroup1_Integer_Flash(ZLG72128_Handle *handle, int32_t value, bool leading_zero);

/**
 * @brief Display float value on Group 1 with flash (0.5s interval)
 * @param handle: Pointer to ZLG72128 handle
 * @param value: Float value to display
 * @param decimal_places: Number of decimal places (1-4)
 * @retval ZLG72128_Status
 * @note Call ZLG72128_UpdateFlash() periodically to maintain flash effect
 */
ZLG72128_Status ZLG72128_DisplayGroup1_Float_Flash(ZLG72128_Handle *handle, float value, uint8_t decimal_places);

/**
 * @brief Display integer value on Group 2 with flash (0.5s interval)
 * @param handle: Pointer to ZLG72128 handle
 * @param value: Integer value to display
 * @param leading_zero: Show leading zeros if true
 * @retval ZLG72128_Status
 * @note Call ZLG72128_UpdateFlash() periodically to maintain flash effect
 */
ZLG72128_Status ZLG72128_DisplayGroup2_Integer_Flash(ZLG72128_Handle *handle, int32_t value, bool leading_zero);

/**
 * @brief Display float value on Group 2 with flash (0.5s interval)
 * @param handle: Pointer to ZLG72128 handle
 * @param value: Float value to display
 * @param decimal_places: Number of decimal places (1-3)
 * @retval ZLG72128_Status
 * @note Call ZLG72128_UpdateFlash() periodically to maintain flash effect
 */
ZLG72128_Status ZLG72128_DisplayGroup2_Float_Flash(ZLG72128_Handle *handle, float value, uint8_t decimal_places);

/**
 * @brief Clear Group 1 display (COM0-COM4)
 * @param handle: Pointer to ZLG72128 handle
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_ClearGroup1(ZLG72128_Handle *handle);

/**
 * @brief Clear Group 2 display (COM5-COM8)
 * @param handle: Pointer to ZLG72128 handle
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_ClearGroup2(ZLG72128_Handle *handle);

/**
 * @brief Update flash state - call this periodically (e.g., in main loop or timer)
 * @param handle: Pointer to ZLG72128 handle
 * @retval ZLG72128_Status
 * @note Only needed when using _Flash functions
 */
ZLG72128_Status ZLG72128_UpdateFlash(ZLG72128_Handle *handle);

/**
 * @brief Set decimal point at specific position
 * @param handle: Pointer to ZLG72128 handle
 * @param position: Position to set decimal point (0-8)
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_SetDecimalPoint(ZLG72128_Handle *handle, uint8_t position);

/**
 * @brief Clear decimal point
 * @param handle: Pointer to ZLG72128 handle
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_ClearDecimalPoint(ZLG72128_Handle *handle);

/**
 * @brief Read key data from ZLG72128
 * @param handle: Pointer to ZLG72128 handle
 * @param key_data: Pointer to store key data
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_ReadKey(ZLG72128_Handle *handle, uint8_t *key_data);

/**
 * @brief Test mode - light up all segments
 * @param handle: Pointer to ZLG72128 handle
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_TestDisplay(ZLG72128_Handle *handle);

/**
 * @brief Write data to ZLG72128 register
 * @param handle: Pointer to ZLG72128 handle
 * @param reg_addr: Register address
 * @param data: Data to write
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_WriteRegister(ZLG72128_Handle *handle, uint8_t reg_addr, uint8_t data);

/**
 * @brief Read data from ZLG72128 register
 * @param handle: Pointer to ZLG72128 handle
 * @param reg_addr: Register address
 * @param data: Pointer to store read data
 * @retval ZLG72128_Status
 */
ZLG72128_Status ZLG72128_ReadRegister(ZLG72128_Handle *handle, uint8_t reg_addr, uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /* __DISPLAY72128_DRIVER_H */
