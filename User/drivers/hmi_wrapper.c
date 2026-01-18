/**
 * @file hmi_wrapper.c
 * @brief Button and Display Integration Wrapper Implementation
 * @description Combines CY8CMBR3116 button controller and ZLG72128 LED display
 * @author Your Name
 * @date December 7, 2025
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hmi_wrapper.h"
#include "hmi_handler.h"
#include "cmsis_os.h"

/* Private define ------------------------------------------------------------*/
#define ADJUST_INTERVAL_MS 100       // Continuous adjustment interval (ms)
#define ADJUST_DELAY_MS    500       // Delay before continuous adjustment starts (ms)
#define MOTOR_STEP_THRESHOLD_MS 200  // Motor step vs continuous threshold (ms)

/* Private types -------------------------------------------------------------*/

/**
 * @brief Button state tracking structure
 */
typedef struct {
    uint32_t press_start_time;      // When button was first pressed (ms)
    uint32_t last_release_time;     // Last release time for double-click (ms)
    uint8_t press_count;            // Press count for double-click detection
    bool is_pressed;                // Current physical state
    bool debounced;                 // Passed debounce period
    bool long_press_triggered;      // Long press event already sent
} ButtonState_t;

extern I2C_HandleTypeDef hi2c2;

/* Private variables ---------------------------------------------------------*/
static ZLG72128_Handle zlg_handle;
static CY8CMBR3116_Handle touch_handle;
static ButtonState_t button_states[16];  // Support up to 16 buttons
static ButtonEventCallback_t event_callback = NULL;
static ButtonEvent_t pending_event = {0};
static bool event_pending = false;

/* Display72128 Wrapper Variables (merged from display72128_wrapper.c) ------*/
ZLG72128_Handle g_display_handle;  // Exported for external access

