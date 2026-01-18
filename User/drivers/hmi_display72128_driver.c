/**
 * @file hmi_display72128_driver.c
 * @brief ZLG72128 LED Display and Keyboard Driver Implementation
 * @author Your Name
 * @date December 6, 2025
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "hmi_display72128_driver.h"
#include "driver_printf_config.h"

/* Conditional printf for Display72128 module */
#if (ENABLE_DRV_PRINTF && ENABLE_DRV_PRINTF_DISPLAY72128)
    #define DISPLAY72128_PRINTF(...)    printf(__VA_ARGS__)
#else
    #define DISPLAY72128_PRINTF(...)    ((void)0)
#endif

/* Private define ------------------------------------------------------------*/

// 7-segment digit encoding lookup table
static const uint8_t digit_table[10] = {
    ZLG72128_SEG_0,  // 0
    ZLG72128_SEG_1,  // 1
    ZLG72128_SEG_2,  // 2
    ZLG72128_SEG_3,  // 3
    ZLG72128_SEG_4,  // 4
    ZLG72128_SEG_5,  // 5
    ZLG72128_SEG_6,  // 6
    ZLG72128_SEG_7,  // 7
    ZLG72128_SEG_8,  // 8
    ZLG72128_SEG_9   // 9
};

/* Private function prototypes -----------------------------------------------*/
static uint8_t Get7SegmentCode(uint8_t digit);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief Initialize ZLG72128 device
 */
ZLG72128_Status ZLG72128_Init(ZLG72128_Handle *handle, I2C_HandleTypeDef *hi2c)
{
    if (handle == NULL || hi2c == NULL)
    {
        return ZLG72128_ERROR;
    }

    // Initialize handle
    handle->hi2c = hi2c;
    handle->device_address = ZLG72128_I2C_ADDR << 1;  // Convert to 8-bit format for HAL
    handle->brightness = 4;  // Default medium brightness
    handle->decimal_point_pos = 0xFF;  // No decimal point
    handle->flash_tick = 0;
    handle->flash_state = true;
    handle->group1_flash_enabled = false;
    handle->group2_flash_enabled = false;
    memset(handle->display_buffer, 0, sizeof(handle->display_buffer));

    // Reset device
    if (ZLG72128_Reset(handle) != ZLG72128_OK)
    {
        DISPLAY72128_PRINTF("ZLG72128: Reset failed\r\n");
        return ZLG72128_ERROR;
    }

    // Clear display
    if (ZLG72128_ClearDisplay(handle) != ZLG72128_OK)
    {
        DISPLAY72128_PRINTF("ZLG72128: Clear display failed\r\n");
        return ZLG72128_ERROR;
    }

    // Set default brightness
    if (ZLG72128_SetBrightness(handle, handle->brightness) != ZLG72128_OK)
    {
        DISPLAY72128_PRINTF("ZLG72128: Set brightness failed\r\n");
        return ZLG72128_ERROR;
    }

    DISPLAY72128_PRINTF("ZLG72128: Initialized successfully\r\n");
    return ZLG72128_OK;
}

/**
 * @brief Reset ZLG72128 device
 */
ZLG72128_Status ZLG72128_Reset(ZLG72128_Handle *handle)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }

    uint8_t cmd = ZLG72128_CMD_RESET;
    HAL_StatusTypeDef status;

    status = HAL_I2C_Master_Transmit(handle->hi2c, handle->device_address, &cmd, 1, ZLG72128_I2C_TIMEOUT);

    if (status != HAL_OK)
    {
        return ZLG72128_ERROR;
    }

    HAL_Delay(10);  // Wait for reset to complete
    return ZLG72128_OK;
}

/**
 * @brief Set display brightness
 */
ZLG72128_Status ZLG72128_SetBrightness(ZLG72128_Handle *handle, uint8_t brightness)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }

    if (brightness > ZLG72128_BRIGHTNESS_MAX)
    {
        brightness = ZLG72128_BRIGHTNESS_MAX;
    }

    handle->brightness = brightness;
    
    // Write brightness to system register
    return ZLG72128_WriteRegister(handle, ZLG72128_REG_SYSTEM_REG, brightness);
}

