#include "USB.h"
#include <Arduino.h>

#include "utils.h"
#include "task_hmi.h"
#include "task_can.h"
#include "task_frame_handler.h"
#include "task_networking.h"
#include "task_accel.h"
#include "utils.h"

#define SYSTEM_MAX_RUNTIME    30//24 * 3600   // [s]

void setup() {
  xTaskCreate(task_hmi, "task_hmi", 1024, NULL, 1, NULL);
  vTaskDelay(10);
  hmi_setLed(led_t{.type = LED_STATUS, .mode = LED_BREATH, .color = WHITE});

  bool error = false;
  error |= !utils_init("MONITOR");
  error |= !utils_systemConfig("system.json");
  error |= !utils_startMsc();
  if (error) {
    hmi_setLed(led_t{.type = LED_STATUS, .mode = LED_BLINK_FAST, .color = RED});
    while (1) yield();
  }

  xTaskCreate(task_accel, "task_accel", 2048, NULL, 1, NULL);
  xTaskCreate(task_networking, "task_networking", 8192, NULL, 1, NULL);
  xTaskCreate(task_can, "task_can", 8192, NULL, 1, NULL);
  xTaskCreate(task_frame_handler, "task_frame_handler", 8192, NULL, 3, NULL);
}

void loop() {
  if(xTaskGetTickCount() / 1000 > SYSTEM_MAX_RUNTIME) {
    esp_restart();
  }
  vTaskDelay(1000);
}
