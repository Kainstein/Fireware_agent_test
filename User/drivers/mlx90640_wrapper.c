/**
 * @file led_driver.c
 * @brief LED Driver implementation with individual LED functions
 */
#include <stdio.h>
#include <stdint.h>
#include "mlx90640_wrapper.h"
#include "MLX90640_API.h"
#include "i2c.h"
#include "driver_printf_config.h"

/* Conditional printf for MLX90640 wrapper module */
#if (ENABLE_DRV_PRINTF && ENABLE_DRV_PRINTF_MLX90640_WRAPPER)
    #define MLX90640_WRAPPER_PRINTF(...)    printf(__VA_ARGS__)
#else
    #define MLX90640_WRAPPER_PRINTF(...)    ((void)0)
#endif

/* Private types -------------------------------------------------------------*/
#define  FPS2HZ   0x02
#define  FPS4HZ   0x03
#define  FPS8HZ   0x04
#define  FPS16HZ  0x05
#define  FPS32HZ  0x06

#define  MLX90640_ADDR 0x33
#define	 RefreshRate FPS16HZ 
#define  TA_SHIFT 8 //Default shift for MLX90640 in open air
/* Private variables ---------------------------------------------------------*/

static uint16_t eeMLX90640[832];  
static float mlx90640To[768];
uint16_t frame[834];
float emissivity=0.95;
int status;
paramsMLX90640 mlx90640;
/* Private function prototypes -----------------------------------------------*/
void MLX90640_I2CInit()
{   
	MX_I2C1_Init();
}


int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data)
{

	uint8_t* p = (uint8_t*) data;

	int ack = 0;                               
	int cnt = 0;
	
	ack = HAL_I2C_Mem_Read(&hi2c1, (slaveAddr<<1), startAddress, I2C_MEMADD_SIZE_16BIT, p, nMemAddressRead*2, 500);

	if (ack != HAL_OK)
	{
			return -1;
	}
	

	for(cnt=0; cnt < nMemAddressRead*2; cnt+=2) {
		uint8_t tempBuffer = p[cnt+1];
		p[cnt+1] = p[cnt];
		p[cnt] = tempBuffer;
	}

	return 0;   
} 


int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data)
{

	uint8_t sa;
	int ack = 0;
	uint8_t cmd[2];
	static uint16_t dataCheck;

	sa = (slaveAddr << 1);

	cmd[0] = data >> 8;
	cmd[1] = data & 0x00FF;


	ack = HAL_I2C_Mem_Write(&hi2c1, sa, writeAddress, I2C_MEMADD_SIZE_16BIT, cmd, sizeof(cmd), 500);

	if (ack != HAL_OK)
	{
			return -1;
	}         
	
	MLX90640_I2CRead(slaveAddr,writeAddress,1, &dataCheck);
	
	if ( dataCheck != data)
	{
			return -2;
	}    
	
	return 0;
}
/* Public functions ----------------------------------------------------------*/

void mlx90640_wrapper_init(void)
{
	MLX90640_SetRefreshRate(MLX90640_ADDR, RefreshRate);
	MLX90640_SetChessMode(MLX90640_ADDR);
	//paramsMLX90640 mlx90640;
	status = MLX90640_DumpEE(MLX90640_ADDR, eeMLX90640);
	if (status != 0) 
		MLX90640_WRAPPER_PRINTF("\r\nload mlx90640 system parameters error with code:%d\r\n",status);
	else
		MLX90640_WRAPPER_PRINTF("\r\nload mlx90640 system parameters success\r\n");
	
	status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
	if (status != 0) 
		MLX90640_WRAPPER_PRINTF("\r\nParameter extraction failed with error code:%d\r\n",status);
	else
		MLX90640_WRAPPER_PRINTF("\r\nParameter extraction success\r\n");
}
static uint32_t mlx90640_test_cnt = 0;
void mlx90640_wrapper_sample_polling(void)
{
	// Only responsible for data collection
	mlx90640_test_cnt++;
	
	int status = MLX90640_GetFrameData(MLX90640_ADDR, frame);
	if (status < 0)
	{
		MLX90640_WRAPPER_PRINTF("GetFrame Error: %d\r\n",status);
		return;
	}
	
	float vdd = MLX90640_GetVdd(frame, &mlx90640);
	float Ta = MLX90640_GetTa(frame, &mlx90640);
	float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
#if MLX90640_PRINT_TUNIT == 1u	
	MLX90640_WRAPPER_PRINTF("\r\n============%d==============MLX90640 Data Collection ==========================\r\n", mlx90640_test_cnt);
	MLX90640_WRAPPER_PRINTF("VDD: %f V, Tr: %f C\r\n", vdd, tr);
#endif	
	// Calculate temperature array from raw frame data
	MLX90640_CalculateTo(frame, &mlx90640, emissivity, tr, mlx90640To);
#if MLX90640_PRINT_TUNIT == 1u	
	// Print temperature array (32x24 pixels)
	for(int i = 0; i < 768; i++)
	{
		if(i%32 == 0 && i != 0)
		{
			MLX90640_WRAPPER_PRINTF("\r\n");
		}
		MLX90640_WRAPPER_PRINTF("%2.2f ", mlx90640To[i]);
	}
	MLX90640_WRAPPER_PRINTF("\r\n");
#endif
}
float read_mlx90640_obj_min_temp(void)
{
	// Only responsible for calculation of the minimum temperature
	float minTemp = mlx90640To[0];
	for(int i = 1; i < 768; i++)
	{
		if(mlx90640To[i] < minTemp)
		{
			minTemp = mlx90640To[i];
		}
	}
	return minTemp;
}
float read_mlx90640_obj_max_temp(void)
{
	// Only responsible for calculation of the maximum temperature
	float maxTemp = mlx90640To[0];
	for(int i = 1; i < 768; i++)
	{
		if(mlx90640To[i] > maxTemp)
		{
			maxTemp = mlx90640To[i];
		}
	}
	return maxTemp;
}
float read_mlx90640_obj_avg_temp(void)
{
	// Only responsible for calculation of the average temperature
	float sum = 0.0f;
	for(int i = 0; i < 768; i++)
	{
		sum += mlx90640To[i];
	}
	return sum / 768.0f;
}