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

#include "USB.h"
#include <Arduino.h>

#include "utils.h"
#include "task_hmi.h"
#include "task_can.h"
#include "task_frame_handler.h"
#include "task_networking.h"
#include "task_accel.h"
#include "utils.h"

#define SYSTEM_MAX_RUNTIME 24 * 3600  // [s]

void setup() {
  // First create HMI task to indicate if an error occured in bootup
  xTaskCreate(task_hmi, "task_hmi", 1024, NULL, 1, NULL);
  vTaskDelay(10);
  hmi_setLed(led_t{.type = LED_STATUS, .mode = LED_BREATH, .color = WHITE});

  // Initialize all utilits (filesystem, usb etc.)
  bool error = false;
  error |= !utils_init("MONITOR");
  error |= !utils_systemConfig("system.json");
  error |= !utils_startMsc();
  if (error) {
    hmi_setLed(led_t{.type = LED_STATUS, .mode = LED_BLINK_FAST, .color = RED});
    while (1) yield();
  }

  // Create all tasks
  xTaskCreate(task_accel, "task_accel", 2048, NULL, 1, NULL);
  xTaskCreate(task_networking, "task_networking", 8192, NULL, 1, NULL);
  xTaskCreate(task_can, "task_can", 8192, NULL, 1, NULL);
  xTaskCreate(task_frame_handler, "task_frame_handler", 8192, NULL, 3, NULL);
}

void loop() {
  static bool serialState = false;
  if(Serial != serialState) {
    serialState = Serial;
    vTaskDelay(500);  // Give terminal some time to accept incoming data
    Serial.printf(CLEAR_TERMINAL);
    Serial.println("****************************************************");
    Serial.println("*               onway AG FleetMonitor              *");
    Serial.println("*     2021 Institute of Networked Solutions OST    *");
    Serial.println("****************************************************");
    Serial.println();
  }

  if (xTaskGetTickCount() / 1000 > SYSTEM_MAX_RUNTIME) {
    esp_restart();
  }
  vTaskDelay(100);
}
