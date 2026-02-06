/**
 * @file pcap_driver.c
 * @brief High-level driver implementation for PCAP04 capacitive sensor system (ESP-IDF)
 */

#include "pcap_driver.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "string.h"

static const char* TAG = "PCAP";

static spi_device_handle_t spi_handle;
static const uint8_t sensor_addr[NUM_SENSORS_PER_CHIP] = {0x00, 0x04, 0x08, 0x0C, 0x10, 0x14};

// Internal SPI transfer functions
static uint8_t spi_transfer_byte(uint8_t data);
static void spi_transfer_bytes(const uint8_t* tx_data, uint8_t* rx_data, size_t len);

void pcap_driver_init(void)
{
    ESP_LOGI(TAG, "Initializing PCAP driver");

    // Initialize multiplexer
    mux_init();

    // Configure SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PCAP_SPI_MOSI_PIN,
        .miso_io_num = PCAP_SPI_MISO_PIN,
        .sclk_io_num = PCAP_SPI_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return;
    }

    // Configure SPI device (CS is handled by multiplexer)
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = PCAP_SPI_CLOCK_HZ,
        .mode = PCAP_SPI_MODE,
        .spics_io_num = PCAP_SPI_CS_PIN,  // -1, CS handled externally
        .queue_size = 1,
        .flags = SPI_DEVICE_NO_DUMMY,
    };

    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "PCAP driver initialized");
}

static uint8_t spi_transfer_byte(uint8_t data)
{
    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &data,
        .rx_buffer = NULL,
        .flags = SPI_TRANS_USE_RXDATA,
    };
    spi_device_polling_transmit(spi_handle, &trans);
    return trans.rx_data[0];
}

static void spi_transfer_bytes(const uint8_t* tx_data, uint8_t* rx_data, size_t len)
{
    spi_transaction_t trans = {
        .length = len * 8,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };
    spi_device_polling_transmit(spi_handle, &trans);
}

static uint8_t spi_transmit_u8(pcap_chip_select_t chip, uint8_t data)
{
    uint8_t rx;
    mux_select_chip(chip);
    rx = spi_transfer_byte(data);
    esp_rom_delay_us(1);
    mux_select_chip(PCAP_CHIP_NONE);
    esp_rom_delay_us(1);
    return rx;
}

bool pcap_init_chip(pcap_chip_select_t chip)
{
    ESP_LOGI(TAG, "Initializing PCAP chip %d", chip);

    // Power-on reset
    spi_transmit_u8(chip, PCAP_POR);

    // Initialize
    spi_transmit_u8(chip, PCAP_INIT);

    ESP_LOGI(TAG, "POR and INIT sent to chip %d", chip);
    return true;
}

bool pcap_write_firmware(pcap_chip_select_t chip, const uint8_t* firmware, uint16_t size)
{
    ESP_LOGI(TAG, "Uploading firmware to PCAP chip %d (%d bytes)", chip, size);

    mux_select_chip(chip);

    // Send write memory command and start address
    spi_transfer_byte(PCAP_WR_MEM);
    spi_transfer_byte(0x00);

    // Transfer firmware bytes
    for (int i = 0; i < size; i++) {
        spi_transfer_byte(firmware[i]);
    }

    mux_select_chip(PCAP_CHIP_NONE);

    ESP_LOGI(TAG, "Firmware upload complete for chip %d", chip);
    return true;
}
 
bool pcap_write_config(pcap_chip_select_t chip, const uint8_t* config, uint16_t size)
{
    ESP_LOGI(TAG, "Writing config to PCAP chip %d (%d bytes)", chip, size);

    mux_select_chip(chip);

    // Send write config command (16-bit)
    spi_transfer_byte((PCAP_WR_CONFIG >> 8) & 0xFF);
    spi_transfer_byte(PCAP_WR_CONFIG & 0xFF);

    // Transfer config bytes
    for (int i = 0; i < size; i++) {
        spi_transfer_byte(config[i]);
    }

    mux_select_chip(PCAP_CHIP_NONE);

    // Allow config to settle
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "Successfully wrote config to chip %d", chip);
    return true;
}

void pcap_read_config(pcap_chip_select_t chip, uint8_t* buffer, uint16_t size)
{
    // TODO: Implement if needed
    (void)chip;
    (void)buffer;
    (void)size;
}

void pcap_start_cdc(pcap_chip_select_t chip)
{
    spi_transmit_u8(chip, PCAP_CDC_START);
}

void pcap_start_rdc(pcap_chip_select_t chip)
{
    spi_transmit_u8(chip, PCAP_RDC_START);
}

void pcap_read_data(pcap_chip_select_t chip, pcap_data_t* data)
{
    float result;
    for (int i = 0; i < NUM_SENSORS_PER_CHIP; i++) {
        mux_select_chip(chip);
        result = pcap_read_sensor(chip, i);
        mux_select_chip(PCAP_CHIP_NONE);

        if (data != NULL) {
            data->raw[i] = result;
        }
    }
}

uint32_t pcap_read_sensor(pcap_chip_select_t chip, uint8_t sensor_num)
{
    uint8_t buffer[4] = {0};

    // Note: chip should already be selected by pcap_read_data
    spi_transfer_byte(PCAP_RD_RESULT | sensor_addr[sensor_num]);

    esp_rom_delay_us(1);

    // Receive 4 bytes
    spi_transfer_bytes(NULL, buffer, 4);

    uint32_t raw = ((uint32_t)buffer[3] << 24) | ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[1] <<  8) | ((uint32_t)buffer[0] <<  0);
    return raw;
}

bool pcap_test_communication(pcap_chip_select_t chip)
{
    uint8_t result;
    bool test_passed;

    ESP_LOGI(TAG, "Testing communication with chip %d", chip);

    mux_select_chip(chip);

    spi_transfer_byte(PCAP_TEST_READ);
    esp_rom_delay_us(1);
    result = spi_transfer_byte(0x00);

    if (result == 0x11) {
        ESP_LOGI(TAG, "Communication test PASSED for chip %d", chip);
        test_passed = true;
    } else {
        ESP_LOGW(TAG, "Communication test FAILED for chip %d (got 0x%02X)", chip, result);
        test_passed = false;
    }

    mux_select_chip(PCAP_CHIP_NONE);
    return test_passed;
}

void pcap_calibrate(pcap_chip_select_t chip, pcap_data_t* data)
{
    ESP_LOGI(TAG, "Calibrating PCAP chip %d (single-sample)", chip);

    if (data == NULL) {
        ESP_LOGE(TAG, "pcap_calibrate called with NULL data pointer");
        return;
    }

    // Take exactly one reading
    pcap_read_data(chip, data);

    // Store that reading as the offset
    for (int i = 0; i < NUM_SENSORS_PER_CHIP; i++) {
        data->offset[i] = data->raw[i];
        ESP_LOGI(TAG, "Calibration complete for chip, dataoffset is: %d", data->offset[i]);
    }

    ESP_LOGI(TAG, "Calibration complete for chip %d", chip);
}
