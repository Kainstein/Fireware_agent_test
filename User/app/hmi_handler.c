/**
 ******************************************************************************
 * @file    app_button_handler.c
 * @brief   Application button event handlers for CS0-CS14
 *          (Merged with app_control.c and app_state.c)
 * @date    2025-12-30
 ******************************************************************************
 */

#include "hmi_handler.h"
#include "app.h"
#include "main.h"
#include "hmi_wrapper.h"
#include "eeprom24c64_driver.h"
#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
#include "hmi_display72128_driver.h"
#include "driver_printf_config.h"
#include "app_printf_config.h"
#include <stdio.h>
#include <string.h>

/* Conditional printf for HMI module */
#if (ENABLE_APP_PRINTF && ENABLE_APP_PRINTF_HMI)
    #define HMI_PRINTF(...)    printf(__VA_ARGS__)
#else
    #define HMI_PRINTF(...)    ((void)0)
#endif

// ============================================================================
// EEPROM Address Map (private to this file)
// ============================================================================
#define EEPROM_ADDR_TEMP_SETPOINT       0x0000  // float (4 bytes)
#define EEPROM_ADDR_BLDC_SETPOINT       0x0004  // uint16_t (2 bytes)
#define EEPROM_ADDR_CHECKSUM            0x0006  // uint16_t (2 bytes)
#define DEFAULT_TEMP_SETPOINT           -10.0f  // -10°C
#define DEFAULT_BLDC_SETPOINT           0       // 0 RPM

// ============================================================================
// Global State (from app_state.c)
// ============================================================================
SystemState_t g_system_state = {0};

// ============================================================================
// Private Variables (from app_control.c)
// ============================================================================
static uint32_t last_flash_toggle_time = 0;
static bool flash_state = true;  // true = ON, false = OFF

// Display cache to prevent redundant I2C writes
static char g_cached_group1_text[6] = "";
static char g_cached_group2_text[6] = "";
static bool g_cached_group1_flash = false;
static bool g_cached_group2_flash = false;
static bool g_cache_initialized = false;  // Force first update

// ============================================================================
// Private Function Prototypes (from app_state.c)
// ============================================================================
static uint16_t CalculateChecksum(void);
static void LoadDefaultValues(void);

// ============================================================================
// AppState Functions (from app_state.c)
// ============================================================================

/**
 * @brief Initialize system state from EEPROM
 */
void AppState_Init(void)
{
    // Clear state structure
    memset(&g_system_state, 0, sizeof(SystemState_t));
    
    // Attempt to load from EEPROM
    AppState_LoadFromEEPROM();
    
    // Set default runtime state (always start OFF)
    g_system_state.temp_is_on = false;
    g_system_state.bldc_is_on = false;
    g_system_state.group1_mode = DISPLAY_MODE_RUNNING;
    g_system_state.group2_mode = DISPLAY_MODE_RUNNING;
    g_system_state.group1_is_flashing = false;
    g_system_state.group2_is_flashing = false;
    
    // Initialize adjustment states
    g_system_state.group1_adjust_state = ADJUST_STATE_IDLE;
    g_system_state.group1_first_press_time = 0;
    g_system_state.group2_adjust_state = ADJUST_STATE_IDLE;
    g_system_state.group2_first_press_time = 0;
    
    // Initialize actual values to 0 (devices are OFF at startup)
    // In real application, these will be updated from sensors
    g_system_state.temp_actual = 0.0f;
    g_system_state.bldc_actual = 0;

#if ENABLE_DRV_PRINTF_HMI_APP_INIT == 0
    // Debug output for initialization
    HMI_PRINTF("[AppState_Init] Loaded from EEPROM:\r\n");
    HMI_PRINTF("  Temp setpoint: %.1f°C, Actual: %.1f°C\r\n", 
           g_system_state.temp_setpoint, g_system_state.temp_actual);
    HMI_PRINTF("  BLDC setpoint: %u RPM, Actual: %u RPM\r\n", 
           g_system_state.bldc_setpoint, g_system_state.bldc_actual);
    HMI_PRINTF("  Temp ON: %d, BLDC ON: %d\r\n", 
           g_system_state.temp_is_on, g_system_state.bldc_is_on);
#endif 
}

float AppState_GetTempSetpoint(void)
{
    return g_system_state.temp_setpoint;
}

void AppState_SetTempSetpoint(float value)
{
    // Clamp to valid range
    if (value < TEMP_MIN) value = TEMP_MIN;
    if (value > TEMP_MAX) value = TEMP_MAX;
    
    g_system_state.temp_setpoint = value;
    g_system_state.temp_last_adjust = HAL_GetTick();
    
    // Only set EEPROM flag if already in adjusting state (not on first display-only press)
    if (g_system_state.group2_adjust_state == ADJUST_STATE_ADJUSTING) {
        g_system_state.eeprom_pending_write = true;
    }
}

float AppState_GetTempActual(void)
{
    return g_system_state.temp_actual;
}

