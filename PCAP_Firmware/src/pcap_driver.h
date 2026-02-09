/**
 * @file pcap_driver.h
 * @brief High-level driver for PCAP04 capacitive sensor system (ESP-IDF)
 *
 * This module provides a unified interface for controlling multiple PCAP04 chips
 * through a multiplexer. It combines multiplexer control and SPI communication
 * to provide simple, high-level functions for chip initialization, configuration,
 * and measurement.
 */

#ifndef PCAP_DRIVER_H
#define PCAP_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/spi_master.h"
#include "pcap04_defs.h"
#include "mux_control.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup SPIConfig SPI Configuration
 * @brief SPI bus settings for PCAP04 communication
 * @{
 */
#define PCAP_SPI_CLOCK_HZ   4000000  ///< SPI clock frequency: 4 MHz
#define PCAP_SPI_MODE       1        ///< SPI mode 1 (CPOL=0, CPHA=1)

// XIAO ESP32C3 SPI pins
#define PCAP_SPI_MOSI_PIN   GPIO_NUM_10
#define PCAP_SPI_MISO_PIN   GPIO_NUM_9
#define PCAP_SPI_SCLK_PIN   GPIO_NUM_8
#define PCAP_SPI_CS_PIN     -1       ///< CS handled by multiplexer, not SPI driver

// Taken from the datasheet
#define PCAP_CONVERSION_NUMBER 134217728
/** @} */

/**
 * @brief Initialize the PCAP driver system
 *
 * Initializes both the multiplexer control and SPI interface.
 * Must be called before any other driver functions.
 */
void pcap_driver_init(void);

/**
 * @brief Initialize a specific PCAP chip
 * @param chip The chip to initialize
 * @return true if initialization successful, false otherwise
 *
 * Performs power-on reset (POR) and initialization sequence for the specified
 * chip. This prepares the chip to receive configuration and measurement commands.
 */
bool pcap_init_chip(pcap_chip_select_t chip);

/**
 * @brief Upload firmware to a PCAP chip
 * @param chip The chip to upload firmware to
 * @param firmware Pointer to firmware data buffer
 * @param size Number of firmware bytes to write (typically 1024 bytes)
 * @return true if upload successful, false otherwise
 */
bool pcap_write_firmware(pcap_chip_select_t chip, const uint8_t* firmware, uint16_t size);

/**
 * @brief Write configuration to a PCAP chip
 * @param chip The chip to configure
 * @param config Pointer to configuration data buffer
 * @param size Number of configuration bytes to write
 * @return true if write successful, false otherwise
 */
bool pcap_write_config(pcap_chip_select_t chip, const uint8_t* config, uint16_t size);

/**
 * @brief Read configuration from a PCAP chip
 * @param chip The chip to read from
 * @param buffer Pointer to buffer for storing configuration data
 * @param size Number of configuration bytes to read
 */
void pcap_read_config(pcap_chip_select_t chip, uint8_t* buffer, uint16_t size);

/**
 * @brief Start capacitance (CDC) measurement on a chip
 * @param chip The chip to start measurement on
 */
void pcap_start_cdc(pcap_chip_select_t chip);

/**
 * @brief Start resistance (RDC) measurement on a chip
 * @param chip The chip to start measurement on
 */
void pcap_start_rdc(pcap_chip_select_t chip);

/**
 * @brief Read measurement results from all sensors on a chip
 * @param chip The chip to read from
 * @param data Pointer to data structure for storing results
 */
void pcap_read_data(pcap_chip_select_t chip, pcap_data_t* data);

/**
 * @brief Read measurement result from a single sensor
 * @param chip The chip to read from
 * @param sensor_num Sensor number (0-5)
 * @return 24-bit measurement value
 */
float pcap_read_sensor(pcap_chip_select_t chip, uint8_t sensor_num);

/**
 * @brief Test SPI communication with a chip
 * @param chip The chip to test
 * @return true if communication test passed, false otherwise
 */
bool pcap_test_communication(pcap_chip_select_t chip);

/**
 * @brief Calibrate PCAP sensors by averaging multiple readings for baseline offset
 * @param chip The chip to calibrate
 * @param data Pointer to data structure for storing calibration offsets
 * @param num_samples Number of readings to average for the offset
 */
void pcap_calibrate(pcap_chip_select_t chip, pcap_data_t* data, uint16_t num_samples);

#ifdef __cplusplus
}
#endif

#endif // PCAP_DRIVER_H
