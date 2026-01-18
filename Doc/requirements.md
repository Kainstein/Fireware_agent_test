# STM32H743 Firmware Requirements
**Last Updated**: 2026-01-02

---

## Hardware

### MCU
STM32H743ZIT6 (Cortex-M7, 480MHz, 2MB Flash, 1MB RAM)

### Peripherals
| Device | Model | Bus | Address | Purpose |
|--------|-------|-----|---------|---------|
| Thermal Camera | MLX90640 | I2C1 | 0x33 | 32x24 thermal array |
| IMU | ISM330IS | I2C2 | - | Motion sensing |
| IR Sensor | MLX90614 | I2C1 | 0x5A | Point temperature |
| EEPROM | M24C64 | I2C3 | - | Persistent storage (8KB) |
| Display | ZLG72128 | I2C2 | 0x60/61 | Dual 7-seg LED |
| Buttons | CY8CMBR3116 | I2C2 | 0x37 | 16-ch capacitive touch |

---

## Display Configuration

**Group1 (BLDC)**: 5-digit, COM[4:0], rightmost=COM0  
**Group2 (Temp)**: 4-digit, COM[8:5], rightmost=COM5

### Modes
- **Running**: Show actual value (ON) or "OFF"
- **Setting**: Show setpoint, flash last bit, 3s timeout

---

## Button Functions

| Pin | Function | Event | Action | Display |
|-----|----------|-------|--------|---------|
| CS0 | TEMP_ON_OFF | SINGLE | Toggle heater | None |
| CS1 | TEMP_UP | PRESS/REL | ±0.5°C; hold: 0.5°C/100ms | Flash G2, 3s |
| CS2 | TEMP_DOWN | PRESS/REL | ±0.5°C; hold: 0.5°C/100ms | Flash G2, 3s |
| CS3 | TILT_FWD | PRESS/REL | <200ms: step; ≥200ms: cont. | None |
| CS5 | BLDC_DOWN | PRESS/REL | ±10 RPM; hold: 10 RPM/100ms | Flash G1, 3s |
| CS6 | BLDC_UP | PRESS/REL | ±10 RPM; hold: 10 RPM/100ms | Flash G1, 3s |
| CS7 | BLDC_ON_OFF | SINGLE | Toggle motor | None |
| CS8 | IMAGE_RIGHT | PRESS/REL | <200ms: step; ≥200ms: cont. | None |
| CS9 | IMAGE_LEFT | PRESS/REL | <200ms: step; ≥200ms: cont. | None |
| CS14 | TILT_REV | PRESS/REL | <200ms: step; ≥200ms: cont. | None |
| CS10-13 | Reserved | - | Future use | - |

---

## Temperature Control (Group2)

**Range**: -20.0 to 220.0°C | **Step**: 0.5°C (normal), 2.0°C (fast after 5s) | **Format**: XXX.X | **Default**: -10.0°C

### First-Press Optimization (2026-01-02)
1. **1st press (IDLE)**: Display setpoint only, NO value change, enter setting mode
2. **Hold ≥100ms**: Begin continuous adjustment (+0.5°C/100ms)
3. **2nd+ press (ADJUSTING)**: Immediate adjustment
4. **Cross-button** (CS1→CS2): 2nd press adjusts immediately
5. **EEPROM**: Write only on actual adjustments, not display-only press

---

## BLDC Motor Control (Group1)

**Range**: 0-22000 RPM | **Step**: 10 RPM (normal), 50 RPM (fast after 5s) | **Format**: XXXXX | **Default**: 0 RPM

### First-Press Optimization (2026-01-02)
Same behavior as temperature (display-only 1st press, adjust on 2nd+ or hold)

---

## Timing Parameters

| Parameter | Value | Notes |
|-----------|-------|-------|
| Debounce | 30ms | Button stability |
| Adjustment interval | 100ms | Continuous rate (CS1/2/5/6) |
| Acceleration threshold | 5000ms | Switch to fast steps after 5s hold |
| First-press hold | 100ms | Triggers adjustment |
| Flash toggle | 300ms | ON/OFF cycle |
| Flash timeout | 3000ms | Exit setting mode |
| Motor threshold | 200ms | Step vs continuous (CS3/8/9/14) |
| EEPROM debounce | 500ms | Write delay |
| Task loop | 50ms | 20Hz polling |