void AppState_SetTempActual(float value)
{
    g_system_state.temp_actual = value;
}

bool AppState_GetTempOnOff(void)
{
    return g_system_state.temp_is_on;
}

void AppState_ToggleTempOnOff(void)
{
    g_system_state.temp_is_on = !g_system_state.temp_is_on;
}

void AppState_SetTempOnOff(bool on_off)
{
    g_system_state.temp_is_on = on_off;
}

uint16_t AppState_GetBlDCSetpoint(void)
{
    return g_system_state.bldc_setpoint;
}

void AppState_SetBlDCSetpoint(uint16_t value)
{
    // Clamp to valid range
    if (value > BLDC_MAX) value = BLDC_MAX;
    
    g_system_state.bldc_setpoint = value;
    g_system_state.bldc_last_adjust = HAL_GetTick();
    
    // Only set EEPROM flag if already in adjusting state (not on first display-only press)
    if (g_system_state.group1_adjust_state == ADJUST_STATE_ADJUSTING) {
        g_system_state.eeprom_pending_write = true;
    }
}

uint16_t AppState_GetBlDCActual(void)
{
    return g_system_state.bldc_actual;
}

void AppState_SetBlDCActual(uint16_t value)
{
    g_system_state.bldc_actual = value;
}

bool AppState_GetBlDCOnOff(void)
{
    return g_system_state.bldc_is_on;
}

void AppState_ToggleBlDCOnOff(void)
{
    g_system_state.bldc_is_on = !g_system_state.bldc_is_on;
}

void AppState_SetBlDCOnOff(bool on_off)
{
    g_system_state.bldc_is_on = on_off;
}

void AppState_StartGroup1Flash(uint32_t current_time)
{
    g_system_state.group1_is_flashing = true;
    g_system_state.group1_flash_start = current_time;
}

void AppState_StopGroup1Flash(void)
{
    g_system_state.group1_is_flashing = false;
}

bool AppState_IsGroup1Flashing(void)
{
    return g_system_state.group1_is_flashing;
}

void AppState_StartGroup2Flash(uint32_t current_time)
{
    g_system_state.group2_is_flashing = true;
    g_system_state.group2_flash_start = current_time;
}

void AppState_StopGroup2Flash(void)
{
    g_system_state.group2_is_flashing = false;
}

bool AppState_IsGroup2Flashing(void)
{
    return g_system_state.group2_is_flashing;
}

void AppState_SetGroup1SettingMode(uint32_t current_time)
{
    g_system_state.group1_mode = DISPLAY_MODE_SETTING;
    AppState_StartGroup1Flash(current_time);
}

void AppState_SetGroup1RunningMode(void)
{
    g_system_state.group1_mode = DISPLAY_MODE_RUNNING;
    AppState_StopGroup1Flash();
}

DisplayMode_t AppState_GetGroup1Mode(void)
{
    return g_system_state.group1_mode;
}

void AppState_SetGroup2SettingMode(uint32_t current_time)
{
    g_system_state.group2_mode = DISPLAY_MODE_SETTING;
    AppState_StartGroup2Flash(current_time);
}

void AppState_SetGroup2RunningMode(void)
{
    g_system_state.group2_mode = DISPLAY_MODE_RUNNING;
    AppState_StopGroup2Flash();
}

DisplayMode_t AppState_GetGroup2Mode(void)
{
    return g_system_state.group2_mode;
}

void AppState_CommitPendingWrites(uint32_t current_time)
{
    if (!g_system_state.eeprom_pending_write) {
        return;
    }
    
    // Check if debounce time has elapsed
    uint32_t time_since_last_adjust = current_time - g_system_state.temp_last_adjust;
    uint32_t time_since_bldc_adjust = current_time - g_system_state.bldc_last_adjust;
    
    // Use the most recent adjustment time
    uint32_t time_since_last = (time_since_last_adjust < time_since_bldc_adjust) 
                               ? time_since_last_adjust : time_since_bldc_adjust;
    
    if (time_since_last >= EEPROM_WRITE_DEBOUNCE_MS) {
        AppState_SaveToEEPROM();
        g_system_state.eeprom_pending_write = false;
        g_system_state.eeprom_last_write = current_time;
    }
}

void AppState_SaveToEEPROM(void)
{
    uint8_t buffer[8];
    
    // Pack temperature setpoint (float, 4 bytes)
    memcpy(&buffer[0], &g_system_state.temp_setpoint, sizeof(float));
    
    // Pack BLDC setpoint (uint16_t, 2 bytes)
    buffer[4] = (uint8_t)(g_system_state.bldc_setpoint & 0xFF);
    buffer[5] = (uint8_t)((g_system_state.bldc_setpoint >> 8) & 0xFF);
    
    // Calculate checksum
    uint16_t checksum = CalculateChecksum();
    buffer[6] = (uint8_t)(checksum & 0xFF);
    buffer[7] = (uint8_t)((checksum >> 8) & 0xFF);
    
    // Write to EEPROM
    EEPROM_WriteBytes(EEPROM_ADDR_TEMP_SETPOINT, buffer, 8);
}

