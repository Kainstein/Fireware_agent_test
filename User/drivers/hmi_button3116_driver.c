/**
 * @file hmi_button3116_driver.c
 * @brief CY8CMBR3116 Capacitive Touch Controller Driver Implementation
 * @author Your Name
 * @date December 7, 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "hmi_button3116_driver.h"
#include "driver_printf_config.h"
#include <stdio.h>
#include <string.h>

/* Conditional printf for Button3116 module */
#if (ENABLE_DRV_PRINTF && ENABLE_DRV_PRINTF_BUTTON3116)
    #define BUTTON3116_PRINTF(...)    printf(__VA_ARGS__)
#else
    #define BUTTON3116_PRINTF(...)    ((void)0)
#endif

/* Private function prototypes -----------------------------------------------*/
static CY8CMBR3116_Status WaitCommandComplete(CY8CMBR3116_Handle *handle);
static void ClearI2CBus(I2C_HandleTypeDef *hi2c);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief Initialize CY8CMBR3116 device
 */
CY8CMBR3116_Status CY8CMBR3116_Init(CY8CMBR3116_Handle *handle, I2C_HandleTypeDef *hi2c, uint8_t device_addr)
{
    if (handle == NULL || hi2c == NULL)
    {
        return CY8CMBR3116_ERROR;
    }

    // Initialize handle
    handle->hi2c = hi2c;
    handle->device_address = device_addr << 1;  // Convert to 8-bit format for HAL
    handle->button_status = 0;
    handle->button_status_prev = 0;
    handle->proximity_status = 0;

    // Extended power-up delay for device stabilization
    HAL_Delay(200);

    // Clear I2C bus if stuck
    ClearI2CBus(hi2c);

    // Retry mechanism for device detection
    uint8_t retry_count = 0;
    const uint8_t max_retries = 3;
    CY8CMBR3116_Status status = CY8CMBR3116_ERROR;

    while (retry_count < max_retries)
    {
        // Check device presence
        status = CY8CMBR3116_CheckDevice(handle);
        
        if (status == CY8CMBR3116_OK)
        {
            // Device detected, check if it's ready
            uint8_t sys_status = 0;
            HAL_Delay(50);  // Brief delay before status check
            
            if (CY8CMBR3116_ReadSystemStatus(handle, &sys_status) == CY8CMBR3116_OK)
            {
                // Check for critical errors
                if (sys_status & CY8CMBR3116_SYS_STAT_CONFIG_ERR)
                {
                    BUTTON3116_PRINTF("CY8CMBR3116: Warning - Configuration error\r\n");
                }
                
                if (sys_status & CY8CMBR3116_SYS_STAT_WDT)
                {
                    BUTTON3116_PRINTF("CY8CMBR3116: Warning - Watchdog reset\r\n");
                }
                
                // Device detected and responding
                break;
            }
        }
        
        retry_count++;
        
        if (retry_count < max_retries)
        {
            HAL_Delay(100);
            ClearI2CBus(hi2c);
        }
    }

    if (status != CY8CMBR3116_OK)
    {
        BUTTON3116_PRINTF("CY8CMBR3116: Initialization failed after %d attempts\r\n", max_retries);
        return CY8CMBR3116_ERROR;
    }

    // Read initial button status
    if (CY8CMBR3116_ReadButtonStatus(handle, &handle->button_status) == CY8CMBR3116_OK)
    {
        handle->button_status_prev = handle->button_status;
    }

    BUTTON3116_PRINTF("CY8CMBR3116: Initialized successfully\r\n");
    return CY8CMBR3116_OK;
}

/**
 * @brief Check if device is present and read device ID
 */
