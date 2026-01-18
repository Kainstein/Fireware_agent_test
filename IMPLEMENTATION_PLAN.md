# Universal Float Display Implementation Plan

## Changes Required:

### 1. hmi_wrapper.c - Add FormatFloatForDisplay function

Add after line 373 (after GetSegmentCode function):

```c
/**
 * @brief Universal float formatter - automatically determines decimal position
 * @param value Float value to format
 * @param digit_values Output array [rightmost COM5...leftmost COM8]
 * @note Handles: 0.005 (0.005), 10.5 (10.50), 105.0 (105.0), -10.5 (-10.5)
 */
static void FormatFloatForDisplay(float value, uint8_t* digit_values)
{
    bool is_negative = (value < 0.0f);
    if (is_negative) value = -value;
    
    // Determine optimal format based on magnitude
    int decimal_places;
    int multiplier;
    
    if (value >= 100.0f) {         // 100.0 - 999.9 → XXX.X format
        decimal_places = 1;
        multiplier = 10;
    } else if (value >= 10.0f) {   // 10.00 - 99.99 → XX.XX format
        decimal_places = 2;
        multiplier = 100;
    } else if (value >= 1.0f) {    // 1.000 - 9.999 → X.XXX format
        decimal_places = 3;
        multiplier = 1000;
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
    int decimal_position = decimal_places;  // Position from right
    bool has_content = false;
    
    for (int i = 3; i >= 0; i--) {
        // Show digit if: within decimal range, or non-zero, or it's the rightmost
        if (i < decimal_position || digits[i] != 0 || i == 0 || has_content) {
            digit_values[i] = GetSegmentCode(digits[i]);
            // Add decimal point after this digit (counting from right)
            if (i == decimal_position - 1) {
                digit_values[i] |= 0x80;  // Set decimal point bit
            }
            if (digits[i] != 0) has_content = true;
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
```

### 2. hmi_wrapper.h - Add function declaration

Add after line 144 (after Display72128_ShowGroup2 declaration):

```c
/**
 * @brief Format float with automatic decimal positioning for 4-digit display
 * @param value Float value to format (0.001 to 999.9)
 * @param digit_values Output array [COM5, COM6, COM7, COM8] (rightmost to leftmost)
 * @note Automatically determines optimal decimal placement based on value magnitude
 */
void Display72128_FormatFloat(float value, uint8_t* digit_values);
```

### 3. hmi_wrapper.c - Make function public

Change line 374 from:
```c
static void FormatFloatForDisplay(float value, uint8_t* digit_values)
```

To:
```c
void Display72128_FormatFloat(float value, uint8_t* digit_values)
```

### 4. hmi_wrapper.c - Export g_display_handle

Change line 47 from:
```c
static ZLG72128_Handle g_display_handle;
```

To:
```c
ZLG72128_Handle g_display_handle;  // Exported for external access
```

Add to hmi_wrapper.h after line 100:
```c
// External display handle for direct access
extern ZLG72128_Handle g_display_handle;
```

### 5. hmi_handler.c - Update startup animation

Replace lines 951-955 with:
```c
#elif STARTUP_ANIM_CHAR_MODE == 2
    char anim_char = '8';  // All segments for Group1
    // Display version using universal float formatter
    uint8_t version_digits[4];
    Display72128_FormatFloat(APP_VERSION, version_digits);
    
    // Write version to Group2 display (COM5-COM8)
    for (int i = 0; i < 4; i++) {
        ZLG72128_WriteDigit(&g_display_handle, 5 + i, version_digits[i]);
    }
```

