/**
 * @file battery_manager.h
 * @brief Helper functions to supervise battery levels.
 *
 */

#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// For the new board version use the uncommented one
// #define BAT_ADC_CHANNEL ADC1_CHANNEL_0   // GPIO0 on ESP32-C3

// For the old board version use anaolog channel 4.
#define BAT_ADC_CHANNEL ADC_CHANNEL_4    // GPIO4 on ESP32-C3

#define BAT_ADC_UNIT        ADC_UNIT_1
#define ADC_ATTEN           ADC_ATTEN_DB_12    // Changed from DB_11 to DB_12
#define ADC_BITWIDTH        ADC_BITWIDTH_12

#define DEFAULT_VREF        1100               // mV
#define NUM_SAMPLES         16
#define DIVIDER_RATIO       2.0f               // 1:1 divider

/**
 * @brief Initialize ADC and calibration for battery voltage measurement.
 *
 * This must be called once before calling read_battery_voltage().
 */
void battery_adc_init(void);

/**
 * @brief Get battery percentage (0–100%).
 *
 * Performs ADC sampling, averaging, calibration,
 * divider compensation, and voltage→percentage mapping.
 */
uint8_t battery_get_percentage(void);

/**
 * @brief Read battery voltage in volts.
 *
 * Performs ADC sampling, averaging, calibration, and divider compensation.
 *
 * @return Battery voltage in volts (V)
 */
float read_battery_voltage(void);

#ifdef __cplusplus
}
#endif

#endif // BATTERY_MANAGER_H