CY8CMBR3116_Status CY8CMBR3116_CheckDevice(CY8CMBR3116_Handle *handle)
{
    if (handle == NULL)
    {
        return CY8CMBR3116_ERROR;
    }

    uint8_t family_id, device_id[2];

    // Read Family ID
    if (CY8CMBR3116_ReadRegister(handle, CY8CMBR3116_REG_FAMILY_ID, &family_id, 1) != CY8CMBR3116_OK)
    {
        BUTTON3116_PRINTF("CY8CMBR3116: Failed to read Family ID register\r\n");
        return CY8CMBR3116_ERROR;
    }

    // Read Device ID (2 bytes)
    if (CY8CMBR3116_ReadRegister(handle, CY8CMBR3116_REG_DEVICE_ID, device_id, 2) != CY8CMBR3116_OK)
    {
        BUTTON3116_PRINTF("CY8CMBR3116: Failed to read Device ID register\r\n");
        return CY8CMBR3116_ERROR;
    }

    BUTTON3116_PRINTF("CY8CMBR3116: Family ID=0x%02X, Device ID=0x%02X%02X\r\n", 
           family_id, device_id[0], device_id[1]);

    // Verify Family ID (must be Cypress CapSense family)
    if (family_id != CY8CMBR3116_FAMILY_ID)
    {
        BUTTON3116_PRINTF("CY8CMBR3116: Invalid Family ID 0x%02X\r\n", family_id);
        return CY8CMBR3116_ERROR;
    }
    
    // Check if device ID matches any supported Cypress CapSense device
    bool device_supported = false;
    
    if (device_id[0] == CY8CMBR3116_DEVICE_ID_H && device_id[1] == CY8CMBR3116_DEVICE_ID_L)
    {
        BUTTON3116_PRINTF("CY8CMBR3116: Detected CY8CMBR3116\r\n");
        device_supported = true;
    }
    else if (device_id[0] == CY8CMBR3108_DEVICE_ID_H && device_id[1] == CY8CMBR3108_DEVICE_ID_L)
    {
        BUTTON3116_PRINTF("CY8CMBR3116: Detected CY8CMBR3108/3110\r\n");
        device_supported = true;
    }
    
    if (!device_supported)
    {
        BUTTON3116_PRINTF("CY8CMBR3116: Unknown device 0x%02X%02X, attempting init\r\n", device_id[0], device_id[1]);
    }

    return CY8CMBR3116_OK;
}

/**
 * @brief Software reset the device
 */
CY8CMBR3116_Status CY8CMBR3116_SoftwareReset(CY8CMBR3116_Handle *handle)
{
    if (handle == NULL)
    {
        return CY8CMBR3116_ERROR;
    }

    CY8CMBR3116_Status status = CY8CMBR3116_SendCommand(handle, CY8CMBR3116_CMD_SW_RESET);
    
    if (status == CY8CMBR3116_OK)
    {
        HAL_Delay(100);  // Wait for reset to complete
    }

    return status;
}

/**
 * @brief Read button status (16 buttons)
 */
CY8CMBR3116_Status CY8CMBR3116_ReadButtonStatus(CY8CMBR3116_Handle *handle, uint16_t *status)
{
    if (handle == NULL || status == NULL)
    {
        return CY8CMBR3116_ERROR;
    }

    uint8_t data[2];

    // Store previous status
    handle->button_status_prev = handle->button_status;

    // Read button status (2 bytes)
    if (CY8CMBR3116_ReadRegister(handle, CY8CMBR3116_REG_BUTTON_STAT, data, 2) != CY8CMBR3116_OK)
    {
        return CY8CMBR3116_ERROR;
    }

    // Combine bytes (little-endian)
    *status = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    handle->button_status = *status;

    return CY8CMBR3116_OK;
}

/**
 * @brief Check if specific button is pressed
 */
CY8CMBR3116_TouchState CY8CMBR3116_IsButtonPressed(CY8CMBR3116_Handle *handle, uint8_t button)
{
    if (handle == NULL || button > 15)
    {
        return TOUCH_RELEASED;
    }

    return (handle->button_status & (1 << button)) ? TOUCH_PRESSED : TOUCH_RELEASED;
}

/**
 * @brief Check if any button state changed
 */
bool CY8CMBR3116_ButtonChanged(CY8CMBR3116_Handle *handle)
{
    if (handle == NULL)
    {
        return false;
    }

    return (handle->button_status != handle->button_status_prev);
}

/**
 * @brief Get which button was just pressed (edge detection)
 */
