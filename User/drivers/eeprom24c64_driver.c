/**
 * @file eeprom24c64_driver.c
 * @brief Driver for M24C64 EEPROM using I2C3 on STM32H743
 * @supports both polling and interrupt modes
 */

#include "eeprom24c64_driver.h"
#include "driver_printf_config.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

/* Conditional printf for EEPROM24C64 module */
#if (ENABLE_DRV_PRINTF && ENABLE_DRV_PRINTF_EEPROM24C64)
    #define EEPROM24C64_PRINTF(...)    printf(__VA_ARGS__)
#else
    #define EEPROM24C64_PRINTF(...)    ((void)0)
#endif


// Address Format with A0=A1=A2=GND:
// Memory Type	Device Address	Binary Format	Hex Value
// Main EEPROM	g_m24c64_main_eeprom_addr	1010 0000	0xA0
// ID Page	id_device_address	1011 0000	0xB0

/* M24C64 EEPROM Defines */
#define M24C64_I2C_TIMEOUT        10000        // Timeout in ms
#define M24C64_WRITE_CYCLE_TIME   5           // Write cycle time in ms
#define M24C64_PAGE_SIZE          32          // EEPROM page size in bytes
#define M24C64_CAPACITY           8192        // EEPROM capacity in bytes (64 Kbit = 8 KB)

/* Additional defines for identification page */
#define M24C64_ID_PAGE_SIZE       32          // Identification page size (32 bytes)
#define M24C64_ID_PAGE_START_ADDR 0x00        // ID page starts at address 0x00
#define M24C64_ID_PAGE_END_ADDR   0x1F        // ID page ends at address 0x1F (31)


/* External variables */
extern I2C_HandleTypeDef hi2c3;

/* Global I2C device addresses - configured for A0=A1=A2=GND */
uint8_t g_m24c64_main_eeprom_addr = 0xA0;    // Main EEPROM memory address (1010 0000)
uint8_t g_m24c64_id_page_addr = 0xB0;        // Identification page address (1011 0000)

/* Note: m24c64_address has been replaced with g_m24c64_main_eeprom_addr throughout */

/* Private variables */
static volatile uint8_t eeprom_operation_complete = 0;
static volatile HAL_StatusTypeDef eeprom_last_status = HAL_OK;

/**
 * @brief Initialize the M24C64 EEPROM driver
 * @param device_address Last 3 bits of I2C device address (A2, A1, A0)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef M24C64_Init(void)
{
    /* Device addresses are now configured as global variables */
    /* g_m24c64_main_eeprom_addr = 0xA0 for A0=A1=A2=GND */
    /* g_m24c64_id_page_addr = 0xB0 for A0=A1=A2=GND */
    
    /* Check if main EEPROM device is ready */
    if (HAL_I2C_IsDeviceReady(&hi2c3, g_m24c64_main_eeprom_addr, 3, M24C64_I2C_TIMEOUT) != HAL_OK) {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}
/*******************************************************************************
 *                  Identification Page Interrupt Functions                    *
 *******************************************************************************/

/**
 * @brief Check if an address is valid for the identification page
 * @param id_address Address to check
 * @return 1 if valid, 0 if invalid
 */
uint8_t M24C64_IsValidIdAddress(uint8_t id_address)
{
    return (id_address <= M24C64_ID_PAGE_END_ADDR);
}

/*******************************************************************************
 *                      Identification Page Functions                          *
 *******************************************************************************/

/**
 * @brief Write a byte to the identification page using polling mode
 * @param id_address Address within ID page (0x00 to 0x1F)
 * @param data Data byte to write
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef M24C64_WriteIdPage_Polling(uint8_t id_address, uint8_t data)
{
    uint8_t buffer[2];
    HAL_StatusTypeDef status;
    
    /* Check if address is valid for ID page */
    if (id_address > M24C64_ID_PAGE_END_ADDR) {
        return HAL_ERROR;
    }
    
    /* For M24C64 ID page access, use device address 0xB0
     * The M24C64 ID page uses a fixed address of 0xB0 (1011 0000)
     * This is different from the main EEPROM which uses 0xA0-0xAF */
    uint8_t id_device_address = 0xB0;  // Fixed ID page device address
    
    /* First, check if the ID page device address is ready */
    status = HAL_I2C_IsDeviceReady(&hi2c3, id_device_address, 3, M24C64_I2C_TIMEOUT);
    if (status != HAL_OK) {
        return HAL_ERROR;  // ID page device not ready
    }
    
    /* Prepare buffer: address, data (only single byte address for ID page) */
    buffer[0] = id_address;  // ID page address (8-bit)
    buffer[1] = data;        // Data
    
    /* Write data to identification page */
    status = HAL_I2C_Master_Transmit(&hi2c3, id_device_address, buffer, 2, M24C64_I2C_TIMEOUT);
    
    /* Wait for write cycle to complete (5ms max) */
    if (status == HAL_OK) {
        HAL_Delay(M24C64_WRITE_CYCLE_TIME);
        
        /* Verify the device is ready after write operation */
        status = HAL_I2C_IsDeviceReady(&hi2c3, id_device_address, 10, M24C64_I2C_TIMEOUT);
    }
    
    return status;
}

