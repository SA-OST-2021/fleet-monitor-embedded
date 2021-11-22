#include <Arduino.h>
#include "USB.h"

#include "hmi.h"
#include "utils.h"
#include "config_parser.h"
#include "task_networking.h"
#include "task_can.h"
#include "task_frame_handler.h"

USBCDC USBSerial;                 // TODO: Move to better place?
ConfigParser configParser;        // TODO: Move to task_can

void setup()
{
  hmi_init();
  utils_init("MONITOR");
  while(!USBSerial) yield();
  USBSerial.println("FleetMonitor_V0.1");

  xTaskCreate(task_networking,
                "task_networking",
                4096,
                NULL,
                1,
                NULL);
  
  xTaskCreate(task_can,
                "task_can",
                4096,
                NULL,
                1,
                NULL);
  
  xTaskCreate(task_frame_handler,
                "task_frame_handler",
                4096,
                NULL,
                1,
                NULL);
  
  // start the Ethernet connection:
  USBSerial.println("Task Initialization Done");
}

void loop()
{
  digitalWrite(STATUS_LED_GREEN, (millis() % 200) < 100);   // TODO: Create HMI function for LED control
}
