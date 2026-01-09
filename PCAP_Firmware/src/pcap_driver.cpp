/**
 * @file pcap_driver.cpp
 * @brief High-level driver implementation for PCAP04 capacitive sensor system
 */

#include "pcap_driver.h"

void PCAP_Driver::begin() {
    mux.begin();
    spi.begin();

    Serial.println("PCAP Driver initialized");
}

bool PCAP_Driver::initChip(pcap_chip_select_t chip) {
    selectChip(chip);

    Serial.print("Initializing PCAP chip ");
    Serial.println(chip);

    // Power-on reset
    spi.sendCommand(PCAP_POR);
    delay(10);

    // Initialize
    spi.sendCommand(PCAP_INIT);
    delay(50);  // Wait for initialization

    Serial.println("  POR and INIT sent");

    return true;
}

bool PCAP_Driver::writeFirmware(pcap_chip_select_t chip, uint8_t* firmware, uint16_t size) {
    selectChip(chip);

    Serial.print("Uploading firmware to PCAP chip ");
    Serial.print(chip);
    Serial.print(" (");
    Serial.print(size);
    Serial.println(" bytes)");

    // Write firmware to internal memory starting at address 0
    spi.writeBytes(PCAP_WR_MEM, 0x00, firmware, size);
    delay(50);  // Allow time for firmware to be processed

    Serial.println("  Firmware upload complete");

    return true;
}

bool PCAP_Driver::writeConfig(pcap_chip_select_t chip, uint8_t* config, uint16_t size) {
    selectChip(chip);

    Serial.print("Writing config to PCAP chip ");
    Serial.print(chip);
    Serial.print(" (");
    Serial.print(size);
    Serial.println(" bytes)");

    // Write configuration registers starting at address 0
    spi.writeBytes(PCAP_WR_CONFIG & 0xFF, 0x00, config, size);
    delay(10);

    return true;
}

void PCAP_Driver::readConfig(pcap_chip_select_t chip, uint8_t* buffer, uint16_t size) {
    selectChip(chip);
    spi.readBytes(PCAP_RD_CONFIG & 0xFF, 0x00, buffer, size);
}

void PCAP_Driver::startCDC(pcap_chip_select_t chip) {
    selectChip(chip);
    spi.sendCommand(PCAP_CDC_START);
}

void PCAP_Driver::startRDC(pcap_chip_select_t chip) {
    selectChip(chip);
    spi.sendCommand(PCAP_RDC_START);
}

void PCAP_Driver::readResults(pcap_chip_select_t chip, pcap_data_t* data) {
    selectChip(chip);

    for (int i = 0; i < NUM_SENSORS_PER_CHIP; i++) {
        data->raw[i] = spi.readResult(i);
    }
}

uint32_t PCAP_Driver::readSensor(pcap_chip_select_t chip, uint8_t sensor_num) {
    selectChip(chip);
    return spi.readResult(sensor_num);
}

void PCAP_Driver::selectChip(pcap_chip_select_t chip) {
    mux.selectChip(chip);
}

bool PCAP_Driver::testCommunication(pcap_chip_select_t chip) {
    selectChip(chip);

    Serial.print("Testing communication with chip ");
    Serial.println(chip);

    // Write a test pattern to memory and read it back
    uint8_t test_data[4] = {0xAA, 0x55, 0xF0, 0x0F};
    uint8_t read_data[4] = {0};

    spi.writeBytes(PCAP_WR_MEM, 0x00, test_data, 4);
    delay(5);
    spi.readBytes(PCAP_RD_MEM, 0x00, read_data, 4);

    bool success = true;
    for (int i = 0; i < 4; i++) {
        if (test_data[i] != read_data[i]) {
            success = false;
            Serial.print("  Mismatch at byte ");
            Serial.print(i);
            Serial.print(": wrote 0x");
            Serial.print(test_data[i], HEX);
            Serial.print(", read 0x");
            Serial.println(read_data[i], HEX);
        }
    }

    if (success) {
        Serial.println("  Communication test PASSED");
    } else {
        Serial.println("  Communication test FAILED");
    }

    return success;
}
