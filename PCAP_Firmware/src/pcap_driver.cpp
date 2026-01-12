/**
 * @file pcap_driver.cpp
 * @brief High-level driver implementation for PCAP04 capacitive sensor system
 */

#include "pcap_driver.h"

const uint8_t sensAdd[NUM_SENSORS_PER_CHIP] = {0x00, 0x04, 0x08, 0x0C, 0x10, 0x14};

void PCAP_Driver::begin() {
    mux.begin();
    SPI.begin();
    spi_settings = SPISettings(PCAP_SPI_CLOCK, PCAP_SPI_BIT_ORDER, PCAP_SPI_MODE);
}

uint8_t PCAP_Driver::spi_transmit(pcap_chip_select_t chip, uint32_t data)
{
    static uint8_t recval = 0;
    SPI.beginTransaction(spi_settings);
    selectChip(chip);
    SPI.transfer((uint8_t)((data >> 16) & 0xFF));
    recval = SPI.transfer16((uint16_t)(data & 0xFFFF)) & 0xFF;
    delayMicroseconds(1);
    selectChip(PCAP_CHIP_NONE);
    delayMicroseconds(1);
    SPI.endTransaction();
    return recval;
};

uint8_t PCAP_Driver::spi_transmit(pcap_chip_select_t chip, uint16_t data)
{
    static uint8_t recval = 0;
    SPI.beginTransaction(spi_settings);
    selectChip(chip);
    recval = SPI.transfer16(data);
    delayMicroseconds(1);
    selectChip(PCAP_CHIP_NONE);
    delayMicroseconds(1);
    SPI.endTransaction();
    return (uint8_t)(recval & 0xFF);
};

uint8_t PCAP_Driver::spi_transmit(pcap_chip_select_t chip, uint8_t data)
{
    static uint8_t recval = 0;
    SPI.beginTransaction(spi_settings);
    selectChip(chip);
    SPI.transfer(data);
    delayMicroseconds(1);
    selectChip(PCAP_CHIP_NONE);
    delayMicroseconds(1);
    SPI.endTransaction();
    return recval;
};

bool PCAP_Driver::initChip(pcap_chip_select_t chip) {
    Serial.print("Initializing PCAP chip ");
    Serial.println(chip);

    // Power-on reset
    spi_transmit(chip, (uint8_t) PCAP_POR);

    // Initialize
    spi_transmit(chip, (uint8_t) PCAP_INIT);
    
    Serial.println("POR and INIT sent");

    return true;
}

bool PCAP_Driver::writeFirmware(pcap_chip_select_t chip, uint8_t* firmware, uint16_t size) {
    Serial.print("Uploading firmware to PCAP chip ");
    Serial.print(chip);
    Serial.print(" (");
    Serial.print(size);
    Serial.println(" bytes)");

    SPI.beginTransaction(spi_settings);
    selectChip(chip);
    
    SPI.transfer(PCAP_WR_MEM);
    SPI.transfer(0x00);

    for (int i = 0; i < size; i++)
    {
        SPI.transfer(firmware[i]);
    }
    selectChip(PCAP_CHIP_NONE);
    SPI.endTransaction();

    Serial.println("Firmware upload complete");

    return true;
}

bool PCAP_Driver::writeConfig(pcap_chip_select_t chip, uint8_t* config, uint16_t size) {
    Serial.print("Writing config to PCAP chip ");
    Serial.print(chip);
    Serial.print(" (");
    Serial.print(size);
    Serial.println(" bytes)");

    SPI.beginTransaction(spi_settings);
    selectChip(chip);

    SPI.transfer16(PCAP_WR_CONFIG);
    for (int i = 0; i < size; i++)
    {
        SPI.transfer(config[i]);
    }

    selectChip(PCAP_CHIP_NONE);
    SPI.endTransaction();
    delay(10);

    Serial.println("Successfully wrote config");
    return true;
}

void PCAP_Driver::readConfig(pcap_chip_select_t chip, uint8_t* buffer, uint16_t size) {
    // TODO
    return;
}

void PCAP_Driver::startCDC(pcap_chip_select_t chip) {
    spi_transmit(chip, (uint8_t) PCAP_CDC_START);
}

void PCAP_Driver::startRDC(pcap_chip_select_t chip) {
    spi_transmit(chip, (uint8_t) PCAP_RDC_START);
}

void PCAP_Driver::readPCAP(pcap_chip_select_t chip, pcap_data_t* data) {
    SPI.beginTransaction(spi_settings);
    
    for (int i = 0; i < NUM_SENSORS_PER_CHIP; i++) {
        selectChip(chip);
        data->raw[i] = readSensor(chip, i);
        selectChip(PCAP_CHIP_NONE);
    }

    SPI.endTransaction();
}

uint32_t PCAP_Driver::readSensor(pcap_chip_select_t chip, uint8_t sensor_num) {
    uint8_t buffer[4] = {0};
    uint32_t result;

    SPI.transfer(PCAP_RD_RESULT | sensAdd[sensor_num]);
    
    // // Sending one after another (need to validate this too)
    // SPI.transfer(PCAP_RD_RESULT);
    // SPI.transfer(sensAdd[sensor_num]);
    
    delayMicroseconds(1);

    for(int i = 0; i < sizeof(uint32_t); i++)
    {
        // Receieve 4 bytes
        SPI.transferBytes(NULL, buffer, sizeof(uint32_t));
    }

    result = (buffer[3]<<24) + (buffer[2]<<16) + (buffer[1]<<8) + (buffer[0]);
    return result;
}

void PCAP_Driver::selectChip(pcap_chip_select_t chip) {
    mux.selectChip(chip);
}

bool PCAP_Driver::testCommunication(pcap_chip_select_t chip) {
    uint8_t result;
    bool testresult;

    Serial.print("Testing communication with chip ");
    Serial.println(chip);

    SPI.beginTransaction(spi_settings);
    selectChip(chip);

    SPI.transfer(PCAP_TEST_READ);

    // Give some time for the PCAP to respond
    delayMicroseconds(1);
    result = SPI.transfer(0x00);

    if (result == 0x11) {
        Serial.println("Communication test PASSED");
        testresult = true;
    }
    else {
        Serial.print("Communication test FAILED with error code: ");
        Serial.println(result);
        testresult = false;
    }

    selectChip(PCAP_CHIP_NONE);
    SPI.endTransaction();
    return testresult;
}
