#include <Arduino.h>
#include <SPI.h>
#include <USB.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "task_networking.h"
#include "task_can.h"
#include "task_frame_handler.h"



USBCDC USBSerial;

#define CAN_LED_RED 1
#define CAN_LED_GREEN 2
#define CAN_LED_BLUE 3
#define STATUS_LED_RED 4
#define STATUS_LED_GREEN 5
#define STATUS_LED_BLUE 6


void setup() {
  // Open serial communications and wait for port to open:
  USB.begin();
  USB.productName("ESP32");
  USBSerial.begin();
  //while (!USBSerial);
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

  pinMode(STATUS_LED_GREEN, OUTPUT);
  pinMode(CAN_LED_GREEN, OUTPUT);
}

void loop() {
    digitalWrite(STATUS_LED_GREEN, HIGH);  
    vTaskDelay(400);
    digitalWrite(STATUS_LED_GREEN, LOW);  
    vTaskDelay(400);
}