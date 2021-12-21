/*
 * Fleet-Monitor Software
 * Copyright (C) 2021 Institute of Networked Solutions OST
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
enum led_mode_e { LED_OFF, LED_ON, LED_BLINK, LED_BLINK_FAST, LED_FLASH, LED_BREATH };
enum led_color_e { NONE, RED, GREEN, BLUE, MAGENTA, CYAN, YELLOW, WHITE };

struct led_t {
  led_type_e type;
  led_mode_e mode;
  led_color_e color;
};

void task_hmi(void *pvParameter);
void hmi_setLed(led_t led);
