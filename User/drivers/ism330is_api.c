/**
 * @file ism330is_api.c
 * @brief I2C2 driver implementation for ISM330IS sensor on STM32H743
 * @author Your Name
 * @version 1.0
 * @date 2025
 */

#include "ism330is_api.h"
#include "main.h"
#include <string.h>
#include <stdio.h>
#include "ism330is_driver.h"
#include "ism330is_reg.h"
#include "driver_printf_config.h"

/* Conditional printf for ISM330IS API module */
#if (ENABLE_DRV_PRINTF && ENABLE_DRV_PRINTF_ISM330IS_API)
    #define ISM330IS_API_PRINTF(...)    printf(__VA_ARGS__)
#else
    #define ISM330IS_API_PRINTF(...)    ((void)0)
#endif

/*******************************************************************************
 *                              PRIVATE VARIABLES                              *
 *******************************************************************************/

/* I2C2 ISM330IS context structure */
static I2C2_ISM330IS_t i2c2_ism330is_ctx = {
    .hi2c = &hi2c2,
    .device_address = ISM330IS_I2C_ADDR_DEFAULT,
    .timeout = I2C2_TIMEOUT
};

/*******************************************************************************
 *                              INITIALIZATION                                 *
 *******************************************************************************/

/**
 * @brief Initialize I2C2 hardware for ISM330IS sensor
 * @return 0 if successful, -1 if error
 */
int32_t I2C2_ISM330IS_HW_Init(void)
{
    /* I2C2 is already initialized by MX_I2C2_Init() in main.c */
    /* Just verify the sensor is present */
    return I2C2_ISM330IS_IsDeviceReady(i2c2_ism330is_ctx.device_address);
}

/**
 * @brief Deinitialize I2C2 for ISM330IS sensor
 * @return 0 if successful, -1 if error
 */
int32_t I2C2_ISM330IS_DeInit(void)
{
    /* HAL I2C deinitialization handled by main.c if needed */
    return 0;
}

/*******************************************************************************
 *                           LOW-LEVEL I2C FUNCTIONS                           *
 *******************************************************************************/

/**
 * @brief Read register(s) from ISM330IS via I2C2
 * @param DevAddr Device I2C address (7-bit format)
 * @param Reg Register address to read from
 * @param pData Pointer to data buffer
 * @param Length Number of bytes to read
 * @return 0 if successful, -1 if error
 */
int32_t I2C2_ISM330IS_ReadReg(uint16_t DevAddr, uint8_t Reg, uint8_t *pData, uint16_t Length)
{
    HAL_StatusTypeDef status;
    
    /* Validate parameters */
    if (pData == NULL || Length == 0) {
        return -1;
    }
    
    /* Use HAL I2C memory read function */
    status = HAL_I2C_Mem_Read(i2c2_ism330is_ctx.hi2c, 
                              (DevAddr),           // Convert to 8-bit address
                              Reg, 
                              I2C_MEMADD_SIZE_8BIT,     // 8-bit register address
                              pData, 
                              Length, 
                              i2c2_ism330is_ctx.timeout);
    
    return (status == HAL_OK) ? 0 : -1;
}

/**
 * @brief Write register(s) to ISM330IS via I2C2
 * @param DevAddr Device I2C address (7-bit format)
 * @param Reg Register address to write to
 * @param pData Pointer to data buffer
 * @param Length Number of bytes to write
 * @return 0 if successful, -1 if error
 */
int32_t I2C2_ISM330IS_WriteReg(uint16_t DevAddr, uint8_t Reg, uint8_t *pData, uint16_t Length)
{
    HAL_StatusTypeDef status;
    
    /* Validate parameters */
    if (pData == NULL || Length == 0) {
        return -1;
    }
    
    /* Use HAL I2C memory write function */
    status = HAL_I2C_Mem_Write(i2c2_ism330is_ctx.hi2c,
                               (DevAddr),          // Convert to 8-bit address
                               Reg,
                               I2C_MEMADD_SIZE_8BIT,    // 8-bit register address
                               pData,
                               Length,
                               i2c2_ism330is_ctx.timeout);
    
    return (status == HAL_OK) ? 0 : -1;
}