CY8CMBR3116_Status CY8CMBR3116_GetPressedButton(CY8CMBR3116_Handle *handle, uint8_t *button)
{
    if (handle == NULL || button == NULL)
    {
        return CY8CMBR3116_ERROR;
    }

    // Find buttons that changed from 0 to 1 (pressed)
    uint16_t pressed = handle->button_status & ~handle->button_status_prev;

    if (pressed == 0)
    {
        *button = 0xFF;  // No button pressed
        return CY8CMBR3116_OK;
    }

    // Find first pressed button
    for (uint8_t i = 0; i < 16; i++)
    {
        if (pressed & (1 << i))
        {
            *button = i;
            return CY8CMBR3116_OK;
        }
    }

    *button = 0xFF;
    return CY8CMBR3116_OK;
}

/**
 * @brief Get which button was just released (edge detection)
 */
CY8CMBR3116_Status CY8CMBR3116_GetReleasedButton(CY8CMBR3116_Handle *handle, uint8_t *button)
{
    if (handle == NULL || button == NULL)
    {
        return CY8CMBR3116_ERROR;
    }

    // Find buttons that changed from 1 to 0 (released)
    uint16_t released = ~handle->button_status & handle->button_status_prev;

    if (released == 0)
    {
        *button = 0xFF;  // No button released
        return CY8CMBR3116_OK;
    }

    // Find first released button
    for (uint8_t i = 0; i < 16; i++)
    {
        if (released & (1 << i))
        {
            *button = i;
            return CY8CMBR3116_OK;
        }
    }

    *button = 0xFF;
    return CY8CMBR3116_OK;
}

/**
 * @brief Read proximity status
 */
CY8CMBR3116_Status CY8CMBR3116_ReadProximityStatus(CY8CMBR3116_Handle *handle, uint8_t *proximity)
{
    if (handle == NULL || proximity == NULL)
    {
        return CY8CMBR3116_ERROR;
    }

    if (CY8CMBR3116_ReadRegister(handle, CY8CMBR3116_REG_PROX_STAT, proximity, 1) != CY8CMBR3116_OK)
    {
        return CY8CMBR3116_ERROR;
    }

    handle->proximity_status = *proximity;
    return CY8CMBR3116_OK;
}

/**
 * @brief Enable/disable specific sensors
 */
CY8CMBR3116_Status CY8CMBR3116_SetSensorEnable(CY8CMBR3116_Handle *handle, uint16_t sensor_mask)
{
    if (handle == NULL)
    {
        return CY8CMBR3116_ERROR;
    }

    uint8_t data[2];
    data[0] = sensor_mask & 0xFF;
    data[1] = (sensor_mask >> 8) & 0xFF;

    return CY8CMBR3116_WriteRegister(handle, CY8CMBR3116_REG_SENSOR_EN, data, 2);
}

/**
 * @brief Set sensitivity for all sensors
 */
CY8CMBR3116_Status CY8CMBR3116_SetSensitivity(CY8CMBR3116_Handle *handle, uint8_t sensitivity)
{
    if (handle == NULL || sensitivity > 3)
    {
        return CY8CMBR3116_ERROR;
    }

    uint8_t data[3];
    
    // Set same sensitivity for all sensors (3 bytes, 2 bits per sensor group)
    memset(data, sensitivity, 3);

    return CY8CMBR3116_WriteRegister(handle, CY8CMBR3116_REG_SENSITIVITY, data, 3);
}

/**
 * @brief Clear latched button status
 */
CY8CMBR3116_Status CY8CMBR3116_ClearLatchedButtons(CY8CMBR3116_Handle *handle)
{
    return CY8CMBR3116_SendCommand(handle, CY8CMBR3116_CMD_CLEAR_LATCHED);
}

/**
 * @brief Reset sensor baseline
 */
CY8CMBR3116_Status CY8CMBR3116_ResetBaseline(CY8CMBR3116_Handle *handle)
{
    return CY8CMBR3116_SendCommand(handle, CY8CMBR3116_CMD_RESET_BASELINE);
}

/**
 * @brief Read system status register
 */
CY8CMBR3116_Status CY8CMBR3116_ReadSystemStatus(CY8CMBR3116_Handle *handle, uint8_t *status)
{
    if (handle == NULL || status == NULL)
    {
        return CY8CMBR3116_ERROR;
    }

    return CY8CMBR3116_ReadRegister(handle, CY8CMBR3116_REG_SYSTEM_STATUS, status, 1);
}

