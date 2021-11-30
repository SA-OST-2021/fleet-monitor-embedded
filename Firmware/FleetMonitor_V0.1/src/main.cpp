#include "USB.h"
#include <Arduino.h>

#include "utils.h"
#include "system_parser.h"
#include "config_parser.h"
#include "hmi_task.h"
#include "task_can.h"
#include "task_frame_handler.h"
#include "task_networking.h"
#include "task_accel.h"
#include "utils.h"

SystemParser systemParser;  // TODO: Move to better place?
ConfigParser configParser;  // TODO: Move to task_can

void setup() {
  utils_init("MONITOR");
  while (!USBSerial) yield();
  USBSerial.printf("\033[2J\033[1;1H");
  USBSerial.println("FleetMonitor_V0.1");

  if (!systemParser.loadFile("system.json")) {
    USBSerial.println("System config loading failed.");
    return;
  }
  USBSerial.println("System config loading was successful.");

  if (!configParser.loadFile("config.json")) {
    USBSerial.println("Config loading failed.");
    return;
  }
  USBSerial.println("Config loading was successful.");

  USBSerial.println("\n");
  USBSerial.printf("getSsid(): %s\n", systemParser.getSsid());
  USBSerial.printf("getPassword(): %s\n", systemParser.getPassword());
  USBSerial.printf("getHostIp(): %s\n", systemParser.getHostIp());
  USBSerial.printf("getHostPort(): %d\n", systemParser.getHostPort());
  USBSerial.printf("getConnectionType(): %d\n", systemParser.getConnectionType());
  USBSerial.printf("getConfigMode(): %d\n", systemParser.getConfigMode());
  if (systemParser.getBootloaderMode()) {
    USBSerial.printf("getBootloaderMode(): true");
    // TODO: Start bootloader mode
  }
  USBSerial.println("\n");

  xTaskCreate(task_hmi, "task_hmi", 1024, NULL, 1, NULL);
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
