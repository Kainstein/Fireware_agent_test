# Quick Start Guide - Button & Display Control

## Fast Integration (5 minutes)

### 1. Add Source Files to Keil Project
In Keil µVision Project window, add these files to the "User" group:
```
☑ User/app_state.c
☑ User/app_control.c
☑ User/app_button_handler.c
☑ User/display72128_wrapper.c
```

### 2. Edit freertos.c
Open `Core/Src/freertos.c` and add these three lines at the top (after existing includes):
```c
#include "app_control.h"
#include "app_button_handler.h"
#include "display_key_wrapper.h"
```

Find the `StartTask03` function and replace the loop content:
```c
void StartTask03(void *argument)
{
    // Add this initialization BEFORE the loop
    AppControl_Init();
    
    for(;;)
    {
        uint32_t current_time = HAL_GetTick();
        
        DisplayKey_ProcessButtons(current_time);
        
        DisplayKeyEvent_t event;
        while (DisplayKey_GetEvent(&event)) {
            AppButton_HandleEvent(&event);
        }
        
        AppControl_ProcessContinuousAdjustment(current_time);
        AppControl_UpdateDisplayFlash(current_time);
        AppControl_UpdateDisplay();
        
        osDelay(50);
    }
}
```

### 3. Build & Test
1. Click **Build** (F7)
2. Fix any compile errors (usually missing includes)
3. **Download** to STM32H743
4. Open serial terminal (115200 baud)
5. Press buttons and watch display/UART output

---

## Quick Test Procedure

### Test 1: Temperature Control
1. **Press CS1** → Temperature increases (+0.5°C), display flashes for 3s
2. **Hold CS1** → Temperature continuously increases
3. **Press CS2** → Temperature decreases
4. **Press CS0** → Toggle heater ON/OFF

### Test 2: BLDC Motor Control
1. **Press CS6** → RPM increases (+10), display flashes for 3s
2. **Hold CS6** → RPM continuously increases
3. **Press CS5** → RPM decreases
4. **Press CS7** → Toggle motor ON/OFF

### Test 3: Motor Positioning
1. **Quick press CS3** → Tilt forward (step)
2. **Hold CS3** → Tilt forward (continuous)
3. **Quick press CS14** → Tilt reverse (step)
4. **Hold CS8/CS9** → Image motor left/right

---

## Expected UART Output

```
[Task03] Button/Display control initialized
[Task03] Temperature setpoint: 25.0°C
[Task03] BLDC setpoint: 0 RPM

[CS1] TEMP_UP pressed - setpoint: 25.5°C
[CS1] TEMP_UP released - final setpoint: 28.0°C

[CS0] TEMP_ON_OFF toggled: ON

[CS6] BLDC_UP pressed - setpoint: 10 RPM
[CS6] BLDC_UP released - final setpoint: 120 RPM

[CS7] BLDC_ON_OFF toggled: ON
```

---

## Common Issues & Fixes

| Problem | Solution |
|---------|----------|
| Display shows nothing | Check I2C2 pins, verify ZLG72128 power |
| Buttons not working | Check CY8CMBR3116 on I2C2, verify wiring |
| Flash not visible | Increase brightness in Display72128_Init() |
| Compile error: undefined reference | Add all .c files to Keil project |
| Values not saving | Check M24C64 EEPROM on I2C3 |

---

## Customization

### Change Flash Timing
Edit `User/app_state.h`:
```c
#define DISPLAY_FLASH_TIMEOUT_MS    3000    // 3s → 5s
#define DISPLAY_FLASH_TOGGLE_MS     300     // 300ms → 500ms
```

### Change Adjustment Speed
Edit `User/app_state.h`:
```c
#define ADJUST_INTERVAL_MS          100     // 100ms → 200ms
```

### Change Temperature Range
Edit `User/app_state.c`:
```c
void AppState_SetTempSetpoint(float value)
{
    if (value < 0.0f) value = 0.0f;
    if (value > 220.0f) value = 220.0f;  // Change max
    // ...
}
```

---

## Next Steps

✅ **Done**: Basic button/display control working  
⬜ **TODO**: Connect temperature sensor feedback  
⬜ **TODO**: Connect BLDC motor feedback  
⬜ **TODO**: Implement motor driver calls  
⬜ **TODO**: Add PID temperature control  

See [Button_Display_Implementation.md](Button_Display_Implementation.md) for complete documentation.
