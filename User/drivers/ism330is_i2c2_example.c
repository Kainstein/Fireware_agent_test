/**
 * @file ism330is_i2c2_example.c
 * @brief Example usage of ISM330IS with I2C2 driver
 * @author Your Name
 * @version 1.0
 * @date 2025
 */

#include "ism330is_api.h"
#include "ism330is_driver.h"
#include "main.h"
#include "driver_printf_config.h"
#include <stdio.h>

/* Conditional printf for ISM330IS I2C2 example module */
#if (ENABLE_DRV_PRINTF && ENABLE_DRV_PRINTF_ISM330IS_I2C2_EXAMPLE)
    #define ISM330IS_I2C2_EXAMPLE_PRINTF(...)    printf(__VA_ARGS__)
#else
    #define ISM330IS_I2C2_EXAMPLE_PRINTF(...)    ((void)0)
#endif

/*******************************************************************************
 *                              EXAMPLE USAGE                                  *
 *******************************************************************************/

/* Global ISM330IS object */
static ISM330IS_Object_t ism330is_obj;

/**
 * @brief Example: Initialize ISM330IS sensor with I2C2
 * @note Add this to your FreeRTOS task or main loop
 */
void ISM330IS_I2C2_Example_Init(void)
{
    int32_t status;
    uint8_t who_am_i = 0;
    
    /* Method 1: Simple initialization */
    status = ISM330IS_I2C2_Init(&ism330is_obj);
    if (status != 0) {
        // Handle initialization error
        __nop(); // Set breakpoint here for debugging
        return;
    }
    
    /* Test: Read WHO_AM_I register */
    status = ISM330IS_ReadID(&ism330is_obj, &who_am_i);
    if (status == ISM330IS_OK && who_am_i == ISM330IS_ID) {
        // Success! Sensor is responding correctly
        __nop(); // Set breakpoint here - sensor detected
    } else {
        // Error: Sensor not detected or communication issue
        __nop(); // Set breakpoint here for debugging
    }
}

/**
 * @brief Example: Alternative initialization method with custom settings
 */
void ISM330IS_I2C2_Example_CustomInit(void)
{
    ISM330IS_IO_t io_ctx;
    int32_t status;
    
    /* Method 2: Manual initialization with custom settings */
    
    /* Step 1: Configure I2C address if needed (default is LOW) */
    // I2C2_ISM330IS_SetAddress(ISM330IS_I2C_ADDR_HIGH); // If SA0 pin is high
    
    /* Step 2: Configure IO structure */
    io_ctx.Init      = I2C2_ISM330IS_Init;
    io_ctx.DeInit    = I2C2_ISM330IS_DeInit;
    io_ctx.BusType   = ISM330IS_I2C_BUS;
    io_ctx.Address   = I2C2_ISM330IS_GetAddress();
    io_ctx.WriteReg  = I2C2_ISM330IS_WriteReg;
    io_ctx.ReadReg   = I2C2_ISM330IS_ReadReg;
    io_ctx.GetTick   = I2C2_GetTick;
    
    /* Step 3: Configure and register bus IO */
    status = I2C2_ISM330IS_ConfigureIO(&io_ctx);
    if (status != 0) {
        // Handle IO configuration error
        return;
    }
    
    status = ISM330IS_RegisterBusIO(&ism330is_obj, &io_ctx);
    if (status != ISM330IS_OK) {
        // Handle registration error
        return;
    }
    
    /* Step 4: Initialize sensor */
    status = ISM330IS_Init(&ism330is_obj);
    if (status != ISM330IS_OK) {
        // Handle initialization error
        return;
    }
}

/**
 * @brief Example: Read accelerometer and gyroscope data
 */
void ISM330IS_I2C2_Example_ReadSensorData(void)
{
    ISM330IS_Axes_t acceleration;
    ISM330IS_Axes_t angular_rate;
    int32_t status;
    
    /* Enable accelerometer */
    status = ISM330IS_ACC_Enable(&ism330is_obj);
    if (status != ISM330IS_OK) {
        return;
    }
    
    /* Enable gyroscope */
    status = ISM330IS_GYRO_Enable(&ism330is_obj);
    if (status != ISM330IS_OK) {
        return;
    }
    
    /* Read accelerometer data */
    status = ISM330IS_ACC_GetAxes(&ism330is_obj, &acceleration);
    if (status == ISM330IS_OK) {
        // acceleration.x, acceleration.y, acceleration.z contain the data
        __nop(); // Set breakpoint to examine acceleration values
    }
    
    /* Read gyroscope data */
    status = ISM330IS_GYRO_GetAxes(&ism330is_obj, &angular_rate);
    if (status == ISM330IS_OK) {
        // angular_rate.x, angular_rate.y, angular_rate.z contain the data
        __nop(); // Set breakpoint to examine gyroscope values
    }
}

/**
 * @brief Example: FreeRTOS task using ISM330IS
 */
void ISM330IS_Task_Example(void const * argument)
{
    /* Initialize sensor */
    ISM330IS_I2C2_Example_Init();
    
    /* Main task loop */
    for(;;)
    {
        /* Read sensor data every 100ms */
        ISM330IS_I2C2_Example_ReadSensorData();
        
        osDelay(100); // 100ms delay
    }
}

/**
 * @brief Example: Test I2C2 connection without full sensor init
 */
void ISM330IS_I2C2_Example_TestConnection(void)
{
    int32_t status;
    uint8_t test_data;
    
    /* Test 1: Check device ready */
    status = I2C2_ISM330IS_IsDeviceReady(ISM330IS_I2C_ADDR_DEFAULT);
    if (status == 0) {
        __nop(); // Device is ready on I2C bus
    } else {
        __nop(); // Device not responding - check connections
        return;
    }
    
    /* Test 2: Try to read WHO_AM_I register directly */
    status = I2C2_ISM330IS_ReadReg(ISM330IS_I2C_ADDR_DEFAULT, ISM330IS_WHO_AM_I, &test_data, 1);
    if (status == 0 && test_data == ISM330IS_ID) {
        __nop(); // Successfully read WHO_AM_I register
    } else {
        __nop(); // Communication error or wrong device
    }
}

/**
 * @brief Troubleshooting: Test both possible I2C addresses
 */
void ISM330IS_I2C2_Example_FindAddress(void)
{
    int32_t status;
    uint8_t who_am_i;
    
    /* Test Address Low (SA0 = 0) */
    status = I2C2_ISM330IS_ReadReg(ISM330IS_I2C_ADDR_LOW, ISM330IS_WHO_AM_I, &who_am_i, 1);
    if (status == 0 && who_am_i == ISM330IS_ID) {
        __nop(); // Device found at LOW address (SA0 = GND)
        I2C2_ISM330IS_SetAddress(ISM330IS_I2C_ADDR_LOW);
        return;
    }
    
    /* Test Address High (SA0 = 1) */
    status = I2C2_ISM330IS_ReadReg(ISM330IS_I2C_ADDR_HIGH, ISM330IS_WHO_AM_I, &who_am_i, 1);
    if (status == 0 && who_am_i == ISM330IS_ID) {
        __nop(); // Device found at HIGH address (SA0 = VCC)
        I2C2_ISM330IS_SetAddress(ISM330IS_I2C_ADDR_HIGH);
        return;
    }
    
    /* Device not found at either address */
    __nop(); // Check wiring, power, and I2C configuration
}