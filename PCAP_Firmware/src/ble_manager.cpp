/**
 * @file ble_manager.cpp
 * @brief BLE communication manager implementation
 */

#include "ble_manager.h"

// Global pointer for callback access
static BLEManager* g_bleManager = nullptr;

// Custom BLE server callbacks implementation
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        if (g_bleManager) {
            g_bleManager->deviceConnected = true;
            Serial.println("BLE Client connected");
        }
    }

    void onDisconnect(BLEServer* pServer) {
        if (g_bleManager) {
            g_bleManager->deviceConnected = false;
            Serial.println("BLE Client disconnected");

            // Restart advertising
            delay(500);
            pServer->startAdvertising();
            Serial.println("BLE advertising restarted");
        }
    }
};

void BLEManager::begin() {
    g_bleManager = this;

    Serial.println("Initializing BLE...");

    // Initialize BLE Device
    BLEDevice::init(BLE_DEVICE_NAME);

    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create BLE Service
    BLEService* pService = pServer->createService(BLE_SERVICE_UUID);

    // Create Sensor Data Characteristic
    pSensorDataCharacteristic = pService->createCharacteristic(
        BLE_SENSOR_DATA_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pSensorDataCharacteristic->addDescriptor(new BLE2902());

    // Create Status Characteristic
    pStatusCharacteristic = pService->createCharacteristic(
        BLE_STATUS_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pStatusCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // iPhone connection issue workaround
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("BLE initialized and advertising");
    Serial.print("Device name: ");
    Serial.println(BLE_DEVICE_NAME);
}

bool BLEManager::isConnected() {
    return deviceConnected;
}

void BLEManager::sendChipData(uint8_t chip_num, pcap_data_t* data) {
    if (!deviceConnected || !pSensorDataCharacteristic) {
        return;
    }

    // Create data packet
    // Format: [chip_num][sensor0_MSB][sensor0_LSB]...[sensor5_MSB][sensor5_LSB]
    // Total: 1 + (6 * 4) = 25 bytes
    uint8_t packet[25];
    packet[0] = chip_num;

    int idx = 1;
    for (int i = 0; i < NUM_SENSORS_PER_CHIP; i++) {
        // Calculate calibrated value
        int32_t calibrated = (int32_t)(data->raw[i] - data->offset[i]);

        // Pack as 4 bytes (32-bit signed integer)
        packet[idx++] = (calibrated >> 24) & 0xFF;
        packet[idx++] = (calibrated >> 16) & 0xFF;
        packet[idx++] = (calibrated >> 8) & 0xFF;
        packet[idx++] = calibrated & 0xFF;
    }

    // Send notification
    pSensorDataCharacteristic->setValue(packet, sizeof(packet));
    pSensorDataCharacteristic->notify();
}

void BLEManager::sendStatus(const char* status) {
    if (!deviceConnected || !pStatusCharacteristic) {
        return;
    }

    pStatusCharacteristic->setValue(status);
    pStatusCharacteristic->notify();
}
