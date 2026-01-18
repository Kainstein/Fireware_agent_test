/**
 * @file uart_protocol.h
 * @brief UART Communication Protocol Structure and Interface
 * @date 2026-01-07
 * @details Protocol based on SDT software communication specification
 *          - Frame format: Sync(2) + Length(1) + Type(1) + Data(N) + Checksum(1)
 *          - Configurable data payload size via UART_PROTO_MAX_DATA_SIZE
 */

#ifndef __UART_PROTOCOL_H__
#define __UART_PROTOCOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32h7xx_hal.h"  // For UART_HandleTypeDef

/* ============================================================================
   UART TX MODE CONFIGURATION
   ============================================================================ */
// UART TX Mode Selection
// Set to 1 for DMA mode (non-blocking, full-duplex, higher performance)
// Set to 0 for blocking mode (simpler, compatible)
#define UART_PROTO_USE_DMA_TX    1

#if UART_PROTO_USE_DMA_TX
// TX busy flag for DMA protection (prevents transmission collision)
extern volatile uint8_t uart1_tx_busy;
#endif

/* ============================================================================
   UART PROTOCOL CONFIGURATION - CONFIGURABLE VIA #DEFINE
   ============================================================================ */
#define UART_PROTO_MAX_DATA_SIZE          512             // Configurable data payload size (default 512)
#define UART_PROTO_FRAME_HEADER_SIZE      4               // Sync(2) + Length(1) + Type(1)
#define UART_PROTO_FRAME_TRAILER_SIZE     1               // Checksum
#define UART_PROTO_MAX_FRAME_SIZE         (UART_PROTO_FRAME_HEADER_SIZE + UART_PROTO_MAX_DATA_SIZE + UART_PROTO_FRAME_TRAILER_SIZE)

/* UART Protocol Synchronization Headers */
#define UART_SYNC_SDT_TO_MCU        0xeb90          // SDT software -> Microcontroller
#define UART_SYNC_MCU_TO_SDT        0x146f          // Microcontroller -> SDT software

/* ============================================================================
   MESSAGE TYPES (Section 3 of Protocol Document)
   ============================================================================ */
typedef enum {
    MSG_TYPE_UNKNOWN                = 0x00,
    MSG_TYPE_CONTROL                = 0x01,         // Control command from SDT
    MSG_TYPE_DATA                   = 0x02,         // Data message
    MSG_TYPE_STATUS                 = 0x03,         // Status feedback response
    MSG_TYPE_ERROR                  = 0x04,         // Error report
    MSG_TYPE_LOOPBACK_TEST          = 0xFF,         // Loopback test - echo frame back to sender
    /* Add more message types as needed */
} UartProtoMsgType_t;

/* ============================================================================
   UART FRAME STRUCTURE
   ============================================================================ */

/**
 * @struct UartProtoFrame_t
 * @brief Complete UART Frame with variable-length data
 * 
 * Layout:
 *   Byte 0-1: sync       - Synchronization header (0xeb90 or 0x146f)
 *   Byte 2:   length     - Total frame length including header, data, and trailer
 *   Byte 3:   msgType    - Message type identifier
 *   Byte 4-N: data       - Data payload (0-512 bytes)
 *   Byte N+1: checksum   - XOR of sync + length + msgType + data bytes
 * 
 * Total size = 4 (header) + UART_PROTO_MAX_DATA_SIZE + 1 (checksum)
 * Actual used size depends on dataLength and length field
 */
typedef struct {
    uint16_t sync;                                  // 2 bytes: 0xeb90 or 0x146f
    uint8_t  length;                                // 1 byte: Total frame length
    uint8_t  msgType;                               // 1 byte: Message type
    uint8_t  data[UART_PROTO_MAX_DATA_SIZE];              // Data payload (configurable, default 512)
    uint8_t  checksum;                              // Frame trailer (1 byte): XOR of header + data
} UartProtoFrame_t;

/**
 * @struct UartProtoRxBuffer_t
 * @brief Receive buffer for frame assembly and state tracking
 */
typedef struct {
    uint8_t   buffer[UART_PROTO_MAX_FRAME_SIZE];          // Receive buffer for raw bytes
    uint16_t  index;                                // Current write position in buffer
    uint16_t  frameLength;                          // Expected frame length from header
    uint8_t   state;                                // Reception state: 0=waiting, 1=receiving, 2=complete
} UartProtoRxBuffer_t;

/**
 * @struct UartProtoTxBuffer_t
 * @brief Transmit buffer for frame queuing
 */
typedef struct {
    uint8_t   buffer[UART_PROTO_MAX_FRAME_SIZE];          // Transmit buffer
    uint16_t  length;                               // Frame length to transmit
    uint8_t   busy;                                 // Flag: 1 if transmitting, 0 if idle
} UartProtoTxBuffer_t;

/* ============================================================================
   ERROR CODES
   ============================================================================ */
