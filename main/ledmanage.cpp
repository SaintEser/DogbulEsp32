#include "ledmanage.h"
#include <cstring>

led_strip_handle_t led_strip;
const char *LED_TAG = "LedStrip";

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

void blink_led(bool s_led_state, uint8_t red, uint8_t green, uint8_t blue)
{
  if (s_led_state) /* If the addressable LED is enabled */
  {
    led_strip_set_pixel(led_strip, 0, red, green, blue); /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
    led_strip_refresh(led_strip);                        /* Refresh the strip to send data */
    ESP_LOGI(LED_TAG, "LED ON!");
  }
  else
  {
    led_strip_clear(led_strip); /* Set all LED off to clear all pixels */
    ESP_LOGI(LED_TAG, "LED OFF!");
  }
}

void check_sim(char *buffer, size_t buffer_size)
{
  if (buffer[0] == '0')
  { // Check if the buffer starts with '0'
    blink_led(1, 16, 0, 0);
  }
  else if (strlen(buffer) > 0)
  { // Check if the buffer is not empty
    blink_led(1, 0, 16, 0);
  }
  ESP_LOGI(LED_TAG, "Response: %s ", buffer);
}