void AppState_LoadFromEEPROM(void)
{
    uint8_t buffer[8];
    
    // Read from EEPROM
    if (EEPROM_ReadBytes(EEPROM_ADDR_TEMP_SETPOINT, buffer, 8) != HAL_OK) {
        // Read failed, use defaults
        LoadDefaultValues();
        return;
    }
    
    // Unpack temperature setpoint
    float temp_setpoint;
    memcpy(&temp_setpoint, &buffer[0], sizeof(float));
    
    // Unpack BLDC setpoint
    uint16_t bldc_setpoint = buffer[4] | (buffer[5] << 8);
    
    // Unpack checksum
    uint16_t stored_checksum = buffer[6] | (buffer[7] << 8);
    
    // Temporarily store values for checksum calculation
    g_system_state.temp_setpoint = temp_setpoint;
    g_system_state.bldc_setpoint = bldc_setpoint;
    
    // Verify checksum
    uint16_t calculated_checksum = CalculateChecksum();
    if (calculated_checksum != stored_checksum) {
        // Checksum mismatch, use defaults
        LoadDefaultValues();
        return;
    }
    
    // Validate ranges
    if (temp_setpoint < TEMP_MIN || temp_setpoint > TEMP_MAX) {
        g_system_state.temp_setpoint = DEFAULT_TEMP_SETPOINT;
    }
    
    if (bldc_setpoint > BLDC_MAX) {
        g_system_state.bldc_setpoint = DEFAULT_BLDC_SETPOINT;
    }
}

static uint16_t CalculateChecksum(void)
{
    uint16_t checksum = 0;
    uint8_t* data = (uint8_t*)&g_system_state.temp_setpoint;
    
    // Checksum of temperature setpoint (4 bytes)
    for (int i = 0; i < 4; i++) {
        checksum += data[i];
    }
    
    // Checksum of BLDC setpoint (2 bytes)
    checksum += (uint8_t)(g_system_state.bldc_setpoint & 0xFF);
    checksum += (uint8_t)((g_system_state.bldc_setpoint >> 8) & 0xFF);
    
    return checksum;
}

static void LoadDefaultValues(void)
{
    g_system_state.temp_setpoint = DEFAULT_TEMP_SETPOINT;
    g_system_state.bldc_setpoint = DEFAULT_BLDC_SETPOINT;
}

// ============================================================================
// AppControl Functions (from app_control.c)
// ============================================================================

void AppControl_Init(void)
{
    // Initialize button and display hardware
    DisplayButton_Status init_status = DisplayButton_Init();
    if (init_status != DISPLAYBUTTON_OK) {
        HMI_PRINTF("WARNING: Button/Display initialization failed!\r\n");
    }
    
    // Initialize state management
    AppState_Init();
    
    // Initialize display driver wrapper
    Display72128_Init();
    
    // Initial display update
    AppControl_UpdateDisplay();
    AppControl_UpdateDisplay();
}

void AppControl_ProcessContinuousAdjustment(uint32_t current_time)
{
    // Temperature UP (CS1) - continuous adjustment while held
    if (g_system_state.temp_up_active) {
        // Check if enough time has elapsed (handles future timestamp correctly)
        if ((int32_t)(current_time - g_system_state.temp_last_adjust) >= 0) {
            float current = AppState_GetTempSetpoint();
            // Calculate hold duration for acceleration
            uint32_t hold_duration = current_time - g_system_state.temp_adjust_start;
            float step = (hold_duration >= ADJUST_ACCEL_THRESHOLD_MS) ? ADJUST_STEP_TEMP_FAST : ADJUST_STEP_TEMP;
            AppState_SetTempSetpoint(current + step);
            g_system_state.temp_last_adjust = current_time;
            
            // Restart flash timer
            AppState_StartGroup2Flash(current_time);
        }
    }
    
    // Temperature DOWN (CS2) - continuous adjustment while held
    if (g_system_state.temp_down_active) {
        // Check if enough time has elapsed (handles future timestamp correctly)
        if ((int32_t)(current_time - g_system_state.temp_last_adjust) >= 0) {
            float current = AppState_GetTempSetpoint();
            // Calculate hold duration for acceleration
            uint32_t hold_duration = current_time - g_system_state.temp_adjust_start;
            float step = (hold_duration >= ADJUST_ACCEL_THRESHOLD_MS) ? ADJUST_STEP_TEMP_FAST : ADJUST_STEP_TEMP;
            AppState_SetTempSetpoint(current - step);
            g_system_state.temp_last_adjust = current_time;
            
            // Restart flash timer
            AppState_StartGroup2Flash(current_time);
        }
    }
    
    // BLDC UP (CS6) - continuous adjustment while held
    if (g_system_state.bldc_up_active) {
        // Check if enough time has elapsed (handles future timestamp correctly)
        if ((int32_t)(current_time - g_system_state.bldc_last_adjust) >= 0) {
            uint16_t current = AppState_GetBlDCSetpoint();
            // Calculate hold duration for acceleration
            uint32_t hold_duration = current_time - g_system_state.bldc_adjust_start;
            uint16_t step = (hold_duration >= ADJUST_ACCEL_THRESHOLD_MS) ? ADJUST_STEP_BLDC_FAST : ADJUST_STEP_BLDC;
            AppState_SetBlDCSetpoint(current + step);
            g_system_state.bldc_last_adjust = current_time;
            
            // Restart flash timer
            AppState_StartGroup1Flash(current_time);
        }
    }
    
    // BLDC DOWN (CS5) - continuous adjustment while held
    if (g_system_state.bldc_down_active) {
        // Check if enough time has elapsed (handles future timestamp correctly)
        if ((int32_t)(current_time - g_system_state.bldc_last_adjust) >= 0) {
            uint16_t current = AppState_GetBlDCSetpoint();
            // Calculate hold duration for acceleration
            uint32_t hold_duration = current_time - g_system_state.bldc_adjust_start;
            uint16_t step = (hold_duration >= ADJUST_ACCEL_THRESHOLD_MS) ? ADJUST_STEP_BLDC_FAST : ADJUST_STEP_BLDC;
            if (current >= step) {
                AppState_SetBlDCSetpoint(current - step);
            } else {
                AppState_SetBlDCSetpoint(0);
            }
            g_system_state.bldc_last_adjust = current_time;
            
            // Restart flash timer
            AppState_StartGroup1Flash(current_time);
        }
    }
    
    // Commit pending EEPROM writes
    AppState_CommitPendingWrites(current_time);
}

