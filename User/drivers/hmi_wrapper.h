/**
 * @file button_display_wrapper.h
 * @brief Button and Display Integration Wrapper Header
 * @description Combines CY8CMBR3116 button controller and ZLG72128 LED display
 * @author Your Name
 * @date December 7, 2025
 */

#ifndef __BUTTON_DISPLAY_WRAPPER_H
#define __BUTTON_DISPLAY_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "hmi_display72128_driver.h"
#include "hmi_button3116_driver.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Display and button system initialization status
 */
typedef enum {
    DISPLAYBUTTON_OK    = 0x00,
    DISPLAYBUTTON_ERROR = 0x01
} DisplayButton_Status;

/**
 * @brief Button event types
 */
typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_PRESSED,        // Button just pressed (after debounce)
    BUTTON_EVENT_RELEASED,       // Button just released
    BUTTON_EVENT_SINGLE_PRESS,   // Short press (< 1000ms)
    BUTTON_EVENT_LONG_PRESS,     // Long press (>= 1000ms)
    BUTTON_EVENT_DOUBLE_CLICK    // Two presses within 400ms window
} ButtonEvent_Type;

/**
 * @brief Button event data
 */
typedef struct {
    uint8_t button_id;           // Button number (0-9)
    ButtonEvent_Type event;      // Event type
    uint32_t duration_ms;        // Press duration (for single/long press)
} ButtonEvent_t;

/**
 * @brief Button event callback function type
 */
typedef void (*ButtonEventCallback_t)(ButtonEvent_t* event);

/* Exported constants --------------------------------------------------------*/

// Button timing parameters
#define BUTTON_DEBOUNCE_TIME_MS      30    // Debounce period
#define BUTTON_LONG_PRESS_TIME_MS    1000  // Long press threshold
#define BUTTON_DOUBLE_CLICK_TIME_MS  400   // Double-click window

/* Exported macro ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize display and button system
 * @retval DisplayButton_Status
 */
DisplayButton_Status DisplayButton_Init(void);

/**
 * @brief Get button/touch controller handle
 * @retval Pointer to CY8CMBR3116_Handle
 */
CY8CMBR3116_Handle* Button_GetHandle(void);

/**
 * @brief Get display controller handle
 * @retval Pointer to ZLG72128_Handle
 */
ZLG72128_Handle* Display_GetHandle(void);

/**
 * @brief Process button states and generate events
 * @param current_time_ms: Current system time in milliseconds
 * @note Call this periodically (e.g., every 10-50ms) to process button events
 */
void Button_Process(uint32_t current_time_ms);

/**
 * @brief Register callback for button events
 * @param callback: Function to call when button events occur
 */
void Button_RegisterCallback(ButtonEventCallback_t callback);

// External display handle for direct access
extern ZLG72128_Handle g_display_handle;

/**
 * @brief Get the last button event (polling mode)
 * @param event: Pointer to store event data
 * @retval true if event available, false otherwise
 */
bool Button_GetEvent(ButtonEvent_t* event);

/**
 * @brief Handle button events for all CS pins (application logic)
 * @param event: Pointer to button event data
 * @param current_time: Current system time in milliseconds
 * @note Implements application-specific button logic for temperature, BLDC, motors
 */
void App_HandleButtonEvents(ButtonEvent_t* event, uint32_t current_time);

/**
 * @brief Process application logic (continuous adjustment, display update)
 * @param current_time: Current system time in milliseconds
 * @note Call from main loop after button processing
 */
void App_Process(uint32_t current_time);

/* Display72128 Wrapper Functions (merged from display72128_wrapper.h) ------*/

/**
 * @brief Initialize display system
 */
void Display72128_Init(void);

/**
 * @brief Show text/number on Group1 (5-digit BLDC display)
 * @param text String to display (e.g., "1500", "OFF")
 * @param flash_last_bit true to hide last digit (flash off state)
 * @note Optimized: Only updates digits that changed, uses caching to avoid redundant I2C writes
 */
void Display72128_ShowGroup1(const char* text, bool flash_last_bit);

/**
 * @brief Show text/number on Group2 (4-digit temperature display)
 * @param text String to display (e.g., "105.0", "OFF")
 * @param flash_last_bit true to hide last digit (flash off state)
 * @note Optimized: Only updates digits that changed, uses caching to avoid redundant I2C writes
 */
void Display72128_ShowGroup2(const char* text, bool flash_last_bit);

/**
 * @brief Format float with automatic decimal positioning for 4-digit display
 * @param value Float value to format (0.001 to 999.9)
 * @param digit_values Output array [COM5, COM6, COM7, COM8] (rightmost to leftmost)
 * @note Automatically determines optimal decimal placement based on value magnitude
 */
void Display72128_FormatFloat(float value, uint8_t* digit_values);

/**
 * @brief Force full refresh of display cache
 * @note Use this if display sync issues occur, typically not needed
 */
void Display72128_RefreshCache(void);

#ifdef __cplusplus
}
#endif

#endif /* __BUTTON_DISPLAY_WRAPPER_H */
