#include "hmi_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void task_hmi(void *pvParameter)
{
    digitalWrite(CAN_LED_RED, 1);
    digitalWrite(CAN_LED_GREEN, 1);
    digitalWrite(CAN_LED_BLUE, 1);
    digitalWrite(STATUS_LED_RED, 1);
    digitalWrite(STATUS_LED_GREEN, 1);
    digitalWrite(STATUS_LED_BLUE, 1);

    pinMode(CAN_LED_RED, OUTPUT);       
    pinMode(CAN_LED_GREEN, OUTPUT);     
    pinMode(CAN_LED_BLUE, OUTPUT);      
    pinMode(STATUS_LED_RED, OUTPUT);    
    pinMode(STATUS_LED_GREEN, OUTPUT);  
    pinMode(STATUS_LED_BLUE, OUTPUT);

    while(true)
    {
      // TODO: Make better example code
      digitalWrite(STATUS_LED_GREEN, 1);
      vTaskDelay(200);
      digitalWrite(STATUS_LED_GREEN, 0);
      vTaskDelay(200);
    } 
}