/**
 * @file pcap_spi.h
 * @brief SPI communication interface for PCAP04 capacitive sensor chips
 *
 * This module provides low-level SPI communication functions for interfacing
 * with PCAP04 chips including command transmission, configuration, and result reading.
 */

#ifndef PCAP_SPI_H
#define PCAP_SPI_H

#include <Arduino.h>
#include <SPI.h>
#include "pcap04_defs.h"

/**
 * @defgroup SPIConfig SPI Configuration
 * @brief SPI bus settings for PCAP04 communication
 * @{
 */
#define PCAP_SPI_CLOCK      1000000  ///< SPI clock frequency: 1 MHz
#define PCAP_SPI_MODE       SPI_MODE0 ///< SPI mode 0 (CPOL=0, CPHA=0)
#define PCAP_SPI_BIT_ORDER  MSBFIRST  ///< Most significant bit first
/** @} */

/**
 * @note Hardware SPI Configuration:
 * This design uses a CD74HC4067 multiplexer with inverted CS logic:
 * - COMMON_I/O is tied to GND
 * - All CS outputs (CS1-CS8) have pull-up resistors (default HIGH/inactive)
 * - When a chip is selected via PCAP_SEL pins, that chip's CS goes LOW (active)
 * - No GPIO needed for CS control - the multiplexer handles it automatically
 *
 * SPI Bus Connections:
 * - MOSI: GPIO 10 (PA6_A10_D10_MOSI)
 * - MISO: GPIO 9  (PA5_A9_D9_MISO)
 * - SCK:  GPIO 8  (PA7_A8_D8_SCK)
 *
 * The multiplexer select pins (PCAP_SEL0-3) control which PCAP chip's CS is active.
 */

/**
 * @class PCAP_SPI
 * @brief SPI communication handler for PCAP04 chips
 *
 * Provides methods for sending commands and reading/writing data to PCAP04
 * capacitive sensing chips over the SPI bus. Handles chip select timing and
 * transaction management.
 */
class PCAP_SPI {
public:
    /**
     * @brief Initialize the SPI interface
     *
     * Configures the SPI bus with appropriate clock speed, mode, and bit order.
     * Sets up the chip select pin as an output.
     */
    void begin();

    /**
     * @brief Send a single command byte to the PCAP chip
     * @param cmd Command byte to send (see PCAP04_DEFS.h for command definitions)
     *
     * Transmits a command without additional data or address bytes.
     * Used for operations like POR, INIT, CDC_START, etc.
     */
    uint8_t sendCommand(uint8_t cmd);
    uint16_t sendCommand16(uint16_t cmd);
    uint32_t sendCommand32(uint32_t cmd);

    /**
     * @brief Write a single byte to a PCAP chip register
     * @param data Data byte to write
     *
     * Performs a single-byte write transaction to the specified register address.
     */
    void writeByte(uint8_t data);

    /**
     * @brief Write multiple bytes to consecutive PCAP chip registers
     * @param cmd Write command byte
     * @param addr Starting register address
     * @param data Pointer to data buffer to write
     * @param len Number of bytes to write
     *
     * Writes a block of data starting at the specified address with auto-increment.
     * Used for configuration and firmware uploads.
     */
    void writeBytes(uint8_t* data, uint16_t len);

    /**
     * @brief Read a single byte from a PCAP chip register
     * @return The byte read
     *
     * Performs a single-byte read transaction from the specified register address.
     */
    uint8_t readByte();

    /**
     * @brief Read multiple bytes from consecutive PCAP chip registers
     * @param cmd Read command byte
     * @param addr Starting register address
     * @param buffer Pointer to buffer for storing read data
     * @param len Number of bytes to read
     *
     * Reads a block of data starting at the specified address with auto-increment.
     * Used for reading configuration and measurement results.
     */
    void readBytes(uint8_t* buffer, uint16_t len);

    /**
     * @brief Read a 24-bit measurement result from a specific sensor
     * @param sensor_num Sensor number (0-5 for six sensors per chip)
     * @return 24-bit capacitance measurement value
     *
     * Reads the three-byte result register for the specified sensor and
     * combines them into a single 32-bit value (with upper byte zero).
     */
    uint32_t readResult(uint8_t sensor_num);

    void setDummyByte(uint8_t num);

private:
    SPISettings spi_settings; ///< SPI bus configuration settings
    uint8_t dummy_byte;
};

#endif // PCAP_SPI_H
