/**
 ******************************************************************************
 * @file    app_button_handler.h
 * @brief   Application button handler, control logic, and state management
 *          (Merged with app_control.h and app_state.h)
 * @date    2025-12-30
 ******************************************************************************
 */

#ifndef APP_BUTTON_HANDLER_H
#define APP_BUTTON_HANDLER_H

#include "hmi_wrapper.h"
#include "stm32h7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Configuration Constants
// ============================================================================
#define ADJUST_INTERVAL_MS          100     // Continuous adjustment interval
#define ADJUST_STEP_TEMP            0.5f    // Temperature adjustment step per press (°C)
#define ADJUST_STEP_BLDC            10      // BLDC speed adjustment step per press (RPM)
#define ADJUST_STEP_TEMP_FAST       2.0f    // Fast temperature step after acceleration (°C)
#define ADJUST_STEP_BLDC_FAST       50      // Fast BLDC step after acceleration (RPM)
#define ADJUST_ACCEL_THRESHOLD_MS   3000    // Acceleration threshold - switch to fast mode after 5 seconds
#define DISPLAY_FLASH_TIMEOUT_MS    3000    // Flash timeout (3 seconds)
#define DISPLAY_FLASH_TOGGLE_MS     200     // Flash toggle period (200ms - ON for 200ms, OFF for 200ms)
// Valid ranges
#define TEMP_MIN                    -20.0f    // Minimum temperature setpoint (°C)
#define TEMP_MAX                    220.0f  // Maximum temperature setpoint (°C)
#define BLDC_MIN                    0       // Minimum BLDC speed setpoint (RPM)
#define BLDC_MAX                    22000   // Maximum BLDC speed setpoint (RPM)
#define MOTOR_STEP_DURATION_MS      200     // Short press = single step
#define EEPROM_WRITE_DEBOUNCE_MS    1000    // 1 second debounce

// Startup Animation Configuration
// Progress effect: characters appear left to right
// Both displays animate SIMULTANEOUSLY step-by-step
// Group1 (5-digit): 5 steps, Group2 (4-digit): 4 steps
// Total time: 400ms × 5 steps × 3 cycles = 6000ms
#define STARTUP_ANIM_CHAR_MODE          2       // 0='-' both displays, 1='8' both displays, 2='8' on Group1 + version on Group2
#define STARTUP_ANIM_REPEAT_COUNT       3       // Number of animation cycles
#define STARTUP_ANIM_GROUP1_DIGITS      5       // Group1 digit count (BLDC display)
#define STARTUP_ANIM_GROUP2_DIGITS      4       // Group2 digit count (Temperature display)
#define STARTUP_ANIM_GROUP1_STEP_MS     400     // Step delay (ms) - both groups update simultaneously


// ============================================================================
// Type Definitions (from app_state.h)
// ============================================================================

/**
 * @brief Display mode enumeration
 */
typedef enum {
    DISPLAY_MODE_RUNNING = 0,
    DISPLAY_MODE_SETTING = 1
} DisplayMode_t;

/**
 * @brief Adjustment state for double-tap detection
 */
typedef enum {
    ADJUST_STATE_IDLE = 0,              // Not in adjustment mode
    ADJUST_STATE_FIRST_PRESS = 1,       // First press received, waiting for second press
    ADJUST_STATE_ADJUSTING = 2          // Second press received, adjustment enabled
} AdjustState_t;

/**
 * @brief System state structure
 */
typedef struct {
    // Temperature control
    float temp_setpoint;        // Target temperature in °C
    float temp_actual;          // Actual temperature in °C
    bool temp_is_on;            // Temperature control on/off
    uint32_t temp_last_adjust;  // Timestamp of last temperature adjustment
    
    // BLDC motor control
    uint16_t bldc_setpoint;     // Target BLDC speed in RPM
    uint16_t bldc_actual;       // Actual BLDC speed in RPM
    bool bldc_is_on;            // BLDC motor on/off
    uint32_t bldc_last_adjust;  // Timestamp of last BLDC adjustment
    
    // Display group 1 (temperature)
    DisplayMode_t group1_mode;      // Current display mode
    bool group1_is_flashing;        // Flash state for setting mode
    uint32_t group1_flash_start;    // Timestamp when flash started
    
    // Display group 2 (BLDC)
    DisplayMode_t group2_mode;      // Current display mode
    bool group2_is_flashing;        // Flash state for setting mode
    uint32_t group2_flash_start;    // Timestamp when flash started
    
    // EEPROM management
    bool eeprom_pending_write;      // Flag indicating pending EEPROM write
    uint32_t eeprom_last_write;     // Timestamp of last EEPROM write
    
    // Continuous adjustment states
    bool temp_up_active;            // Temperature UP button is held
    bool temp_down_active;          // Temperature DOWN button is held
    bool bldc_up_active;            // BLDC UP button is held
    bool bldc_down_active;          // BLDC DOWN button is held
    uint32_t temp_adjust_start;     // Timestamp when temp adjustment started (for acceleration)
    uint32_t bldc_adjust_start;     // Timestamp when BLDC adjustment started (for acceleration)
    
    // Double-tap adjustment states
    AdjustState_t group1_adjust_state;  // Group1 (BLDC) adjustment state
    uint32_t group1_first_press_time;   // Timestamp of first press for Group1
    AdjustState_t group2_adjust_state;  // Group2 (Temperature) adjustment state
    uint32_t group2_first_press_time;   // Timestamp of first press for Group2
    
    // Motor control states
    bool tilt_motor_active;         // Tilt motor is running
    bool tilt_motor_forward;        // Tilt motor direction (true=forward)
    uint32_t tilt_press_start;      // Timestamp when tilt button pressed
    bool image_motor_active;        // Image motor is running
    bool image_motor_right;         // Image motor direction (true=right)
    uint32_t image_press_start;     // Timestamp when image button pressed
} SystemState_t;