---

## Data Persistence

**Saved**: Temp setpoint, BLDC setpoint  
**Not Saved**: ON/OFF states (start OFF for safety)

---

## Display Format

**Group1**: `22000`, `1500`, ` 100`, `  OFF` (right-aligned)  
**Group2**: `220.0`, `100.5`, ` 5.5`, `  OFF` (right-aligned)

---

## Key Implementation Notes

1. **State Tracking**: Use `ADJUST_STATE_IDLE` vs `ADJUST_STATE_ADJUSTING`
2. **EEPROM Protection**: Check state before setting write flag
3. **Mode Independence**: ON/OFF ≠ display mode
4. **Flash Element**: Last bit only (COM0 or COM5), others static
5. **Safety**: All systems start OFF at power-on

## 1. Button Pin Assignment & Functions

| CS Pin | Name | Function | Event Type | Adjustment Step | Display Effect |
|--------|------|----------|-----------|-----------------|----------------|
| **CS0** | TEMP_ON_OFF | Toggle heater ON/OFF | SINGLE_PRESS | - | No flash |
| **CS1** | TEMP_UP | +0.5°C per press; hold +0.5°C/100ms | PRESSED/RELEASED | 0.5°C | Flash Group2 last bit |
| **CS2** | TEMP_DOWN | -0.5°C per press; hold -0.5°C/100ms | PRESSED/RELEASED | 0.5°C | Flash Group2 last bit |
| **CS3** | TILT_FORWARD | Tilt motor forward | PRESSED/RELEASED | - | No flash |
| **CS4** | - | Not used | - | - | - |
| **CS5** | BLDC_SPEED_DOWN | -10 RPM per press; hold -10 RPM/100ms | PRESSED/RELEASED | 10 RPM | Flash Group1 last bit |
| **CS6** | BLDC_SPEED_UP | +10 RPM per press; hold +10 RPM/100ms | PRESSED/RELEASED | 10 RPM | Flash Group1 last bit |
| **CS7** | BLDC_ON_OFF | Toggle motor ON/OFF | SINGLE_PRESS | - | No flash |
| **CS8** | IMAGE_MOVE_RIGHT | Image motor right | PRESSED/RELEASED | - | No flash |
| **CS9** | IMAGE_MOVE_LEFT | Image motor left | PRESSED/RELEASED | - | No flash |
| **CS10** | - | Reserved | - | - | - |
| **CS11** | - | Reserved | - | - | - |
| **CS12** | - | Reserved | - | - | - |
| **CS13** | - | Reserved | - | - | - |
| **CS14** | TILT_REVERSE | Tilt motor reverse | PRESSED/RELEASED | - | No flash |
| **CS15** | - | Not used | - | - | - |

---

## 2. Display System Architecture

### Display Groups
- **Group1 (BLDC)**: 5-digit LED (COM[4:0], COM0 = rightmost/last bit)
- **Group2 (Temperature)**: 4-digit LED (COM[8:5], COM5 = rightmost/last bit)

### Operating Modes (Per Group)

Each group operates in one of two modes:

#### A. Running Mode (Default at power-on)
**Purpose**: Display real-time operational data

| System State | Display Content |
|--------------|-----------------|
| ON | Actual measured value (e.g., "25.3°C", "1500 RPM") |
| OFF | "OFF" text (literal characters) |

#### B. Setting Mode (Triggered by adjustment buttons)
**Purpose**: Display and edit setpoint values

| System State | Display Content |
|--------------|-----------------|
| ON or OFF | Setpoint value (e.g., "105.0°C", "2200 RPM") with **last bit flashing** |

**Key Point**: Setting mode is **independent** from ON/OFF state. You can adjust setpoint while system is OFF.

---

## 3. Mode Switching Rules

### Enter Setting Mode
**Trigger**: Press CS1, CS2, CS5, or CS6
- **Group1**: CS5 (BLDC_DOWN) or CS6 (BLDC_UP) → Enter setting mode
- **Group2**: CS1 (TEMP_UP) or CS2 (TEMP_DOWN) → Enter setting mode

**Actions on entering**:
1. Switch display to show setpoint value
2. Start flashing last bit
3. Start 3-second timeout timer

