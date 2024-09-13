#include "Arduino.h"
#include "led_strip.h"
#include "BG95ESP32.h"

// #define _DEBUG 1 //BG95AT.cpp does not read _DEBUG 1 somehow??
#define RX_PIN 4
#define TX_PIN 5
#define PIN_DTR 3
#define PWR_PIN 2
#define SIM_RST 6
#define LED_PIN 8
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


static const char *TAG = "LedStrip";
static uint8_t s_led_state = 0;

HardwareSerial SerialAT(1);

BG95ESP32 bg95 = BG95ESP32(SIM_RST, PWR_PIN);

void PowerToggle();
// bool PowerCheck();
static void blink_led(void);
static void configure_led(void);
static led_strip_handle_t led_strip;

typedef __FlashStringHelper *__StrPtr;

char buffer[BUFFER_SIZE];

// the setup function runs once when you press reset or power the board
void setup()
{
  pinMode(PWR_PIN, OUTPUT); // BG95 Power pin initiated
  digitalWrite(PWR_PIN, LOW); //Evaluation board'da LOW olmazsa reset oluyor devamlı 
  pinMode(SIM_RST, OUTPUT); // BG95 Reset pin initiated
  digitalWrite(SIM_RST, HIGH);
  SerialMon.begin(115200); // Setup debug stream
  delay(10);
  SerialAT.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN); // Setup data stream
  delay(10);
  Log.begin(LOG_LEVEL_VERBOSE, &SerialMon);     //Define Log_Level according to detail needs
  bg95.begin(SerialAT);     //Initiate communication with bg95, very important
  bg95.init();              //Initiate and make sure bg95 turned on 

  configure_led();
  
  Log.info("This is an info message." NL);
  Log.error("This is an error message." NL);
}

void loop()
{
  blink_led();
  s_led_state = !s_led_state;
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  //delay(1000);
  //bg95.sendCommand("",buffer,BUFFER_SIZE);
  //Log.notice(S_F("Response: \"%s\" " CR),buffer);
  
}

static void blink_led(void)
{

  if (s_led_state) /* If the addressable LED is enabled */
  {
    led_strip_set_pixel(led_strip, 0, 0, 0, 16); /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
    led_strip_refresh(led_strip);                /* Refresh the strip to send data */
    ESP_LOGI(TAG, "LED ON!");
  }
  else
  {
    led_strip_clear(led_strip); /* Set all LED off to clear all pixels */
    ESP_LOGI(TAG, "LED OFF!");
  }
}

static void configure_led(void)
{
  ESP_LOGI(TAG, "Example configured to blink addressable LED!");

  led_strip_config_t strip_config = {
      /* LED strip initialization with the GPIO and pixels number*/
      .strip_gpio_num = LED_PIN,
      .max_leds = 1, // at least one LED on board
      .led_pixel_format = LED_PIXEL_FORMAT_GRB,
      .led_model = LED_MODEL_WS2812,

  };
  led_strip_rmt_config_t rmt_config = {
      .resolution_hz = 10 * 1000 * 1000, // 10MHz
      .mem_block_symbols = 0,
  };
  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
  led_strip_clear(led_strip); /* Set all LED off to clear all pixels */
}