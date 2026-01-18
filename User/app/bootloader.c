/**
 ******************************************************************************
 * @file    bootloader.c
 * @brief   Bootloader Jump Interface Implementation
 * @details Implements software jump from application to bootloader
 * @date    January 13, 2026
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "bootloader.h"
#include "main.h"
#include "stm32h7xx_hal.h"
#include "app.h"
#include "hmi_wrapper.h"

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim7;
extern TIM_HandleTypeDef htim17;
extern ZLG72128_Handle g_display_handle;

/* Private variables ---------------------------------------------------------*/
static uint8_t cs14_press_count = 0;  // CS14 button press counter

/* Private function prototypes -----------------------------------------------*/
static void DeInitAllPeripherals(void);

/* ============================================================================
   PUBLIC FUNCTIONS
   ============================================================================ */

/**
 * @brief Set bootloader entry flag in backup SRAM
 */
void Bootloader_SetEntryFlag(void)
{
    // Enable backup SRAM clock
    __HAL_RCC_BKPRAM_CLK_ENABLE();
    
    // Set magic flag value
    *(volatile uint32_t*)BOOTLOADER_FLAG_ADDR = BOOTLOADER_FLAG_VALUE;
}

/**
 * @brief Clear bootloader entry flag in backup SRAM
 */
void Bootloader_ClearEntryFlag(void)
{
    // Enable backup SRAM clock
    __HAL_RCC_BKPRAM_CLK_ENABLE();
    
    // Clear flag
    *(volatile uint32_t*)BOOTLOADER_FLAG_ADDR = 0;
}

/**
 * @brief Check if bootloader entry is requested
 */
bool Bootloader_IsEntryRequested(void)
{
    // Enable backup SRAM clock
    __HAL_RCC_BKPRAM_CLK_ENABLE();
    
    // Check flag value
    return (*(volatile uint32_t*)BOOTLOADER_FLAG_ADDR == BOOTLOADER_FLAG_VALUE);
}

/**
 * @brief Prepare system for bootloader entry
 */
void Bootloader_PrepareForJump(void)
{
    // Disable all interrupts
    __disable_irq();
    
    // Deinitialize all peripherals
    DeInitAllPeripherals();
    
    // Disable SysTick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
}

/**
 * @brief Jump from application to bootloader
 */
void Bootloader_JumpToBootloader(void)
{
    // Function pointer type for bootloader entry
    typedef void (*pFunction)(void);
    
    // Prepare for jump
    Bootloader_PrepareForJump();
    
    // Get bootloader stack pointer (first word at bootloader address)
    uint32_t bootloader_stack = *(volatile uint32_t*)BOOTLOADER_ADDRESS;
    
    // Get bootloader reset handler address (second word at bootloader address)
    uint32_t bootloader_reset = *(volatile uint32_t*)(BOOTLOADER_ADDRESS + 4);
    
    // Create function pointer to bootloader
    pFunction JumpToBootloader = (pFunction)bootloader_reset;
    
    // Set main stack pointer
    __set_MSP(bootloader_stack);
    
    // Jump to bootloader
    JumpToBootloader();
    
    // Should never reach here
    while(1);
}

/**
 * @brief Request bootloader entry and perform jump
 */
void Bootloader_RequestEntry(void)
{
    // Set bootloader entry flag
    Bootloader_SetEntryFlag();
    
    // Jump to bootloader
    Bootloader_JumpToBootloader();
}

/* ============================================================================
   PRIVATE FUNCTIONS
   ============================================================================ */

/**
 * @brief Deinitialize all peripherals to clean state
 */
static void DeInitAllPeripherals(void)
{
    // Deinitialize UARTs
    HAL_UART_DeInit(&huart1);
    HAL_UART_DeInit(&huart2);
    
    // Deinitialize I2C
    HAL_I2C_DeInit(&hi2c1);
    HAL_I2C_DeInit(&hi2c2);
    
    // Deinitialize Timers
    HAL_TIM_Base_DeInit(&htim2);
    HAL_TIM_Base_DeInit(&htim3);
    HAL_TIM_Base_DeInit(&htim4);
    HAL_TIM_Base_DeInit(&htim7);
    HAL_TIM_Base_DeInit(&htim17);
    
    // Reset all peripherals to default state
    HAL_DeInit();
}

/* ============================================================================
   CS14 TRIGGER FUNCTIONS
   ============================================================================ */

/**
 * @brief Reset CS14 press counter
 */
void Bootloader_ResetCS14Counter(void)
{
    cs14_press_count = 0;
}

/**
 * @brief Increment CS14 press counter
 */
void Bootloader_IncrementCS14Counter(void)
{
    if (cs14_press_count < 255) {  // Prevent overflow
        cs14_press_count++;
    }
}

/**
 * @brief Check if CS14 press count reached trigger threshold
 */
bool Bootloader_ShouldTrigger(void)
{
    return (cs14_press_count >= BOOTLOADER_TRIGGER_COUNT);
}

/**
 * @brief Get current CS14 press count
 */
uint8_t Bootloader_GetCS14Count(void)
{
    return cs14_press_count;
}

/* ============================================================================
   ISP DISPLAY FUNCTIONS
   ============================================================================ */

/**
 * @brief Display application version on Group2 display
 */
void Bootloader_DisplayVersion(void)
{
    uint8_t version_digits[4];
    
    // Format version using universal float formatter
    Display72128_FormatFloat(APP_VERSION, version_digits);
    
    // Write version to Group2 display (COM5-COM8)
    for (int i = 0; i < 4; i++) {
        ZLG72128_WriteDigit(&g_display_handle, 5 + i, version_digits[i]);
    }
}

/**
 * @brief Display ISP programming progress on Group1 and version on Group2
 */
void Bootloader_DisplayISPProgress(uint8_t progress_percent)
{
    // Clamp progress to 0-100
    if (progress_percent > 100) {
        progress_percent = 100;
    }
    
    // Calculate number of dashes to display (5 positions, each = 20%)
    uint8_t dash_count = progress_percent / 20;
    
    // Get segment code for dash '-' (G segment only = 0x40)
    uint8_t dash_segment = 0x40;
    
    // Update Group1 display (COM1-COM5) - 5 digits
    for (int i = 0; i < 5; i++) {
        if (i < dash_count) {
            // Show dash
            ZLG72128_WriteDigit(&g_display_handle, 1 + i, dash_segment);
        } else {
            // Clear (blank)
            ZLG72128_WriteDigit(&g_display_handle, 1 + i, 0x00);
        }
    }
    
    // Display version on Group2 (remains stable)
    Bootloader_DisplayVersion();
}
