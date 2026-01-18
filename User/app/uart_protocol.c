/**
 * @file uart_protocol.c
 * @brief UART Communication Protocol Implementation
 * @date 2026-01-07
 * @details Implementation of frame packing/unpacking, checksum calculation, and protocol handling
 */

#include "uart_protocol.h"
#include "app_printf_config.h"
#include "usart.h"
#include "tim.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

/* Conditional printf for UART protocol module */
#if (ENABLE_APP_PRINTF && ENABLE_APP_PRINTF_UART_PROTOCOL)
    #define UART_PRINTF(...)    printf(__VA_ARGS__)
#else
    #define UART_PRINTF(...)    ((void)0)
#endif

/* ============================================================================
   GLOBAL VARIABLES FOR UART FRAME RECEPTION
   ============================================================================ */

// UART handle reference (internal use only)
static UART_HandleTypeDef *huart1_handle = NULL;

// DUAL BUFFER ARCHITECTURE (exported for uart_callbacks.c)
uint8_t rxByte = 0;                                      // Single byte buffer for HAL UART ISR
uint8_t rxBufferA[UART_PROTO_MAX_FRAME_SIZE];          // Buffer A (ping-pong)
uint8_t rxBufferB[UART_PROTO_MAX_FRAME_SIZE];          // Buffer B (ping-pong)
volatile uint8_t* activeRxBuffer = NULL;                // Pointer to buffer ISR writes to
volatile uint16_t activeRxIndex = 0;                    // Current write index in active buffer

// Boolean completion flags (simple volatile variables, NOT FreeRTOS event flags!)
volatile uint8_t bufferA_ready = 0;                    // 1 = BufferA has complete frame
volatile uint8_t bufferB_ready = 0;                    // 1 = BufferB has complete frame
volatile uint16_t bufferA_length = 0;                  // Length of complete frame in BufferA
volatile uint16_t bufferB_length = 0;                  // Length of complete frame in BufferB

// Frame processing structures (internal use only)
static UartProtoRxBuffer_t g_uartRxBuf;
static UartProtoTxBuffer_t g_uartTxBuf;
static UartProtoFrame_t g_receivedFrame;

// TX buffer (static allocation for safety)
static uint8_t txBuffer[UART_PROTO_MAX_FRAME_SIZE];

#if UART_PROTO_USE_DMA_TX
// DMA TX busy flag (prevents transmission collision)
volatile uint8_t uart1_tx_busy = 0;
#endif

/* ============================================================================
   FUNCTION IMPLEMENTATIONS
   ============================================================================ */

/**
 * @brief Initialize UART protocol buffers
 */
UartProtoError_t UartProto_Init(UartProtoRxBuffer_t *rxBuf, UartProtoTxBuffer_t *txBuf)
{
    if (rxBuf == NULL || txBuf == NULL)
        return UART_PROTO_ERR_INVALID_FRAME;
    
    memset(rxBuf, 0, sizeof(UartProtoRxBuffer_t));
    memset(txBuf, 0, sizeof(UartProtoTxBuffer_t));
    rxBuf->state = 0;  // Waiting state
    txBuf->busy = 0;   // Not transmitting
    
    return UART_PROTO_OK;
}

/**
 * @brief Calculate checksum for frame (XOR of header + data)
 * 
 * Checksum = XOR of all bytes: sync[0] XOR sync[1] XOR length XOR msgType XOR data[0]...data[n-1]
 */
uint8_t UartProto_CalculateChecksum(const UartProtoFrame_t *frame, uint16_t dataLength)
{
    uint8_t checksum = 0;
    uint16_t i;
    
    if (frame == NULL)
        return 0;
    
    /* XOR sync bytes */
    checksum ^= (frame->sync & 0xFF);        // Low byte of sync
    checksum ^= ((frame->sync >> 8) & 0xFF); // High byte of sync
    
    /* XOR length and message type */
    checksum ^= frame->length;
    checksum ^= frame->msgType;
    
    /* XOR all data bytes */
    for (i = 0; i < dataLength && i < UART_PROTO_MAX_DATA_SIZE; i++)
    {
        checksum ^= frame->data[i];
    }
    
    return checksum;
}

