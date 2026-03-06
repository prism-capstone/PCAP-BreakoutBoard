/**
 * @file nn_inference.c
 * @brief Neural network inference implementation using ESP-TFLite-Micro
 *
 * This implementation uses Espressif's optimized TensorFlow Lite Micro port
 * which includes ESP-NN acceleration for better performance on ESP32 chips.
 */

#include "nn_inference.h"
#include "esp_log.h"
#include "esp_timer.h"

// TensorFlow Lite Micro headers
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "model_data.h"

static const char* TAG = "NN";

// Tensor arena size - adjust based on your model's requirements
// Start with 78KB and increase if needed
constexpr int kTensorArenaSize = 78 * 1024;

// Aligned tensor arena for better performance
static uint8_t tensor_arena[kTensorArenaSize] __attribute__((aligned(16)));

// TFLite Micro objects
static const tflite::Model* model = nullptr;
static tflite::MicroInterpreter* interpreter = nullptr;
static TfLiteTensor* input_tensor = nullptr;
static TfLiteTensor* output_tensor = nullptr;

// State tracking
static bool nn_ready = false;
static uint32_t last_inference_time_us = 0;
static uint32_t total_inference_time_us = 0;
static uint32_t inference_count = 0;

// Sliding window size (from scalers.json "window": 400)
#define NN_WINDOW_SIZE 400

// Circular buffer: one ring per sensor per chip
static float sensor_buffers[NUM_PCAP_CHIPS][NUM_SENSORS_PER_CHIP][NN_WINDOW_SIZE];
static int   buffer_head[NUM_PCAP_CHIPS][NUM_SENSORS_PER_CHIP];   // next write index
static int   buffer_count[NUM_PCAP_CHIPS][NUM_SENSORS_PER_CHIP];  // samples stored (0..NN_WINDOW_SIZE)

// Input scaler parameters (from scalers.json)
// StandardScaler: normalized = (value - mean) / scale
const float INPUT_SCALER_MEAN = 6.191217956661442f;
const float INPUT_SCALER_SCALE = 0.9138432435934595f;

// Op resolver - add only the operations your model needs to save memory
// Common ops for hysteresis models: FULLY_CONNECTED, RELU, TANH, etc.
static tflite::MicroMutableOpResolver<10> op_resolver;

// Normalize input using StandardScaler parameters
static inline float normalize_input(float raw_value)
{
    return (raw_value - INPUT_SCALER_MEAN) / INPUT_SCALER_SCALE;
}

bool nn_init(void)
{
    ESP_LOGI(TAG, "Initializing neural network inference engine");

    // Load the model
    model = tflite::GetModel(model_int8_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG, "Model schema version %lu does not match supported version %d",
                 model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    ESP_LOGI(TAG, "Model schema version OK");

    op_resolver.AddFullyConnected();
    op_resolver.AddRelu();
    op_resolver.AddTanh();
    op_resolver.AddSoftmax();
    op_resolver.AddReshape();
    op_resolver.AddQuantize();
    op_resolver.AddDequantize();

    ESP_LOGI(TAG, "Creating interpreter with arena size: %d bytes", kTensorArenaSize);

    // Create the interpreter
    static tflite::MicroInterpreter static_interpreter(
        model, op_resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    ESP_LOGI(TAG, "Interpreter created, allocating tensors...");

    // Allocate tensors
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        ESP_LOGE(TAG, "AllocateTensors failed with status: %d", allocate_status);
        ESP_LOGE(TAG, "Arena size: %d, Arena used: %zu", 
                 kTensorArenaSize, interpreter->arena_used_bytes());
        return false;
    }

    ESP_LOGI(TAG, "Tensors allocated successfully");

    // Get input and output tensors
    input_tensor = interpreter->input(0);
    output_tensor = interpreter->output(0);

    if (input_tensor == nullptr || output_tensor == nullptr) {
        ESP_LOGE(TAG, "Failed to get input/output tensors");
        return false;
    }

    // Log tensor information
    ESP_LOGI(TAG, "Model loaded successfully");
    ESP_LOGI(TAG, "Input tensor: dims=%d, type=%d",
             input_tensor->dims->size, input_tensor->type);
    ESP_LOGI(TAG, "Output tensor: dims=%d, type=%d",
             output_tensor->dims->size, output_tensor->type);
    ESP_LOGI(TAG, "Arena used: %zu bytes of %d bytes", 
             interpreter->arena_used_bytes(), kTensorArenaSize);

    nn_ready = true;
    return true;
}

void nn_compensate_chip(pcap_data_t* data, int chip_idx)
{
    for (int i = 0; i < NUM_SENSORS_PER_CHIP; i++) {
        float input = (PCAP_SCALING_NUM * (float)(data->raw[i] - data->offset[i])) / PCAP_CONVERSION_NUMBER;

        // Push sample into circular buffer
        sensor_buffers[chip_idx][i][buffer_head[chip_idx][i]] = input;
        buffer_head[chip_idx][i] = (buffer_head[chip_idx][i] + 1) % NN_WINDOW_SIZE;
        if (buffer_count[chip_idx][i] < NN_WINDOW_SIZE) {
            buffer_count[chip_idx][i]++;
        }

        // Pass through until the window is fully populated (~4 seconds at 100Hz)
        if (!nn_ready || buffer_count[chip_idx][i] < NN_WINDOW_SIZE) {
            data->final_val[i] = input;
            continue;
        }

        int64_t start_time = esp_timer_get_time();

        // Fill input tensor with the window oldest→newest
        int oldest = buffer_head[chip_idx][i];  // head points to oldest when buffer is full
        if (input_tensor->type == kTfLiteFloat32) {
            float* input_data = input_tensor->data.f;
            for (int j = 0; j < NN_WINDOW_SIZE; j++) {
                int buf_idx = (oldest + j) % NN_WINDOW_SIZE;
                input_data[j] = normalize_input(sensor_buffers[chip_idx][i][buf_idx]);
            }
        } else if (input_tensor->type == kTfLiteInt8) {
            int8_t* input_data = input_tensor->data.int8;
            float q_scale = input_tensor->params.scale;
            int q_zero = input_tensor->params.zero_point;
            for (int j = 0; j < NN_WINDOW_SIZE; j++) {
                int buf_idx = (oldest + j) % NN_WINDOW_SIZE;
                float norm = normalize_input(sensor_buffers[chip_idx][i][buf_idx]);
                input_data[j] = (int8_t)((int32_t)(norm / q_scale) + q_zero);
            }
        }

        // Run inference
        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk) {
            ESP_LOGW(TAG, "Inference failed chip=%d sensor=%d", chip_idx, i);
            data->final_val[i] = input;
            continue;
        }

        // Read output
        if (output_tensor->type == kTfLiteFloat32) {
            data->final_val[i] = output_tensor->data.f[0];
        } else if (output_tensor->type == kTfLiteInt8) {
            float q_scale = output_tensor->params.scale;
            int q_zero = output_tensor->params.zero_point;
            data->final_val[i] = (output_tensor->data.int8[0] - q_zero) * q_scale;
        }

        // Track timing
        int64_t end_time = esp_timer_get_time();
        last_inference_time_us = (uint32_t)(end_time - start_time);
        total_inference_time_us += last_inference_time_us;
        inference_count++;
    }
}

bool nn_is_ready(void)
{
    return nn_ready;
}

uint32_t nn_get_inference_time_us(void)
{
    if (inference_count == 0) {
        return 0;
    }
    return total_inference_time_us / inference_count;
}