/**
 * @brief Read a byte from the identification page using polling mode
 * @param id_address Address within ID page (0x00 to 0x1F)
 * @param pData Pointer to store read data
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef M24C64_ReadIdPage_Polling(uint8_t id_address, uint8_t *pData)
{
    uint8_t address_buffer[1];
    HAL_StatusTypeDef status;
    
    /* Check if address is valid for ID page */
    if (id_address > M24C64_ID_PAGE_END_ADDR) {
        return HAL_ERROR;
    }
    
    /* For M24C64 ID page access, use device address 0xB0 */
    uint8_t id_device_address = 0xB0;  // Fixed ID page device address
    
    /* First, check if the ID page device address is ready */
    status = HAL_I2C_IsDeviceReady(&hi2c3, id_device_address, 3, M24C64_I2C_TIMEOUT);
    if (status != HAL_OK) {
        return HAL_ERROR;  // ID page device not ready
    }
    
    /* Prepare address buffer */
    address_buffer[0] = id_address;  // ID page address (8-bit)
    
    /* Set memory pointer in ID page */
    status = HAL_I2C_Master_Transmit(&hi2c3, id_device_address, address_buffer, 1, M24C64_I2C_TIMEOUT);
    if (status != HAL_OK) {
        return status;
    }
    
    /* Read data from identification page */
    return HAL_I2C_Master_Receive(&hi2c3, id_device_address, pData, 1, M24C64_I2C_TIMEOUT);
}



/*******************************************************************************
 *                      Polling (Non-Interrupt) Mode Functions                 *
 *******************************************************************************/

/**
 * @brief Write a byte to EEPROM using polling mode
 * @param memory_address 16-bit memory address
 * @param data Data byte to write
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef M24C64_WriteByte_Polling(uint16_t memory_address, uint8_t data)
{
    uint8_t buffer[3];
    HAL_StatusTypeDef status;
    
    /* Check if address is valid */
    if (memory_address >= M24C64_CAPACITY) {
        return HAL_ERROR;
    }
    
    /* First, check if the main EEPROM device address is ready */
    status = HAL_I2C_IsDeviceReady(&hi2c3, g_m24c64_main_eeprom_addr, 3, M24C64_I2C_TIMEOUT);
    if (status != HAL_OK) {
        return HAL_ERROR;  // Main EEPROM device not ready
    }
    
    /* Prepare buffer: high byte address, low byte address, data */
    buffer[0] = (uint8_t)((memory_address >> 8) & 0xFF);   // Address MSB
    buffer[1] = (uint8_t)(memory_address & 0xFF);          // Address LSB
    buffer[2] = data;                                       // Data
    
    /* Write data to EEPROM */
    status = HAL_I2C_Master_Transmit(&hi2c3, g_m24c64_main_eeprom_addr, buffer, 3, M24C64_I2C_TIMEOUT);
    
    /* Wait for write cycle to complete using ACK polling */
    if (status == HAL_OK) {
        status = M24C64_WaitForWriteCompletion();
    }
    
    return status;
}

/**
 * @brief Read a byte from EEPROM using polling mode
 * @param memory_address 16-bit memory address
 * @param pData Pointer to store read data
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef M24C64_ReadByte_Polling(uint16_t memory_address, uint8_t *pData)
{
    uint8_t address_buffer[2];
    HAL_StatusTypeDef status;
    
    /* Check if address is valid */
    if (memory_address >= M24C64_CAPACITY) {
        return HAL_ERROR;
    }
    
    /* First, check if the main EEPROM device address is ready */
    status = HAL_I2C_IsDeviceReady(&hi2c3, g_m24c64_main_eeprom_addr, 3, M24C64_I2C_TIMEOUT);
    if (status != HAL_OK) {
        return HAL_ERROR;  // Main EEPROM device not ready
    }
    
    /* Prepare buffer: high byte address, low byte address */
    address_buffer[0] = (uint8_t)((memory_address >> 8) & 0xFF);   // Address MSB
    address_buffer[1] = (uint8_t)(memory_address & 0xFF);          // Address LSB
    
    /* Set memory pointer */
    status = HAL_I2C_Master_Transmit(&hi2c3, g_m24c64_main_eeprom_addr, address_buffer, 2, M24C64_I2C_TIMEOUT);
    if (status != HAL_OK) {
        return status;
    }
    
    /* Read data from EEPROM */
    return HAL_I2C_Master_Receive(&hi2c3, g_m24c64_main_eeprom_addr, pData, 1, M24C64_I2C_TIMEOUT);
}