/**
 * @brief Verify frame checksum
 */
int UartProto_VerifyChecksum(const UartProtoFrame_t *frame, uint16_t dataLength)
{
    uint8_t calculatedChecksum;
    
    if (frame == NULL)
        return 0;
    
    calculatedChecksum = UartProto_CalculateChecksum(frame, dataLength);
    
    return (calculatedChecksum == frame->checksum) ? 1 : 0;
}

/**
 * @brief Pack frame for transmission
 * 
 * Creates frame layout:
 *   [Sync(2)] [Length(1)] [Type(1)] [Data(0-512)] [Checksum(1)]
 */
uint16_t UartProto_PackFrame(UartProtoFrame_t *frame, uint16_t sync, uint8_t msgType, 
                                 const uint8_t *data, uint16_t dataLength)
{
    if (frame == NULL)
        return 0;
    
    /* Validate data length */
    if (dataLength > UART_PROTO_MAX_DATA_SIZE)
        return 0;
    
    /* Validate sync value */
    if (sync != UART_SYNC_SDT_TO_MCU && sync != UART_SYNC_MCU_TO_SDT)
        return 0;
    
    /* Set frame header */
    frame->sync = sync;
    frame->msgType = msgType;
    frame->length = UART_PROTO_FRAME_HEADER_SIZE + dataLength + UART_PROTO_FRAME_TRAILER_SIZE;
    
    /* Copy data payload */
    if (data != NULL && dataLength > 0)
    {
        memcpy(frame->data, data, dataLength);
    }
    memset(&frame->data[dataLength], 0, UART_PROTO_MAX_DATA_SIZE - dataLength);
    
    /* Calculate and set checksum */
    frame->checksum = UartProto_CalculateChecksum(frame, dataLength);
    
    /* Return total frame length */
    return frame->length;
}

/**
 * @brief Unpack and validate received frame
 */
int16_t UartProto_UnpackFrame(const uint8_t *buffer, uint16_t bufferLength, UartProtoFrame_t *frame)
{
    uint16_t dataLength;
    uint16_t totalFrameLength;
    
    if (buffer == NULL || frame == NULL || bufferLength < UART_PROTO_FRAME_HEADER_SIZE)
        return -UART_PROTO_ERR_INVALID_FRAME;
    
    /* Extract header from buffer */
    frame->sync = (buffer[0] << 8) | buffer[1];  // Big-endian sync
    frame->length = buffer[2];
    frame->msgType = buffer[3];
    
    /* Validate sync header */
    if (frame->sync != UART_SYNC_SDT_TO_MCU && frame->sync != UART_SYNC_MCU_TO_SDT)
        return -UART_PROTO_ERR_INVALID_SYNC;
    
    /* Validate length field */
    if (frame->length < UART_PROTO_FRAME_HEADER_SIZE + UART_PROTO_FRAME_TRAILER_SIZE)
        return -UART_PROTO_ERR_INVALID_LENGTH;
    
    /* Since frame->length is uint8_t (max 255), and UART_PROTO_MAX_FRAME_SIZE is 517,
       we need to check if the data portion would overflow the data array.
       data_length = length - header(4) - trailer(1) must fit in data[512] */
    if (frame->length > (UART_PROTO_FRAME_HEADER_SIZE + UART_PROTO_MAX_DATA_SIZE + UART_PROTO_FRAME_TRAILER_SIZE))
        return -UART_PROTO_ERR_BUFFER_OVERFLOW;
    
    /* Check if we have complete frame in buffer */
    if (bufferLength < frame->length)
        return -UART_PROTO_ERR_INVALID_FRAME;
    
    /* Calculate data length */
    dataLength = frame->length - UART_PROTO_FRAME_HEADER_SIZE - UART_PROTO_FRAME_TRAILER_SIZE;
    
    /* Copy data from buffer */
    if (dataLength > 0)
    {
        memcpy(frame->data, &buffer[UART_PROTO_FRAME_HEADER_SIZE], dataLength);
    }
    memset(&frame->data[dataLength], 0, UART_PROTO_MAX_DATA_SIZE - dataLength);
    
    /* Extract and verify checksum */
    frame->checksum = buffer[frame->length - 1];
    
    if (!UartProto_VerifyChecksum(frame, dataLength))
        return -UART_PROTO_ERR_INVALID_CHECKSUM;
    
    return (int16_t)dataLength;
}

