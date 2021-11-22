#include <Arduino.h>
#include "USB.h"

#include "utils.h"
#include "task_networking.h"
#include "task_can.h"
#include "task_frame_handler.h"

// TODO: Move to "hmi.h" file
#define CAN_LED_RED       1
#define CAN_LED_GREEN     2
#define CAN_LED_BLUE      3
#define STATUS_LED_RED    4
#define STATUS_LED_GREEN  5
#define STATUS_LED_BLUE   6

USBCDC USBSerial;                 // TODO: Move to better place?

void setup()
{
  // TODO: move to "hmi_init()"
  pinMode(CAN_LED_RED, OUTPUT);       digitalWrite(CAN_LED_RED, 1);
  pinMode(CAN_LED_GREEN, OUTPUT);     digitalWrite(CAN_LED_GREEN, 1);
  pinMode(CAN_LED_BLUE, OUTPUT);      digitalWrite(CAN_LED_BLUE, 1);
  pinMode(STATUS_LED_RED, OUTPUT);    digitalWrite(STATUS_LED_RED, 1);
  pinMode(STATUS_LED_GREEN, OUTPUT);  digitalWrite(STATUS_LED_GREEN, 1);
  pinMode(STATUS_LED_BLUE, OUTPUT);   digitalWrite(STATUS_LED_BLUE, 1);

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
  digitalWrite(STATUS_LED_GREEN, (millis() % 200) < 100);
}