/**
 * @brief Write multiple bytes to EEPROM using polling mode
 * @param memory_address Starting address (must be aligned to page boundary for best performance)
 * @param pData Pointer to data buffer
 * @param size Size of data (max M24C64_PAGE_SIZE)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef M24C64_WriteBytes_Polling(uint16_t memory_address, uint8_t *pData, uint16_t size)
{
    HAL_StatusTypeDef status;
    
    /* Check if size is valid */
    if (size > M24C64_PAGE_SIZE) {
        size = M24C64_PAGE_SIZE;
    }
    
    /* Use static buffer for address + data (max 32 + 2 = 34 bytes) */
    static uint8_t buffer[M24C64_PAGE_SIZE + 2];
    
    /* Prepare buffer: high byte address, low byte address, data */
    buffer[0] = (uint8_t)((memory_address >> 8) & 0xFF);   // Address MSB
    buffer[1] = (uint8_t)(memory_address & 0xFF);          // Address LSB
    memcpy(&buffer[2], pData, size);                       // Data
    
    /* Write data to EEPROM */
    status = HAL_I2C_Master_Transmit(&hi2c3, g_m24c64_main_eeprom_addr, buffer, size + 2, M24C64_I2C_TIMEOUT);
    
    /* Wait for write cycle to complete (5ms max) */
    if (status == HAL_OK) {
        HAL_Delay(M24C64_WRITE_CYCLE_TIME);
    }
    
    return status;
}

/**
 * @brief Read multiple bytes from EEPROM using polling mode
 * @param memory_address 16-bit memory address
 * @param pData Pointer to store read data
 * @param size Number of bytes to read
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef M24C64_ReadBytes_Polling(uint16_t memory_address, uint8_t *pData, uint16_t size)
{
    uint8_t address_buffer[2];
    HAL_StatusTypeDef status;
    
    /* Check if size is valid */
    if (size > M24C64_CAPACITY - memory_address) {
        size = M24C64_CAPACITY - memory_address;
    }
    
    /* Prepare buffer: high byte address, low byte address */
    address_buffer[0] = (uint8_t)((memory_address >> 8) & 0xFF);   // Address MSB
    address_buffer[1] = (uint8_t)(memory_address & 0xFF);          // Address LSB
    
    /* Set memory pointer */
    status = HAL_I2C_Master_Transmit(&hi2c3, g_m24c64_main_eeprom_addr, address_buffer, 2, M24C64_I2C_TIMEOUT);
    if (status != HAL_OK) {
        return status;
    }
    
    /* Read data from EEPROM */
    return HAL_I2C_Master_Receive(&hi2c3, g_m24c64_main_eeprom_addr, pData, size, M24C64_I2C_TIMEOUT);
}

/*******************************************************************************
 *                      Interrupt Mode Functions                               *
 *******************************************************************************/

/**
 * @brief I2C operation complete callback
 * @param hi2c Pointer to I2C handle
 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C3) {
        eeprom_operation_complete = 1;
        eeprom_last_status = HAL_OK;
    }
}

/**
 * @brief I2C reception complete callback
 * @param hi2c Pointer to I2C handle
 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C3) {
        eeprom_operation_complete = 1;
        eeprom_last_status = HAL_OK;
    }
}



/**
 * @brief I2C error callback
 * @param hi2c Pointer to I2C handle
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C3) {
        eeprom_operation_complete = 1;
        eeprom_last_status = HAL_ERROR;
    }
}



/**
 * @brief Wait for EEPROM operation to complete
 * @param timeout Timeout in milliseconds
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef M24C64_WaitForOperation(uint32_t timeout)
{
    uint32_t start_time = HAL_GetTick();
    
    while (!eeprom_operation_complete) {
        if (HAL_GetTick() - start_time > timeout) {
            return HAL_TIMEOUT;
        }
    }
    
    eeprom_operation_complete = 0;
    return eeprom_last_status;
}

/**
 * @brief Write a byte to EEPROM using interrupt mode
 * @param memory_address 16-bit memory address
 * @param data Data byte to write
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef M24C64_WriteByte_IT(uint16_t memory_address, uint8_t data)
{
    static uint8_t buffer[3];
    HAL_StatusTypeDef status;
    
    /* Prepare buffer: high byte address, low byte address, data */
    buffer[0] = (uint8_t)((memory_address >> 8) & 0xFF);   // Address MSB
    buffer[1] = (uint8_t)(memory_address & 0xFF);          // Address LSB
    buffer[2] = data;                                       // Data
    
    /* Reset operation flag */
    eeprom_operation_complete = 0;
    
    /* Write data to EEPROM */
    status = HAL_I2C_Master_Transmit_IT(&hi2c3, g_m24c64_main_eeprom_addr, buffer, 3);
    
    return status;
}