/**
 * @brief Get error description string
 */
const char* UartProto_GetErrorString(UartProtoError_t error)
{
    switch (error)
    {
        case UART_PROTO_OK:
            return "No error";
        case UART_PROTO_ERR_INVALID_SYNC:
            return "Invalid synchronization header";
        case UART_PROTO_ERR_INVALID_LENGTH:
            return "Invalid frame length";
        case UART_PROTO_ERR_INVALID_CHECKSUM:
            return "Checksum verification failed";
        case UART_PROTO_ERR_BUFFER_OVERFLOW:
            return "Receive buffer overflow";
        case UART_PROTO_ERR_INVALID_FRAME:
            return "Invalid frame structure";
        case UART_PROTO_ERR_TIMEOUT:
            return "Frame reception timeout";
        default:
            return "Unknown error";
    }
}

/* ============================================================================
   UART PROTOCOL INITIALIZATION AND ISR CALLBACKS
   ============================================================================ */

/**
 * @brief Initialize UART protocol and start byte reception
 * @param huart Pointer to UART handle (typically &huart1)
 */
void UartProto_StartReception(UART_HandleTypeDef *huart)
{
    if (huart == NULL)
        return;
    
    // Store UART handle reference
    huart1_handle = huart;
    
    // Initialize protocol buffers
    UartProto_Init(&g_uartRxBuf, &g_uartTxBuf);
    
    // Initialize dual buffers (zero-copy design)
    memset(rxBufferA, 0, UART_PROTO_MAX_FRAME_SIZE);
    memset(rxBufferB, 0, UART_PROTO_MAX_FRAME_SIZE);
    
    // Initialize buffer pointers and indexes
    activeRxBuffer = rxBufferA;  // Start with BufferA
    activeRxIndex = 0;
    
    // Initialize boolean flags (simple variables, NOT FreeRTOS event flags!)
    bufferA_ready = 0;
    bufferB_ready = 0;
    bufferA_length = 0;
    bufferB_length = 0;
    
    // Start first byte reception (interrupt mode)
    HAL_UART_Receive_IT(huart1_handle, &rxByte, 1);
    
    UART_PRINTF("[UART] Dual buffer protocol initialized on USART1 (zero-copy design)\r\n");
}

/* Note: HAL_UART_RxCpltCallback() is now implemented in uart_callbacks.c */

/**
 * @brief Send UART frame to PC (blocking transmission)
 * @details Serializes frame structure into byte buffer and transmits via HAL_UART_Transmit
 * @param frame Pointer to frame structure to transmit
 * @param dataLength Length of data payload (0 to UART_PROTO_MAX_DATA_SIZE)
 * @return UART_PROTO_OK if successful, error code otherwise
 */