/*******************************************************************************
 *                              UTILITY FUNCTIONS                              *
 *******************************************************************************/

/**
 * @brief Check if ISM330IS device is ready on I2C2
 * @param DevAddr Device I2C address (7-bit format)
 * @return 0 if device ready, -1 if not ready or error
 */
int32_t I2C2_ISM330IS_IsDeviceReady(uint16_t DevAddr)
{
    HAL_StatusTypeDef status;
    
    status = HAL_I2C_IsDeviceReady(i2c2_ism330is_ctx.hi2c,
                                   (DevAddr),      // Convert to 8-bit address
                                   3,                   // Number of trials
                                   i2c2_ism330is_ctx.timeout);
    
    return (status == HAL_OK) ? 0 : -1;
}

/**
 * @brief Get system tick count (for ISM330IS driver compatibility)
 * @return Current tick count in milliseconds
 */
uint32_t I2C2_GetTick(void)
{
    return HAL_GetTick();
}

/*******************************************************************************
 *                          ISM330IS INTEGRATION                               *
 *******************************************************************************/

/**
 * @brief Configure I2C2 bus IO structure for ISM330IS
 * @param pIO Pointer to IO structure to initialize
 * @return 0 if successful, -1 if error
 */
int32_t I2C2_ISM330IS_ConfigureIO(ISM330IS_IO_t *pIO)
{
    if (pIO == NULL) {
        return -1;
    }
    
    /* Configure IO structure for I2C2 */
    pIO->Init      = I2C2_ISM330IS_HW_Init;
    pIO->DeInit    = I2C2_ISM330IS_DeInit;
    pIO->BusType   = ISM330IS_I2C_BUS;
    pIO->Address   = i2c2_ism330is_ctx.device_address;
    pIO->WriteReg  = I2C2_ISM330IS_WriteReg;
    pIO->ReadReg   = I2C2_ISM330IS_ReadReg;
    pIO->GetTick   = I2C2_GetTick;
    
    return 0;
}

/**
 * @brief Initialize ISM330IS sensor object with I2C2 communication
 * @param pObj Pointer to ISM330IS object
 * @return 0 if successful, -1 if error
 */
int32_t I2C2_ISM330IS_SensorInit(ISM330IS_Object_t *pObj)
{
    ISM330IS_IO_t io_ctx;
    int32_t status;
    
    if (pObj == NULL) {
        return -1;
    }
    
    /* Configure I2C2 IO structure */
    status = I2C2_ISM330IS_ConfigureIO(&io_ctx);
    if (status != 0) {
        return -1;
    }
    
    /* Register I2C2 IO with ISM330IS object (using existing library function) */
    status = ISM330IS_RegisterBusIO(pObj, &io_ctx);
    if (status != ISM330IS_OK) {
        return -1;
    }
    
    /* Initialize the sensor */
    status = ISM330IS_Init(pObj);
    if (status != ISM330IS_OK) {
        return -1;
    }
    
    return 0;
}

/*******************************************************************************
 *                        CONFIGURATION FUNCTIONS                              *
 *******************************************************************************/

/**
 * @brief Set I2C2 device address for ISM330IS (for SA0 pin configuration)
 * @param addr Device address (ISM330IS_I2C_ADDR_LOW or ISM330IS_I2C_ADDR_HIGH)
 */
void I2C2_ISM330IS_SetAddress(uint16_t addr)
{
    i2c2_ism330is_ctx.device_address = addr;
}

/**
 * @brief Get current I2C2 device address
 * @return Current device address
 */
uint16_t I2C2_ISM330IS_GetAddress(void)
{
    return i2c2_ism330is_ctx.device_address;
}

/**
 * @brief Set I2C2 timeout value
 * @param timeout Timeout value in milliseconds
 */
void I2C2_ISM330IS_SetTimeout(uint32_t timeout)
{
    i2c2_ism330is_ctx.timeout = timeout;
}