/**
 * @brief Read a byte from EEPROM using interrupt mode
 * @param memory_address 16-bit memory address
 * @param pData Pointer to store read data
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef M24C64_ReadByte_IT(uint16_t memory_address, uint8_t *pData)
{
    static uint8_t address_buffer[2];
    HAL_StatusTypeDef status;
    
    /* Prepare buffer: high byte address, low byte address */
    address_buffer[0] = (uint8_t)((memory_address >> 8) & 0xFF);   // Address MSB
    address_buffer[1] = (uint8_t)(memory_address & 0xFF);          // Address LSB
    
    /* Reset operation flag */
    eeprom_operation_complete = 0;
    
    /* Set memory pointer */
    status = HAL_I2C_Master_Transmit_IT(&hi2c3, g_m24c64_main_eeprom_addr, address_buffer, 2);
    if (status != HAL_OK) {
        return status;
    }
    
    /* Wait for transmit to complete */
    status = M24C64_WaitForOperation(M24C64_I2C_TIMEOUT);
    if (status != HAL_OK) {
        return status;
    }
    
    /* Reset operation flag */
    eeprom_operation_complete = 0;
    
    /* Read data from EEPROM */
    return HAL_I2C_Master_Receive_IT(&hi2c3, g_m24c64_main_eeprom_addr, pData, 1);
}

/**
 * @brief Write a page to EEPROM using interrupt mode
 * @param memory_address Starting address (must be aligned to page boundary for best performance)
 * @param pData Pointer to data buffer
 * @param size Size of data (max M24C64_PAGE_SIZE)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef M24C64_WritePage_IT(uint16_t memory_address, uint8_t *pData, uint16_t size)
{
    static uint8_t buffer[M24C64_PAGE_SIZE + 2];  // Address bytes + page size
    HAL_StatusTypeDef status;
    
    /* Check if size is valid */
    if (size > M24C64_PAGE_SIZE) {
        size = M24C64_PAGE_SIZE;
    }
    
    /* Prepare buffer: high byte address, low byte address, data */
    buffer[0] = (uint8_t)((memory_address >> 8) & 0xFF);   // Address MSB
    buffer[1] = (uint8_t)(memory_address & 0xFF);          // Address LSB
    memcpy(&buffer[2], pData, size);                       // Data
    
    /* Reset operation flag */
    eeprom_operation_complete = 0;
    
    /* Write data to EEPROM */
    status = HAL_I2C_Master_Transmit_IT(&hi2c3, g_m24c64_main_eeprom_addr, buffer, size + 2);
    
    return status;
}

/**
 * @brief Read multiple bytes from EEPROM using interrupt mode
 * @param memory_address 16-bit memory address
 * @param pData Pointer to store read data
 * @param size Number of bytes to read
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef M24C64_ReadBytes_IT(uint16_t memory_address, uint8_t *pData, uint16_t size)
{
    static uint8_t address_buffer[2];
    HAL_StatusTypeDef status;
    
    /* Check if size is valid */
    if (size > M24C64_CAPACITY - memory_address) {
        size = M24C64_CAPACITY - memory_address;
    }
    
    /* Prepare buffer: high byte address, low byte address */
    address_buffer[0] = (uint8_t)((memory_address >> 8) & 0xFF);   // Address MSB
    address_buffer[1] = (uint8_t)(memory_address & 0xFF);          // Address LSB
    
    /* Reset operation flag */
    eeprom_operation_complete = 0;
    
    /* Set memory pointer */
    status = HAL_I2C_Master_Transmit_IT(&hi2c3, g_m24c64_main_eeprom_addr, address_buffer, 2);
    if (status != HAL_OK) {
        return status;
    }
    
    /* Wait for transmit to complete */
    status = M24C64_WaitForOperation(M24C64_I2C_TIMEOUT);
    if (status != HAL_OK) {
        return status;
    }
    
    /* Reset operation flag */
    eeprom_operation_complete = 0;
    
    /* Read data from EEPROM */
    return HAL_I2C_Master_Receive_IT(&hi2c3, g_m24c64_main_eeprom_addr, pData, size);
}