/**
 * @brief Clear all display segments
 */
ZLG72128_Status ZLG72128_ClearDisplay(ZLG72128_Handle *handle)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }

    ZLG72128_Status status = ZLG72128_OK;

    // Clear all 9 digits
    for (uint8_t i = 0; i < 9; i++)
    {
        if (ZLG72128_WriteDigit(handle, i, ZLG72128_SEG_BLANK) != ZLG72128_OK)
        {
            status = ZLG72128_ERROR;
        }
        handle->display_buffer[i] = ZLG72128_SEG_BLANK;
    }

    handle->decimal_point_pos = 0xFF;  // Clear decimal point
    return status;
}

/**
 * @brief Write raw segment data to a specific digit position
 */
ZLG72128_Status ZLG72128_WriteDigit(ZLG72128_Handle *handle, uint8_t position, uint8_t segment_data)
{
    if (handle == NULL || position > 8)
    {
        return ZLG72128_ERROR;
    }

    uint8_t reg_addr = ZLG72128_DIG0_ADDR + position;
    return ZLG72128_WriteRegister(handle, reg_addr, segment_data);
}

/**
 * @brief Display a number on specified digit position
 */
ZLG72128_Status ZLG72128_DisplayNumber(ZLG72128_Handle *handle, uint8_t position, uint8_t number, bool show_dot)
{
    if (handle == NULL || position > 8 || number > 9)
    {
        return ZLG72128_ERROR;
    }

    uint8_t segment_data = Get7SegmentCode(number);
    
    if (show_dot)
    {
        segment_data |= ZLG72128_SEG_DOT;
    }

    handle->display_buffer[position] = segment_data;
    return ZLG72128_WriteDigit(handle, position, segment_data);
}

/**
 * @brief Display integer value across multiple digits
 */
ZLG72128_Status ZLG72128_DisplayInteger(ZLG72128_Handle *handle, int32_t value, uint8_t start_pos, uint8_t num_digits, bool leading_zero)
{
    if (handle == NULL || start_pos + num_digits > 9)
    {
        return ZLG72128_ERROR;
    }

    bool is_negative = (value < 0);
    if (is_negative)
    {
        value = -value;
    }

    // Extract digits from right to left
    uint8_t digits[10];
    uint8_t digit_count = 0;

    if (value == 0)
    {
        digits[0] = 0;
        digit_count = 1;
    }
    else
    {
        while (value > 0 && digit_count < num_digits)
        {
            digits[digit_count++] = value % 10;
            value /= 10;
        }
    }

    // Display digits from left to right
    uint8_t pos = start_pos;
    
    // Display minus sign if negative
    if (is_negative && pos < start_pos + num_digits)
    {
        ZLG72128_WriteDigit(handle, pos++, ZLG72128_SEG_MINUS);
    }

    // Display leading zeros if requested
    if (leading_zero)
    {
        for (int i = num_digits - 1; i >= (int)digit_count; i--)
        {
            if (pos < start_pos + num_digits)
            {
                ZLG72128_DisplayNumber(handle, pos++, 0, false);
            }
        }
    }
    else
    {
        // Fill with blanks
        for (int i = num_digits - 1; i >= (int)digit_count; i--)
        {
            if (pos < start_pos + num_digits && !is_negative)
            {
                ZLG72128_WriteDigit(handle, pos++, ZLG72128_SEG_BLANK);
            }
        }
    }

    // Display actual digits (from most significant to least)
    for (int i = digit_count - 1; i >= 0; i--)
    {
        if (pos < start_pos + num_digits)
        {
            ZLG72128_DisplayNumber(handle, pos++, digits[i], false);
        }
    }

    return ZLG72128_OK;
}

/**
 * @brief Display floating point value with decimal point
 */
