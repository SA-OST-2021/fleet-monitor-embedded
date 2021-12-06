#pragma once

#include <Arduino.h>

#define USER_BUTTON      0
#define CAN_LED_RED      1
#define CAN_LED_GREEN    2
#define CAN_LED_BLUE     3
#define STATUS_LED_RED   4
#define STATUS_LED_GREEN 5
#define STATUS_LED_BLUE  6

#define LED_BLINK_TIME      500   // [ms]
#define LED_BLINK_FAST_TIME 100   // [ms]
#define LED_BREATH_TIME     2000  // [ms]
#define LONG_PRESS_TIME     5000  // [ms]

enum led_type_e { LED_STATUS, LED_CAN };
enum led_mode_e { LED_OFF, LED_ON, LED_BLINK, LED_BLINK_FAST, LED_BREATH };
enum led_color_e { NONE, RED, GREEN, BLUE, MAGENTA, CYAN, YELLOW, WHITE };

struct led_t {
  led_type_e type;
  led_mode_e mode;
  led_color_e color;
};

void task_hmi(void *pvParameter);
void hmi_setLed(led_t led);
