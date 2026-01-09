/**
 * @file pcap_driver.h
 * @brief High-level driver for PCAP04 capacitive sensor system
 *
 * This module provides a unified interface for controlling multiple PCAP04 chips
 * through a multiplexer. It combines multiplexer control and SPI communication
 * to provide simple, high-level functions for chip initialization, configuration,
 * and measurement.
 */

#ifndef PCAP_DRIVER_H
#define PCAP_DRIVER_H

#include "pcap04_defs.h"
#include "pcap_spi.h"
#include "mux_control.h"

/**
 * @class PCAP_Driver
 * @brief Main driver class for PCAP04 capacitive sensor system
 *
 * This class provides the primary interface for interacting with multiple PCAP04
 * chips. It manages chip selection via multiplexer and handles all SPI communication.
 * Typical usage involves initializing the driver, configuring chips, and reading
 * measurement results.
 */
class PCAP_Driver {
public:
    /**
     * @brief Initialize the PCAP driver system
     *
     * Initializes both the multiplexer control and SPI interface.
     * Must be called before any other driver functions.
     */
    void begin();

    /**
     * @brief Initialize a specific PCAP chip
     * @param chip The chip to initialize
     * @return true if initialization successful, false otherwise
     *
     * Performs power-on reset (POR) and initialization sequence for the specified
     * chip. This prepares the chip to receive configuration and measurement commands.
     */
    bool initChip(pcap_chip_select_t chip);

    /**
     * @brief Upload firmware to a PCAP chip
     * @param chip The chip to upload firmware to
     * @param firmware Pointer to firmware data buffer
     * @param size Number of firmware bytes to write (typically 1024 bytes)
     * @return true if upload successful, false otherwise
     *
     * Uploads firmware code to the chip's internal memory. This must be done
     * after power-on reset and before configuration.
     */
    bool writeFirmware(pcap_chip_select_t chip, uint8_t* firmware, uint16_t size);

    /**
     * @brief Write configuration to a PCAP chip
     * @param chip The chip to configure
     * @param config Pointer to configuration data buffer
     * @param size Number of configuration bytes to write
     * @return true if write successful, false otherwise
     *
     * Uploads a complete configuration to the specified chip. Configuration data
     * should match the PCAP04 register layout (typically 52 bytes).
     */
    bool writeConfig(pcap_chip_select_t chip, uint8_t* config, uint16_t size);

    /**
     * @brief Read configuration from a PCAP chip
     * @param chip The chip to read from
     * @param buffer Pointer to buffer for storing configuration data
     * @param size Number of configuration bytes to read
     *
     * Retrieves the current configuration from the specified chip.
     * Useful for verifying uploaded configurations.
     */
    void readConfig(pcap_chip_select_t chip, uint8_t* buffer, uint16_t size);

    /**
     * @brief Start capacitance (CDC) measurement on a chip
     * @param chip The chip to start measurement on
     *
     * Triggers a capacitance-to-digital conversion. Results can be read after
     * the conversion completes (typically 5-10ms depending on configuration).
     */
    void startCDC(pcap_chip_select_t chip);

    /**
     * @brief Start resistance (RDC) measurement on a chip
     * @param chip The chip to start measurement on
     *
     * Triggers a resistance-to-digital conversion. Results can be read after
     * the conversion completes.
     */
    void startRDC(pcap_chip_select_t chip);

    /**
     * @brief Read measurement results from all sensors on a chip
     * @param chip The chip to read from
     * @param data Pointer to data structure for storing results
     *
     * Reads the measurement results for all six sensors on the specified chip
     * and stores them in the provided data structure.
     */
    void readResults(pcap_chip_select_t chip, pcap_data_t* data);

    /**
     * @brief Read measurement result from a single sensor
     * @param chip The chip to read from
     * @param sensor_num Sensor number (0-5)
     * @return 24-bit measurement value
     *
     * Reads the measurement result from a specific sensor on the chip.
     * Useful when only one sensor value is needed.
     */
    uint32_t readSensor(pcap_chip_select_t chip, uint8_t sensor_num);

    /**
     * @brief Select a chip via the multiplexer
     * @param chip The chip to select
     *
     * Routes the SPI bus to the specified chip through the multiplexer.
     * Normally called internally by other functions, but can be used directly
     * for custom low-level operations.
     */
    void selectChip(pcap_chip_select_t chip);

    /**
     * @brief Test SPI communication with a chip
     * @param chip The chip to test
     * @return true if communication test passed, false otherwise
     *
     * Performs a loopback test by writing and reading back test data to verify
     * SPI communication is working correctly. Useful for debugging connections.
     */
    bool testCommunication(pcap_chip_select_t chip);

private:
    PCAP_SPI spi;       ///< SPI communication interface
    MuxControl mux;     ///< Multiplexer control for chip selection
};

#endif // PCAP_DRIVER_H
