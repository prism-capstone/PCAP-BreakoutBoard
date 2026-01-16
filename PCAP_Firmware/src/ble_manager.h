/**
 * @file ble_manager.h
 * @brief BLE communication manager for transmitting PCAP sensor data (ESP-IDF NimBLE)
 *
 * This module provides BLE functionality for the PCAP sensor system,
 * allowing wireless transmission of sensor data to mobile applications.
 */

#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "pcap04_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup BLEConfig BLE Configuration
 * @brief UUIDs and configuration for BLE service
 * @{
 */
#define BLE_DEVICE_NAME         "PCAP-Sensor"
/** @} */

/**
 * @brief Initialize BLE server and characteristics
 *
 * Sets up the BLE device, service, and characteristics for
 * sensor data transmission using NimBLE stack.
 */
void ble_manager_init(void);

/**
 * @brief Check if a BLE client is connected
 * @return true if client connected, false otherwise
 */
bool ble_is_connected(void);

/**
 * @brief Send sensor data for a single chip over BLE
 * @param chip_num The chip number (0-7)
 * @param data Pointer to the sensor data structure
 *
 * Transmits the calibrated sensor readings from one chip
 * to the connected BLE client.
 */
void ble_send_chip_data(uint8_t chip_num, pcap_data_t* data);

/**
 * @brief Send status message over BLE
 * @param status Status string to transmit
 *
 * Sends a status or diagnostic message to the connected client.
 */
void ble_send_status(const char* status);

#ifdef __cplusplus
}
#endif

#endif // BLE_MANAGER_H