/*******************************************************************************
 *                          TEMPERATURE FUNCTIONS                              *
 *******************************************************************************/

/**
 * @brief Get the ISM330IS temperature sensor data
 * @param pObj the device pObj
 * @param Temperature pointer where the temperature value is written (in degrees Celsius)
 * @return 0 in case of success, -1 if error
 */
int32_t I2C2_ISM330IS_GetTemperature(ISM330IS_Object_t *pObj, float *Temperature)
{
    int32_t ret = 0;
    int16_t temp_raw = 0;

    /* Read raw temperature data */
    if (ism330is_temperature_raw_get(&(pObj->Ctx), &temp_raw) != ISM330IS_OK)
    {
        ret = -1;
    }
    else
    {
        /* Convert raw temperature to Celsius */
        /* Formula: Temperature (°C) = 25 + (raw_value / 256) */
        /* Reference: 0 LSB = 25°C, 256 LSB/°C sensitivity */
        *Temperature = 25.0f + ((float)temp_raw / 256.0f);
    }

    return ret;
}

/**
 * @brief Get the ISM330IS temperature sensor raw data
 * @param pObj the device pObj  
 * @param TempRaw pointer where the raw temperature value is written
 * @return 0 in case of success, -1 if error
 */
/**
 * @brief Get raw temperature data from ISM330IS sensor
 * @param pObj Pointer to sensor object  
 * @param TempRaw Pointer to store raw temperature value
 * @return 0 if success, error code otherwise
 */
int32_t I2C2_ISM330IS_GetTemperatureRaw(ISM330IS_Object_t *pObj, int16_t *TempRaw)
{
    int32_t ret = 0;

    /* Read raw temperature data */
    if (ism330is_temperature_raw_get(&(pObj->Ctx), TempRaw) != ISM330IS_OK)
    {
        ret = -1;
    }

    return ret;
}

/*******************************************************************************
 *                          ANGLE CALCULATION FUNCTIONS                        *
 *******************************************************************************/

/* Global angle state variable */
static ISM330IS_AngleState_t i2c2_angle_state = {0};

/**
 * @brief Calculate pitch and roll angles from accelerometer data (static method)
 * @param pObj the device pObj
 * @param pitch pointer to store pitch angle in degrees
 * @param roll pointer to store roll angle in degrees
 * @return 0 in case of success, -1 if error
 */
int32_t I2C2_ISM330IS_GetAccelAngles(ISM330IS_Object_t *pObj, float *pitch, float *roll)
{
    int32_t ret = 0;
    ISM330IS_Axes_t acceleration;
    
    /* Read accelerometer data */
    if (ISM330IS_ACC_GetAxes(pObj, &acceleration) != ISM330IS_OK)
    {
        ret = -1;
    }
    else
    {
        /* Convert from mg to g (divide by 1000) */
        float ax = (float)acceleration.x / 1000.0f;
        float ay = (float)acceleration.y / 1000.0f; 
        float az = (float)acceleration.z / 1000.0f;
        
        /* Calculate pitch and roll angles */
        *pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / M_PI;
        *roll = atan2f(ay, az) * 180.0f / M_PI;
    }
    
    return ret;
}

/**
 * @brief Calculate tilt angle (magnitude of tilt from vertical)
 * @param pObj the device pObj
 * @param tilt_angle pointer to store tilt angle in degrees
 * @return 0 in case of success, -1 if error
 */
int32_t I2C2_ISM330IS_GetTiltAngle(ISM330IS_Object_t *pObj, float *tilt_angle)
{
    int32_t ret = 0;
    ISM330IS_Axes_t acceleration;
    
    /* Read accelerometer data */
    if (ISM330IS_ACC_GetAxes(pObj, &acceleration) != ISM330IS_OK)
    {
        ret = -1;
    }
    else
    {
        /* Convert from mg to g */
        float ax = (float)acceleration.x / 1000.0f;
        float ay = (float)acceleration.y / 1000.0f;
        float az = (float)acceleration.z / 1000.0f;
        
        /* Calculate magnitude */
        float magnitude = sqrtf(ax * ax + ay * ay + az * az);
        
        /* Calculate tilt angle from vertical (Z-axis) */
        *tilt_angle = acosf(fabsf(az) / magnitude) * 180.0f / M_PI;
    }
    
    return ret;
}