void AppControl_UpdateDisplayFlash(uint32_t current_time)
{
    // Get fresh timestamp to avoid race conditions with button event timestamps
    current_time = HAL_GetTick();
    
    // Check Group1 flash timeout
    if (AppState_IsGroup1Flashing()) {
        uint32_t elapsed = current_time - g_system_state.group1_flash_start;
        if (elapsed >= DISPLAY_FLASH_TIMEOUT_MS) {
            AppState_SetGroup1RunningMode();
            g_system_state.group1_adjust_state = ADJUST_STATE_IDLE;
        }
    }
    
    // Check Group2 flash timeout
    if (AppState_IsGroup2Flashing()) {
        uint32_t elapsed = current_time - g_system_state.group2_flash_start;
        if (elapsed >= DISPLAY_FLASH_TIMEOUT_MS) {
            AppState_SetGroup2RunningMode();
            g_system_state.group2_adjust_state = ADJUST_STATE_IDLE;
        }
    }
    
    // Toggle flash state every DISPLAY_FLASH_TOGGLE_MS
    if (current_time - last_flash_toggle_time >= DISPLAY_FLASH_TOGGLE_MS) {
        flash_state = !flash_state;
        last_flash_toggle_time = current_time;
    }
}

void AppControl_UpdateDisplay(void)
{
    char group1_text[6] = {0};  // 5 digits + null
    char group2_text[6] = {0};  // 5 chars (XXX.X) + null
    
    // ---- Update Group1 (BLDC) ----
    if (AppState_GetGroup1Mode() == DISPLAY_MODE_SETTING) {
        // Setting mode - show setpoint
        uint16_t setpoint = AppState_GetBlDCSetpoint();
        snprintf(group1_text, sizeof(group1_text), "%u", setpoint);
    } else {
        // Running mode - show OFF or actual value based on ON/OFF state
        if (AppState_GetBlDCOnOff()) {
            // Device is ON - show actual value
            uint16_t actual = AppState_GetBlDCActual();
            snprintf(group1_text, sizeof(group1_text), "%u", actual);
        } else {
            // Device is OFF - show OFF text
            snprintf(group1_text, sizeof(group1_text), "OFF");
        }
    }
    
    // ---- Update Group2 (Temperature) ----
    if (AppState_GetGroup2Mode() == DISPLAY_MODE_SETTING) {
        // Setting mode - show setpoint
        float setpoint = AppState_GetTempSetpoint();
        snprintf(group2_text, sizeof(group2_text), "%.1f", setpoint);
    } else {
        // Running mode - show OFF or actual value based on ON/OFF state
        if (AppState_GetTempOnOff()) {
            // Device is ON - show actual value
            float actual = AppState_GetTempActual();
            snprintf(group2_text, sizeof(group2_text), "%.1f", actual);
        } else {
            // Device is OFF - show OFF text
            snprintf(group2_text, sizeof(group2_text), "OFF");
        }
    }
    
    // ---- Apply Flash State ----
    bool group1_flash_last_bit = AppState_IsGroup1Flashing() && !flash_state;
    bool group2_flash_last_bit = AppState_IsGroup2Flashing() && !flash_state;
    
    // ---- OPTIMIZED: Only update display if values changed ----
    // This prevents redundant I2C writes and eliminates flickering
    bool group1_changed = !g_cache_initialized || 
                         (strcmp(group1_text, g_cached_group1_text) != 0) || 
                         (group1_flash_last_bit != g_cached_group1_flash);
    bool group2_changed = !g_cache_initialized || 
                         (strcmp(group2_text, g_cached_group2_text) != 0) || 
                         (group2_flash_last_bit != g_cached_group2_flash);
    
    if (group1_changed) {
        strncpy(g_cached_group1_text, group1_text, sizeof(g_cached_group1_text) - 1);
        g_cached_group1_flash = group1_flash_last_bit;
        Display72128_ShowGroup1(group1_text, group1_flash_last_bit);
    }
    
    if (group2_changed) {
        strncpy(g_cached_group2_text, group2_text, sizeof(g_cached_group2_text) - 1);
        g_cached_group2_flash = group2_flash_last_bit;
        Display72128_ShowGroup2(group2_text, group2_flash_last_bit);
    }
    
    // Mark cache as initialized after first update
    if (!g_cache_initialized) {
        g_cache_initialized = true;
    }
}