UartProtoError_t UartProto_SendFrame(const UartProtoFrame_t *frame, uint16_t dataLength)
{
    uint8_t txBuffer[UART_PROTO_MAX_FRAME_SIZE];
    uint16_t txLength;
    HAL_StatusTypeDef halStatus;
    
    if (frame == NULL)
    {
        UART_PRINTF("[UART] TX Error: NULL frame pointer\r\n");
        return UART_PROTO_ERR_INVALID_FRAME;
    }
    
    if (dataLength > UART_PROTO_MAX_DATA_SIZE)
    {
        UART_PRINTF("[UART] TX Error: Data length %u exceeds max %u\r\n", dataLength, UART_PROTO_MAX_DATA_SIZE);
        return UART_PROTO_ERR_BUFFER_OVERFLOW;
    }
    
    if (huart1_handle == NULL)
    {
        UART_PRINTF("[UART] TX Error: UART handle not initialized\r\n");
        return UART_PROTO_ERR_INVALID_FRAME;
    }
    
    // Calculate actual frame length from frame->length field
    txLength = frame->length;
    
    if (txLength > UART_PROTO_MAX_FRAME_SIZE)
    {
        UART_PRINTF("[UART] TX Error: Frame length %u exceeds max %u\r\n", txLength, UART_PROTO_MAX_FRAME_SIZE);
        return UART_PROTO_ERR_BUFFER_OVERFLOW;
    }
    
    // Serialize frame into byte buffer
    // Byte 0-1: sync (big-endian)
    txBuffer[0] = (frame->sync >> 8) & 0xFF;
    txBuffer[1] = frame->sync & 0xFF;
    // Byte 2: length
    txBuffer[2] = frame->length;
    // Byte 3: msgType
    txBuffer[3] = frame->msgType;
    // Byte 4 to 4+dataLength-1: data
    if (dataLength > 0)
    {
        memcpy(&txBuffer[4], frame->data, dataLength);
    }
    // Last byte: checksum
    txBuffer[txLength - 1] = frame->checksum;
    
#if UART_PROTO_USE_DMA_TX
    // ============ DMA Mode: Full-Duplex Non-blocking TX ============
    // RS-232 supports simultaneous TX and RX - no need to stop RX
    
    // Check if previous TX still in progress (busy flag protection)
    if (uart1_tx_busy)
    {
        UART_PRINTF("[UART] TX busy, frame dropped\r\n");
        return UART_PROTO_ERR_TIMEOUT;  // Could define UART_PROTO_ERR_BUSY
    }
    
    // Set busy flag before starting DMA
    uart1_tx_busy = 1;
    
    // Transmit frame via UART DMA (non-blocking, RX continues independently)
    halStatus = HAL_UART_Transmit_DMA(huart1_handle, txBuffer, txLength);
    
    if (halStatus != HAL_OK)
    {
        UART_PRINTF("[UART] TX Error: HAL_UART_Transmit_DMA failed with status %d\r\n", halStatus);
        uart1_tx_busy = 0;  // Clear busy flag on error
        return UART_PROTO_ERR_TIMEOUT;
    }
    
    // RX continues independently during TX (full-duplex RS-232)
    UART_PRINTF("[UART] TX DMA Started: Sync=0x%04X Type=0x%02X Len=%u Data=%u bytes\r\n",
           frame->sync, frame->msgType, frame->length, dataLength);
    
#else
    // ============ Blocking Mode: Half-Duplex (Compatible) ============
    
    // Stop RX before TX to prevent state corruption
    extern uint8_t rxByte;
    extern TIM_HandleTypeDef htim4;
    HAL_UART_AbortReceive_IT(huart1_handle);
    HAL_TIM_Base_Stop_IT(&htim4);
    
    // Transmit frame via UART (blocking mode)
    halStatus = HAL_UART_Transmit(huart1_handle, txBuffer, txLength, 1000);  // 1 second timeout
    
    if (halStatus != HAL_OK)
    {
        UART_PRINTF("[UART] TX Error: HAL_UART_Transmit failed with status %d\r\n", halStatus);
        // Restart RX even on error
        HAL_UART_Receive_IT(huart1_handle, &rxByte, 1);
        return UART_PROTO_ERR_TIMEOUT;
    }
    
    // Restart RX immediately after successful TX
    HAL_StatusTypeDef rxStatus = HAL_UART_Receive_IT(huart1_handle, &rxByte, 1);
    if (rxStatus != HAL_OK)
    {
        UART_PRINTF("[UART] RX restart failed after TX (status=%d)\r\n", rxStatus);
    }
    
    UART_PRINTF("[UART] TX Frame: Sync=0x%04X Type=0x%02X Len=%u Data=%u bytes\r\n",
           frame->sync, frame->msgType, frame->length, dataLength);
#endif
    
    return UART_PROTO_OK;
}