// ============================================================================
// Global Variables
// ============================================================================
extern SystemState_t g_system_state;

// ============================================================================
// FUNCTION DECLARATIONS - Button Event Handler
// ============================================================================

/**
 * @brief Handle button events from display/key controller
 * Call this function when a button event is available
 * @param event Pointer to button event structure
 */
void AppButton_HandleEvent(const ButtonEvent_t* event);

// ============================================================================
// FUNCTION DECLARATIONS - Initialization
// ============================================================================

/**
 * @brief Initialize application control system
 * Must be called before any other app control functions
 */
void AppControl_Init(void);

/**
 * @brief Initialize system state from EEPROM
 */
void AppState_Init(void);

// ============================================================================
// FUNCTION DECLARATIONS - Main Control Loop
// ============================================================================

/**
 * @brief Process continuous button adjustments
 * Call this in main loop to handle held buttons (CS1/CS2/CS5/CS6)
 * @param current_time Current HAL_GetTick() value
 */
void AppControl_ProcessContinuousAdjustment(uint32_t current_time);

/**
 * @brief Update display flash state
 * Call this in main loop to handle flash timeout and toggle
 * @param current_time Current HAL_GetTick() value
 */
void AppControl_UpdateDisplayFlash(uint32_t current_time);

/**
 * @brief Update display content
 * Call this in main loop to refresh display based on current state
 */
void AppControl_UpdateDisplay(void);

// ============================================================================
// FUNCTION DECLARATIONS - Temperature Control
// ============================================================================

/**
 * @brief Get current temperature setpoint
 * @return Temperature setpoint in °C
 */
float AppState_GetTempSetpoint(void);

/**
 * @brief Set temperature setpoint
 * @param value Target temperature in °C (0-220°C)
 */
void AppState_SetTempSetpoint(float value);

/**
 * @brief Get actual/measured temperature
 * @return Actual temperature in °C
 */
float AppState_GetTempActual(void);

/**
 * @brief Set actual/measured temperature
 * @param value Measured temperature in °C
 */
void AppState_SetTempActual(float value);

/**
 * @brief Get temperature control on/off state
 * @return true if temperature control is enabled, false otherwise
 */
bool AppState_GetTempOnOff(void);

/**
 * @brief Toggle temperature control on/off
 */
void AppState_ToggleTempOnOff(void);

/**
 * @brief Set temperature control on/off state
 * @param on_off true to enable, false to disable
 */
void AppState_SetTempOnOff(bool on_off);

/**
 * @brief Start continuous temperature UP adjustment
 * Called when CS1 (TEMP_UP) is pressed
 * @param current_time Current HAL_GetTick() value
 */
void AppControl_StartTempUp(uint32_t current_time);

/**
 * @brief Stop continuous temperature UP adjustment
 * Called when CS1 (TEMP_UP) is released
 */
void AppControl_StopTempUp(void);

/**
 * @brief Start continuous temperature DOWN adjustment
 * Called when CS2 (TEMP_DOWN) is pressed
 * @param current_time Current HAL_GetTick() value
 */
void AppControl_StartTempDown(uint32_t current_time);

/**
 * @brief Stop continuous temperature DOWN adjustment
 * Called when CS2 (TEMP_DOWN) is released
 */
void AppControl_StopTempDown(void);

// ============================================================================
// FUNCTION DECLARATIONS - BLDC Motor Control
// ============================================================================

/**
 * @brief Get current BLDC setpoint
 * @return BLDC speed setpoint in RPM
 */
uint16_t AppState_GetBlDCSetpoint(void);

/**
 * @brief Set BLDC setpoint
 * @param value Target BLDC speed in RPM (0-22000)
 */
void AppState_SetBlDCSetpoint(uint16_t value);

/**
 * @brief Get actual BLDC speed
 * @return Actual BLDC speed in RPM
 */
uint16_t AppState_GetBlDCActual(void);

/**
 * @brief Set actual BLDC speed
 * @param value Measured BLDC speed in RPM
 */
void AppState_SetBlDCActual(uint16_t value);

/**
 * @brief Get BLDC motor on/off state
 * @return true if BLDC motor is enabled, false otherwise
 */
bool AppState_GetBlDCOnOff(void);

/**
 * @brief Toggle BLDC motor on/off
 */
