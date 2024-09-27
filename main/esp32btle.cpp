#include "esp32btle.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"

#define GATTS_TAG "GATTS_BATTERY_DEMO"
#define ESP_APP_ID 0x55
#define SAMPLE_DEVICE_NAME "ESP_BATTERY"

uint8_t esp32btle::battery_level = 50; // Example battery level
esp_gatt_char_prop_t esp32btle::battery_property = 0;
esp_attr_value_t esp32btle::gatts_battery_level_val = {
    .attr_max_len = sizeof(uint8_t),
    .attr_len = sizeof(uint8_t),
    .attr_value = &esp32btle::battery_level,
};

// Configure the advertising parameters
esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .peer_addr = {0},
    .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// Global variable to store the attribute handle
uint16_t battery_level_handle = 0;

esp32btle::esp32btle() {}

void esp32btle::init()
{
  esp_err_t ret;

  // Initialize NVS
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Initialize the ESP32 Bluetooth controller
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  ESP_ERROR_CHECK(ret);

  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  ESP_ERROR_CHECK(ret);

  ret = esp_bluedroid_init();
  ESP_ERROR_CHECK(ret);

  ret = esp_bluedroid_enable();
  ESP_ERROR_CHECK(ret);

  // Register the GATT server
  ret = esp_ble_gatts_register_callback(gatts_profile_event_handler);
  ESP_ERROR_CHECK(ret);

  ret = esp_ble_gatts_app_register(ESP_APP_ID);
  ESP_ERROR_CHECK(ret);

  // Set the device name
  esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);

  // Configure the advertising data
  esp_ble_adv_data_t adv_data = {
      .set_scan_rsp = false,
      .include_name = true,
      .include_txpower = false,
      .min_interval = 0x0006,
      .max_interval = 0x0010,
      .appearance = 0x00,
      .manufacturer_len = 0,
      .p_manufacturer_data = NULL,
      .service_data_len = 0,
      .p_service_data = NULL,
      .service_uuid_len = 16,
      .p_service_uuid = (uint8_t *)"\x00\x18\x0F\x00\x00\x00\x10\x00\x80\x00\x00\x80\x5F\x9B\x34\xFB",
      .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
  };

  ret = esp_ble_gap_config_adv_data(&adv_data);
  ESP_ERROR_CHECK(ret);

  // Start advertising
  ret = esp_ble_gap_start_advertising(&adv_params);
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(GATTS_TAG, "Bluetooth initialized and advertising started");
}

void esp32btle::update_battery_level(uint8_t battery)
{
  battery_level = battery;
  esp_ble_gatts_set_attr_value(battery_level_handle, sizeof(uint8_t), &battery_level);
  ESP_LOGI(GATTS_TAG, "Battery level updated to %d", battery_level);
}

void esp32btle::gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                            esp_gatt_if_t gatts_if,
                                            esp_ble_gatts_cb_param_t *param)
{
  ESP_LOGE(GATTS_TAG, "event = %x", event);
  switch (event)
  {
  case ESP_GATTS_REG_EVT:
    ESP_LOGI(GATTS_TAG, "ESP_GATTS_REG_EVT");
    break;
  case ESP_GATTS_CONNECT_EVT:
    ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
    break;
  case ESP_GATTS_DISCONNECT_EVT:
    ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = %d", param->disconnect.reason);
    // Restart advertising after disconnection
    esp_ble_gap_start_advertising(&adv_params);
    break;
  default:
    break;
  }
}