typedef enum {
    UART_PROTO_OK                         = 0x00,
    UART_PROTO_ERR_INVALID_SYNC           = 0x01,
    UART_PROTO_ERR_INVALID_LENGTH         = 0x02,
    UART_PROTO_ERR_INVALID_CHECKSUM       = 0x03,
    UART_PROTO_ERR_BUFFER_OVERFLOW        = 0x04,
    UART_PROTO_ERR_INVALID_FRAME          = 0x05,
    UART_PROTO_ERR_TIMEOUT                = 0x06,
} UartProtoError_t;

/* ============================================================================
   UART PROTOCOL FUNCTION DECLARATIONS
   ============================================================================ */

/**
 * @brief Initialize UART protocol buffers
 * @param rxBuf Pointer to receive buffer structure
 * @param txBuf Pointer to transmit buffer structure
 * @return UART_PROTO_OK if successful
 */
UartProtoError_t UartProto_Init(UartProtoRxBuffer_t *rxBuf, UartProtoTxBuffer_t *txBuf);

/**
 * @brief Calculate checksum for frame (XOR of header + data)
 * @param frame Pointer to frame structure
 * @param dataLength Length of data payload (0 to UART_PROTO_MAX_DATA_SIZE)
 * @return Calculated checksum byte
 */
uint8_t UartProto_CalculateChecksum(const UartProtoFrame_t *frame, uint16_t dataLength);

/**
 * @brief Verify frame checksum
 * @param frame Pointer to frame structure
 * @param dataLength Length of data payload
 * @return 1 if checksum valid, 0 if invalid
 */
int UartProto_VerifyChecksum(const UartProtoFrame_t *frame, uint16_t dataLength);

/**
 * @brief Pack frame for transmission
 * Creates a complete frame ready for transmission with proper header and checksum
 * @param frame Pointer to frame structure (output)
 * @param sync Synchronization header (UART_SYNC_*_*)
 * @param msgType Message type (UartProtoMsgType_t)
 * @param data Pointer to data payload (NULL if no data)
 * @param dataLength Length of data payload (0 to UART_PROTO_MAX_DATA_SIZE)
 * @return Packed frame length in bytes, or 0 if error
 */
uint16_t UartProto_PackFrame(UartProtoFrame_t *frame, uint16_t sync, uint8_t msgType, 
                                 const uint8_t *data, uint16_t dataLength);

/**
 * @brief Unpack received frame and validate
 * Extracts and validates frame structure from raw buffer
 * @param buffer Pointer to raw received buffer
 * @param bufferLength Length of received buffer
 * @param frame Pointer to output frame structure (output)
 * @return Data length if valid frame, or negative error code (UartProtoError_t)
 */
int16_t UartProto_UnpackFrame(const uint8_t *buffer, uint16_t bufferLength, UartProtoFrame_t *frame);

/**
 * @brief Get error description string
 * @param error Error code
 * @return Error description string
 */
const char* UartProto_GetErrorString(UartProtoError_t error);

/**
 * @brief Initialize UART protocol and start byte reception
 * @param huart Pointer to UART handle (typically &huart1)
 */
void UartProto_StartReception(UART_HandleTypeDef *huart);

/**
 * @brief Process complete UART frame (called from Task2 when timeout triggers)
 * @param buffer Pointer to buffer containing complete frame
 * @param length Length of frame in buffer
 */
void UartProto_ProcessFrame(const uint8_t* buffer, uint16_t length);

/**
 * @brief Send UART frame to PC (blocking transmission)
 * @param frame Pointer to frame structure to transmit
 * @param dataLength Length of data payload (0 to UART_PROTO_MAX_DATA_SIZE)
 * @return UART_PROTO_OK if successful, error code otherwise
 */
UartProtoError_t UartProto_SendFrame(const UartProtoFrame_t *frame, uint16_t dataLength);

/* ============================================================================
   EXPORTED VARIABLES (for uart_callbacks.c) - DUAL BUFFER ARCHITECTURE
   ============================================================================ */

// These variables are defined in uart_protocol.c and used by uart_callbacks.c
extern uint8_t rxByte;                              // Single byte buffer for HAL UART ISR
extern uint8_t rxBufferA[UART_PROTO_MAX_FRAME_SIZE];      // Buffer A (ping-pong)
extern uint8_t rxBufferB[UART_PROTO_MAX_FRAME_SIZE];      // Buffer B (ping-pong)
extern volatile uint8_t* activeRxBuffer;            // Pointer to buffer ISR is currently writing to
extern volatile uint16_t activeRxIndex;             // Current write index in active buffer
extern volatile uint8_t bufferA_ready;              // Boolean flag: BufferA has complete frame (NOT FreeRTOS event!)
extern volatile uint8_t bufferB_ready;              // Boolean flag: BufferB has complete frame (NOT FreeRTOS event!)
extern volatile uint16_t bufferA_length;            // Length of complete frame in BufferA
extern volatile uint16_t bufferB_length;            // Length of complete frame in BufferB

#ifdef __cplusplus
}
#endif

#endif /* __UART_PROTOCOL_H__ */
