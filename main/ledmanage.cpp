#include "ledmanage.h"
#include <cstring>

const char *LED_TAG = "LedStrip";
led_strip_handle_t led_strip;
uint8_t intensity = 128; // 50% intensity (128 out of 255)

volatile led_effect_t current_led_effect = LED_EFFECT_OFF;

// Function to get RGB values for a given LED color with intensity
RgbValue getRgbValue(LedColor color, uint8_t intensity)
{
  RgbValue baseColor;
  switch (color)
  {
  case LED_COLOR_RED:
    baseColor = (RgbValue){255, 0, 0};
    break;
  case LED_COLOR_GREEN:
    baseColor = (RgbValue){0, 255, 0};
    break;
  case LED_COLOR_BLUE:
    baseColor = (RgbValue){0, 0, 255};
    break;
  case LED_COLOR_YELLOW:
    baseColor = (RgbValue){255, 255, 0};
    break;
  case LED_COLOR_CYAN:
    baseColor = (RgbValue){0, 255, 255};
    break;
  case LED_COLOR_MAGENTA:
    baseColor = (RgbValue){255, 0, 255};
    break;
  case LED_COLOR_WHITE:
    baseColor = (RgbValue){255, 255, 255};
    break;
  case LED_COLOR_BLACK:
  default:
    baseColor = (RgbValue){0, 0, 0};
    break;
  }

  // Scale the RGB values by the intensity
  baseColor.red = (uint8_t)((baseColor.red * intensity) / 255);
  baseColor.green = (uint8_t)((baseColor.green * intensity) / 255);
  baseColor.blue = (uint8_t)((baseColor.blue * intensity) / 255);

  return baseColor;
}

void configure_led(void)
{
  ESP_LOGI(LED_TAG, "Example configured to blink addressable LED!");

  led_strip_config_t strip_config = {
      /* LED strip initialization with the GPIO and pixels number*/
      .strip_gpio_num = LED_PIN,
      .max_leds = 1, // at least one LED on board
      .led_pixel_format = LED_PIXEL_FORMAT_GRB,
      .led_model = LED_MODEL_WS2812,
      .flags = 0, // Initialize flags to 0
  };
  led_strip_rmt_config_t rmt_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = 10 * 1000 * 1000, // 10MHz
      .mem_block_symbols = 0,
      .flags = 0, // Use DMA to transmit data
  };
  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
  led_strip_clear(led_strip); /* Set all LED off to clear all pixels */
}

void led_set_on(LedColor color, uint8_t intensity) // what is  uint8_t and values it can take?
{
  RgbValue rgbColor = getRgbValue(color, intensity);
  led_strip_set_pixel(led_strip, 0, rgbColor.red, rgbColor.green, rgbColor.blue);
  led_strip_refresh(led_strip); /* Refresh the strip to send data */
}

void led_set_off(void)
{
  led_strip_clear(led_strip); /* Set all LED off to clear all pixels */
}

// Central control function
void manageLedEffects(void)
{
  switch (current_led_effect)
  {
  case LED_EFFECT_OFF:
    led_set_off();
    break;
  // case LED_EFFECT_BLINK:
  //   led_effect_blink(1000, 100);
  //   break;
  //  case LED_EFFECT_FADE:
  //    led_effect_fade(5000);
  //    break;
  case LED_EFFECT_ON:
    led_set_on(LED_COLOR_WHITE, intensity);
    break;
  }
}

void blink_led(bool s_led_state, uint8_t red, uint8_t green, uint8_t blue)
{
  if (s_led_state) /* If the addressable LED is enabled */
  {
    led_strip_set_pixel(led_strip, 0, red, green, blue);
    led_strip_refresh(led_strip); /* Refresh the strip to send data */
    ESP_LOGI(LED_TAG, "LED ON!");
  }
  else
  {
    led_strip_clear(led_strip); /* Set all LED off to clear all pixels */
    ESP_LOGI(LED_TAG, "LED OFF!");
  }
}

void simdetect_led(bool sim_detected)
{
  if (sim_detected)
  {
    // blink_led(true, 0, 16, 0); /* Green */
    led_set_on(LED_COLOR_GREEN, 16);
  }
  else
  {
    // blink_led(true, 16, 0, 0); /* Red */
    led_set_on(LED_COLOR_RED, 16);
  }
}

// Function prototypes for LED effects
// void led_effect_blink(int duration, int interval, led_color_t color);

// void led_effect_fade(int duration, led_color_t color);