void AppControl_StartTempUp(uint32_t current_time)
{
    // Check if this is the first press (display-only) or subsequent press (adjustment)
    if (g_system_state.group2_adjust_state == ADJUST_STATE_IDLE) {
        // First press: Enter setting mode, display setpoint, NO value adjustment
        g_system_state.group2_adjust_state = ADJUST_STATE_ADJUSTING;
        AppState_SetGroup2SettingMode(current_time);
        AppState_StartGroup2Flash(current_time);
    } else {
        // Second/subsequent press: Perform immediate adjustment
        float current = AppState_GetTempSetpoint();
        AppState_SetTempSetpoint(current + ADJUST_STEP_TEMP);
        AppState_StartGroup2Flash(current_time);
    }
    
    // Enable continuous adjustment (works for both first hold and subsequent press+hold)
    g_system_state.temp_up_active = true;
    // Record adjustment start time for acceleration tracking
    g_system_state.temp_adjust_start = current_time;
    // Set last_adjust to future to prevent immediate re-adjustment
    // Continuous adjustment will wait ADJUST_INTERVAL_MS from now
    g_system_state.temp_last_adjust = current_time + ADJUST_INTERVAL_MS;
}

void AppControl_StopTempUp(void)
{
    g_system_state.temp_up_active = false;
    g_system_state.temp_adjust_start = 0;
}

void AppControl_StartTempDown(uint32_t current_time)
{
    // Check if this is the first press (display-only) or subsequent press (adjustment)
    if (g_system_state.group2_adjust_state == ADJUST_STATE_IDLE) {
        // First press: Enter setting mode, display setpoint, NO value adjustment
        g_system_state.group2_adjust_state = ADJUST_STATE_ADJUSTING;
        AppState_SetGroup2SettingMode(current_time);
        AppState_StartGroup2Flash(current_time);
    } else {
        // Second/subsequent press: Perform immediate adjustment
        float current = AppState_GetTempSetpoint();
        AppState_SetTempSetpoint(current - ADJUST_STEP_TEMP);
        AppState_StartGroup2Flash(current_time);
    }
    
    // Enable continuous adjustment (works for both first hold and subsequent press+hold)
    g_system_state.temp_down_active = true;
    // Record adjustment start time for acceleration tracking
    g_system_state.temp_adjust_start = current_time;
    // Set last_adjust to future to prevent immediate re-adjustment
    // Continuous adjustment will wait ADJUST_INTERVAL_MS from now
    g_system_state.temp_last_adjust = current_time + ADJUST_INTERVAL_MS;
}

void AppControl_StopTempDown(void)
{
    g_system_state.temp_down_active = false;
    g_system_state.temp_adjust_start = 0;
}

void AppControl_StartBlDCUp(uint32_t current_time)
{
    // Check if this is the first press (display-only) or subsequent press (adjustment)
    if (g_system_state.group1_adjust_state == ADJUST_STATE_IDLE) {
        // First press: Enter setting mode, display setpoint, NO value adjustment
        g_system_state.group1_adjust_state = ADJUST_STATE_ADJUSTING;
        AppState_SetGroup1SettingMode(current_time);
        AppState_StartGroup1Flash(current_time);
    } else {
        // Second/subsequent press: Perform immediate adjustment
        uint16_t current = AppState_GetBlDCSetpoint();
        AppState_SetBlDCSetpoint(current + ADJUST_STEP_BLDC);
        AppState_StartGroup1Flash(current_time);
    }
    
    // Enable continuous adjustment (works for both first hold and subsequent press+hold)
    g_system_state.bldc_up_active = true;
    // Record adjustment start time for acceleration tracking
    g_system_state.bldc_adjust_start = current_time;
    // Set last_adjust to future to prevent immediate re-adjustment
    // Continuous adjustment will wait ADJUST_INTERVAL_MS from now
    g_system_state.bldc_last_adjust = current_time + ADJUST_INTERVAL_MS;
}

