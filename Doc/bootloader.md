# Bootloader Documentation

## Overview
This document describes the bootloader implementation for the HPHT_H743ZIT6 firmware. The bootloader provides In-System Programming (ISP) capability via UART1 using ST's official UART bootloader protocol (AN3155).

## Requirements

### Functional Requirements
1. **Software Jump to Bootloader**
   - Application can trigger bootloader entry via software request
   - No hardware reset required for ISP mode entry
   - Bootloader flag stored in Backup SRAM (0x38800000)

2. **Button Trigger Mechanism**
   - **Trigger**: CS14 button press during startup animation
   - **Count**: Single press (1x) triggers bootloader entry
   - **Detection**: Counter increments in animation loop, resets at animation start
   - **Response**: Bootloader entry requested when count reaches threshold

3. **UART ISP Port**
   - **Port**: UART1 (PA9/PA10)
   - **Baud Rate**: 115200 bps
   - **Format**: 8 data bits, Even parity, 1 stop bit (8E1)
   - **Protocol**: ST Official UART Bootloader Protocol (AN3155)

4. **Display Feedback**
   - Show ISP progress on ZLG72128 LED displays
   - Display connection status, operation type, and progress percentage
   - Use both Group1 (BLDC, 5-digit) and Group2 (Temp, 4-digit) displays

### Memory Layout
- **Bootloader Region**: 0x08000000 - 0x0800FFFF (64KB)
  - Contains bootloader code
  - Flash write-protected
  - Handles ISP protocol communication
  
- **Application Region**: 0x08010000 - 0x081FFFFF (1984KB)
  - Contains main application firmware
  - Loaded by bootloader on normal boot
  - Can request bootloader entry via software flag
  
- **Backup SRAM**: 0x38800000
  - Stores bootloader entry flag
  - Retains data during software reset
  - Checked by bootloader on startup

### Boot Sequence
1. **Power-on/Reset**
   - Bootloader starts at 0x08000000
   - Initialize critical peripherals (RCC, GPIO, UART1)

2. **Check Entry Conditions**
   - Check Backup SRAM flag (0x38800000) for software request
   - Validate application firmware (magic number, CRC)
   - If either condition met: Enter ISP mode

3. **ISP Mode (if triggered)**
   - Initialize UART1 in 8E1 format @ 115200 baud
   - Initialize display for progress feedback
   - Enter ST protocol command loop
   - Process commands: Get, GetVersion, GetID, Read, Write, Erase, Go
   - Display operation status and progress

4. **Application Boot (normal path)**
   - Verify application firmware integrity
   - Set Vector Table Offset Register (VTOR) to 0x08010000
   - Set stack pointer from application vector table
   - Jump to application reset handler

## Communication Protocol

### ST Official UART Bootloader Protocol (AN3155)

#### Protocol Overview
- **Standard**: ST Microelectronics AN3155 specification
- **Advantages**:
  - Industry-proven, well-documented protocol
  - Simple XOR checksum (vs complex CRC16)
  - Compatible with existing ST tools (STM32CubeProgrammer, Flash Loader Demonstrator)
  - Reduces development time by ~25% (1.5-2 weeks vs 2-3 weeks)
  - Easier debugging and maintenance

#### Frame Format
All commands follow this structure:
```
[Command Byte] [Complement Byte] → ACK/NACK
[Data Bytes (if any)] [Checksum] → ACK/NACK
```

- **ACK**: 0x79 (command accepted)
- **NACK**: 0x1F (command rejected)
- **Checksum**: XOR of all data bytes

#### Command Set

| Command | Code | Description |
|---------|------|-------------|
| Get | 0x00 | Get supported command list |
| Get Version | 0x01 | Get bootloader version and supported protocol |
| Get ID | 0x02 | Get chip ID (0x0450 for STM32H743) |
| Read Memory | 0x11 | Read data from any valid memory address |
| Go | 0x21 | Jump to application code at specified address |
| Write Memory | 0x31 | Write data to RAM or Flash |
| Erase | 0x43 | Erase Flash memory pages |
| Extended Erase | 0x44 | Extended Flash erase (recommended) |
| Write Protect | 0x63 | Enable write protection |
| Write Unprotect | 0x73 | Disable write protection |
| Readout Protect | 0x82 | Enable readout protection |
| Readout Unprotect | 0x92 | Disable readout protection |

