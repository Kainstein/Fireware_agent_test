/**
 * @file driver.h
 * @brief Central driver header file - includes all driver interfaces
 * @date 2026-01-07
 */

#ifndef __DRIVER_H__
#define __DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* HMI Drivers */
#include "hmi_button3116_driver.h"
#include "hmi_display72128_driver.h"
#include "hmi_led_driver.h"
#include "hmi_wrapper.h"

/* Sensor Drivers */
#include "eeprom24c64_driver.h"
#include "ism330is_driver.h"
#include "ism330is_api.h"
#include "mlx90614_api.h"
#include "mlx90614_wrapper.h"
#include "mlx90640_driver.h"
#include "mlx90640_api.h"
#include "mlx90640_wrapper.h"

/* Utilities */

#ifdef __cplusplus
}
#endif

#endif /* __DRIVER_H__ */