/**
 * @brief Initialize the angle calculation system
 * @param pObj the device pObj
 * @return 0 in case of success, -1 if error
 */
int32_t I2C2_ISM330IS_AngleInit(ISM330IS_Object_t *pObj)
{
    int32_t ret = 0;
    
    /* Initialize with accelerometer-based angles */
    ret = I2C2_ISM330IS_GetAccelAngles(pObj, &i2c2_angle_state.pitch, &i2c2_angle_state.roll);
    
    if (ret == 0)
    {
        i2c2_angle_state.yaw = 0.0f;  /* Cannot determine yaw from accelerometer alone */
        i2c2_angle_state.last_timestamp = HAL_GetTick();
        i2c2_angle_state.initialized = 1;
    }
    
    return ret;
}

/**
 * @brief Update angles using complementary filter
 * @param pObj the device pObj
 * @param pitch pointer to store pitch angle in degrees
 * @param roll pointer to store roll angle in degrees
 * @param yaw pointer to store yaw angle in degrees
 * @return 0 in case of success, -1 if error
 */
int32_t I2C2_ISM330IS_GetComplementaryAngles(ISM330IS_Object_t *pObj, float *pitch, float *roll, float *yaw)
{
    int32_t ret = 0;
    ISM330IS_Axes_t acceleration, angular_rate;
    float accel_pitch, accel_roll;
    float gyro_pitch, gyro_roll, gyro_yaw;
    uint32_t current_time;
    float dt;
    
    /* Filter coefficient (0.98 means 98% gyro, 2% accel) */
    const float alpha = 0.98f;
    
    if (!i2c2_angle_state.initialized)
    {
        return I2C2_ISM330IS_AngleInit(pObj);
    }
    
    /* Read sensor data */
    if (ISM330IS_ACC_GetAxes(pObj, &acceleration) != ISM330IS_OK) return -1;
    if (ISM330IS_GYRO_GetAxes(pObj, &angular_rate) != ISM330IS_OK) return -1;
    
    /* Calculate time delta */
    current_time = HAL_GetTick();
    dt = (current_time - i2c2_angle_state.last_timestamp) / 1000.0f; /* Convert to seconds */
    i2c2_angle_state.last_timestamp = current_time;
    
    /* Skip calculation if dt is too large (first call or long delay) */
    if (dt > 1.0f) {
        dt = 0.01f; /* Assume 10ms */
    }
    
    /* Calculate accelerometer-based angles */
    float ax = (float)acceleration.x / 1000.0f;
    float ay = (float)acceleration.y / 1000.0f;
    float az = (float)acceleration.z / 1000.0f;
    
    accel_pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / M_PI;
    accel_roll = atan2f(ay, az) * 180.0f / M_PI;
    
    /* Calculate gyroscope-based angle changes */
    /* Convert from mdps to dps (divide by 1000) then multiply by dt */
    gyro_pitch = (float)angular_rate.x / 1000.0f * dt;
    gyro_roll = (float)angular_rate.y / 1000.0f * dt;
    gyro_yaw = (float)angular_rate.z / 1000.0f * dt;
    
    /* Apply complementary filter */
    i2c2_angle_state.pitch = alpha * (i2c2_angle_state.pitch + gyro_pitch) + (1.0f - alpha) * accel_pitch;
    i2c2_angle_state.roll = alpha * (i2c2_angle_state.roll + gyro_roll) + (1.0f - alpha) * accel_roll;
    i2c2_angle_state.yaw = i2c2_angle_state.yaw + gyro_yaw; /* Yaw can only come from gyro */
    
    /* Normalize yaw to [-180, 180] */
    while (i2c2_angle_state.yaw > 180.0f) i2c2_angle_state.yaw -= 360.0f;
    while (i2c2_angle_state.yaw < -180.0f) i2c2_angle_state.yaw += 360.0f;
    
    /* Return calculated angles */
    *pitch = i2c2_angle_state.pitch;
    *roll = i2c2_angle_state.roll;
    *yaw = i2c2_angle_state.yaw;
    
    return ret;
}

