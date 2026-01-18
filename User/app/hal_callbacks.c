/**
 * @file hal_callbacks.c
 * @brief UART Protocol HAL Callback Implementations
 * @date 2026-01-09
 * @details Implements HAL interrupt callbacks for UART frame processing
 */

#include "hal_callbacks.h"
#include "uart_protocol.h"
#include "usart.h"
#include "tim.h"
#include "cmsis_os.h"
#include <stdio.h>

// External dual buffer variables from uart_protocol.c
extern uint8_t rxByte;
extern uint8_t rxBufferA[];
extern uint8_t rxBufferB[];
extern volatile uint8_t* activeRxBuffer;
extern volatile uint16_t activeRxIndex;
extern volatile uint8_t bufferA_ready;
extern volatile uint8_t bufferB_ready;
extern volatile uint16_t bufferA_length;
extern volatile uint16_t bufferB_length;

/**
 * @brief HAL UART Receive Complete Callback (ISR context)
 * @details Called when 1 byte is received via HAL_UART_Receive_IT()
 *          Implements timer-based 3.5T frame detection following IOT modbus pattern
 * @param huart Pointer to UART handle
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        // Check buffer overflow before storing
        if (activeRxIndex >= UART_PROTO_MAX_FRAME_SIZE)
        {
            // Buffer full - trigger manual swap (same as timer timeout)
            // Mark current buffer as complete
            if (activeRxBuffer == rxBufferA)
            {
                bufferA_ready = 1;
                bufferA_length = activeRxIndex;
            }
            else
            {
                bufferB_ready = 1;
                bufferB_length = activeRxIndex;
            }
            
            // ATOMIC SWAP to other buffer
            activeRxBuffer = (activeRxBuffer == rxBufferA) ? rxBufferB : rxBufferA;
            activeRxIndex = 0;
            
            // Stop timer and continue receiving
            HAL_TIM_Base_Stop_IT(&htim4);
            HAL_UART_Receive_IT(&huart1, &rxByte, 1);
            return;
        }
        
        // Store byte in active buffer (NOT cast needed - it's uint8_t*)
        activeRxBuffer[activeRxIndex++] = rxByte;
        
        // Timer handling: First byte vs subsequent bytes (IOT pattern)
        if (activeRxIndex == 1)
        {
            // First byte of new frame - stop timer only (don't start yet)
            HAL_TIM_Base_Stop_IT(&htim4);
        }
        else
        {
            // Subsequent bytes - reset and restart timer for 3.5T silence detection
            HAL_TIM_Base_Stop_IT(&htim4);
            __HAL_TIM_SET_COUNTER(&htim4, 0);
            HAL_TIM_Base_Start_IT(&htim4);
        }
        
        // Re-enable UART interrupt for next byte
        HAL_UART_Receive_IT(&huart1, &rxByte, 1);
    }
}

/**
 * @brief HAL TIM Period Elapsed Callback (ISR context)
 * @details Called when timer expires - signals 3.5T silence detected (frame complete)
 * @param htim Pointer to TIM handle
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // TIM4: UART Frame Timeout Handler (Dual Buffer Implementation)
    if (htim->Instance == TIM4)
    {
        // Stop timer (frame complete after 3.5T silence)
        HAL_TIM_Base_Stop_IT(&htim4);
        
        // CRITICAL: Atomic buffer swap to prevent race conditions
        // Mark which buffer just completed
        if (activeRxBuffer == rxBufferA)
        {
            bufferA_ready = 1;              // Signal BufferA has complete frame
            bufferA_length = activeRxIndex; // Save frame length
#if ENABLE_APP_PRINTF_UART_PROTOCOL
            printf("[UART] Switching to BufferB\r\n");
#endif
        }
        else
        {
            bufferB_ready = 1;              // Signal BufferB has complete frame
            bufferB_length = activeRxIndex; // Save frame length
#if ENABLE_APP_PRINTF_UART_PROTOCOL
            printf("[UART] Switching to BufferA\r\n");
#endif
        }
        
        // ATOMIC SWAP to other buffer for next frame reception
        // TIM4 priority (4) > USART1 priority (5) ensures this completes atomically
        activeRxBuffer = (activeRxBuffer == rxBufferA) ? rxBufferB : rxBufferA;
        activeRxIndex = 0;  // Reset index for new buffer
        
        // UART reception continues uninterrupted - already armed for next byte
    }
}

/**
 * @brief HAL UART TX Complete Callback (DMA mode only)
 * @details Called when DMA TX completes - clears busy flag
 *          RX continues independently (full-duplex RS-232)
 * @param huart Pointer to UART handle
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
#if UART_PROTO_USE_DMA_TX
        // Clear TX busy flag (allow next transmission)
        extern volatile uint8_t uart1_tx_busy;
        uart1_tx_busy = 0;
        
        // Note: No need to restart RX - it never stopped (full-duplex)
#endif
    }
}


