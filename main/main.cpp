#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>

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
#include "esp32btle.h"

// Assuming LedManager is a global object, you need to declare it
#define DELAY(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define DELAY_1S DELAY(1000)

// #define _DEBUG 1 //BG95AT.cpp does not read _DEBUG 1 somehow??
#define RX_PIN 4
#define TX_PIN 5
#define PIN_DTR 3
#define PWR_PIN 2
#define SIM_RST 6
#define PIXEL_INDEX 0 // There is only 1 led

#define SerialMon Serial
#define SerialAT SIM_SERIAL_TYPE
#define BUFFER_SIZE 256
#define STATUS_SIZE 16
#define BG95 "BG95"

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
uint8_t battery;
float fVoltage = 0;

bool sim_detected = false;

const TickType_t xFiveMinutes = pdMS_TO_TICKS(300000); // 5 minutes in milliseconds
const TickType_t x30sec = pdMS_TO_TICKS(30000);        // 30 seconds in milliseconds

esp32btle bluetoothService;

void periodicBatt(TimerHandle_t xTimer)
{
  ESP_LOGI("Interval Timer", "Method is called every 30 sec");
  // AT+CBC Battery Charge Level, response is +CBC: <bcs>,<bcl>,<voltage>, example: +CBC: 0,100,4111
  bg95.sendCommand("+CBC", buffer, BUFFER_SIZE);
  // parse the response for bcl only
  char *bcl = strstr(buffer, "+CBC: 0,");
  char *comma = NULL;
  if (bcl != NULL)
  {
    bcl += 8; // Skip "+CBC: 0,"
    // Find the next comma
    comma = strchr(bcl, ',');
    if (comma != NULL)
    {
      *comma = '\0'; // Null-terminate the string
    }
    battery = atoi(bcl);
    ESP_LOGI("Battery Level", "Battery Level: %d", battery);
    esp32btle::battery_level = battery;
  }

  bg95.sendCommand("+CBC", buffer, BUFFER_SIZE);
  // parse the response for voltage only by looking at second comma. response is +CBC: <bcs>,<bcl>,<voltage>, example: +CBC: 0,100,4111
  char *voltage = strstr(buffer, "+CBC: ");
  if (voltage != NULL)
  {
    voltage += 7; // Skip "+CBC: "
    // Find the next comma
    comma = strchr(voltage, ',');
    if (comma != NULL)
    {
      comma = strchr(comma + 1, ',');
      if (comma != NULL)
      {
        voltage = comma + 1;
        int volt = atoi(voltage);
        ESP_LOGI("Battery Voltage", "Battery Voltage: %d", volt);
      }
    }
  }
}

// Create the software timer
TimerHandle_t timerHandle = xTimerCreate(
    "FiveMinuteTimer", // Name of the timer
    x30sec,            // Timer period
    pdTRUE,            // Auto-reload? (true means yes)
    (void *)0,         // Timer ID (not used here)
    periodicBatt       // Callback function
);

void handleTimerStart(TimerHandle_t timerHandle)
{
  if (timerHandle == NULL)
  {
    ESP_LOGI("Timer", "Failed to create timer");
  }
  else
  {
    // Start the timer
    if (xTimerStart(timerHandle, 0) != pdPASS)
    {
      ESP_LOGI("Timer", "Failed to start timer");
    }
  }
}

void setup()
{
  pinMode(PWR_PIN, OUTPUT);   // BG95 Power pin initiated
  digitalWrite(PWR_PIN, LOW); // Evaluation board'da LOW olmazsa reset oluyor devamlı
  pinMode(SIM_RST, OUTPUT);   // BG95 Reset pin initiated
  digitalWrite(SIM_RST, HIGH);
  SerialMon.begin(115200); // Setup debug stream
  DELAY(10);
  SerialAT.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN); // Setup data stream
  DELAY(10);
  // Log.begin(LOG_LEVEL_VERBOSE, &SerialMon); // Define Log_Level according to detail needs

  // INIT BG95
  bg95.begin(SerialAT); // Initiate communication with bg95, very important
  bg95.init();          // Initiate and make sure bg95 turned on

  // INIT LED
  configure_led();
  // INIT TIMER
  periodicBatt(timerHandle);

  handleTimerStart(timerHandle);
  bluetoothService.init();
  ESP_LOGI("BG95", "Setup completed");
}

void loop()
{
  // blink_led(1, 0, 0, 0);
  //  DELAY_1S;
  sim_detected = bg95.check_sim(bg95.getSimState());
  simdetect_led(sim_detected);
  //  ESP_LOGI("BG95", "Response: %s", buffer);
  //  BG95ESP32PinStatus pinState1 = bg95.getPinState1();
  //  vTaskDelay(pdMS_TO_TICKS(2000));
  //  BG95ESP32PinStatus pinState2 = bg95.getPinState2();
  //  Log.notice(S_F("Pin2 Count: %d"  NL), pinState2.pin);
  vTaskDelay(pdMS_TO_TICKS(4000));
  led_set_on(LED_COLOR_MAGENTA, 16);
  DELAY(1000);
  led_set_off();
}
