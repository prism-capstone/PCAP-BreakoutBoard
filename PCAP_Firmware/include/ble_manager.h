/**
 * @file ble_manager.h
 * @brief BLE communication manager for transmitting PCAP sensor data
 *
 * This module provides BLE functionality for the PCAP sensor system,
 * allowing wireless transmission of sensor data to mobile applications.
 */

#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "pcap04_defs.h"

/**
 * @defgroup BLEConfig BLE Configuration
 * @brief UUIDs and configuration for BLE service
 * @{
 */
#define BLE_DEVICE_NAME         "PCAP-Sensor"
#define BLE_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BLE_SENSOR_DATA_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BLE_STATUS_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26a9"
/** @} */

// Forward declaration
class MyServerCallbacks;

/**
 * @class BLEManager
 * @brief Manages BLE server and sensor data transmission
 *
 * This class handles BLE initialization, client connections, and
 * transmitting PCAP sensor data to connected mobile devices.
 */
class BLEManager {
public:
    /**
     * @brief Initialize BLE server and characteristics
     *
     * Sets up the BLE device, service, and characteristics for
     * sensor data transmission.
     */
    void begin();

    /**
     * @brief Check if a BLE client is connected
     * @return true if client connected, false otherwise
     */
    bool isConnected();

    /**
     * @brief Send sensor data for a single chip over BLE
     * @param chip_num The chip number (0-7)
     * @param data Pointer to the sensor data structure
     *
     * Transmits the calibrated sensor readings from one chip
     * to the connected BLE client.
     */
    void sendChipData(uint8_t chip_num, pcap_data_t* data);

    /**
     * @brief Send status message over BLE
     * @param status Status string to transmit
     *
     * Sends a status or diagnostic message to the connected client.
     */
    void sendStatus(const char* status);

    // Public so callback can access it
    bool deviceConnected = false;

private:
    BLEServer* pServer = nullptr;
    BLECharacteristic* pSensorDataCharacteristic = nullptr;
    BLECharacteristic* pStatusCharacteristic = nullptr;
    bool oldDeviceConnected = false;

    friend class MyServerCallbacks;
};

#endif // BLE_MANAGER_H