/**
 * @brief Get complete sensor data (acceleration, gyroscope, temperature, angles)
 * @param pObj the device pObj
 * @param acceleration pointer to store acceleration data
 * @param angular_rate pointer to store gyroscope data
 * @param temperature pointer to store temperature in Celsius
 * @param pitch pointer to store pitch angle in degrees
 * @param roll pointer to store roll angle in degrees
 * @param yaw pointer to store yaw angle in degrees
 * @return 0 in case of success, -1 if error
 */
int32_t I2C2_ISM330IS_GetAllSensorData(ISM330IS_Object_t *pObj, ISM330IS_Axes_t *acceleration, 
                                       ISM330IS_Axes_t *angular_rate, float *temperature, 
                                       float *pitch, float *roll, float *yaw)
{
    int32_t ret = 0;
    
    /* Read raw sensor data */
    if (ISM330IS_ACC_GetAxes(pObj, acceleration) != ISM330IS_OK) ret = -1;
    if (ISM330IS_GYRO_GetAxes(pObj, angular_rate) != ISM330IS_OK) ret = -1;
    
    /* Read temperature */
    if (I2C2_ISM330IS_GetTemperature(pObj, temperature) != 0) ret = -1;
    
    /* Calculate angles */
    if (I2C2_ISM330IS_GetComplementaryAngles(pObj, pitch, roll, yaw) != 0) ret = -1;
    
    return ret;
}

/*******************************************************************************
 *                              TEST DEMO FUNCTION                             *
 *******************************************************************************/

/**
 * @brief Comprehensive test demo for ISM330IS with I2C2
 * @note This function tests all aspects of the ISM330IS I2C2 connection
 * @note Add this to your FreeRTOS task or call from main for testing
 */
