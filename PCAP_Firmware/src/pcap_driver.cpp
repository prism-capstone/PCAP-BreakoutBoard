/**
 * @file pcap_driver.cpp
 * @brief High-level driver implementation for PCAP04 capacitive sensor system
 */

#include "pcap_driver.h"

const uint8_t sensAdd[NUM_SENSORS_PER_CHIP] = {0x00, 0x04, 0x08, 0x0C, 0x10, 0x14};

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
    selectChip(PCAP_CHIP_NONE);
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
    spi.sendCommand16(PCAP_WR_MEM);
    spi.writeBytes(firmware, PCAP_FW_SIZE);

    Serial.println("  Firmware upload complete");

    selectChip(PCAP_CHIP_NONE);

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
    spi.sendCommand16(PCAP_WR_CONFIG);
    spi.writeBytes(config, PCAP_CONFIG_SIZE);
    selectChip(PCAP_CHIP_NONE);
    delay(10);

    return true;
}

void PCAP_Driver::readConfig(pcap_chip_select_t chip, uint8_t* buffer, uint16_t size) {
    selectChip(chip);
    spi.sendCommand16(PCAP_RD_CONFIG);
    spi.readBytes(buffer, size);
    selectChip(PCAP_CHIP_NONE);
}

void PCAP_Driver::startCDC(pcap_chip_select_t chip) {
    selectChip(chip);
    spi.sendCommand(PCAP_CDC_START);
    selectChip(PCAP_CHIP_NONE);
}

void PCAP_Driver::startRDC(pcap_chip_select_t chip) {
    selectChip(chip);
    spi.sendCommand(PCAP_RDC_START);
    selectChip(PCAP_CHIP_NONE);
}

void PCAP_Driver::readPCAP(pcap_chip_select_t chip, pcap_data_t* data) {
    selectChip(chip);

    for (int i = 0; i < NUM_SENSORS_PER_CHIP; i++) {
        data->raw[i] = spi.readResult(sensAdd[i]);
    }

    selectChip(PCAP_CHIP_NONE);
}

uint32_t PCAP_Driver::readSensor(pcap_chip_select_t chip, uint8_t sensor_num) {
    uint32_t result;
    selectChip(chip);
    result = spi.readResult(sensAdd[sensor_num]);
    selectChip(PCAP_CHIP_NONE);
    return result;
}

void PCAP_Driver::selectChip(pcap_chip_select_t chip) {
    mux.selectChip(chip);
}

bool PCAP_Driver::testCommunication(pcap_chip_select_t chip) {
    selectChip(chip);

    Serial.print("Testing communication with chip ");
    Serial.println(chip);

    spi.sendCommand(PCAP_TEST_READ);
    uint8_t result = spi.readByte();

    if (result == 0x11) {
        Serial.println("  Communication test PASSED");
        return true;
    } 
    else if (result == 0x88) {
        Serial.println("  Communication test FAILED: Big/little endian swap");
        return false;
    }
    else if (result == 0xEE) {
        Serial.println("  Communication test FAILED: During read cycle all bits are inverted");
        return false;
    }
    else if (result == 0x77) {
        Serial.println("  Communication test FAILED: Inverted bits and bit/little-endian swap");
        return false;
    }
    else {
        Serial.println("  Communication test FAILED: Unknown result ");
        Serial.print(result);
        return false;
    }

    selectChip(PCAP_CHIP_NONE);
}
