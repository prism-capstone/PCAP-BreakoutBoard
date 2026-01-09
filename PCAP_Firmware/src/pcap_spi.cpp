/**
 * @file pcap_spi.cpp
 * @brief SPI communication implementation for PCAP04 chips
 */

#include "pcap_spi.h"

void PCAP_SPI::begin() {
    // No CS pin control needed - multiplexer handles CS automatically
    // COMMON_I/O is tied to GND, pull-ups on CS outputs handle inactive state

    SPI.begin();
    spi_settings = SPISettings(PCAP_SPI_CLOCK, PCAP_SPI_BIT_ORDER, PCAP_SPI_MODE);
}

void PCAP_SPI::sendCommand(uint8_t cmd) {
    // CS is controlled by multiplexer - chip select via PCAP_SEL pins before calling
    SPI.beginTransaction(spi_settings);
    delayMicroseconds(1);

    SPI.transfer(cmd);

    delayMicroseconds(1);
    SPI.endTransaction();

    delayMicroseconds(10);  // Allow PCAP to process
}

void PCAP_SPI::writeByte(uint8_t cmd, uint8_t addr, uint8_t data) {
    SPI.beginTransaction(spi_settings);
    delayMicroseconds(1);

    SPI.transfer(cmd);
    SPI.transfer(addr);
    SPI.transfer(data);

    delayMicroseconds(1);
    SPI.endTransaction();

    delayMicroseconds(10);
}

void PCAP_SPI::writeBytes(uint8_t cmd, uint8_t addr, uint8_t* data, uint16_t len) {
    SPI.beginTransaction(spi_settings);
    delayMicroseconds(1);

    SPI.transfer(cmd);
    SPI.transfer(addr);

    for (uint16_t i = 0; i < len; i++) {
        SPI.transfer(data[i]);
    }

    delayMicroseconds(1);
    SPI.endTransaction();

    delayMicroseconds(10);
}

uint8_t PCAP_SPI::readByte(uint8_t cmd, uint8_t addr) {
    uint8_t result;

    SPI.beginTransaction(spi_settings);
    delayMicroseconds(1);

    SPI.transfer(cmd);
    SPI.transfer(addr);
    result = SPI.transfer(0x00);  // Dummy byte to read

    delayMicroseconds(1);
    SPI.endTransaction();

    delayMicroseconds(10);
    return result;
}

void PCAP_SPI::readBytes(uint8_t cmd, uint8_t addr, uint8_t* buffer, uint16_t len) {
    SPI.beginTransaction(spi_settings);
    delayMicroseconds(1);

    SPI.transfer(cmd);
    SPI.transfer(addr);

    for (uint16_t i = 0; i < len; i++) {
        buffer[i] = SPI.transfer(0x00);
    }

    delayMicroseconds(1);
    SPI.endTransaction();

    delayMicroseconds(10);
}

uint32_t PCAP_SPI::readResult(uint8_t sensor_num) {
    uint32_t result = 0;
    uint8_t buffer[3];

    // Each result is 3 bytes, starting at address sensor_num * 3
    uint8_t addr = sensor_num * 3;

    readBytes(PCAP_RD_RESULT, addr, buffer, 3);

    // Combine bytes into 24-bit value (MSB first)
    result = ((uint32_t)buffer[0] << 16) |
             ((uint32_t)buffer[1] << 8) |
             buffer[2];

    return result;
}