void AppState_ToggleBlDCOnOff(void);

/**
 * @brief Set BLDC motor on/off state
 * @param on_off true to enable, false to disable
 */
void AppState_SetBlDCOnOff(bool on_off);

/**
 * @brief Start continuous BLDC speed UP adjustment
 * Called when CS6 (BLDC_UP) is pressed
 * @param current_time Current HAL_GetTick() value
 */
void AppControl_StartBlDCUp(uint32_t current_time);

/**
 * @brief Stop continuous BLDC speed UP adjustment
 * Called when CS6 (BLDC_UP) is released
 */
void AppControl_StopBlDCUp(void);

/**
 * @brief Start continuous BLDC speed DOWN adjustment
 * Called when CS5 (BLDC_DOWN) is pressed
 * @param current_time Current HAL_GetTick() value
 */
void AppControl_StartBlDCDown(uint32_t current_time);

/**
 * @brief Stop continuous BLDC speed DOWN adjustment
 * Called when CS5 (BLDC_DOWN) is released
 */
void AppControl_StopBlDCDown(void);

// ============================================================================
// FUNCTION DECLARATIONS - Motor Control
// ============================================================================

/**
 * @brief Start tilt motor movement
 * @param forward true for forward, false for reverse
 * @param current_time Current HAL_GetTick() value
 */
void AppControl_StartTiltMotor(bool forward, uint32_t current_time);

/**
 * @brief Stop tilt motor movement
 * @param current_time Current HAL_GetTick() value
 */
void AppControl_StopTiltMotor(uint32_t current_time);

/**
 * @brief Start image motor movement
 * @param right true for right, false for left
 * @param current_time Current HAL_GetTick() value
 */
void AppControl_StartImageMotor(bool right, uint32_t current_time);

/**
 * @brief Stop image motor movement
 * @param current_time Current HAL_GetTick() value
 */
void AppControl_StopImageMotor(uint32_t current_time);

// ============================================================================
// FUNCTION DECLARATIONS - Display Flash Control
// ============================================================================

/**
 * @brief Start flashing display group 1 (temperature)
 * @param current_time Current system time in milliseconds
 */
void AppState_StartGroup1Flash(uint32_t current_time);

/**
 * @brief Stop flashing display group 1
 */
void AppState_StopGroup1Flash(void);

/**
 * @brief Check if display group 1 is flashing
 * @return true if flashing, false otherwise
 */
bool AppState_IsGroup1Flashing(void);

/**
 * @brief Start flashing display group 2 (BLDC)
 * @param current_time Current system time in milliseconds
 */
void AppState_StartGroup2Flash(uint32_t current_time);

/**
 * @brief Stop flashing display group 2
 */
void AppState_StopGroup2Flash(void);

/**
 * @brief Check if display group 2 is flashing
 * @return true if flashing, false otherwise
 */
bool AppState_IsGroup2Flashing(void);

// ============================================================================
// FUNCTION DECLARATIONS - Display Mode Control
// ============================================================================

/**
 * @brief Set display group 1 to setting mode with flash
 * @param current_time Current system time in milliseconds
 */
void AppState_SetGroup1SettingMode(uint32_t current_time);

/**
 * @brief Set display group 1 to running mode (no flash)
 */
void AppState_SetGroup1RunningMode(void);

/**
 * @brief Get display group 1 mode
 * @return Current display mode
 */
DisplayMode_t AppState_GetGroup1Mode(void);

/**
 * @brief Set display group 2 to setting mode with flash
 * @param current_time Current system time in milliseconds
 */
void AppState_SetGroup2SettingMode(uint32_t current_time);

/**
 * @brief Set display group 2 to running mode (no flash)
 */
void AppState_SetGroup2RunningMode(void);

/**
 * @brief Get display group 2 mode
 * @return Current display mode
 */
DisplayMode_t AppState_GetGroup2Mode(void);

// ============================================================================
// FUNCTION DECLARATIONS - EEPROM Management
// ============================================================================

/**
 * @brief Process pending EEPROM writes with debounce
 * @param current_time Current system time in milliseconds
 */
void AppState_CommitPendingWrites(uint32_t current_time);

/**
 * @brief Save current state to EEPROM
 */
void AppState_SaveToEEPROM(void);

/**
 * @brief Load state from EEPROM
 */
void AppState_LoadFromEEPROM(void);

// ============================================================================
// FUNCTION DECLARATIONS - Startup Animation
// ============================================================================

/**
 * @brief Show startup animation on both display groups simultaneously
 * @details Progress effect: dashes appear left to right
 *          Group1 (5-digit): -, --, ---, ----, -----
 *          Group2 (4-digit): -, --, ---, ----
 *          Repeats 3 times with synchronized timing (total 2700ms)
 * @note Directly writes to display hardware, cache is invalidated afterward
 */
void AppControl_ShowStartupAnimation(void);

/**
 * @brief Invalidate display cache to force full refresh on next update
 * @note Use after direct hardware writes (e.g., startup animation)
 */
void AppControl_InvalidateDisplayCache(void);

#endif // APP_BUTTON_HANDLER_H

