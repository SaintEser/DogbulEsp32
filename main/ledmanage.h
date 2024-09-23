#ifndef LEDMANAGE_H
#define LEDMANAGE_H
#define LED_PIN 8
#include "led_strip.h"
#include "esp_log.h"

extern const char *LED_TAG;
extern led_strip_handle_t led_strip;

void blink_led(bool s_led_state, uint8_t red, uint8_t green, uint8_t blue);
void configure_led(void);
void check_sim(char *buffer, size_t buffer_size);

#endif // LED_H