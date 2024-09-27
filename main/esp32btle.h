#ifndef ESP32_BTLE_H
#define ESP32_BTLE_H

#include "esp_gatts_api.h"

class esp32btle
{
public:
  esp32btle();
  void init();
  void update_battery_level(uint8_t new_level);
  static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                          esp_gatt_if_t gatts_if,
                                          esp_ble_gatts_cb_param_t *param);
  static uint8_t battery_level;

private:
  static esp_gatt_char_prop_t battery_property;
  static esp_attr_value_t gatts_battery_level_val;
};

#endif // ESP32_BTLE_H