/**
 * @brief Process complete UART frame (called from Task2 polling loop)
 * @details Unpacks frame, validates checksum, and executes command handler
 *          ZERO-COPY DESIGN: Reads directly from completed buffer pointer
 *          Task checks bufferA_ready/bufferB_ready flags before calling
 * @param buffer Pointer to buffer containing complete frame (rxBufferA or rxBufferB)
 * @param length Length of frame in buffer
 */
void UartProto_ProcessFrame(const uint8_t* buffer, uint16_t length)
{
    int16_t dataLength;
    
    // Validate parameters
    if (buffer == NULL || length < UART_PROTO_FRAME_HEADER_SIZE)
    {
        UART_PRINTF("[UART] Invalid frame parameters (buffer=%p, length=%u)\r\n", buffer, length);
        return;
    }
    
    // ZERO-COPY: Read directly from passed buffer pointer (no memcpy!)
    // ISR is already writing to the OTHER buffer, so this is safe
    dataLength = UartProto_UnpackFrame(buffer, length, &g_receivedFrame);
    
    if (dataLength < 0)
    {
        // Frame validation failed
        UART_PRINTF("[UART] Frame error: %s\r\n", UartProto_GetErrorString((UartProtoError_t)(-dataLength)));
        return;
    }
    
    // Frame valid - log and process
    UART_PRINTF("[UART] RX Frame: Sync=0x%04X Type=0x%02X Len=%u Data=%u bytes\r\n",
           g_receivedFrame.sync,
           g_receivedFrame.msgType,
           g_receivedFrame.length,
           (uint16_t)dataLength);
    
    // Command handler dispatch based on msgType
    if (g_receivedFrame.msgType == MSG_TYPE_LOOPBACK_TEST)
    {
        // Loopback test: Echo frame back to PC with reversed sync direction
        UartProtoFrame_t loopbackFrame;
        uint16_t newSync;
        uint16_t packedLength;
        UartProtoError_t txResult;
        
        UART_PRINTF("[UART] Loopback test detected, echoing frame back...\r\n");
        
        // Reverse sync direction: SDT->MCU (0xeb90) becomes MCU->SDT (0x146f)
        newSync = (g_receivedFrame.sync == UART_SYNC_SDT_TO_MCU) ? 
                   UART_SYNC_MCU_TO_SDT : UART_SYNC_SDT_TO_MCU;
        
        // Pack frame with same msgType and data, but reversed sync
        packedLength = UartProto_PackFrame(&loopbackFrame, newSync, 
                                               g_receivedFrame.msgType,
                                               g_receivedFrame.data, 
                                               (uint16_t)dataLength);
        
        if (packedLength == 0)
        {
            UART_PRINTF("[UART] Loopback error: Failed to pack frame\r\n");
        }
        else
        {
            // Send loopback frame back to PC
            txResult = UartProto_SendFrame(&loopbackFrame, (uint16_t)dataLength);
            
            if (txResult != UART_PROTO_OK)
            {
                UART_PRINTF("[UART] Loopback error: Send failed with code %d\r\n", txResult);
            }
            else
            {
                UART_PRINTF("[UART] Loopback test completed successfully\r\n");
                
                // Small delay to ensure TX completes before next RX
                osDelay(2);
            }
        }
    }
    // TODO: Add more command handlers here based on msgType
    // Example: else if (g_receivedFrame.msgType == MSG_TYPE_CONTROL) { ... } 
}