#### Example Command Sequences

**1. Get Command (Discovery)**
```
Host → Device: 0x00 0xFF
Device → Host: 0x79 (ACK)
Device → Host: [N] [cmd1] [cmd2] ... [cmdN] [checksum]
Host → Device: 0x79 (ACK)
```

**2. Write Memory**
```
Host → Device: 0x31 0xCE
Device → Host: 0x79 (ACK)
Host → Device: [Addr3] [Addr2] [Addr1] [Addr0] [AddrXOR]
Device → Host: 0x79 (ACK)
Host → Device: [N-1] [Data0] [Data1] ... [DataN-1] [DataXOR]
Device → Host: 0x79 (ACK)
```

**3. Go Command (Jump to App)**
```
Host → Device: 0x21 0xDE
Device → Host: 0x79 (ACK)
Host → Device: [Addr3] [Addr2] [Addr1] [Addr0] [AddrXOR]
Device → Host: 0x79 (ACK)
[Device jumps to address]
```

### Update Request Methods

1. **CS14 Button During Startup Animation**
   - Primary user trigger method
   - Single press during animation loop
   - Counter tracked in `AppControl_ShowStartupAnimation()`
   - Calls `Bootloader_RequestEntry()` when threshold reached

2. **Software Flag (Application Request)**
   - Application writes magic value to Backup SRAM (0x38800000)
   - Performs software reset
   - Bootloader detects flag on next boot
   - Used for remote firmware update requests

3. **Invalid Application Firmware**
   - Bootloader detects corrupted application
   - Automatically enters ISP mode for recovery
   - Displays error status on LED displays

## Display Feedback

### ISP Status Display Format

- **Group1 (BLDC Display - 5 digits)**: Programming progress bar
  - Uses "-" character to indicate progress
  - 0%: blank (no dashes)
  - 20%: one dash (-)
  - 40%: two dashes (--)
  - 60%: three dashes (---)
  - 80%: four dashes (----)
  - 100%: five dashes (-----)
  - Each dash represents 20% completion
  - Updated during Write and Erase operations

- **Group2 (Temp Display - 4 digits)**: Application version display
  - Shows current application firmware version
  - Same format as startup animation (STARTUP_ANIM_CHAR_MODE == 2)
  - Example: "0.005" for version 0.005, "2.34" for version 2.34
  - Uses `Display72128_FormatFloat(APP_VERSION)` function
  - Remains stable throughout ISP operations

### Display Update Functions
- `Bootloader_DisplayISPProgress(progress_percent)` - Updates Group1 progress bar
  - Input: progress_percent (0-100)
  - Calculates dash count: `dashes = progress_percent / 20`
  - Writes dash segments to Group1 display
- `Bootloader_DisplayVersion()` - Updates Group2 with APP_VERSION
  - Called once at ISP mode entry
  - Remains displayed throughout ISP session
- Both functions non-blocking to maintain ISP communication responsiveness

## Security

### Firmware Verification
- **Magic Number**: 0xDEADBEEF at application start
- **CRC32**: Full application image checksum
- **Size Check**: Validate firmware size against available space
- **Version Check**: Optional version comparison (prevent downgrade)

### Protection Mechanisms
- **Flash Write Protection**: Bootloader region protected against accidental writes
- **Timeout**: 30-second timeout in ISP mode if no communication
- **Invalid Command Rejection**: NACK response to unsupported commands
- **Address Range Validation**: Prevent writes to bootloader region

### Backup SRAM Configuration
- Enable backup SRAM clock: `__HAL_RCC_BKPRAM_CLK_ENABLE()`
- Enable backup regulator: `HAL_PWREx_EnableBkUpReg()`
- Flag structure:
  ```c
  #define BOOTLOADER_FLAG_ADDR 0x38800000
  #define BOOTLOADER_MAGIC     0xBEEF
  typedef struct {
      uint32_t magic;
      uint32_t request;
  } BootloaderFlag_t;
  ```

## Implementation Status

### Phase 1: Basic Infrastructure ⏳
- [x] Bootloader.h interface definition
- [x] Bootloader.c framework
- [x] CS14 button trigger detection
- [x] Memory layout documentation
- [ ] Linker script modifications (bootloader @ 0x08000000)
- [ ] Application linker script (start @ 0x08010000, VTOR setup)