ZLG72128_Status ZLG72128_DisplayFloat(ZLG72128_Handle *handle, float value, uint8_t start_pos, uint8_t num_digits, uint8_t decimal_places)
{
    if (handle == NULL || start_pos + num_digits > 9 || decimal_places >= num_digits)
    {
        return ZLG72128_ERROR;
    }

    bool is_negative = (value < 0.0f);
    if (is_negative)
    {
        value = -value;
    }

    // Calculate multiplier to convert to integer
    int32_t multiplier = 1;
    for (uint8_t i = 0; i < decimal_places; i++)
    {
        multiplier *= 10;
    }

    int32_t int_value = (int32_t)(value * multiplier + 0.5f);  // Round

    // Extract all digits
    uint8_t digits[10];
    uint8_t digit_count = 0;

    if (int_value == 0)
    {
        digits[0] = 0;
        digit_count = 1;
    }
    else
    {
        while (int_value > 0 && digit_count < num_digits)
        {
            digits[digit_count++] = int_value % 10;
            int_value /= 10;
        }
    }

    // Ensure we have enough digits for decimal places
    while (digit_count <= decimal_places)
    {
        digits[digit_count++] = 0;
    }

    // Display from left to right
    uint8_t pos = start_pos;

    // Display minus sign if negative
    if (is_negative && pos < start_pos + num_digits)
    {
        ZLG72128_WriteDigit(handle, pos++, ZLG72128_SEG_MINUS);
    }

    // Display integer part
    for (int i = digit_count - 1; i >= (int)decimal_places; i--)
    {
        if (pos < start_pos + num_digits)
        {
            bool show_dot = (i == (int)decimal_places);  // Show dot after this digit
            ZLG72128_DisplayNumber(handle, pos++, digits[i], show_dot);
            if (show_dot)
            {
                handle->decimal_point_pos = pos - 1;
            }
        }
    }

    // Display decimal part
    for (int i = decimal_places - 1; i >= 0; i--)
    {
        if (pos < start_pos + num_digits)
        {
            ZLG72128_DisplayNumber(handle, pos++, digits[i], false);
        }
    }

    return ZLG72128_OK;
}

/**
 * @brief Display integer value on Group 1 (COM0-COM4, 5 digits)
 */
ZLG72128_Status ZLG72128_DisplayGroup1_Integer(ZLG72128_Handle *handle, int32_t value, bool leading_zero)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }
    
    // Disable flash for static display
    handle->group1_flash_enabled = false;
    
    return ZLG72128_DisplayInteger(handle, value, 0, 5, leading_zero);
}

/**
 * @brief Display float value on Group 1 (COM0-COM4, 5 digits)
 */
ZLG72128_Status ZLG72128_DisplayGroup1_Float(ZLG72128_Handle *handle, float value, uint8_t decimal_places)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }
    
    // Disable flash for static display
    handle->group1_flash_enabled = false;
    
    if (decimal_places > 4)
    {
        decimal_places = 4;
    }
    return ZLG72128_DisplayFloat(handle, value, 0, 5, decimal_places);
}

/**
 * @brief Display integer value on Group 2 (COM5-COM8, 4 digits)
 */
ZLG72128_Status ZLG72128_DisplayGroup2_Integer(ZLG72128_Handle *handle, int32_t value, bool leading_zero)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }
    
    // Disable flash for static display
    handle->group2_flash_enabled = false;
    
    return ZLG72128_DisplayInteger(handle, value, 5, 4, leading_zero);
}

/**
 * @brief Display float value on Group 2 (COM5-COM8, 4 digits)
 */
ZLG72128_Status ZLG72128_DisplayGroup2_Float(ZLG72128_Handle *handle, float value, uint8_t decimal_places)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }
    
    // Disable flash for static display
    handle->group2_flash_enabled = false;
    
    if (decimal_places > 3)
    {
        decimal_places = 3;
    }
    return ZLG72128_DisplayFloat(handle, value, 5, 4, decimal_places);
}

/**
 * @brief Display integer value on Group 1 with flash (0.5s interval)
 */
ZLG72128_Status ZLG72128_DisplayGroup1_Integer_Flash(ZLG72128_Handle *handle, int32_t value, bool leading_zero)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }
    
    // Display the value
    ZLG72128_Status status = ZLG72128_DisplayInteger(handle, value, 0, 5, leading_zero);
    
    // Enable flash mode
    handle->group1_flash_enabled = true;
    handle->flash_tick = HAL_GetTick();
    
    return status;
}

/**
 * @brief Display float value on Group 1 with flash (0.5s interval)
 */
