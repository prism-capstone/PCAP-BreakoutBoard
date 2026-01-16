/**
 * @file nn_inference.h
 * @brief Neural network inference for hysteresis compensation (ESP-IDF TFLite Micro)
 *
 * This module provides neural network inference capabilities for compensating
 * hysteresis in PCAP capacitive sensor readings using TensorFlow Lite Micro.
 */

#ifndef NN_INFERENCE_H
#define NN_INFERENCE_H

#include <stdint.h>
#include <stdbool.h>
#include "pcap04_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the neural network inference engine
 *
 * Loads the TFLite model and allocates the tensor arena.
 * Must be called before any inference operations.
 *
 * @return true if initialization successful, false otherwise
 */
bool nn_init(void);

/**
 * @brief Run inference to compensate for hysteresis
 *
 * Takes raw sensor readings and applies the neural network model
 * to produce hysteresis-compensated output values.
 *
 * @param raw_input Array of raw sensor readings (float)
 * @param compensated_output Array to store compensated values (float)
 * @param num_sensors Number of sensor values to process
 */
void nn_compensate(const float* raw_input, float* compensated_output, int num_sensors);

/**
 * @brief Run inference on a full chip's data
 *
 * Convenience function that processes all sensors for one PCAP chip.
 * Updates the final_val field in the pcap_data structure with
 * hysteresis-compensated values.
 *
 * @param data Pointer to PCAP data structure (raw values used as input,
 *             final_val updated with compensated output)
 */
void nn_compensate_chip(pcap_data_t* data);

/**
 * @brief Check if the neural network is ready for inference
 *
 * @return true if model is loaded and ready, false otherwise
 */
bool nn_is_ready(void);

/**
 * @brief Get inference timing statistics
 *
 * @return Average inference time in microseconds
 */
uint32_t nn_get_inference_time_us(void);

#ifdef __cplusplus
}
#endif

#endif // NN_INFERENCE_H
