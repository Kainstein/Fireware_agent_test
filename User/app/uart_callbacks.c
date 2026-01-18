/**
 * @file uart_callbacks.c
 * @brief UART Protocol HAL Callback Implementations
 * @date 2026-01-09
 * @details Implements HAL interrupt callbacks for UART frame processing
 */

#include "uart_callbacks.h"
#include "uart_protocol.h"
#include "usart.h"
#include "tim.h"
#include "cmsis_os.h"
#include <stdio.h>

// External variables from uart_protocol.c
extern uint8_t rxByte;
extern uint8_t rxBuffer[];
extern uint16_t rxIndex;
extern volatile uint8_t g_uart1_frame_complete;  // Simple boolean flag instead of RTOS event

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
        if (rxIndex == 1)
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
        }
        else
        {
            bufferB_ready = 1;              // Signal BufferB has complete frame
            bufferB_length = activeRxIndex; // Save frame length
        }
        
        // ATOMIC SWAP to other buffer for next frame reception
        // TIM4 priority (4) > USART1 priority (5) ensures this completes atomically
        activeRxBuffer = (activeRxBuffer == rxBufferA) ? rxBufferB : rxBufferA;
        activeRxIndex = 0;  // Reset index for new buffer
        
        // UART reception continues uninterrupted - already armed for next byte
    }
}
