#include <Arduino.h>
#include "USB.h"

#include "utils.h"
#include "config_parser.h"
#include "hmi_task.h"
#include "task_networking.h"
#include "task_can.h"
#include "task_frame_handler.h"

USBCDC USBSerial;                 // TODO: Move to better place?
ConfigParser configParser;        // TODO: Move to task_can

void setup()
{
  utils_init("MONITOR");
  while(!USBSerial) yield();
  USBSerial.printf("\033[2J\033[1;1H");
  USBSerial.println("FleetMonitor_V0.1");

  if(!configParser.loadFile("config.json"))
  {
    USBSerial.println("Config loading failed.");
    return;
  }
  USBSerial.println("Config loading was successful.");

  USBSerial.println("\n");
  USBSerial.printf("getName(FEEE): %s\n", configParser.getName("FEEE"));
  USBSerial.printf("getFilter(FEAE): %d\n", configParser.getFilter("FEAE"));
  USBSerial.printf("getFilter(FD09): %d\n", configParser.getFilter("FD09"));
  USBSerial.printf("getFilter(FE56): %d\n", configParser.getFilter("FE56"));
  USBSerial.printf("getInterval(FEE5): %d\n", configParser.getInterval("FEE5"));
  USBSerial.printf("getInterval(FEF5): %d\n", configParser.getInterval("FEF5"));
  USBSerial.printf("isEnabled(FE6B): %d\n", configParser.isEnabled("FE6B"));
  USBSerial.printf("isEnabled(FEF2): %d\n", configParser.isEnabled("FEF2"));
  USBSerial.println("\n");

  xTaskCreate(task_hmi,
                "task_hmi",
                1024,
                NULL,
                1,
                NULL);

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
                16096,
                NULL,
                1,
                NULL);
  
  // start the Ethernet connection:
  USBSerial.println("Task Initialization Done");
}

void loop()
{
}