ZLG72128_Status ZLG72128_DisplayGroup1_Float_Flash(ZLG72128_Handle *handle, float value, uint8_t decimal_places)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }
    
    if (decimal_places > 4)
    {
        decimal_places = 4;
    }
    
    // Display the value
    ZLG72128_Status status = ZLG72128_DisplayFloat(handle, value, 0, 5, decimal_places);
    
    // Enable flash mode
    handle->group1_flash_enabled = true;
    handle->flash_tick = HAL_GetTick();
    
    return status;
}

/**
 * @brief Display integer value on Group 2 with flash (0.5s interval)
 */
ZLG72128_Status ZLG72128_DisplayGroup2_Integer_Flash(ZLG72128_Handle *handle, int32_t value, bool leading_zero)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }
    
    // Display the value
    ZLG72128_Status status = ZLG72128_DisplayInteger(handle, value, 5, 4, leading_zero);
    
    // Enable flash mode
    handle->group2_flash_enabled = true;
    handle->flash_tick = HAL_GetTick();
    
    return status;
}

/**
 * @brief Display float value on Group 2 with flash (0.5s interval)
 */
ZLG72128_Status ZLG72128_DisplayGroup2_Float_Flash(ZLG72128_Handle *handle, float value, uint8_t decimal_places)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }
    
    if (decimal_places > 3)
    {
        decimal_places = 3;
    }
    
    // Display the value
    ZLG72128_Status status = ZLG72128_DisplayFloat(handle, value, 5, 4, decimal_places);
    
    // Enable flash mode
    handle->group2_flash_enabled = true;
    handle->flash_tick = HAL_GetTick();
    
    return status;
}

/**
 * @brief Clear Group 1 display (COM0-COM4)
 */
ZLG72128_Status ZLG72128_ClearGroup1(ZLG72128_Handle *handle)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }

    ZLG72128_Status status = ZLG72128_OK;

    // Clear digits 0-4 (Group 1)
    for (uint8_t i = 0; i < 5; i++)
    {
        if (ZLG72128_WriteDigit(handle, i, ZLG72128_SEG_BLANK) != ZLG72128_OK)
        {
            status = ZLG72128_ERROR;
        }
        handle->display_buffer[i] = ZLG72128_SEG_BLANK;
    }

    return status;
}

/**
 * @brief Clear Group 2 display (COM5-COM8)
 */
ZLG72128_Status ZLG72128_ClearGroup2(ZLG72128_Handle *handle)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }

    ZLG72128_Status status = ZLG72128_OK;

    // Clear digits 5-8 (Group 2)
    for (uint8_t i = 5; i < 9; i++)
    {
        if (ZLG72128_WriteDigit(handle, i, ZLG72128_SEG_BLANK) != ZLG72128_OK)
        {
            status = ZLG72128_ERROR;
        }
        handle->display_buffer[i] = ZLG72128_SEG_BLANK;
    }

    return status;
}

/**
 * @brief Update flash state - call this periodically (e.g., in main loop or timer)
 */
ZLG72128_Status ZLG72128_UpdateFlash(ZLG72128_Handle *handle)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }

    // Check if flash is enabled for any group
    if (!handle->group1_flash_enabled && !handle->group2_flash_enabled)
    {
        return ZLG72128_OK;
    }

    // Get current tick
    uint32_t current_tick = HAL_GetTick();
    
    // Check if 500ms has elapsed
    if (current_tick - handle->flash_tick >= ZLG72128_FLASH_INTERVAL)
    {
        handle->flash_tick = current_tick;
        handle->flash_state = !handle->flash_state;
        
        // Update Group 1 if flash enabled
        if (handle->group1_flash_enabled)
        {
            for (uint8_t i = 0; i < 5; i++)
            {
                if (handle->flash_state)
                {
                    // Show display
                    ZLG72128_WriteDigit(handle, i, handle->display_buffer[i]);
                }
                else
                {
                    // Blank display
                    ZLG72128_WriteDigit(handle, i, ZLG72128_SEG_BLANK);
                }
            }
        }
        
        // Update Group 2 if flash enabled
        if (handle->group2_flash_enabled)
        {
            for (uint8_t i = 5; i < 9; i++)
            {
                if (handle->flash_state)
                {
                    // Show display
                    ZLG72128_WriteDigit(handle, i, handle->display_buffer[i]);
                }
                else
                {
                    // Blank display
                    ZLG72128_WriteDigit(handle, i, ZLG72128_SEG_BLANK);
                }
            }
        }
    }
    
    return ZLG72128_OK;
}

