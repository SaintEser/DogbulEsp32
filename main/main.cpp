#include "Arduino.h"
#include "esp_log.h"

#include "BG95ESP32.h"

#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"

#include "ledmanage.h"
#define DELAY(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define DELAY_1S DELAY(1000)
#define DEVICE_NAME "ESP32_C3_BLE" // Defines the Bluetooth device name broadcasted to clients

// #define _DEBUG 1 //BG95AT.cpp does not read _DEBUG 1 somehow??
#define RX_PIN 4
#define TX_PIN 5
#define PIN_DTR 3
#define PWR_PIN 2
#define SIM_RST 6

#define SerialMon Serial
#define SerialAT SIM_SERIAL_TYPE
#define BUFFER_SIZE 256
#define STATUS_SIZE 16

#define STRING_IS(s1, s2) strcasecmp_P(s1, PSTR(s2)) == 0
#define BUFFER_IS(s) STRING_IS(buffer, s)
#define BUFFER_IS_P(s) strcasecmp_P(buffer, s) == 0
#define PRINT(s) Serial.print(S_F(s))
#define PRINT_LN(s) Serial.println(S_F(s))
#define STRLCPY_P(s1, s2, size) strlcpy_P(s1, s2, size)

// Attribute state (initial value for characteristics)
// static uint8_t char_value[] = {0x00};

HardwareSerial SerialAT(1);

BG95ESP32 bg95 = BG95ESP32(SIM_RST, PWR_PIN);

typedef __FlashStringHelper *__StrPtr;

char buffer[BUFFER_SIZE];

uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 16;
bool s_led_state = 0;

// the setup function runs once when you press reset or power the board
void setup()
{
  pinMode(PWR_PIN, OUTPUT);   // BG95 Power pin initiated
  digitalWrite(PWR_PIN, LOW); // Evaluation board'da LOW olmazsa reset oluyor devamlı
  pinMode(SIM_RST, OUTPUT);   // BG95 Reset pin initiated
  digitalWrite(SIM_RST, HIGH);
  SerialMon.begin(115200); // Setup debug stream
  vTaskDelay(pdMS_TO_TICKS(10));
  SerialAT.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN); // Setup data stream
  vTaskDelay(pdMS_TO_TICKS(10));
  Log.begin(LOG_LEVEL_VERBOSE, &SerialMon); // Define Log_Level according to detail needs
  bg95.begin(SerialAT);                     // Initiate communication with bg95, very important
  bg95.init();                              // Initiate and make sure bg95 turned on
  configure_led();

  Log.info("This is an info message." NL);
  Log.error("This is an error message." NL);
}

void loop()
{
  blink_led(1, 0, 0, 0);
  // s_led_state = 1;
  //  s_led_state = !s_led_state;
  //  DELAY_1S;
  bg95.getSimState(buffer, BUFFER_SIZE);
  check_sim(buffer, BUFFER_SIZE);

  Log.notice(S_F("Response: %s " NL), buffer);
  // BG95ESP32PinStatus pinState1 = bg95.getPinState1();
  // Log.notice(S_F("Pin Count: %d"  NL), pinState1.pin);
  // Log.notice(S_F("Puk Count: %d"  NL), pinState1.puk);
  // vTaskDelay(pdMS_TO_TICKS(2000));
  // BG95ESP32PinStatus pinState2 = bg95.getPinState2();
  // Log.notice(S_F("Pin2 Count: %d"  NL), pinState2.pin);
  // Log.notice(S_F("Puk2 Count: %d"  NL), pinState2.puk);
  vTaskDelay(pdMS_TO_TICKS(4000));
  // delay(1000);
  // bg95.sendCommand("+C",buffer,BUFFER_SIZE);
  // Log.notice(S_F("Response: \"%s\" " CR),buffer);
}