/**
 * @brief Wait for write cycle to complete (polling ACK from device)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef M24C64_WaitForWriteCompletion(void)
{
    uint32_t start_time = HAL_GetTick();
    HAL_StatusTypeDef status;
    
    /* Wait for device to be ready by polling ACK */
    while ((status = HAL_I2C_IsDeviceReady(&hi2c3, g_m24c64_main_eeprom_addr, 1, 1)) != HAL_OK) {
        if (HAL_GetTick() - start_time > M24C64_I2C_TIMEOUT) {
            return HAL_TIMEOUT;
        }
    }
    
    return HAL_OK;
}

void I2C_BusRecovery(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    uint8_t i;
    
    // Configure SCL and SDA pins as GPIO output open drain
    GPIO_InitStruct.Pin = I2C3_SCL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(I2C3_SCL_GPIO_Port, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = I2C3_SDA_Pin;
    HAL_GPIO_Init(I2C3_SDA_GPIO_Port, &GPIO_InitStruct);
    
    // Set SDA high
    HAL_GPIO_WritePin(I2C3_SDA_GPIO_Port, I2C3_SDA_Pin, GPIO_PIN_SET);
    
    // Toggle SCL 9 times to release stuck devices
    for (i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(I2C3_SCL_GPIO_Port, I2C3_SCL_Pin, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(I2C3_SCL_GPIO_Port, I2C3_SCL_Pin, GPIO_PIN_SET);
        HAL_Delay(1);
    }
    
    // Generate STOP condition
    HAL_GPIO_WritePin(I2C3_SDA_GPIO_Port, I2C3_SDA_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(I2C3_SCL_GPIO_Port, I2C3_SCL_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(I2C3_SDA_GPIO_Port, I2C3_SDA_Pin, GPIO_PIN_SET);
    
    // Re-initialize I2C3
//    HAL_I2C_DeInit(&hi2c3);
//    MX_I2C3_Init(); // Re-initialize I2C3 - call your I2C3 initialization function
}

/**
 * @brief Check if main EEPROM device is ready
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef M24C64_IsMainDeviceReady(void)
{
    return HAL_I2C_IsDeviceReady(&hi2c3, g_m24c64_main_eeprom_addr, 3, M24C64_I2C_TIMEOUT);
}


/**
 * @brief Test I2C line states and pull-up effectiveness
 */
void M24C64_TestI2CLines(void)
{
    HAL_StatusTypeDef status;
    int success_count = 0;
    
    // Test device ready multiple times to check consistency
    for (int i = 0; i < 10; i++) {
        status = HAL_I2C_IsDeviceReady(&hi2c3, g_m24c64_main_eeprom_addr, 1, 1000);
        if (status == HAL_OK) {
            success_count++;
        }
        HAL_Delay(10);
    }
    __nop();
    if (success_count == 10) {
        __nop();  // Breakpoint: Excellent - 100% success rate (good pull-ups)
    } else if (success_count >= 7) {
        __nop();  // Breakpoint: Good - >70% success rate (marginal pull-ups)
    } else if (success_count >= 3) {
        __nop();  // Breakpoint: Poor - 30-70% success rate (weak pull-ups)
    } else {
        __nop();  // Breakpoint: Failed - <30% success rate (no/bad pull-ups)
    }
}

/*******************************************************************************
 *                      Convenience Wrapper Functions                          *
 *******************************************************************************/

/**
 * @brief Write multiple bytes to EEPROM (simplified API wrapper)
 * @param address Starting memory address
 * @param pData Pointer to data buffer
 * @param size Number of bytes to write
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef EEPROM_WriteBytes(uint16_t address, uint8_t *pData, uint16_t size)
{
    return M24C64_WriteBytes_Polling(address, pData, size);
}

/**
 * @brief Read multiple bytes from EEPROM (simplified API wrapper)
 * @param address Starting memory address
 * @param pData Pointer to buffer for read data
 * @param size Number of bytes to read
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef EEPROM_ReadBytes(uint16_t address, uint8_t *pData, uint16_t size)
{
    return M24C64_ReadBytes_Polling(address, pData, size);
}








