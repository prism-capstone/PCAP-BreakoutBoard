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
 * @brief Run windowed inference on a full chip's data
 *
 * Maintains a 400-sample circular buffer per sensor and runs inference
 * once the window is full. Passes through raw values until then.
 *
 * @param data     Pointer to PCAP data structure (raw/offset used as input,
 *                 final_val updated with compensated output)
 * @param chip_idx Chip index (used to select the correct sample buffer)
 */
void nn_compensate_chip(pcap_data_t* data, int chip_idx);

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
