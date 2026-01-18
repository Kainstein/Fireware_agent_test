/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "app_printf_config.h"

/* Conditional printf for FreeRTOS tasks */
#if (ENABLE_APP_PRINTF && ENABLE_APP_PRINTF_TASK)
    #define TASK_PRINTF(...)    printf(__VA_ARGS__)
#else
    #define TASK_PRINTF(...)    ((void)0)
#endif

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "app.h"
#include "driver.h"
#include "utilities.h"
#include "usart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticSemaphore_t osStaticSemaphoreDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// Task03 Configuration - Button Processing Loop Delay
// Change this value to adjust button responsiveness:
//   50ms  = 20Hz (recommended - good responsiveness)
//   100ms = 10Hz (slower but less CPU usage)
//   20ms  = 50Hz (very responsive but more CPU usage)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

// UART dual buffer variables (defined in uart_protocol.c)
extern uint8_t rxBufferA[];
extern uint8_t rxBufferB[];
extern volatile uint8_t bufferA_ready;
extern volatile uint8_t bufferB_ready;
extern volatile uint16_t bufferA_length;
extern volatile uint16_t bufferB_length;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 512,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myTask02 */
osThreadId_t myTask02Handle;
const osThreadAttr_t myTask02_attributes = {
  .name = "myTask02",
  .stack_size = 2048,
  .priority = (osPriority_t) osPriorityRealtime1,
};
/* Definitions for myTask03 */
osThreadId_t myTask03Handle;
const osThreadAttr_t myTask03_attributes = {
  .name = "myTask03",
  .stack_size = 512 * 8,
  .priority = (osPriority_t) osPriorityRealtime2,
};
/* Definitions for myTask04 */
osThreadId_t myTask04Handle;
const osThreadAttr_t myTask04_attributes = {
  .name = "myTask04",
  .stack_size = 512 * 8,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for myTask05 */
osThreadId_t myTask05Handle;
const osThreadAttr_t myTask05_attributes = {
  .name = "myTask05",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for myTask06 */
osThreadId_t myTask06Handle;
const osThreadAttr_t myTask06_attributes = {
  .name = "myTask06",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myTimer01 */
osTimerId_t myTimer01Handle;
const osTimerAttr_t myTimer01_attributes = {
  .name = "myTimer01"
};
/* Definitions for myBinarySem01 */
osSemaphoreId_t myBinarySem01Handle;
osStaticSemaphoreDef_t myBinarySem01ControlBlock;
const osSemaphoreAttr_t myBinarySem01_attributes = {
  .name = "myBinarySem01",
  .cb_mem = &myBinarySem01ControlBlock,
  .cb_size = sizeof(myBinarySem01ControlBlock),
};
/* Definitions for myBinarySem02 */
osSemaphoreId_t myBinarySem02Handle;
osStaticSemaphoreDef_t myBinarySem02ControlBlock;
const osSemaphoreAttr_t myBinarySem02_attributes = {
  .name = "myBinarySem02",
  .cb_mem = &myBinarySem02ControlBlock,
  .cb_size = sizeof(myBinarySem02ControlBlock),
};
/* Definitions for myCountingSem01 */
osSemaphoreId_t myCountingSem01Handle;
osStaticSemaphoreDef_t myCountingSem01ControlBlock;
const osSemaphoreAttr_t myCountingSem01_attributes = {
  .name = "myCountingSem01",
  .cb_mem = &myCountingSem01ControlBlock,
  .cb_size = sizeof(myCountingSem01ControlBlock),
};
/* Definitions for myCountingSem02 */
osSemaphoreId_t myCountingSem02Handle;
osStaticSemaphoreDef_t myCountingSem02ControlBlock;
const osSemaphoreAttr_t myCountingSem02_attributes = {
  .name = "myCountingSem02",
  .cb_mem = &myCountingSem02ControlBlock,
  .cb_size = sizeof(myCountingSem02ControlBlock),
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTask02(void *argument);
void StartTask03(void *argument);
void StartTask04(void *argument);
void StartTask05(void *argument);
void StartTask06(void *argument);
void Callback01(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of myBinarySem01 */
  myBinarySem01Handle = osSemaphoreNew(1, 1, &myBinarySem01_attributes);

  /* creation of myBinarySem02 */
  myBinarySem02Handle = osSemaphoreNew(1, 1, &myBinarySem02_attributes);

  /* creation of myCountingSem01 */
  myCountingSem01Handle = osSemaphoreNew(2, 0, &myCountingSem01_attributes);

  /* creation of myCountingSem02 */
  myCountingSem02Handle = osSemaphoreNew(2, 0, &myCountingSem02_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of myTimer01 */
  myTimer01Handle = osTimerNew(Callback01, osTimerPeriodic, NULL, &myTimer01_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
	
	// UART frame reception now uses simple boolean flag (g_uart1_frame_complete)
	// No RTOS event flags needed - simpler and more efficient
	
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of myTask02 */
  myTask02Handle = osThreadNew(StartTask02, NULL, &myTask02_attributes);

  /* creation of myTask03 */
  myTask03Handle = osThreadNew(StartTask03, NULL, &myTask03_attributes);

  /* creation of myTask04 */
  myTask04Handle = osThreadNew(StartTask04, NULL, &myTask04_attributes);

  /* creation of myTask05 */
  myTask05Handle = osThreadNew(StartTask05, NULL, &myTask05_attributes);

  /* creation of myTask06 */
  myTask06Handle = osThreadNew(StartTask06, NULL, &myTask06_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
	/* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
	/* Infinite loop */
	for(;;)
	{
		osDelay(1000);
		LED_STATUS_Toggle();
	}
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the myTask02 thread.
* @param argument: Not used
* @retval None
*/
void StartTask02(void *argument)
{
  /* USER CODE BEGIN StartTask02 */
	
	// Initialize UART protocol and start byte-by-byte reception
	UartProto_StartReception(&huart1);
	TASK_PRINTF("[Task02] UART dual buffer protocol initialized on USART1\r\n");
	
	__nop();
	
	/* Infinite loop - UART frame processing (dual flag polling, zero-copy) */
	for(;;)
	{
		// Check BufferA ready flag (set by TIM4 ISR)
		if (bufferA_ready)
		{
			// Process frame directly from BufferA (zero-copy)
			UartProto_ProcessFrame(rxBufferA, bufferA_length);
			
			// Clear flag after processing
			bufferA_ready = 0;
		}
		
		// Check BufferB ready flag (set by TIM4 ISR)
		if (bufferB_ready)
		{
			// Process frame directly from BufferB (zero-copy)
			UartProto_ProcessFrame(rxBufferB, bufferB_length);
			
			// Clear flag after processing
			bufferB_ready = 0;
		}
		
		// Small delay to prevent 100% CPU usage
		// Latency: Max 5ms between frame arrival and processing
		osDelay(5);
		__nop();
	}
}	
  /* USER CODE END StartTask02 */
/**
* @brief Function implementing the myTask03 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask03 */
void StartTask03(void *argument)
{
  /* USER CODE BEGIN StartTask03 */
	// Initialize application control system (includes display and button init)
	AppControl_Init();
	
	// Show startup animation on both groups simultaneously (6000ms total)
	AppControl_ShowStartupAnimation();
	
	// Display default values after animation
	AppControl_UpdateDisplay();
	
	TASK_PRINTF("[Task02] Button/Display control initialized\r\n");
	TASK_PRINTF("[Task02] Temperature setpoint: %.1f°C\r\n", AppState_GetTempSetpoint());
	TASK_PRINTF("[Task02] BLDC setpoint: %u RPM\r\n", AppState_GetBlDCSetpoint());
	
	uint32_t last_time = osKernelGetTickCount();
	
	__nop();
	/* Infinite loop */
	for(;;)
	{
		// Get current time in milliseconds
		uint32_t current_time = osKernelGetTickCount();
		
		// Process button state machine
		Button_Process(current_time);
		
		// Get button events (polling mode)
		ButtonEvent_t event;
		while (Button_GetEvent(&event))
		{
			// Handle button events through application button handler
			AppButton_HandleEvent(&event);
		}
		
		// Process continuous adjustments (held buttons)
		AppControl_ProcessContinuousAdjustment(current_time);
		
		// Update display flash state (timeout and toggle)
		AppControl_UpdateDisplayFlash(current_time);
		
		// Update display content
		AppControl_UpdateDisplay();
		
		// Loop delay for button responsiveness (configurable via TASK03_LOOP_DELAY_MS)
		osDelay(30);
		__nop();
	}
  /* USER CODE END StartTask02 */
}

/* USER CODE BEGIN Header_StartTask04 */
/**J
* @brief Function implementing the myTask04 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask04 */
void StartTask04(void *argument)
{
  /* USER CODE BEGIN StartTask04 */
//	eeprom_status = M24C64_Init();
//	M24C64_TestI2CLines();
//	M24C64_WriteByte_Polling(32, 0x55);
//	M24C64_ReadByte_Polling(32, eeprom_read_buf);
//	I2C2_ISM330IS_TestDemo();
	__nop();
	mlx90614_wrapper_init();
	mlx90640_wrapper_init();		
	__nop();
	/* Infinite loop */
	for(;;)
	{
		// Loop delay for button responsiveness (configurable via TASK03_LOOP_DELAY_MS)
		mlx90614_wrapper_sample_polling();
		osDelay(100);
		mlx90640_wrapper_sample_polling();
		osDelay(500);
		AppState_SetTempActual(read_mlx90640_obj_avg_temp());
		__nop();
	}
  /* USER CODE END StartTask04 */
}

/* USER CODE BEGIN Header_StartTask05 */
/**
* @brief Function implementing the myTask05 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask05 */
void StartTask05(void *argument)
{
	/* USER CODE BEGIN StartTask05 */
	/* Infinite loop */
	for(;;)
	{
		osDelay(1000);
		__nop();
	}
	/* USER CODE END StartTask05 */
}

/* USER CODE BEGIN Header_StartTask06 */
/**
* @brief Function implementing the myTask06 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask06 */
void StartTask06(void *argument)
{
	/* USER CODE BEGIN StartTask06 */
	/* Infinite loop */
	for(;;)
	{
        osDelay(1);
	}
	/* USER CODE END StartTask06 */
}

/* Callback01 function */
void Callback01(void *argument)
{
  /* USER CODE BEGIN Callback01 */

  /* USER CODE END Callback01 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE END Application */