/**
 * @brief Set decimal point at specific position
 */
ZLG72128_Status ZLG72128_SetDecimalPoint(ZLG72128_Handle *handle, uint8_t position)
{
    if (handle == NULL || position > 8)
    {
        return ZLG72128_ERROR;
    }

    handle->decimal_point_pos = position;
    
    // Read current digit value and add dot
    uint8_t current = handle->display_buffer[position];
    current |= ZLG72128_SEG_DOT;
    
    return ZLG72128_WriteDigit(handle, position, current);
}

/**
 * @brief Clear decimal point
 */
ZLG72128_Status ZLG72128_ClearDecimalPoint(ZLG72128_Handle *handle)
{
    if (handle == NULL || handle->decimal_point_pos == 0xFF)
    {
        return ZLG72128_OK;
    }

    uint8_t position = handle->decimal_point_pos;
    
    // Read current digit value and remove dot
    uint8_t current = handle->display_buffer[position];
    current &= ~ZLG72128_SEG_DOT;
    
    handle->decimal_point_pos = 0xFF;
    return ZLG72128_WriteDigit(handle, position, current);
}

/**
 * @brief Read key data from ZLG72128
 */
ZLG72128_Status ZLG72128_ReadKey(ZLG72128_Handle *handle, uint8_t *key_data)
{
    if (handle == NULL || key_data == NULL)
    {
        return ZLG72128_ERROR;
    }

    return ZLG72128_ReadRegister(handle, ZLG72128_REG_KEY_DATA, key_data);
}

/**
 * @brief Test mode - light up all segments
 */
ZLG72128_Status ZLG72128_TestDisplay(ZLG72128_Handle *handle)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }

    ZLG72128_Status status = ZLG72128_OK;

    // Light up all segments (display 8 on all digits)
    for (uint8_t i = 0; i < 9; i++)
    {
        if (ZLG72128_DisplayNumber(handle, i, 8, true) != ZLG72128_OK)
        {
            status = ZLG72128_ERROR;
        }
    }

    return status;
}

/**
 * @brief Write data to ZLG72128 register
 */
ZLG72128_Status ZLG72128_WriteRegister(ZLG72128_Handle *handle, uint8_t reg_addr, uint8_t data)
{
    if (handle == NULL)
    {
        return ZLG72128_ERROR;
    }

    uint8_t buffer[2] = {reg_addr, data};
    HAL_StatusTypeDef status;

    status = HAL_I2C_Master_Transmit(handle->hi2c, handle->device_address, buffer, 2, ZLG72128_I2C_TIMEOUT);

    if (status != HAL_OK)
    {
        return ZLG72128_ERROR;
    }

    return ZLG72128_OK;
}

/**
 * @brief Read data from ZLG72128 register
 */
ZLG72128_Status ZLG72128_ReadRegister(ZLG72128_Handle *handle, uint8_t reg_addr, uint8_t *data)
{
    if (handle == NULL || data == NULL)
    {
        return ZLG72128_ERROR;
    }

    HAL_StatusTypeDef status;

    // Write register address
    status = HAL_I2C_Master_Transmit(handle->hi2c, handle->device_address, &reg_addr, 1, ZLG72128_I2C_TIMEOUT);
    if (status != HAL_OK)
    {
        return ZLG72128_ERROR;
    }

    // Read data
    status = HAL_I2C_Master_Receive(handle->hi2c, handle->device_address, data, 1, ZLG72128_I2C_TIMEOUT);
    if (status != HAL_OK)
    {
        return ZLG72128_ERROR;
    }

    return ZLG72128_OK;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Get 7-segment code for a digit
 */
static uint8_t Get7SegmentCode(uint8_t digit)
{
    if (digit > 9)
    {
        return ZLG72128_SEG_BLANK;
    }

    return digit_table[digit];
}
