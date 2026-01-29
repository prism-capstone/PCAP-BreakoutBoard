/**
 * @file ble_manager.c
 * @brief BLE communication manager implementation (ESP-IDF NimBLE)
 */

#include "ble_manager.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "freertos/semphr.h"

static const char* TAG = "BLE";

// UUIDs (same as Arduino version for compatibility)
// Service UUID: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
static const ble_uuid128_t service_uuid =
    BLE_UUID128_INIT(0x4b, 0x91, 0x31, 0xc3, 0xc9, 0xc5, 0xcc, 0x8f,
                     0x9e, 0x45, 0xb5, 0x1f, 0x01, 0xc2, 0xaf, 0x4f);

// Sensor Data UUID: beb5483e-36e1-4688-b7f5-ea07361b26a8
static const ble_uuid128_t sensor_data_uuid =
    BLE_UUID128_INIT(0xa8, 0x26, 0x1b, 0x36, 0x07, 0xea, 0xf5, 0xb7,
                     0x88, 0x46, 0xe1, 0x36, 0x3e, 0x48, 0xb5, 0xbe);

// Status UUID: beb5483e-36e1-4688-b7f5-ea07361b26a9
static const ble_uuid128_t status_uuid =
    BLE_UUID128_INIT(0xa9, 0x26, 0x1b, 0x36, 0x07, 0xea, 0xf5, 0xb7,
                     0x88, 0x46, 0xe1, 0x36, 0x3e, 0x48, 0xb5, 0xbe);

// Thread safety
static SemaphoreHandle_t ble_mutex = NULL;

// Connection state
static bool device_connected = false;
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t sensor_data_handle;
static uint16_t status_handle;

// Characteristic values
static uint8_t sensor_data_val[25] = {0};
static char status_val[64] = "Ready";

// Forward declarations
static int ble_gap_event(struct ble_gap_event *event, void *arg);
static void ble_host_task(void *param);
static void ble_on_sync(void);
static void ble_on_reset(int reason);

// GATT access callback
static int gatt_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const ble_uuid_t *uuid = ctxt->chr->uuid;

    // Handle sensor data characteristic
    if (ble_uuid_cmp(uuid, &sensor_data_uuid.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            int rc = os_mbuf_append(ctxt->om, sensor_data_val, sizeof(sensor_data_val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
    }

    // Handle status characteristic
    if (ble_uuid_cmp(uuid, &status_uuid.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            int rc = os_mbuf_append(ctxt->om, status_val, strlen(status_val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
    }

    return BLE_ATT_ERR_UNLIKELY;
}

// GATT service definition
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                // Sensor Data characteristic
                .uuid = &sensor_data_uuid.u,
                .access_cb = gatt_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &sensor_data_handle,
            },
            {
                // Status characteristic
                .uuid = &status_uuid.u,
                .access_cb = gatt_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &status_handle,
            },
            {
                0, // Terminator
            },
        },
    },
    {
        0, // Terminator
    },
};

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "BLE connection %s; status=%d",
                 event->connect.status == 0 ? "established" : "failed",
                 event->connect.status);
        if (event->connect.status == 0) {
            if (xSemaphoreTake(ble_mutex, portMAX_DELAY) == pdTRUE) {
                device_connected = true;
                conn_handle = event->connect.conn_handle;
                xSemaphoreGive(ble_mutex);
            }
        } else {
            // Connection failed, restart advertising
            ble_on_sync();
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "BLE disconnect; reason=%d", event->disconnect.reason);
        if (xSemaphoreTake(ble_mutex, portMAX_DELAY) == pdTRUE) {
            device_connected = false;
            conn_handle = BLE_HS_CONN_HANDLE_NONE;
            xSemaphoreGive(ble_mutex);
        }
        // Restart advertising
        ble_on_sync();
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "Advertising complete");
        ble_on_sync();
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "Subscribe event; attr_handle=%d", event->subscribe.attr_handle);
        break;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU update; mtu=%d", event->mtu.value);
        break;

    default:
        break;
    }
    return 0;
}