### Exit Setting Mode (Return to Running Mode)
**Trigger**: Automatic timeout only
- **Timeout**: 3 seconds after last button press
- **CS0/CS7 (ON/OFF buttons)**: Do NOT affect mode - mode only exits via timeout
- **Restart timer**: Any press of CS1/CS2/CS5/CS6 during the 3 seconds resets timer to 0

**Actions on exiting**:
1. Stop flashing
2. Switch display to show actual value (if ON) or "OFF" (if OFF)

---

## 4. Flash Display Specification

### Visual Behavior
**Example**: Setpoint is "105.0°C"
- **t=0ms**: Display shows "**105.0**"
- **t=300ms**: Display shows "**105.**" (last digit OFF, decimal point remains)
- **t=600ms**: Display shows "**105.0**" (last digit ON)
- **t=900ms**: Display shows "**105.**" (last digit OFF)
- ... continues for 3 seconds total

### Flash Parameters
| Parameter | Value | Configurable |
|-----------|-------|--------------|
| Flash toggle period | 300ms | Yes (#define DISPLAY_FLASH_TOGGLE_MS) |
| Flash duration | 3 seconds | Yes (#define DISPLAY_FLASH_TIMEOUT_MS) |
| Flashing element | Last bit only (COM0 or COM5) | No |
| Other digits | Static (always visible) | No |

### Flash Control Logic
- **Start conditions**:
  - Group1: Press CS5 or CS6
  - Group2: Press CS1 or CS2
- **Restart conditions**: Press CS1/CS2/CS5/CS6 again within 3 seconds → reset timer to 0
- **Stop conditions**: 3-second timeout expires

---

## 5. Button Behavior Details

### A. Temperature Control (Group2)

#### CS0 (TEMP_ON_OFF)
- **Function**: Toggle heater power
- **Event**: SINGLE_PRESS
- **Action**: Toggle ON ↔ OFF
- **Display impact**: 
  - Does NOT change mode
  - If in running mode: switches between actual value and "OFF"
  - If in setting mode: no display change (stays in setting mode)

#### CS1 (TEMP_UP) & CS2 (TEMP_DOWN)
- **Function**: Adjust temperature setpoint
- **Events**: PRESSED + RELEASED
- **Range**: 0.0°C to 220.0°C
- **Adjustment**:
  - **Single press**: +0.5°C (or -0.5°C) once
  - **Hold**: +0.5°C (or -0.5°C) every 100ms continuously
- **Display impact**:
  - Enters setting mode
  - Shows setpoint with last bit flashing
  - Resets 3-second timer on each press

---

### B. BLDC Motor Control (Group1)

#### CS7 (BLDC_ON_OFF)
- **Function**: Toggle motor power
- **Event**: SINGLE_PRESS
- **Action**: Toggle ON ↔ OFF
- **Display impact**:
  - Does NOT change mode
  - If in running mode: switches between actual RPM and "OFF"
  - If in setting mode: no display change

#### CS5 (BLDC_SPEED_DOWN) & CS6 (BLDC_SPEED_UP)
- **Function**: Adjust motor speed setpoint
- **Events**: PRESSED + RELEASED
- **Range**: 0 RPM to 22000 RPM
- **Adjustment**:
  - **Single press**: +10 RPM (or -10 RPM) once
  - **Hold**: +10 RPM (or -10 RPM) every 100ms continuously
- **Display impact**:
  - Enters setting mode
  - Shows setpoint with last bit flashing
  - Resets 3-second timer on each press

---

### C. Tilt Motor Control (Stepper)

#### CS3 (TILT_FORWARD) & CS14 (TILT_REVERSE)
- **Function**: Manual tilt positioning
- **Events**: PRESSED + RELEASED
- **Behavior**:
  - **PRESSED**: Motor starts immediately (forward or reverse)
  - **RELEASED**: Motor stops immediately
  - **Short press (<200ms)**: Single step movement
  - **Long press (≥200ms)**: Continuous movement (runs while held, stops on release)
- **No display feedback**: No dedicated display for tilt position

---

### D. Image Stepper Motor Control

#### CS8 (IMAGE_MOVE_RIGHT) & CS9 (IMAGE_MOVE_LEFT)
- **Function**: Manual image positioning
- **Events**: PRESSED + RELEASED
- **Behavior**:
  - **PRESSED**: Motor starts immediately (right or left)
  - **RELEASED**: Motor stops immediately
  - **Short press (<200ms)**: Single step movement
  - **Long press (≥200ms)**: Continuous movement (runs while held, stops on release)
- **No display feedback**: No dedicated display for image position

---

## 6. Timing Parameters Summary

| Parameter | Value | Purpose |
|-----------|-------|---------|
| Button debounce | 30ms | Stable press detection |
| Adjustment interval (hold) | 100ms | Repeat rate for CS1/CS2/CS5/CS6 |
| Flash toggle period | 300ms | ON/OFF cycle for last bit |
| Flash timeout | 3000ms | Auto-exit setting mode |
| Motor step threshold | 200ms | Short press vs continuous |
| EEPROM write debounce | 500ms | Delay after last change before saving |

---

## 7. EEPROM Storage Policy

### Saved to EEPROM (Persistent)
- Temperature setpoint (°C)
- BLDC speed setpoint (RPM)

### NOT Saved to EEPROM
- ON/OFF states (heater and motor) - **Always start OFF at power-on**
- Display mode (setting vs running)
- Flash state

### Write Strategy
- **Debounced write**: Wait 500ms after last setpoint change
- **Purpose**: Avoid excessive EEPROM wear during continuous adjustment
- **Implementation**: Set pending write flag → commit in main loop after timeout

---

## 8. Power-On Behavior

**Initial State**:
1. Load temperature setpoint from EEPROM (default: -10.0°C if invalid/empty)
2. Load BLDC speed setpoint from EEPROM (default: 0 RPM if invalid/empty)
3. Set heater to **OFF**
4. Set BLDC motor to **OFF**
5. Set both groups to **Running Mode**
6. Display "OFF" on both Group1 and Group2

**Rationale**: Safety - always start with systems disabled, require explicit user activation.

---

## 9. Display Value Format

### Group1 (BLDC Speed)
- **Format**: XXXXX (5-digit integer, no decimal)
- **Range**: 0 - 22000
- **Alignment**: Right-aligned, no leading zeros
- **Examples**:
  - 100 RPM → "  100" (uses COM2-COM0)
  - 1500 RPM → " 1500" (uses COM3-COM0)
  - 22000 RPM → "22000" (uses COM4-COM0)
- **OFF state**: Display "  OFF" text (right-aligned)

### Group2 (Temperature)
- **Format**: XXX.X (3.1 float, with decimal point)
- **Range**: -20.0 to 220.0
- **Alignment**: Right-aligned, no leading zeros
- **Negative display**: Minus sign shown on leftmost available position
- **Examples**:
  - -20.0°C → "-20.0" (uses COM8-COM5, minus on COM8)
  - -10.5°C → "-10.5" (uses COM8-COM5, minus on COM8)
  - -9.5°C → " -9.5" (uses COM7-COM5, minus on COM7)
  - 5.5°C → "  5.5" (uses COM6-COM5)
  - 100.5°C → "100.5" (uses COM8-COM5)
  - 220.0°C → "220.0" (uses COM8-COM5)
- **OFF state**: Display "  OFF" text (right-aligned)

---

## 10. Key Design Principles

### Independence of Controls
✅ **ON/OFF control** (CS0, CS7) and **Display mode** (CS1/CS2/CS5/CS6) are completely independent
- You can adjust setpoint while system is OFF
- Toggling ON/OFF does not change display mode
- Display mode only changes via adjustment buttons or timeout

### Flash Purpose
✅ **Visual feedback** that user is in "adjustment mode"
- Last bit flashing = "you are editing the setpoint"
- Flash stops = "setting complete, now showing real data"

### Motor Safety
✅ **Direct control** - motors stop immediately when button released
- No latching behavior for tilt/image motors
- Operator has full control at all times

### EEPROM Protection
✅ **Debounced writes** prevent excessive wear
- Wait 500ms after last change
- Typical scenario: user adjusts → holds → releases → wait 500ms → save once



# 2026-01-02 0920

## 11. CS1/CS2/CS5/CS6 Button Optimization - First Press Display Behavior (2026-01-02)

### Requirements Summary
Optimize the adjustment button behavior to provide better user feedback by distinguishing between "display-only" and "adjustment" actions.

### Functional Specification

#### First Press Behavior (within 3-second timer window)
**Trigger**: First press of CS1, CS2, CS5, or CS6 when in IDLE state (running mode)

**Actions**:
1. ✅ Enter setting mode (switch display to show setpoint)
2. ✅ Start flashing last bit (COM0 for Group1, COM5 for Group2)
3. ✅ Start 3-second timeout timer
4. ❌ **DO NOT adjust value immediately** (display-only)
5. ✅ Enable continuous adjustment mechanism for hold detection

**If first button is held** (without release):
- After 100ms: Begin continuous adjustment (+0.5°C or +10 RPM every 100ms)
- Timer resets on each adjustment

#### Second and Subsequent Press Behavior
**Trigger**: Press CS1, CS2, CS5, or CS6 when already in ADJUSTING state (setting mode active)

**Actions**:
1. ✅ **Immediately adjust value** (+0.5°C/-0.5°C or +10 RPM/-10 RPM)
2. ✅ Restart 3-second timer
3. ✅ Continue flashing last bit
4. ✅ Enable continuous adjustment if held

#### Cross-Button Coordination
**Scenario**: Press CS1 (TEMP_UP), then press CS2 (TEMP_DOWN) within 3 seconds

**Behavior**:
- CS1 (first press): Display setpoint, no adjustment
- CS2 (second press): **Immediate adjustment** (-0.5°C)
- **Rationale**: User intent is clear after any second button press within the same group

### Implementation Requirements

#### 1. State Tracking
- Use existing `ADJUST_STATE_IDLE` vs `ADJUST_STATE_ADJUSTING` to differentiate:
  - `IDLE`: Next press is first press (display-only, unless held 100ms)
  - `ADJUSTING`: Next press adjusts immediately

#### 2. EEPROM Write Protection
**Rule**: First display-only press must NOT trigger EEPROM write
- Only write to EEPROM when actual value changes occur
- Applies to both single press events and hold/continuous adjustments

#### 3. Hold vs Multi-Press Detection
**Specification**:
- **First press + hold**: Start continuous adjustment after 100ms delay
- **First press + release + second press**: Second press adjusts immediately
- **Mechanism**: Existing `temp_last_adjust = current_time + 100ms` logic handles this automatically

### Timing Parameters (Unchanged)
| Parameter | Value | Purpose |
|-----------|-------|---------|
| First continuous adjustment delay | 100ms | Delay before first adjustment on hold |
| Continuous adjustment interval | 100ms | Repeat rate during hold |
| Flash timeout | 3000ms | Auto-exit setting mode |

### Example Scenarios

#### Scenario A: Quick Press (Display Only)
1. User presses CS1 → Display shows setpoint "100.5°C", no value change
2. User releases CS1 quickly (< 100ms) → Still shows "100.5°C"
3. Timer counts down, after 3s → Returns to running mode

#### Scenario B: Press and Hold (Display → Adjust)
1. User presses CS1 → Display shows setpoint "100.5°C", no immediate change
2. User holds CS1 for >100ms → Value becomes "101.0°C", then "101.5°C", etc.
3. User releases CS1 → Timer continues, flash active
4. After 3s → Returns to running mode

#### Scenario C: Multiple Quick Presses
1. User presses CS1 → Display shows "100.5°C", no adjustment
2. User releases CS1 quickly
3. User presses CS1 again (within 3s) → **Immediately adjusts** to "101.0°C"
4. User presses CS1 again → Adjusts to "101.5°C"
5. Timer resets on each press, after 3s idle → Returns to running mode

#### Scenario D: Cross-Button Operation
1. User presses CS1 → Display shows "100.5°C", no adjustment
2. User releases CS1
3. User presses CS2 (within 3s) → **Immediately adjusts** to "100.0°C"
4. User continues pressing CS2 → "99.5°C", "99.0°C", etc.

### Benefits
- ✅ **Better UX**: User can check current setpoint without accidentally changing it
- ✅ **Safety**: Reduces risk of unintentional adjustments
- ✅ **Efficiency**: Fewer unnecessary EEPROM writes
- ✅ **Ergonomic**: Hold-to-adjust still works naturally for rapid changes

---

## 12. Acceleration Feature - Fast Adjustment Mode (2026-01-02)

### Requirements Summary
Provide two-speed adjustment mode: fine control for short holds, coarse control for long holds.

### Functional Specification

#### Normal Mode (0-5 seconds)
**Trigger**: Button held for less than 5 seconds

**Step Sizes**:
- Temperature: **0.5°C per 100ms**
- BLDC: **10 RPM per 100ms**

**Purpose**: Fine/precise adjustment for target values close to desired setpoint

#### Fast Mode (≥5 seconds)
**Trigger**: Button held continuously for 5 seconds or more

**Step Sizes**:
- Temperature: **2.0°C per 100ms** (4x faster)
- BLDC: **50 RPM per 100ms** (5x faster)

**Purpose**: Coarse/rapid adjustment for large range changes

### Behavior

1. **Initial press**: Display setpoint (first-press optimization applies)
2. **0-100ms hold**: No adjustment (first-press display-only)
3. **100ms-5s hold**: Adjust with normal steps (0.5°C / 10 RPM)
4. **≥5s hold**: Automatically switch to fast steps (2.0°C / 50 RPM)
5. **Release**: Return to IDLE state on next timeout

### Implementation Details

**Macro Definitions** (hmi_handler.h):
```c
#define ADJUST_STEP_TEMP            0.5f    // Normal temperature step (°C)
#define ADJUST_STEP_BLDC            10      // Normal BLDC step (RPM)
#define ADJUST_STEP_TEMP_FAST       2.0f    // Fast temperature step (°C)
#define ADJUST_STEP_BLDC_FAST       50      // Fast BLDC step (RPM)
#define ADJUST_ACCEL_THRESHOLD_MS   5000    // Acceleration threshold (5s)
```

**State Tracking**:
- `temp_adjust_start`: Timestamp when temp button pressed
- `bldc_adjust_start`: Timestamp when BLDC button pressed
- Hold duration calculated as: `current_time - adjust_start`

**Step Selection Logic**:
```c
float step = (hold_duration >= ADJUST_ACCEL_THRESHOLD_MS) 
             ? ADJUST_STEP_TEMP_FAST 
             : ADJUST_STEP_TEMP;
```

### Example Scenarios

#### Scenario A: Fine Adjustment (Short Hold)
1. User presses CS1 (TEMP_UP) → Display shows "100.0°C"
2. User holds for 2 seconds → "100.5, 101.0, 101.5, 102.0, ..." (0.5°C steps)
3. User releases → Final value "110.0°C"
4. **Total change**: +10°C in 2 seconds

#### Scenario B: Coarse Adjustment (Long Hold)
1. User presses CS1 (TEMP_UP) → Display shows "20.0°C"
2. User holds for 7 seconds:
   - 0-5s: "20.5, 21.0, 21.5, ... 45.0" (0.5°C steps, +25°C)
   - 5-7s: "47.0, 49.0, 51.0, ... 85.0" (2.0°C steps, +40°C)
3. User releases → Final value "85.0°C"
4. **Total change**: +65°C in 7 seconds

#### Scenario C: BLDC Coarse Adjustment
1. User presses CS6 (BLDC_UP) → Display shows "100 RPM"
2. User holds for 10 seconds:
   - 0-5s: Normal steps reach "600 RPM" (+500 RPM)
   - 5-10s: Fast steps reach "3100 RPM" (+2500 RPM)
3. User releases → Final value "3100 RPM"
4. **Total change**: +3000 RPM in 10 seconds

### Benefits

- ✅ **Ergonomic**: Short taps for fine-tuning, long holds for coarse changes
- ✅ **Efficient**: Reach distant values quickly without excessive button presses
- ✅ **Intuitive**: Automatic speed-up without mode switching
- ✅ **Universal**: Works for all 4 adjustment buttons (CS1/CS2/CS5/CS6)

### Notes

1. Acceleration is **seamless** - no visual indication of mode change
2. Separate tracking for temperature and BLDC groups
3. Timer resets on button release
4. Compatible with first-press optimization (display-only 1st press)
5. EEPROM write protection still applies (no write on display-only press)