void AppControl_StopBlDCUp(void)
{
    g_system_state.bldc_up_active = false;
    g_system_state.bldc_adjust_start = 0;
}

void AppControl_StartBlDCDown(uint32_t current_time)
{
    // Check if this is the first press (display-only) or subsequent press (adjustment)
    if (g_system_state.group1_adjust_state == ADJUST_STATE_IDLE) {
        // First press: Enter setting mode, display setpoint, NO value adjustment
        g_system_state.group1_adjust_state = ADJUST_STATE_ADJUSTING;
        AppState_SetGroup1SettingMode(current_time);
        AppState_StartGroup1Flash(current_time);
    } else {
        // Second/subsequent press: Perform immediate adjustment
        uint16_t current = AppState_GetBlDCSetpoint();
        if (current >= ADJUST_STEP_BLDC) {
            AppState_SetBlDCSetpoint(current - ADJUST_STEP_BLDC);
        } else {
            AppState_SetBlDCSetpoint(0);
        }
        AppState_StartGroup1Flash(current_time);
    }
    
    // Enable continuous adjustment (works for both first hold and subsequent press+hold)
    g_system_state.bldc_down_active = true;
    // Record adjustment start time for acceleration tracking
    g_system_state.bldc_adjust_start = current_time;
    // Set last_adjust to future to prevent immediate re-adjustment
    // Continuous adjustment will wait ADJUST_INTERVAL_MS from now
    g_system_state.bldc_last_adjust = current_time + ADJUST_INTERVAL_MS;
}

void AppControl_StopBlDCDown(void)
{
    g_system_state.bldc_down_active = false;
    g_system_state.bldc_adjust_start = 0;
}

void AppControl_StartTiltMotor(bool forward, uint32_t current_time)
{
    g_system_state.tilt_motor_active = true;
    g_system_state.tilt_motor_forward = forward;
    g_system_state.tilt_press_start = current_time;
    
    // TODO: Call actual motor driver function
    // TiltMotor_Start(forward ? TILT_FORWARD : TILT_REVERSE);
}

void AppControl_StopTiltMotor(uint32_t current_time)
{
    if (!g_system_state.tilt_motor_active) {
        return;
    }
    
    uint32_t duration = current_time - g_system_state.tilt_press_start;
    
    g_system_state.tilt_motor_active = false;
    
    // TODO: Call actual motor driver function
    // TiltMotor_Stop();
    
    // Check if it was a step command or continuous
    if (duration < MOTOR_STEP_DURATION_MS) {
        // Single step command
        // Already moved for duration
    } else {
        // Continuous movement (already handled by motor running)
    }
}

void AppControl_StartImageMotor(bool right, uint32_t current_time)
{
    g_system_state.image_motor_active = true;
    g_system_state.image_motor_right = right;
    g_system_state.image_press_start = current_time;
    
    // TODO: Call actual motor driver function
    // ImageMotor_Start(right ? IMAGE_RIGHT : IMAGE_LEFT);
}

void AppControl_StopImageMotor(uint32_t current_time)
{
    if (!g_system_state.image_motor_active) {
        return;
    }
    
    uint32_t duration = current_time - g_system_state.image_press_start;
    
    g_system_state.image_motor_active = false;
    
    // TODO: Call actual motor driver function
    // ImageMotor_Stop();
    
    // Check if it was a step command or continuous
    if (duration < MOTOR_STEP_DURATION_MS) {
        // Single step command
        // Already moved for duration
    } else {
        // Continuous movement (already handled by motor running)
    }
}

// ============================================================================
// Button Event Handler (original app_button_handler.c)
// ============================================================================

