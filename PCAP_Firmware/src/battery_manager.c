/**
 * @file battery_manager.c
 * @brief Battery manager for monitoring battery life
 */

#include <stdio.h>
#include "battery_manager.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static esp_adc_cal_characteristics_t adc_chars;

void battery_adc_init(void)
{
    adc1_config_width(ADC_BITWIDTH);
    adc1_config_channel_atten(BAT_ADC_CHANNEL, ADC_ATTEN);

    esp_adc_cal_characterize(
        BAT_ADC_UNIT,
        ADC_ATTEN,
        ADC_BITWIDTH,
        DEFAULT_VREF,
        &adc_chars
    );
}

static uint8_t voltage_to_percent(float v)
{
    if (v >= 4.20f) return 100;
    if (v <= 3.30f) return 0;

    if (v > 4.00f) return (uint8_t)(80 + (v - 4.00f) * 100.0f);
    if (v > 3.85f) return (uint8_t)(60 + (v - 3.85f) * 133.3f);
    if (v > 3.70f) return (uint8_t)(40 + (v - 3.70f) * 133.3f);
    if (v > 3.55f) return (uint8_t)(20 + (v - 3.55f) * 133.3f);

    return (uint8_t)((v - 3.30f) * 80.0f);
}

float read_battery_voltage(void)
{
    uint32_t adc_accum = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        adc_accum += adc1_get_raw(BAT_ADC_CHANNEL);
    }

    uint32_t adc_avg = adc_accum / NUM_SAMPLES;

    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_avg, &adc_chars);

    // Compensate for resistor divider
    return (voltage_mv * DIVIDER_RATIO) / 1000.0f;
}

uint8_t battery_get_percentage(void)
{
    static float filtered_v = 4.2f;   // simple smoothing
    float v = read_battery_voltage();

    // Low-pass filter to reduce ADC noise
    filtered_v = 0.8f * filtered_v + 0.2f * v;

    return voltage_to_percent(filtered_v);
}