// Display cache for Group1 (5 digits) and Group2 (4 digits)
static uint8_t g_group1_cache[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t g_group2_cache[4] = {0xFF, 0xFF, 0xFF, 0xFF};

// String cache to prevent redundant updates
static char g_group1_text_cache[6] = {0};
static bool g_group1_flash_cache = false;
static char g_group2_text_cache[5] = {0};
static bool g_group2_flash_cache = false;

// 7-segment encoding for special characters
#define SEG_O  0x3F  // 'O'
#define SEG_F  0x71  // 'F'
#define SEG_BLANK 0x00

/* Private function prototypes -----------------------------------------------*/
static void Button_GenerateEvent(uint8_t button_id, ButtonEvent_Type event_type, uint32_t duration_ms);
static void Button_ProcessSingle(uint8_t button_id, bool current_state, uint32_t current_time);
static bool IsAnimationString(const char* text);
static uint8_t GetSegmentCode(int digit_or_char);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief Initialize display and button system
 */
DisplayButton_Status DisplayButton_Init(void)
{
    DisplayButton_Status status = DISPLAYBUTTON_OK;

    // Initialize ZLG72128 LED display driver
    if (ZLG72128_Init(&zlg_handle, &hi2c2) != ZLG72128_OK)
    {
        status = DISPLAYBUTTON_ERROR;
    }

    // Initialize CY8CMBR3116 touch controller
    if (CY8CMBR3116_Init(&touch_handle, &hi2c2, CY8CMBR3116_I2C_ADDR_DEFAULT) != CY8CMBR3116_OK)
    {
        status = DISPLAYBUTTON_ERROR;
    }

    return status;
}

/**
 * @brief Get button/touch controller handle
 */
CY8CMBR3116_Handle* Button_GetHandle(void)
{
    return &touch_handle;
}

/**
 * @brief Get display controller handle
 */
ZLG72128_Handle* Display_GetHandle(void)
{
    return &zlg_handle;
}

/**
 * @brief Register callback for button events
 */
void Button_RegisterCallback(ButtonEventCallback_t callback)
{
    event_callback = callback;
}

/**
 * @brief Get the last button event (polling mode)
 */
bool Button_GetEvent(ButtonEvent_t* event)
{
    if (event_pending && event != NULL)
    {
        *event = pending_event;
        event_pending = false;
        return true;
    }
    return false;
}

/**
 * @brief Process button states and generate events
 */
void Button_Process(uint32_t current_time_ms)
{
    uint16_t button_status = 0;
    
    // Read current button status from touch controller
    if (CY8CMBR3116_ReadButtonStatus(&touch_handle, &button_status) != CY8CMBR3116_OK)
    {
        return; // Skip this cycle if read fails
    }
    
    // Process all 16 buttons of CY8CMBR3116-LQXIT
    // Hardware uses: CS0-CS3, CS5-CS14 (CS4 and CS15 not connected)
    for (uint8_t i = 0; i < 16; i++)
    {
        bool current_state = (button_status & (1 << i)) != 0;
        Button_ProcessSingle(i, current_state, current_time_ms);
    }
}

/**
 * @brief Handle button events - Application Logic
 * @param event: Button event structure
 * @param current_time: Current time in milliseconds
 * @note This function now forwards events to the handler layer
 */
void App_HandleButtonEvents(ButtonEvent_t* event, uint32_t current_time)
{
    if (event == NULL) return;
    
    // Forward to application handler
    AppButton_HandleEvent(event);
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Generate button event (callback or store for polling)
 */
static void Button_GenerateEvent(uint8_t button_id, ButtonEvent_Type event_type, uint32_t duration_ms)
{
    ButtonEvent_t event = {
        .button_id = button_id,
        .event = event_type,
        .duration_ms = duration_ms
    };
    
    // Call callback if registered
    if (event_callback != NULL)
    {
        event_callback(&event);
    }
    
    // Store for polling mode (only if no event is pending)
    if (!event_pending)
    {
        pending_event = event;
        event_pending = true;
    }
}

/**
 * @brief Process a single button state machine
 */
static void Button_ProcessSingle(uint8_t button_id, bool current_state, uint32_t current_time)
{
    ButtonState_t* btn = &button_states[button_id];
    
    // Button just pressed
    if (current_state && !btn->is_pressed)
    {
        btn->is_pressed = true;
        btn->press_start_time = current_time;
        btn->debounced = false;
        btn->long_press_triggered = false;
    }
    // Button still pressed
    else if (current_state && btn->is_pressed)
    {
        uint32_t press_duration = current_time - btn->press_start_time;
        
        // Check debounce
        if (!btn->debounced && press_duration >= BUTTON_DEBOUNCE_TIME_MS)
        {
            btn->debounced = true;
            Button_GenerateEvent(button_id, BUTTON_EVENT_PRESSED, 0);
        }
        
        // Check long press (trigger once when threshold reached)
        if (btn->debounced && !btn->long_press_triggered && 
            press_duration >= BUTTON_LONG_PRESS_TIME_MS)
        {
            btn->long_press_triggered = true;
            Button_GenerateEvent(button_id, BUTTON_EVENT_LONG_PRESS, press_duration);
        }
    }
    // Button just released
    else if (!current_state && btn->is_pressed)
    {
        uint32_t press_duration = current_time - btn->press_start_time;
        
        btn->is_pressed = false;
        
        // Only process if button was debounced
        if (btn->debounced)
        {
            // Generate release event
            Button_GenerateEvent(button_id, BUTTON_EVENT_RELEASED, press_duration);
            
            // Check if it was a short press (not already triggered as long press)
            if (!btn->long_press_triggered && press_duration < BUTTON_LONG_PRESS_TIME_MS)
            {
                // Check for double-click
                uint32_t time_since_last_release = current_time - btn->last_release_time;
                
                if (btn->press_count == 1 && time_since_last_release <= BUTTON_DOUBLE_CLICK_TIME_MS)
                {
                    // Double-click detected!
                    Button_GenerateEvent(button_id, BUTTON_EVENT_DOUBLE_CLICK, 0);
                    btn->press_count = 0; // Reset counter
                }
                else
                {
                    // First press or timeout - start new sequence
                    if (btn->press_count == 0)
                    {
                        btn->press_count = 1;
                    }
                    else
                    {
                        // Previous press timed out, send single press for it
                        Button_GenerateEvent(button_id, BUTTON_EVENT_SINGLE_PRESS, press_duration);
                        btn->press_count = 1; // Start new sequence
                    }
                }
            }
            else if (btn->long_press_triggered)
            {
                // Long press already sent, just reset double-click counter
                btn->press_count = 0;
            }
            
            btn->last_release_time = current_time;
        }
    }
    // Button still released
    else if (!current_state && !btn->is_pressed)
    {
        // Check if pending single press timed out
        if (btn->press_count == 1)
        {
            uint32_t time_since_last_release = current_time - btn->last_release_time;
            
            if (time_since_last_release > BUTTON_DOUBLE_CLICK_TIME_MS)
            {
                // Timeout - send single press event
                Button_GenerateEvent(button_id, BUTTON_EVENT_SINGLE_PRESS, 0);
                btn->press_count = 0;
            }
        }
    }
}

/* Application Functions -----------------------------------------------------*/

/**
 * @brief Check if string is a pure animation string (all '-' or all '8')
 * @param text String to check
 * @return true if pure animation string, false otherwise
 */
static bool IsAnimationString(const char* text)
{
    if (text == NULL || text[0] == '\0') {
        return false;
    }
    
    int len = strlen(text);
    bool all_dash = true;
    bool all_eight = true;
    
    // Check if all characters are '-'
    for (int i = 0; i < len; i++) {
        if (text[i] != '-') {
            all_dash = false;
            break;
        }
    }
    if (all_dash) return true;
    
    // Check if all characters are '8'
    for (int i = 0; i < len; i++) {
        if (text[i] != '8') {
            all_eight = false;
            break;
        }
    }
    
    return all_eight;
}

/**
 * @brief Get 7-segment code for a digit or character
 * @param digit_or_char Numeric digit (0-9) or character ('0'-'9', '-', '8')
 * @return 7-segment code
 */
static uint8_t GetSegmentCode(int digit_or_char)
{
    static const uint8_t segment_table[10] = {
        0x3F,  // 0: 0011 1111
        0x06,  // 1: 0000 0110
        0x5B,  // 2: 0101 1011
        0x4F,  // 3: 0100 1111
        0x66,  // 4: 0110 0110
        0x6D,  // 5: 0110 1101
        0x7D,  // 6: 0111 1101
        0x07,  // 7: 0000 0111
        0x7F,  // 8: 0111 1111
        0x6F   // 9: 0110 1111
    };
    
    // Handle numeric digits (0-9) directly
    if (digit_or_char >= 0 && digit_or_char <= 9) {
        return segment_table[digit_or_char];
    }
    
    // Handle character input
    char ch = (char)digit_or_char;
    
    // Handle dash character (G segment only - middle horizontal line)
    if (ch == '-') {
        return 0x40;  // Segment G
    }
    
    // Handle digit characters '0'-'9'
    if (ch >= '0' && ch <= '9') {
        return segment_table[ch - '0'];
    }
    
    return 0x00;  // Blank for unsupported characters
}

/**
 * @brief Universal float formatter - automatically determines decimal position
 * @param value Float value to format
 * @param digit_values Output array [rightmost COM5...leftmost COM8]
 * @note Handles: 0.005 (0.005), 10.5 (10.50), 105.0 (105.0), -10.5 (-10.5)
 */
void Display72128_FormatFloat(float value, uint8_t* digit_values)
{
    bool is_negative = (value < 0.0f);
    if (is_negative) value = -value;
    
    // Determine optimal format based on magnitude
    int decimal_places;
    int multiplier;
    
    if (value >= 100.0f) {         // 100.0 - 999.9 → XXX.X format
        decimal_places = 1;
        multiplier = 10;
    } else if (value >= 10.0f) {   // 10.0 - 99.9 → XX.X format
        decimal_places = 1;
        multiplier = 10;
    } else if (value >= 1.0f) {    // 1.00 - 9.99 → X.XX format
        decimal_places = 2;
        multiplier = 100;
    } else {                       // 0.001 - 0.999 → 0.XXX format
        decimal_places = 3;
        multiplier = 1000;
    }
    
    // Convert to integer with proper precision
    int int_value = (int)(value * multiplier + 0.5f);
    
    // Extract digits (rightmost to leftmost)
    int digits[4];
    for (int i = 0; i < 4; i++) {
        digits[i] = int_value % 10;
        int_value /= 10;
    }
    
    // Build display segments with decimal point
    // digits[0]=rightmost (COM5), digits[3]=leftmost (COM8)
    // decimal_position is counted from right: 0=after rightmost digit
    int decimal_position = decimal_places;  // Position from right
    bool has_content = false;
    
    for (int i = 3; i >= 0; i--) {
        int digit_idx = i;  // Display position i uses digits[i]
        // Show digit if: at/right of decimal point, or non-zero, or it's the rightmost, or has content
        if (digit_idx <= decimal_position || digits[digit_idx] != 0 || digit_idx == 0 || has_content) {
            digit_values[i] = GetSegmentCode(digits[digit_idx]);
            // Add decimal point: position counted from right, 0=rightmost
            // For 0.005: decimal_position=3, so decimal on digits[3] (leftmost '0.')
            if (digit_idx == decimal_position) {
                digit_values[i] |= 0x80;  // Set decimal point bit
            }
            if (digits[digit_idx] != 0) has_content = true;
        } else {
            digit_values[i] = SEG_BLANK;
        }
    }
    
    // Handle negative sign (replace leftmost blank or push content)
    if (is_negative) {
        for (int i = 3; i >= 0; i--) {
            if (digit_values[i] == SEG_BLANK) {
                digit_values[i] = 0x40;  // Minus sign
                break;
            }
        }
    }
}

/**
 * @brief Process application logic
 * @note Call from main loop after Button_Process()
 */
void App_Process(uint32_t current_time)
{
    // Forward to application handler layer
    AppControl_ProcessContinuousAdjustment(current_time);
    AppControl_UpdateDisplay();
    AppState_CommitPendingWrites(current_time);
}

/**
 * @brief Get system state (for external access)
 */
SystemState_t* App_GetSystemState(void)
{
    return &g_system_state;
}

/* ============================================================================
   Display72128 Wrapper Functions (merged from display72128_wrapper)
   ============================================================================ */

void Display72128_Init(void)
{
    // Initialize display driver
    ZLG72128_Init(&g_display_handle, &hi2c2);
    
    // Set default brightness
    ZLG72128_SetBrightness(&g_display_handle, 4);
    
    // Clear all displays
    ZLG72128_ClearDisplay(&g_display_handle);
    
    // Initialize caches
    memset(g_group1_cache, 0xFF, sizeof(g_group1_cache));
    memset(g_group2_cache, 0xFF, sizeof(g_group2_cache));
    memset(g_group1_text_cache, 0, sizeof(g_group1_text_cache));
    memset(g_group2_text_cache, 0, sizeof(g_group2_text_cache));
    g_group1_flash_cache = false;
    g_group2_flash_cache = false;
}

void Display72128_RefreshCache(void)
{
    // Force full refresh by invalidating all caches
    memset(g_group1_cache, 0xFF, sizeof(g_group1_cache));
    memset(g_group2_cache, 0xFF, sizeof(g_group2_cache));
    memset(g_group1_text_cache, 0, sizeof(g_group1_text_cache));
    memset(g_group2_text_cache, 0, sizeof(g_group2_text_cache));
    g_group1_flash_cache = !g_group1_flash_cache;  // Force update
    g_group2_flash_cache = !g_group2_flash_cache;  // Force update
}

void Display72128_ShowGroup1(const char* text, bool flash_last_bit)
{
    if (text == NULL) return;
    
    // Check if text and flash state are identical to cache (no update needed)
    if (strcmp(text, g_group1_text_cache) == 0 && flash_last_bit == g_group1_flash_cache) {
        return;  // No change - skip I2C operations
    }
    
    // Update cache
    strncpy(g_group1_text_cache, text, sizeof(g_group1_text_cache) - 1);
    g_group1_flash_cache = flash_last_bit;
    
    uint8_t digit_values[5] = {SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK};
    
    // Check if text is "OFF"
    if (strcmp(text, "OFF") == 0) {
        // Display "OFF" on Group1 (positions 0-4, COM0-COM4)
        // Right-aligned: "  OFF" - COM2=O, COM1=F, COM0=F (rightmost)
        digit_values[0] = flash_last_bit ? SEG_BLANK : SEG_F;  // Rightmost COM0
        digit_values[1] = SEG_F;
        digit_values[2] = SEG_O;
    } else if (IsAnimationString(text)) {
        // Handle pure animation character strings (e.g., "-", "--", "---" or "8", "88", "888")
        // Display left-to-right: text[0] → COM4 (leftmost), text[4] → COM0 (rightmost)
        int len = strlen(text);
        for (int i = 0; i < len && i < 5; i++) {
            char ch = text[i];
            if (ch == '-' || ch == '8' || (ch >= '0' && ch <= '9')) {
                // Map: text[0]→COM4, text[1]→COM3, text[2]→COM2, text[3]→COM1, text[4]→COM0
                digit_values[4 - i] = GetSegmentCode(ch);
            }
        }
    } else {
        // Display number - parse and show right-aligned
        int value = atoi(text);
        
        // Extract digits (right-aligned)
        int digits[5] = {-1, -1, -1, -1, -1};
        int digit_count = 0;
        int temp_value = value;
        
        if (temp_value == 0) {
            digits[0] = 0;
            digit_count = 1;
        } else {
            while (temp_value > 0 && digit_count < 5) {
                digits[digit_count] = temp_value % 10;
                temp_value /= 10;
                digit_count++;
            }
        }
        
        // Build digit values from right to left (COM0 is rightmost)
        for (int i = 0; i < digit_count; i++) {
            if (i == 0 && flash_last_bit) {
                // Hide last digit (flash off)
                digit_values[i] = SEG_BLANK;
            } else {
                // Get 7-segment code for digit
                digit_values[i] = GetSegmentCode(digits[i]);
            }
        }
        
        // Explicitly ensure unused high-order positions are blank
        // (fixes issue where going from 5-digit to 4-digit leaves residual)
        for (int i = digit_count; i < 5; i++) {
            digit_values[i] = SEG_BLANK;
        }
    }
    
    // Force update all positions to ensure clean display (no residual digits)
    for (uint8_t i = 0; i < 5; i++) {
        ZLG72128_WriteDigit(&g_display_handle, i, digit_values[i]);
        g_group1_cache[i] = digit_values[i];
    }
}

void Display72128_ShowGroup2(const char* text, bool flash_last_bit)
{
    if (text == NULL) return;
    
    // Check if text and flash state are identical to cache (no update needed)
    if (strcmp(text, g_group2_text_cache) == 0 && flash_last_bit == g_group2_flash_cache) {
        return;  // No change - skip I2C operations
    }
    
    // Update cache
    strncpy(g_group2_text_cache, text, sizeof(g_group2_text_cache) - 1);
    g_group2_flash_cache = flash_last_bit;
    
    uint8_t digit_values[4] = {SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK};
    
    // Check if text is "OFF"
    if (strcmp(text, "OFF") == 0) {
        // Display "OFF" on Group2 (positions 5-8, COM5-COM8)
        // Right-aligned: " OFF" - COM7=O, COM6=F, COM5=F (rightmost)
        digit_values[0] = flash_last_bit ? SEG_BLANK : SEG_F;  // Rightmost COM5
        digit_values[1] = SEG_F;
        digit_values[2] = SEG_O;
    } else if (IsAnimationString(text)) {
        // Handle pure animation character strings (e.g., "-", "--", "---" or "8", "88", "888")
        // IsAnimationString() already filters out negative numbers like "-10.5"
        // Display left-to-right: text[0] → COM8 (leftmost), text[3] → COM5 (rightmost)
        int len = strlen(text);
        for (int i = 0; i < len && i < 4; i++) {
            // Map: text[0]→COM8, text[1]→COM7, text[2]→COM6, text[3]→COM5
            digit_values[3 - i] = GetSegmentCode(text[i]);
        }
    } else {
        // Display float number (e.g., "105.0" or "-10.0")
        float value = atof(text);
        
        // Handle negative sign
        bool is_negative = (value < 0.0f);
        if (is_negative) {
            value = -value;  // Work with absolute value
        }
        
        // Convert to integer (multiply by 10 to get XXX.X format)
        int int_value = (int)(value * 10.0f + 0.5f);  // Round
        
        // Extract digits for XXX.X format
        int digit_8 = int_value % 10;          // Tenths place (rightmost)
        int digit_7 = (int_value / 10) % 10;   // Ones place (with decimal point)
        int digit_6 = (int_value / 100) % 10;  // Tens place
        int digit_5 = (int_value / 1000) % 10; // Hundreds place
        
        // Display from right to left
        // Position 0 (COM5 = rightmost) - tenths digit
        if (flash_last_bit) {
            digit_values[0] = SEG_BLANK;
        } else {
            digit_values[0] = GetSegmentCode(digit_8);
        }
        
        // Position 1 (COM6) - ones digit with decimal point
        digit_values[1] = GetSegmentCode(digit_7) | 0x80;  // Add decimal point bit
        
        // Position 2 (COM7) - tens digit or negative sign
        if (is_negative && int_value < 100) {
            // For negative numbers < 10.0 (like -9.5), show negative sign here
            digit_values[2] = 0x40;  // Minus sign (segment G only)
        } else if (int_value >= 100) {
            // Show tens digit if >= 10.0
            digit_values[2] = GetSegmentCode(digit_6);
        } else {
            digit_values[2] = SEG_BLANK;
        }
        
        // Position 3 (COM8) - hundreds digit or negative sign
        if (is_negative && int_value >= 100) {
            // For negative numbers >= 10.0 (like -10.5), show negative sign here
            digit_values[3] = 0x40;  // Minus sign (segment G only)
        } else if (int_value >= 1000) {
            // Show hundreds digit if >= 100.0
            digit_values[3] = GetSegmentCode(digit_5);
        } else {
            digit_values[3] = SEG_BLANK;
        }
    }
    
    // Only update digits that changed (intelligent diff update)
    for (uint8_t i = 0; i < 4; i++) {
        if (digit_values[i] != g_group2_cache[i]) {
            ZLG72128_WriteDigit(&g_display_handle, i + 5, digit_values[i]);
            g_group2_cache[i] = digit_values[i];
        }
    }
}

/* ============================================================================
   Display72128 Wrapper Implementation
   ============================================================================ */