void AppButton_HandleEvent(const ButtonEvent_t* event)
{
    uint32_t current_time = HAL_GetTick();
    
    HMI_PRINTF("[Button] Event received: ID=%u, Event=%u\r\n", event->button_id, event->event);
    
    switch (event->button_id) {
        
        // ====================================================================
        // CS0 - TEMP_ON_OFF
        // ====================================================================
        case 0:
            if (event->event == BUTTON_EVENT_SINGLE_PRESS) {
                HMI_PRINTF("[Button] CS0: TEMP_ON_OFF single press\r\n");
                AppState_ToggleTempOnOff();
            }
            break;
        
        // ====================================================================
        // CS1 - TEMP_UP
        // ====================================================================
        case 1:
            if (event->event == BUTTON_EVENT_PRESSED) {
                HMI_PRINTF("[Button] CS1: TEMP_UP pressed\r\n");
                AppControl_StartTempUp(current_time);
            }
            else if (event->event == BUTTON_EVENT_RELEASED) {
                HMI_PRINTF("[Button] CS1: TEMP_UP released\r\n");
                AppControl_StopTempUp();
            }
            break;
        
        // ====================================================================
        // CS2 - TEMP_DOWN
        // ====================================================================
        case 2:
            if (event->event == BUTTON_EVENT_PRESSED) {
                HMI_PRINTF("[Button] CS2: TEMP_DOWN pressed\r\n");
                AppControl_StartTempDown(current_time);
            }
            else if (event->event == BUTTON_EVENT_RELEASED) {
                HMI_PRINTF("[Button] CS2: TEMP_DOWN released\r\n");
                AppControl_StopTempDown();
            }
            break;
        
        // ====================================================================
        // CS3 - TILT_FORWARD
        // ====================================================================
        case 3:
            if (event->event == BUTTON_EVENT_PRESSED) {
                HMI_PRINTF("[Button] CS3: TILT_FORWARD pressed\r\n");
                AppControl_StartTiltMotor(true, current_time);
            }
            else if (event->event == BUTTON_EVENT_RELEASED) {
                HMI_PRINTF("[Button] CS3: TILT_FORWARD released\r\n");
                AppControl_StopTiltMotor(current_time);
            }
            break;
        
        // ====================================================================
        // CS4 - Not used
        // ====================================================================
        case 4:
            // Not used
            break;
        
        // ====================================================================
        // CS5 - BLDC_SPEED_DOWN
        // ====================================================================
        case 5:
            if (event->event == BUTTON_EVENT_PRESSED) {
                HMI_PRINTF("[Button] CS5: BLDC_SPEED_DOWN pressed\r\n");
                AppControl_StartBlDCDown(current_time);
            }
            else if (event->event == BUTTON_EVENT_RELEASED) {
                HMI_PRINTF("[Button] CS5: BLDC_SPEED_DOWN released\r\n");
                AppControl_StopBlDCDown();
            }
            break;
        
        // ====================================================================
        // CS6 - BLDC_SPEED_UP
        // ====================================================================
        case 6:
            if (event->event == BUTTON_EVENT_PRESSED) {
                HMI_PRINTF("[Button] CS6: BLDC_SPEED_UP pressed\r\n");
                AppControl_StartBlDCUp(current_time);
            }
            else if (event->event == BUTTON_EVENT_RELEASED) {
                HMI_PRINTF("[Button] CS6: BLDC_SPEED_UP released\r\n");
                AppControl_StopBlDCUp();
            }
            break;
        
        // ====================================================================
        // CS7 - BLDC_ON_OFF
        // ====================================================================
        case 7:
            if (event->event == BUTTON_EVENT_SINGLE_PRESS) {
                HMI_PRINTF("[Button] CS7: BLDC_ON_OFF single press\r\n");
                AppState_ToggleBlDCOnOff();
            }
            break;
        
        // ====================================================================
        // CS8 - IMAGE_MOVE_RIGHT
        // ====================================================================
        case 8:
            if (event->event == BUTTON_EVENT_PRESSED) {
                HMI_PRINTF("[Button] CS8: IMAGE_MOVE_RIGHT pressed\r\n");
                AppControl_StartImageMotor(true, current_time);
            }
            else if (event->event == BUTTON_EVENT_RELEASED) {
                HMI_PRINTF("[Button] CS8: IMAGE_MOVE_RIGHT released\r\n");
                AppControl_StopImageMotor(current_time);
            }
            break;
        
        // ====================================================================
        // CS9 - IMAGE_MOVE_LEFT
        // ====================================================================
        case 9:
            if (event->event == BUTTON_EVENT_PRESSED) {
                HMI_PRINTF("[Button] CS9: IMAGE_MOVE_LEFT pressed\r\n");
                AppControl_StartImageMotor(false, current_time);
            }
            else if (event->event == BUTTON_EVENT_RELEASED) {
                HMI_PRINTF("[Button] CS9: IMAGE_MOVE_LEFT released\r\n");
                AppControl_StopImageMotor(current_time);
            }
            break;
        
        // ====================================================================
        // CS10-CS13 - Reserved
        // ====================================================================
        case 10:
        case 11:
        case 12:
        case 13:
            // Reserved for future use
            break;
        
        // ====================================================================
        // CS14 - TILT_REVERSE / BOOTLOADER TRIGGER
        // ====================================================================
        case 14:
            if (event->event == BUTTON_EVENT_PRESSED) {
                // Increment bootloader trigger counter (checked during startup animation)
                Bootloader_IncrementCS14Counter();
                
                HMI_PRINTF("[Button] CS14: TILT_REVERSE pressed (bootloader count: %d)\r\n", 
                           Bootloader_GetCS14Count());
                AppControl_StartTiltMotor(false, current_time);
            }
            else if (event->event == BUTTON_EVENT_RELEASED) {
                HMI_PRINTF("[Button] CS14: TILT_REVERSE released\r\n");
                AppControl_StopTiltMotor(current_time);
            }
            break;
        
        // ====================================================================
        // CS15 - Not used
        // ====================================================================
        case 15:
            // Not used
            break;
        
        default:
            break;
    }
}