/**
 * @brief Send command to device
 */
CY8CMBR3116_Status CY8CMBR3116_SendCommand(CY8CMBR3116_Handle *handle, uint8_t command)
{
    if (handle == NULL)
    {
        return CY8CMBR3116_ERROR;
    }

    // Write command to control register
    if (CY8CMBR3116_WriteRegister(handle, CY8CMBR3116_REG_CTRL_CMD, &command, 1) != CY8CMBR3116_OK)
    {
        return CY8CMBR3116_ERROR;
    }

    // Wait for command to complete (except for reset)
    if (command != CY8CMBR3116_CMD_SW_RESET)
    {
        return WaitCommandComplete(handle);
    }

    return CY8CMBR3116_OK;
}

/**
 * @brief Write data to CY8CMBR3116 register
 */
CY8CMBR3116_Status CY8CMBR3116_WriteRegister(CY8CMBR3116_Handle *handle, uint8_t reg_addr, uint8_t *data, uint16_t length)
{
    if (handle == NULL || data == NULL)
    {
        return CY8CMBR3116_ERROR;
    }

    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Write(handle->hi2c, handle->device_address, reg_addr, 
                               I2C_MEMADD_SIZE_8BIT, data, length, CY8CMBR3116_I2C_TIMEOUT);

    if (status != HAL_OK)
    {
        BUTTON3116_PRINTF("CY8CMBR3116: I2C Write Error - HAL Status=%d, I2C ErrorCode=0x%08lX, Reg=0x%02X\r\n", 
               status, handle->hi2c->ErrorCode, reg_addr);
        return CY8CMBR3116_ERROR;
    }

    return CY8CMBR3116_OK;
}

/**
 * @brief Read data from CY8CMBR3116 register
 */
CY8CMBR3116_Status CY8CMBR3116_ReadRegister(CY8CMBR3116_Handle *handle, uint8_t reg_addr, uint8_t *data, uint16_t length)
{
    if (handle == NULL || data == NULL)
    {
        return CY8CMBR3116_ERROR;
    }

    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Read(handle->hi2c, handle->device_address, reg_addr, 
                              I2C_MEMADD_SIZE_8BIT, data, length, CY8CMBR3116_I2C_TIMEOUT);

    if (status != HAL_OK)
    {
        BUTTON3116_PRINTF("CY8CMBR3116: I2C Read Error - HAL Status=%d, I2C ErrorCode=0x%08lX, Reg=0x%02X\r\n", 
               status, handle->hi2c->ErrorCode, reg_addr);
        return CY8CMBR3116_ERROR;
    }

    return CY8CMBR3116_OK;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Wait for command to complete
 */
static CY8CMBR3116_Status WaitCommandComplete(CY8CMBR3116_Handle *handle)
{
    uint8_t cmd_status;
    uint32_t timeout = HAL_GetTick() + CY8CMBR3116_CMD_TIMEOUT;

    do
    {
        if (CY8CMBR3116_ReadRegister(handle, CY8CMBR3116_REG_CTRL_CMD, &cmd_status, 1) != CY8CMBR3116_OK)
        {
            return CY8CMBR3116_ERROR;
        }

        // Command complete when register returns to 0
        if (cmd_status == 0)
        {
            return CY8CMBR3116_OK;
        }

        HAL_Delay(1);

    } while (HAL_GetTick() < timeout);

    return CY8CMBR3116_TIMEOUT;
}

/**
 * @brief Clear I2C bus if stuck (clock stretching or SDA stuck low)
 * @note This performs a software I2C bus recovery by toggling clock lines
 */
static void ClearI2CBus(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == NULL)
    {
        return;
    }

    // Store I2C state
    uint32_t temp_cr1 = hi2c->Instance->CR1;
    
    // Disable I2C peripheral
    hi2c->Instance->CR1 &= ~I2C_CR1_PE;
    
    // Wait for peripheral to be disabled
    HAL_Delay(2);
    
    // Re-enable I2C peripheral
    hi2c->Instance->CR1 = temp_cr1;
    
    // Small delay to stabilize
    HAL_Delay(5);
}