### Phase 2: ST Protocol Implementation ⏳
- [ ] UART1 initialization (8E1, 115200 baud)
- [ ] Frame reception state machine
- [ ] Checksum calculation and verification
- [ ] Command dispatcher
- [ ] Individual command handlers:
  - [ ] Get (0x00)
  - [ ] Get Version (0x01)
  - [ ] Get ID (0x02)
  - [ ] Read Memory (0x11)
  - [ ] Go (0x21)
  - [ ] Write Memory (0x31)
  - [ ] Extended Erase (0x44)

### Phase 3: Display Integration ⏳
- [x] Display progress function interface
- [ ] ISP status display implementation
- [ ] Progress percentage calculation
- [ ] Display update during operations

### Phase 4: Application Integration ⏳
- [x] CS14 trigger in startup animation
- [x] Bootloader entry request function
- [ ] Software reset after flag set
- [ ] Application VTOR configuration
- [ ] Application vector table setup

### Complexity Assessment
- **Overall Difficulty**: ⭐⭐ (Easy-Medium)
- **ST Protocol Benefits**:
  - Simple XOR checksum vs CRC16 (saves 1-2 days)
  - Well-documented reference (saves 2-3 days debugging)
  - Existing tool compatibility (saves 3-5 days tool development)

### Time Estimates
- **Phase 1**: 2-3 days (linker scripts, infrastructure)
- **Phase 2**: 5-7 days (ST protocol implementation)
- **Phase 3**: 1-2 days (display integration)
- **Phase 4**: 1-2 days (application integration)
- **Phase 5**: 3-4 days (testing, Python tool)
- **Total**: 1.5-2 weeks (vs 2-3 weeks for custom protocol)

## Testing Checklist

### Basic Functionality
- [ ] Bootloader starts on power-on
- [ ] CS14 button triggers ISP mode
- [ ] Software flag triggers ISP mode
- [ ] Invalid app triggers ISP mode recovery
- [ ] Normal boot jumps to application
- [ ] Display shows correct ISP status

### ST Protocol Commands
- [ ] UART communication at 115200 8E1
- [ ] Get command returns command list
- [ ] Get Version returns protocol version
- [ ] Get ID returns 0x0450 (STM32H743)
- [ ] Read Memory reads Flash/RAM correctly
- [ ] Write Memory programs Flash correctly
- [ ] Extended Erase erases specified pages
- [ ] Go command jumps to application

### Error Handling
- [ ] NACK on invalid commands
- [ ] NACK on checksum errors
- [ ] NACK on invalid addresses
- [ ] Timeout exits ISP mode
- [ ] Protected region write rejection
- [ ] Display shows error conditions

### Integration Testing
- [ ] Full firmware update via Python tool
- [ ] Application runs after update
- [ ] Version display shows updated version
- [ ] CS14 trigger works in new firmware
- [ ] Multiple update cycles successful

## Notes

### Design Decisions
1. **ST Protocol Selection**: Chosen for proven reliability, tool compatibility, and reduced development time
2. **CS14 Single Press**: Simplified from CS3/3-press for better user experience
3. **Backup SRAM**: Retains flag across software reset without external dependencies
4. **Display Feedback**: Provides user visibility into ISP process without additional hardware

### Known Limitations
- Bootloader size limited to 64KB (currently sufficient)
- UART1 dedicated to ISP during bootloader mode
- Display updates may slow ISP operations slightly (non-blocking implementation mitigates)
- No encryption/authentication (can be added in future if needed)

### Future Enhancements
- Dual-bank Flash configuration for safer updates
- Application encryption/signing
- USB ISP port support
- Ethernet ISP port support
- Remote firmware update via existing network stack

## References

### ST Official Documentation
- **AN3155**: USART protocol used in the STM32 bootloader
- **AN2606**: STM32 microcontroller system memory boot mode
- **RM0433**: STM32H7x3 Reference Manual
- **PM0253**: STM32H7 Programming Manual

### Application Notes
- **AN4657**: STM32 in-application programming (IAP) using the USART
- **AN4852**: STM32 Cube in-application programming (IAP) using UART

### Development Tools
- STM32CubeProgrammer: Official ST programming tool (supports AN3155)
- Flash Loader Demonstrator: Legacy tool for UART bootloader
- Python pyserial library: For custom ISP tool development
