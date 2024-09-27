#ifndef LEDMANAGE_H
#define LEDMANAGE_H
#define LED_PIN 8
#include "..\managed_components\espressif__led_strip\include\led_strip.h" // Include the header file for the LED strip :

#include "esp_log.h"

typedef enum
{
  LED_EFFECT_OFF,
  LED_EFFECT_ON,
  // LED_EFFECT_RAINBOW,
  // LED_EFFECT_BLINK,
  // LED_EFFECT_FADE,
  // LED_EFFECT_BEACON,
} led_effect_t;

typedef enum
{
  LED_COLOR_RED,
  LED_COLOR_GREEN,
  LED_COLOR_BLUE,
  LED_COLOR_YELLOW,
  LED_COLOR_CYAN,
  LED_COLOR_MAGENTA,
  LED_COLOR_WHITE,
  LED_COLOR_BLACK
} LedColor;

// Define a structure to hold RGB values
typedef struct
{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} RgbValue;

// State variable
extern volatile led_effect_t current_led_effect;

void blink_led(bool s_led_state, uint8_t red, uint8_t green, uint8_t blue);
void configure_led(void);
void simdetect_led(bool sim_detected);
void manageLedEffects(void);
void led_set_on(LedColor color, uint8_t intensity);
void led_set_off(void);
// Public methods and variables can be accessed from outside
RgbValue getRgbValue(LedColor color, uint8_t intensity);

#endif // LED_H