void I2C2_ISM330IS_TestDemo(void)
{
    ISM330IS_Object_t ism330is_obj;
    int32_t status;
    uint8_t who_am_i = 0;
    ISM330IS_Axes_t acceleration;
    ISM330IS_Axes_t angular_rate;
    float temperature = 0.0f;
    uint8_t data_ready = 0;
    uint32_t test_count = 0;
    
    ISM330IS_API_PRINTF("\r\n========================================\r\n");
    ISM330IS_API_PRINTF("ISM330IS I2C2 Connection Test Demo\r\n");
    ISM330IS_API_PRINTF("========================================\r\n");
    
    /* Test 1: I2C Bus Connectivity */
    ISM330IS_API_PRINTF("\r\n--- Test 1: I2C Bus Connectivity ---\r\n");
    status = I2C2_ISM330IS_IsDeviceReady(ISM330IS_I2C_ADDR_DEFAULT);
    if (status == 0) {
        ISM330IS_API_PRINTF("I2C2 Device Ready Test: PASSED\r\n");
    } else {
        ISM330IS_API_PRINTF("I2C2 Device Ready Test: FAILED\r\n");
        ISM330IS_API_PRINTF("   Check I2C2 wiring and sensor power\r\n");
        
        /* Try alternative address */
        ISM330IS_API_PRINTF("   Trying alternative I2C address...\r\n");
        I2C2_ISM330IS_SetAddress(ISM330IS_I2C_ADDR_HIGH);
        status = I2C2_ISM330IS_IsDeviceReady(ISM330IS_I2C_ADDR_HIGH);
        if (status == 0) {
            ISM330IS_API_PRINTF("Device found at HIGH address (SA0=VCC)\r\n");
        } else {
            ISM330IS_API_PRINTF("Device not found at either address\r\n");
            return; /* Exit test if no device found */
        }
    }
    
    /* Test 2: WHO_AM_I Register Read */
    ISM330IS_API_PRINTF("\r\n--- Test 2: WHO_AM_I Register ---\r\n");
    status = I2C2_ISM330IS_ReadReg(I2C2_ISM330IS_GetAddress(), ISM330IS_WHO_AM_I, &who_am_i, 1);
    if (status == 0) {
        ISM330IS_API_PRINTF("WHO_AM_I Register Read: SUCCESS\r\n");
        ISM330IS_API_PRINTF("   Expected: 0x%02X, Read: 0x%02X\r\n", ISM330IS_ID, who_am_i);
        if (who_am_i == ISM330IS_ID) {
            ISM330IS_API_PRINTF("WHO_AM_I Value Check: PASSED\r\n");
        } else {
            ISM330IS_API_PRINTF("WHO_AM_I Value Check: FAILED\r\n");
            ISM330IS_API_PRINTF("Wrong device or communication error\r\n");
            return;
        }
    } else {
        ISM330IS_API_PRINTF("❌ WHO_AM_I Register Read: FAILED\r\n");
        return;
    }
    
    /* Test 3: Full Sensor Initialization */
    ISM330IS_API_PRINTF("\r\n--- Test 3: Sensor Initialization ---\r\n");
    status = I2C2_ISM330IS_SensorInit(&ism330is_obj);
    if (status == 0) {
        ISM330IS_API_PRINTF("ISM330IS I2C2 Initialization: SUCCESS\r\n");
    } else {
        ISM330IS_API_PRINTF("ISM330IS I2C2 Initialization: FAILED\r\n");
        return;
    }
    
    /* Test 4: Read Device ID using High-Level Function */
    ISM330IS_API_PRINTF("\r\n--- Test 4: Device ID via High-Level API ---\r\n");
    status = ISM330IS_ReadID(&ism330is_obj, &who_am_i);
    if (status == ISM330IS_OK && who_am_i == ISM330IS_ID) {
        ISM330IS_API_PRINTF("High-Level ReadID: SUCCESS (0x%02X)\r\n", who_am_i);
    } else {
        ISM330IS_API_PRINTF("High-Level ReadID: FAILED\r\n");
    }
    
    /* Test 5: Enable Accelerometer */
    ISM330IS_API_PRINTF("\r\n--- Test 5: Accelerometer Enable ---\r\n");
    status = ISM330IS_ACC_Enable(&ism330is_obj);
    if (status == ISM330IS_OK) {
        ISM330IS_API_PRINTF("Accelerometer Enable: SUCCESS\r\n");
    } else {
        ISM330IS_API_PRINTF("Accelerometer Enable: FAILED\r\n");
    }
    
    /* Test 6: Enable Gyroscope */
    ISM330IS_API_PRINTF("\r\n--- Test 6: Gyroscope Enable ---\r\n");
    status = ISM330IS_GYRO_Enable(&ism330is_obj);
    if (status == ISM330IS_OK) {
        ISM330IS_API_PRINTF("Gyroscope Enable: SUCCESS\r\n");
    } else {
        ISM330IS_API_PRINTF("Gyroscope Enable: FAILED\r\n");
    }
    
    /* Test 7: Set Data Rates */
    ISM330IS_API_PRINTF("\r\n--- Test 7: Configure Data Rates ---\r\n");
    status = ISM330IS_ACC_SetOutputDataRate(&ism330is_obj, 104.0f); /* 104 Hz */
    if (status == ISM330IS_OK) {
        ISM330IS_API_PRINTF("Accelerometer ODR Set: 104 Hz\r\n");
    } else {
        ISM330IS_API_PRINTF("Accelerometer ODR Set: FAILED\r\n");
    }
    
    status = ISM330IS_GYRO_SetOutputDataRate(&ism330is_obj, 104.0f); /* 104 Hz */
    if (status == ISM330IS_OK) {
        ISM330IS_API_PRINTF("Gyroscope ODR Set: 104 Hz\r\n");
    } else {
        ISM330IS_API_PRINTF("Gyroscope ODR Set: FAILED\r\n");
    }
    
    /* Test 8: Data Reading Test (10 samples) */
    ISM330IS_API_PRINTF("\r\n--- Test 8: Sensor Data Reading ---\r\n");
    ISM330IS_API_PRINTF("Reading 10 samples...\r\n");
    
    for (test_count = 0; test_count < 10; test_count++) {
        /* Wait for new data */
        HAL_Delay(50); /* Wait 50ms between readings */
        
        /* Check data ready status */
        status = ISM330IS_ACC_Get_DRDY_Status(&ism330is_obj, &data_ready);
        if (status == ISM330IS_OK && data_ready) {
            ISM330IS_API_PRINTF("Sample %lu - Data Ready: YES\r\n", test_count + 1);
        } else {
            ISM330IS_API_PRINTF("Sample %lu - Data Ready: NO (reading anyway)\r\n", test_count + 1);
        }
        
        /* Read Accelerometer Data */
        status = ISM330IS_ACC_GetAxes(&ism330is_obj, &acceleration);
        if (status == ISM330IS_OK) {
            ISM330IS_API_PRINTF("ACC: X=%6ld, Y=%6ld, Z=%6ld [mg]\r\n", 
                   acceleration.x, acceleration.y, acceleration.z);
        } else {
            ISM330IS_API_PRINTF("ACC: Read FAILED\r\n");
        }
        
        /* Read Gyroscope Data */
        status = ISM330IS_GYRO_GetAxes(&ism330is_obj, &angular_rate);
        if (status == ISM330IS_OK) {
            ISM330IS_API_PRINTF("GYR: X=%6ld, Y=%6ld, Z=%6ld [mdps]\r\n", 
                   angular_rate.x, angular_rate.y, angular_rate.z);
        } else {
            ISM330IS_API_PRINTF("GYR: Read FAILED\r\n");
        }
        
        /* Read Temperature (using new temperature function) */
        status = I2C2_ISM330IS_GetTemperature(&ism330is_obj, &temperature);
        if (status == 0) {
            ISM330IS_API_PRINTF("TEMP: %.2f C\r\n", temperature);
        } else {
            ISM330IS_API_PRINTF("TEMP: Read FAILED\r\n");
        }
        
        /* Read Angles (static accelerometer-based) */
        static float pitch_static, roll_static, tilt_angle;
        status = I2C2_ISM330IS_GetAccelAngles(&ism330is_obj, &pitch_static, &roll_static);
        if (status == 0) {
            ISM330IS_API_PRINTF("   ANGLES: Pitch=%.1f,Roll=%.1f", pitch_static, roll_static);
            
            /* Add tilt angle */
            if (I2C2_ISM330IS_GetTiltAngle(&ism330is_obj, &tilt_angle) == 0) {
                ISM330IS_API_PRINTF("Tilt=%.1f", tilt_angle);
            }
            ISM330IS_API_PRINTF("\r\n");
        } else {
            ISM330IS_API_PRINTF("ANGLES: Read FAILED\r\n");
        }
        
        ISM330IS_API_PRINTF("\r\n");
    }
    
    /* Test 9: Complementary Filter Angle Test */
    ISM330IS_API_PRINTF("--- Test 9: Complementary Filter Angles ---\r\n");
    
    /* Initialize angle calculation */
    status = I2C2_ISM330IS_AngleInit(&ism330is_obj);
    if (status == 0) {
        ISM330IS_API_PRINTF("Angle System Initialized\r\n");
        
        /* Test complementary filter angles for 5 readings */
        for (int i = 0; i < 5; i++) {
            float pitch_comp, roll_comp, yaw_comp;
            status = I2C2_ISM330IS_GetComplementaryAngles(&ism330is_obj, &pitch_comp, &roll_comp, &yaw_comp);
            if (status == 0) {
                ISM330IS_API_PRINTF("Sample %d: P=%.1f, R=%.1f, Y=%.1f\r\n", 
                       i+1, pitch_comp, roll_comp, yaw_comp);
            } else {
                ISM330IS_API_PRINTF("Sample %d: FAILED\r\n", i+1);
            }
            HAL_Delay(100); /* 100ms between readings */
        }
    } else {
        ISM330IS_API_PRINTF("Angle System Initialization FAILED\r\n");
    }
    
    /* Test 10: Performance Test */
    ISM330IS_API_PRINTF("\r\n--- Test 10: Performance Test ---\r\n");
    uint32_t start_time = HAL_GetTick();
    uint32_t read_count = 0;
    
    /* Read 100 times as fast as possible */
    for (int i = 0; i < 100; i++) {
        status = ISM330IS_ACC_GetAxes(&ism330is_obj, &acceleration);
        if (status == ISM330IS_OK) {
            read_count++;
        }
    }
    
    uint32_t elapsed_time = HAL_GetTick() - start_time;
    ISM330IS_API_PRINTF("Performance: %lu reads in %lu ms\r\n", read_count, elapsed_time);
    ISM330IS_API_PRINTF("Average: %.2f ms per read\r\n", (float)elapsed_time / read_count);
    
    /* Test 11: Configuration Readback */
    ISM330IS_API_PRINTF("\r\n--- Test 11: Configuration Readback ---\r\n");
    float odr_value;
    
    status = ISM330IS_ACC_GetOutputDataRate(&ism330is_obj, &odr_value);
    if (status == ISM330IS_OK) {
        ISM330IS_API_PRINTF("ACC ODR Readback: %.1f Hz\r\n", odr_value);
    } else {
        ISM330IS_API_PRINTF("ACC ODR Readback: FAILED\r\n");
    }
    
    status = ISM330IS_GYRO_GetOutputDataRate(&ism330is_obj, &odr_value);
    if (status == ISM330IS_OK) {
        ISM330IS_API_PRINTF("GYR ODR Readback: %.1f Hz\r\n", odr_value);
    } else {
        ISM330IS_API_PRINTF("GYR ODR Readback: FAILED\r\n");
    }
    
    /* Test 12: Complete Sensor Data Function */
    ISM330IS_API_PRINTF("\r\n--- Test 12: Complete Sensor Data ---\r\n");
    float temp_all, pitch_all, roll_all, yaw_all;
    status = I2C2_ISM330IS_GetAllSensorData(&ism330is_obj, &acceleration, &angular_rate, 
                                       &temp_all, &pitch_all, &roll_all, &yaw_all);
    if (status == 0) {
        ISM330IS_API_PRINTF("Complete Data Read: SUCCESS\r\n");
        ISM330IS_API_PRINTF("   ALL-IN-ONE: ACC(%ld,%ld,%ld) GYR(%ld,%ld,%ld) TEMP(%.1f C) ANG(%.1f,%.1%.1f\r\n",
               acceleration.x, acceleration.y, acceleration.z,
               angular_rate.x, angular_rate.y, angular_rate.z,
               temp_all, pitch_all, roll_all, yaw_all);
    } else {
        ISM330IS_API_PRINTF("Complete Data Read: FAILED\r\n");
    }
    
    /* Test Summary */
    ISM330IS_API_PRINTF("\r\n========================================\r\n");
    ISM330IS_API_PRINTF("ISM330IS I2C2 Test Demo Complete! \r\n");
    ISM330IS_API_PRINTF("========================================\r\n");
    ISM330IS_API_PRINTF("I2C Address Used: 0x%02X\r\n", I2C2_ISM330IS_GetAddress());
    if (temperature != 0.0f) {
        ISM330IS_API_PRINTF("ast Temperature: %.2f C\r\n", temperature);
    }
    ISM330IS_API_PRINTF("Last Acceleration: X=%ld, Y=%ld, Z=%ld mg\r\n", 
           acceleration.x, acceleration.y, acceleration.z);
    ISM330IS_API_PRINTF("Last Angular Rate: X=%ld, Y=%ld, Z=%ld mdps\r\n", 
           angular_rate.x, angular_rate.y, angular_rate.z);
    ISM330IS_API_PRINTF("I2C Timeout: %lu ms\r\n", i2c2_ism330is_ctx.timeout);
    ISM330IS_API_PRINTF("\r\nSensor is ready for normal operation! \r\n");
}


