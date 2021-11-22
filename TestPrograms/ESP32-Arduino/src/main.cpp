#include <Arduino.h>
#include <SPI.h>
#include <USB.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "task_networking.h"
#include "task_can.h"




USBCDC USBSerial;

#define STATUS_LED_RED 1
#define STATUS_LED_BLUE 2
#define STATUS_LED_GREEN 3



static QueueHandle_t tx_task_queue;


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
  
  // start the Ethernet connection:
  USBSerial.println("Task Initialization Done");

  pinMode(STATUS_LED_RED, OUTPUT);
}

void loop() {
    digitalWrite(STATUS_LED_RED, HIGH);  
    vTaskDelay(400);
    digitalWrite(STATUS_LED_RED, LOW);  
    vTaskDelay(400);
}