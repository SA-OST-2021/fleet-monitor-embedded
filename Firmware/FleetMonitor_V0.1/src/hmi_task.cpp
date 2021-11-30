#include "hmi_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TASK_IMU_FREQ    10    // TODO: Change to frequency

QueueHandle_t hmiQueue;

static void setLedColor(led_type_e type, led_color_e color);

void task_hmi(void *pvParameter)
{
  const TickType_t task_freq = TASK_IMU_FREQ;
  TickType_t task_last_tick = xTaskGetTickCount();
  hmiQueue = xQueueCreate(10, sizeof(led_t));
  uint32_t blinkTimer = 0, breathTimer = 0;
  bool blinkFlag = false;
  led_t leds[2];

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
    led_t led;
    if (xQueueReceive(hmiQueue, &(led), (TickType_t) 0) == pdPASS) {
      if(led.type == LED_STATUS) {
        leds[0].type = LED_STATUS;
        leds[0].color = led.color;
        leds[0].mode = led.mode;
      }
      if(led.type == LED_CAN) {
        leds[1].type = LED_CAN;
        leds[1].color = led.color;
        leds[1].mode = led.mode;
      }
    }

    for(int i = 0; i < sizeof(leds) / sizeof(leds[0]); i++) {
      switch(leds[i].mode) {
        case LED_OFF:
          setLedColor(leds[i].type, NONE);
          break;
        case LED_ON:
          setLedColor(leds[i].type, leds[i].color);
          break;
        case LED_BLINK:
          setLedColor(leds[i].type, blinkFlag ? leds[i].color : NONE);
          break;
        case LED_BREATH:
          break;
      }
    }

    vTaskDelayUntil(&task_last_tick, task_freq);
    blinkTimer++;
    if(blinkTimer >= LED_BLINK_TIME / TASK_IMU_FREQ) {
      blinkTimer = 0;
      blinkFlag = !blinkFlag;
    }
  } 
}

void hmi_setLed(const led_t led)
{
  xQueueSend(hmiQueue, (void *)&led, (TickType_t) 0);
}

static void setLedColor(led_type_e type, led_color_e color) {
  if(type == LED_STATUS) {
    digitalWrite(STATUS_LED_RED, color == RED || color == YELLOW || color == MAGENTA || color == WHITE);
    digitalWrite(STATUS_LED_GREEN, color == GREEN || color == YELLOW || color == CYAN || color == WHITE);
    digitalWrite(STATUS_LED_BLUE, color == BLUE || color == MAGENTA || color == CYAN || color == WHITE);
  }
  if(type == LED_CAN) {
    digitalWrite(CAN_LED_RED, color == RED || color == YELLOW || color == MAGENTA || color == WHITE);
    digitalWrite(CAN_LED_GREEN, color == GREEN || color == YELLOW || color == CYAN || color == WHITE);
    digitalWrite(CAN_LED_BLUE, color == BLUE || color == MAGENTA || color == CYAN || color == WHITE);
  }
}