static void ble_on_sync(void)
{
    // Start advertising
    struct ble_gap_adv_params adv_params = {0};
    struct ble_hs_adv_fields fields = {0};
    int rc;

    // Set advertising flags
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    // Set device name
    fields.name = (uint8_t *)BLE_DEVICE_NAME;
    fields.name_len = strlen(BLE_DEVICE_NAME);
    fields.name_is_complete = 1;

    // Set TX power level
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.tx_pwr_lvl_is_present = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error setting advertisement fields; rc=%d", rc);
        return;
    }

    // Configure advertising parameters
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = 0x20;  // 20ms
    adv_params.itvl_max = 0x40;  // 40ms

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error starting advertisement; rc=%d", rc);
        return;
    }

    ESP_LOGI(TAG, "BLE advertising started");
}

static void ble_on_reset(int reason)
{
    ESP_LOGE(TAG, "BLE stack reset; reason=%d", reason);
}

static void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing BLE manager");

    // Create mutex for thread safety
    ble_mutex = xSemaphoreCreateMutex();
    if (ble_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create BLE mutex");
        return;
    }

    // Initialize NVS (required for BLE)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize NimBLE
    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init NimBLE port: %s", esp_err_to_name(ret));
        return;
    }

    // Configure NimBLE host
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;

    // Initialize GAP and GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // Register GATT services
    int rc = ble_gatts_count_cfg(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error counting GATT services; rc=%d", rc);
        return;
    }

    rc = ble_gatts_add_svcs(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error adding GATT services; rc=%d", rc);
        return;
    }

    // Set device name
    ble_svc_gap_device_name_set(BLE_DEVICE_NAME);

    // Start NimBLE host task
    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "BLE manager initialized, device name: %s", BLE_DEVICE_NAME);
}

bool ble_is_connected(void)
{
    bool connected;
    if (xSemaphoreTake(ble_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        connected = device_connected;
        xSemaphoreGive(ble_mutex);
    } else {
        connected = false;
    }
    return connected;
}

void ble_send_chip_data(uint8_t chip_num, pcap_data_t* data)
{
    // Take mutex with timeout to avoid blocking sensor task
    if (xSemaphoreTake(ble_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;
    }

    if (!device_connected || conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        xSemaphoreGive(ble_mutex);
        return;
    }

    // Create data packet
    // Format: [chip_num][sensor0_4B]...[sensor5_4B] = 25 bytes
    sensor_data_val[0] = chip_num;

    int idx = 1;
    for (int i = 0; i < NUM_SENSORS_PER_CHIP; i++) {
        // Calculate calibrated value
        int32_t calibrated = (int32_t)(data->raw[i] - data->offset[i]);

        // Pack as 4 bytes (32-bit signed integer, big-endian)
        sensor_data_val[idx++] = (calibrated >> 24) & 0xFF;
        sensor_data_val[idx++] = (calibrated >> 16) & 0xFF;
        sensor_data_val[idx++] = (calibrated >> 8) & 0xFF;
        sensor_data_val[idx++] = calibrated & 0xFF;
    }

    // Send notification
    struct os_mbuf *om = ble_hs_mbuf_from_flat(sensor_data_val, sizeof(sensor_data_val));
    if (om) {
        ble_gatts_notify_custom(conn_handle, sensor_data_handle, om);
    }

    xSemaphoreGive(ble_mutex);
}

void ble_send_status(const char* status)
{
    if (xSemaphoreTake(ble_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;
    }

    if (!device_connected || conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        xSemaphoreGive(ble_mutex);
        return;
    }

    strncpy(status_val, status, sizeof(status_val) - 1);
    status_val[sizeof(status_val) - 1] = '\0';

    // Send notification
    struct os_mbuf *om = ble_hs_mbuf_from_flat(status_val, strlen(status_val));
    if (om) {
        ble_gatts_notify_custom(conn_handle, status_handle, om);
    }

    xSemaphoreGive(ble_mutex);
}

void ble_send_battery(uint8_t battery_percentage)
{
    if (xSemaphoreTake(ble_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;
    }

    if (!device_connected || conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        xSemaphoreGive(ble_mutex);
        return;
    }

    // Create battery packet
    // Format: [0xFF][battery_percentage] = 2 bytes
    uint8_t battery_data[2];
    battery_data[0] = 0xFF;  // Battery message identifier
    battery_data[1] = battery_percentage;

    // Send notification using sensor data characteristic
    struct os_mbuf *om = ble_hs_mbuf_from_flat(battery_data, sizeof(battery_data));
    if (om) {
        ble_gatts_notify_custom(conn_handle, sensor_data_handle, om);
    }

    xSemaphoreGive(ble_mutex);
}