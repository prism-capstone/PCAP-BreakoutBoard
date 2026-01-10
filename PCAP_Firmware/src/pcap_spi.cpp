/**
 * @file pcap_spi.cpp
 * @brief SPI communication implementation for PCAP04 chips
 */

/*
 * IMPORTANT NOTE ON SPI COMMUNICATION:
 * ====================================
 * SPI is a full-duplex protocol - you CANNOT receive without transmitting.
 * Every SPI.transfer() call simultaneously sends AND receives one byte.
 * 
 * During read operations, when we want to receive data from the device,
 * we must still send something on MOSI. This is typically called a "dummy byte".
 * 
 * By default, this driver sends 0x00 as the dummy byte during reads.
 * However, some devices may interpret certain byte values as commands even
 * during what we consider "read-only" operations.
 * 
 * If you find that 0x00 interferes with your device:
 * 1. Check the device datasheet for a "safe" dummy byte value
 * 2. Common alternatives: 0xFF, 0xAA, 0x55
 * 3. Use setDummyByte() to configure it:
 *    sensor.setDummyByte(0xFF);
 * 
 * Some devices explicitly ignore MOSI during reads, in which case the
 * dummy byte value doesn't matter.
 */

#include "pcap_spi.h"

void PCAP_SPI::begin() {
    // No CS pin control needed - multiplexer handles CS automatically
    // COMMON_I/O is tied to GND, pull-ups on CS outputs handle inactive state

    SPI.begin();
    spi_settings = SPISettings(PCAP_SPI_CLOCK, PCAP_SPI_BIT_ORDER, PCAP_SPI_MODE);
    setDummyByte(0x00);
}

uint8_t PCAP_SPI::sendCommand(uint8_t cmd) {
    // CS is controlled by multiplexer - chip select via PCAP_SEL pins before calling
    uint8_t result; 
    SPI.beginTransaction(spi_settings);
    delayMicroseconds(1);

    result = SPI.transfer(cmd);

    delayMicroseconds(1);
    SPI.endTransaction();

    delayMicroseconds(10);  // Allow PCAP to process

    return result;
}

uint16_t PCAP_SPI::sendCommand16(uint16_t cmd) {
    // CS is controlled by multiplexer - chip select via PCAP_SEL pins before calling
    uint16_t result; 
    SPI.beginTransaction(spi_settings);
    delayMicroseconds(1);

    result = SPI.transfer16(cmd);

    delayMicroseconds(1);
    SPI.endTransaction();

    delayMicroseconds(10);  // Allow PCAP to process

    return result;
}

uint32_t PCAP_SPI::sendCommand32(uint32_t cmd) {
    // CS is controlled by multiplexer - chip select via PCAP_SEL pins before calling
    uint32_t result; 
    SPI.beginTransaction(spi_settings);
    delayMicroseconds(1);

    result = SPI.transfer32(cmd);

    delayMicroseconds(1);
    SPI.endTransaction();

    delayMicroseconds(10);  // Allow PCAP to process

    return result;
}

void PCAP_SPI::writeByte(uint8_t data) {
    SPI.beginTransaction(spi_settings);
    delayMicroseconds(1);

    SPI.transfer(data);

    delayMicroseconds(1);
    SPI.endTransaction();

    delayMicroseconds(10);
}

void PCAP_SPI::writeBytes(uint8_t* data, uint16_t len) {
    SPI.beginTransaction(spi_settings);
    delayMicroseconds(1);

    SPI.transferBytes(data, NULL, len);

    delayMicroseconds(1);
    SPI.endTransaction();

    delayMicroseconds(10);
}

uint8_t PCAP_SPI::readByte() {
    uint8_t result;

    SPI.beginTransaction(spi_settings);
    delayMicroseconds(1);

    result = SPI.transfer(dummy_byte); // Dummy byte to read

    delayMicroseconds(1);
    SPI.endTransaction();

    delayMicroseconds(10);
    return result;
}

void PCAP_SPI::readBytes(uint8_t* buffer, uint16_t len) {
    SPI.beginTransaction(spi_settings);
    delayMicroseconds(1);

    SPI.transferBytes(NULL, buffer, len);

    delayMicroseconds(1);
    SPI.endTransaction();

    delayMicroseconds(10);
}

uint32_t PCAP_SPI::readResult(uint8_t sensor_addr) {
    uint32_t result = 0;
    uint8_t buffer[4] = {0};

    SPI.beginTransaction(spi_settings);

    // Send request to read sensor data
    SPI.transfer(PCAP_RD_RESULT | sensor_addr);

    // Receive 4 bytes back
    SPI.transferBytes(NULL, buffer, 4);

    SPI.endTransaction();

    result = ((buffer[3] << 24) + (buffer[2] << 16) + (buffer[1] << 8) + (buffer[0]));

    return result;
}

void PCAP_SPI::setDummyByte(uint8_t num) {
    dummy_byte = num;
}
