/**
 * @file model_data.h
 * @brief TensorFlow Lite model data for hysteresis compensation
 *
 * IMPORTANT: This is a placeholder file!
 *
 * To generate your actual model data:
 *
 * 1. Train your model in Python/TensorFlow
 * 2. Convert to TFLite with INT8 quantization:
 *
 *    import tensorflow as tf
 *    import numpy as np
 *
 *    # Load your trained model
 *    model = tf.keras.models.load_model('hysteresis_model.h5')
 *
 *    # Convert with INT8 quantization for ESP32
 *    converter = tf.lite.TFLiteConverter.from_keras_model(model)
 *    converter.optimizations = [tf.lite.Optimize.DEFAULT]
 *    converter.target_spec.supported_types = [tf.int8]
 *    converter.inference_input_type = tf.int8
 *    converter.inference_output_type = tf.int8
 *
 *    # Provide representative dataset for calibration
 *    def representative_dataset():
 *        for sample in calibration_samples:
 *            yield [sample.astype(np.float32)]
 *    converter.representative_dataset = representative_dataset
 *
 *    tflite_model = converter.convert()
 *
 *    # Save as C array
 *    with open('model_data.h', 'w') as f:
 *        f.write('#ifndef MODEL_DATA_H\n')
 *        f.write('#define MODEL_DATA_H\n\n')
 *        f.write('alignas(8) const unsigned char g_model_data[] = {\n')
 *        for i, byte in enumerate(tflite_model):
 *            f.write(f'0x{byte:02x},')
 *            if (i + 1) % 12 == 0:
 *                f.write('\n')
 *        f.write('\n};\n')
 *        f.write(f'const unsigned int g_model_data_len = {len(tflite_model)};\n\n')
 *        f.write('#endif // MODEL_DATA_H\n')
 *
 * 3. Replace this file with the generated model_data.h
 */

#ifndef MODEL_DATA_H
#define MODEL_DATA_H

#include <stdint.h>

// Placeholder model - a minimal valid TFLite model structure
// This will NOT work for actual inference - replace with your real model!

// NOTE: When you have your real model, ensure it has:
// - Input shape matching your sensor data (e.g., [1, 6] for 6 sensors)
// - Output shape matching your compensated output
// - Operations that are added in nn_inference.c op_resolver

alignas(8) const unsigned char g_model_data[] = {
    // This is a placeholder - replace with your actual model bytes
    // The real model will be much larger (typically 10KB - 100KB+)
    0x00
};

const unsigned int g_model_data_len = sizeof(g_model_data);

// Define this to skip NN initialization during testing without a model
#define NN_MODEL_PLACEHOLDER 1

#endif // MODEL_DATA_H
