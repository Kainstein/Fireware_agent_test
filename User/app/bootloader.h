/**
 ******************************************************************************
 * @file    bootloader.h
 * @brief   Bootloader Jump Interface Header
 * @details Provides functions to jump from application to bootloader
 *          for firmware update via UART1 ISP
 * @date    January 13, 2026
 ******************************************************************************
 */

#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
   BOOTLOADER MEMORY MAP
   ============================================================================ */
#define BOOTLOADER_ADDRESS       0x08000000UL    // Bootloader start address
#define APPLICATION_ADDRESS      0x08010000UL    // Application start address (64KB offset)
#define BOOTLOADER_FLAG_ADDR     0x38800000UL    // Backup SRAM for bootloader entry flag
#define BOOTLOADER_FLAG_VALUE    0xDEADBEEFUL    // Magic value to indicate bootloader entry

/* CS14 Button Trigger Configuration */
#define BOOTLOADER_TRIGGER_COUNT 1               // Number of CS14 presses required to trigger bootloader

/* ============================================================================
   BOOTLOADER CONTROL FUNCTIONS
   ============================================================================ */

/**
 * @brief Jump from application to bootloader
 * @note This function performs a software jump to bootloader without system reset
 *       It will NOT return - execution continues in bootloader
 * 
 * Steps:
 * 1. Set bootloader entry flag in backup SRAM
 * 2. Deinitialize all peripherals
 * 3. Disable interrupts
 * 4. Jump to bootloader reset vector
 */
void Bootloader_JumpToBootloader(void);

/**
 * @brief Prepare system for bootloader entry
 * @note Call this before jumping to bootloader to ensure clean peripheral state
 *       Deinitializes all peripherals used by application
 */
void Bootloader_PrepareForJump(void);

/**
 * @brief Check if bootloader entry is requested
 * @return true if bootloader entry flag is set, false otherwise
 * @note This function is typically called by application at startup
 */
bool Bootloader_IsEntryRequested(void);

/**
 * @brief Set bootloader entry flag in backup SRAM
 * @note Application can call this before system reset to enter bootloader
 */
void Bootloader_SetEntryFlag(void);

/**
 * @brief Clear bootloader entry flag in backup SRAM
 */
void Bootloader_ClearEntryFlag(void);

/**
 * @brief Request bootloader entry and perform jump
 * @note High-level function that handles flag setting and jump
 *       This is the main function to call from application
 */
void Bootloader_RequestEntry(void);

/* ============================================================================
   CS14 TRIGGER FUNCTIONS
   ============================================================================ */

/**
 * @brief Reset CS14 press counter
 * @note Call at the start of startup animation
 */
void Bootloader_ResetCS14Counter(void);

/**
 * @brief Increment CS14 press counter
 * @note Call when CS14 button is pressed during animation
 */
void Bootloader_IncrementCS14Counter(void);

/**
 * @brief Check if CS14 press count reached trigger threshold
 * @return true if bootloader should be triggered, false otherwise
 */
bool Bootloader_ShouldTrigger(void);

/**
 * @brief Get current CS14 press count
 * @return Current press count
 */
uint8_t Bootloader_GetCS14Count(void);

/* ============================================================================
   ISP DISPLAY FUNCTIONS
   ============================================================================ */

/**
 * @brief Display ISP programming progress on Group1 (dash bar) and version on Group2
 * @param progress_percent: Progress percentage (0-100)
 * @note Group1 shows progress bar: 0%=blank, 20%=-, 40%=--, 60%=---, 80%=----, 100%=-----
 *       Group2 shows APP_VERSION using Display72128_FormatFloat()
 */
void Bootloader_DisplayISPProgress(uint8_t progress_percent);

/**
 * @brief Display application version on Group2 display
 * @note Uses Display72128_FormatFloat() for consistent formatting
 */
void Bootloader_DisplayVersion(void);

#ifdef __cplusplus
}
#endif

#endif /* __BOOTLOADER_H */