/* ============================================================================
   Startup Animation
   ============================================================================ */

/**
 * @brief Show startup animation on both display groups simultaneously
 * @details Progress effect: dashes appear left to right
 *          Group1 (5-digit): -, --, ---, ----, -----
 *          Group2 (4-digit): -, --, ---, ----
 *          Repeats 3 times with synchronized timing (total 2700ms)
 * @note Directly writes to display hardware, cache is invalidated afterward
 */
void AppControl_ShowStartupAnimation(void)
{
    char group1_text[6] = {0};
    char group2_text[6] = {0};  // Increased to 6 bytes for "0.005" + null terminator
    
    // Reset CS14 press counter at animation start
    Bootloader_ResetCS14Counter();
    
    // Select animation character based on macro setting
    #if STARTUP_ANIM_CHAR_MODE == 0
        char anim_char = '-';  // G segment only (horizontal line)
    #elif STARTUP_ANIM_CHAR_MODE == 1
        char anim_char = '8';  // All segments (full digit)
    #elif STARTUP_ANIM_CHAR_MODE == 2
        char anim_char = '8';  // All segments for Group1
        // Display version using universal float formatter
        uint8_t version_digits[4];
        Display72128_FormatFloat(APP_VERSION, version_digits);
        
        // Write version to Group2 display (COM5-COM8)
        for (int i = 0; i < 4; i++) {
            ZLG72128_WriteDigit(&g_display_handle, 5 + i, version_digits[i]);
        }
    #else
        char anim_char = '-';  // Default to '-'
    #endif
    
    // Repeat animation for specified number of cycles
    for (uint8_t cycle = 0; cycle < STARTUP_ANIM_REPEAT_COUNT; cycle++)
    {
        // Animate both groups simultaneously, step by step
        // Use the maximum digit count to iterate through all steps
        uint8_t max_steps = (STARTUP_ANIM_GROUP1_DIGITS > STARTUP_ANIM_GROUP2_DIGITS) 
                            ? STARTUP_ANIM_GROUP1_DIGITS 
                            : STARTUP_ANIM_GROUP2_DIGITS;
        
        for (uint8_t step = 1; step <= max_steps; step++)
        {
            // Update Group1 if we haven't exceeded its digit count
            if (step <= STARTUP_ANIM_GROUP1_DIGITS)
            {
                // Build progressive character string for Group1
                for (uint8_t i = 0; i < step; i++) {
                    group1_text[i] = anim_char;
                }
                group1_text[step] = '\0';
                
                // Write to Group1 display
                Display72128_ShowGroup1(group1_text, false);
            }
            
            #if STARTUP_ANIM_CHAR_MODE != 2
            // Update Group2 if we haven't exceeded its digit count (skip for mode 2)
            if (step <= STARTUP_ANIM_GROUP2_DIGITS)
            {
                // Build progressive character string for Group2
                for (uint8_t i = 0; i < step; i++) {
                    group2_text[i] = anim_char;
                }
                group2_text[step] = '\0';
                
                // Write to Group2 display
                Display72128_ShowGroup2(group2_text, false);
            }
            #endif
            
            // Check if CS14 was pressed during animation (trigger bootloader)
            if (Bootloader_ShouldTrigger()) {
                // CS14 pressed - enter bootloader mode
                HMI_PRINTF("[Bootloader] CS14 trigger detected, entering ISP mode...\r\n");
                Bootloader_RequestEntry();
                // Will not return from above function
            }
            
            // Wait before next step (use Group1 timing as base)
            osDelay(STARTUP_ANIM_GROUP1_STEP_MS);
        }
    }
    
    // Clear both displays after animation completes
    Display72128_ShowGroup1("", false);
    
    #if STARTUP_ANIM_CHAR_MODE == 2
    // For mode 2, clear Group2 using direct hardware write (since we wrote directly)
    for (int i = 5; i <= 8; i++) {
        ZLG72128_WriteDigit(&g_display_handle, i, 0x00);  // Clear each digit including decimal points
    }
    osDelay(50);  // Allow hardware clear to complete
    // Clear wrapper cache after direct hardware writes
    Display72128_RefreshCache();
    #else
    Display72128_ShowGroup2("", false);
    osDelay(50);
    #endif
    
    // Invalidate cache to force fresh display update
    AppControl_InvalidateDisplayCache();
    
    osDelay(100);  // Pause before showing default values
}

/* ============================================================================
   Display Cache Management
   ============================================================================ */

/**
 * @brief Invalidate display cache to force full refresh on next update
 * @note Use after direct hardware writes (e.g., startup animation)
 */
void AppControl_InvalidateDisplayCache(void)
{
    g_cache_initialized = false;     // Force re-initialization
    g_cached_group1_text[0] = '\0';  // Clear text caches
    g_cached_group2_text[0] = '\0';
}

