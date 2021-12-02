#include "USB.h"
#include <Arduino.h>

#include "utils.h"
#include "hmi_task.h"
#include "task_can.h"
#include "task_frame_handler.h"
#include "task_networking.h"
#include "task_accel.h"
#include "utils.h"

void setup() {
  xTaskCreate(task_hmi, "task_hmi", 1024, NULL, 1, NULL);
  vTaskDelay(10);
  // hmi_setLed(led_t{.type = LED_STATUS, .mode = LED_BREATH, .color = GREEN});

  utils_init("MONITOR");
  utils_systemConfig("system.json");

  // TODO: Remove blocking USBSerial in final version
  while (!USBSerial) yield();
  USBSerial.printf(CLEAR_TERMINAL);
  USBSerial.println("FleetMonitor_V0.1");

  xTaskCreate(task_accel, "task_accel", 2048, NULL, 1, NULL);
  xTaskCreate(task_networking, "task_networking", 32096, NULL, 1, NULL);
  xTaskCreate(task_can, "task_can", 14096, NULL, 1, NULL);
  xTaskCreate(task_frame_handler, "task_frame_handler", 64096, NULL, 3, NULL);

  USBSerial.println("Task Initialization Done");
  vTaskDelay(100);
  USBSerial.println();
  vTaskDelay(100);
}

void loop() {}
