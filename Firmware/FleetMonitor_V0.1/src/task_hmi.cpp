#include "task_hmi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Adafruit_TinyUSB.h"
#include "utils.h"

#define TASK_IMU_FREQ 10  // TODO: Change to frequency

QueueHandle_t hmiQueue;

static void setLedColor(led_type_e type, led_color_e color, uint8_t brightness = 255);

static const uint8_t gamma8[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,
    2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,   6,
    6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  10,  11,  11,  11,  12,  12,  13,
    13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,
    24,  25,  25,  26,  27,  27,  28,  29,  29,  30,  31,  32,  32,  33,  34,  35,  35,  36,  37,  38,  39,  39,
    40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  50,  51,  52,  54,  55,  56,  57,  58,  59,  60,  61,
    62,  63,  64,  66,  67,  68,  69,  70,  72,  73,  74,  75,  77,  78,  79,  81,  82,  83,  85,  86,  87,  89,
    90,  92,  93,  95,  96,  98,  99,  101, 102, 104, 105, 107, 109, 110, 112, 114, 115, 117, 119, 120, 122, 124,
    126, 127, 129, 131, 133, 135, 137, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167,
    169, 171, 173, 175, 177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213, 215, 218,
    220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255};

void task_hmi(void *pvParameter) {
  const TickType_t task_freq = TASK_IMU_FREQ;
  TickType_t task_last_tick = xTaskGetTickCount();
  hmiQueue = xQueueCreate(10, sizeof(led_t));
  uint32_t blinkTimer = 0, blinkFastTimer = 0, breathTimer = 0;
  bool blinkFlag = false, blinkFastFlag = false, breathFlag = true;
  bool updateLedFlag = true;
  uint8_t breathValue = 0;
  led_t leds[2];
  led_t nextLeds[2];

  digitalWrite(CAN_LED_RED, 1);
  digitalWrite(CAN_LED_GREEN, 1);
  digitalWrite(CAN_LED_BLUE, 1);
  digitalWrite(STATUS_LED_RED, 1);
  digitalWrite(STATUS_LED_GREEN, 1);
  digitalWrite(STATUS_LED_BLUE, 1);

  pinMode(USER_BUTTON, INPUT_PULLUP);
  pinMode(CAN_LED_RED, OUTPUT);
  pinMode(CAN_LED_GREEN, OUTPUT);
  pinMode(CAN_LED_BLUE, OUTPUT);
  pinMode(STATUS_LED_RED, OUTPUT);
  pinMode(STATUS_LED_GREEN, OUTPUT);
  pinMode(STATUS_LED_BLUE, OUTPUT);

  while (true) {
    led_t led;

    static uint32_t longPressTimer = 0;
    if (!digitalRead(USER_BUTTON)) {
      longPressTimer++;
    } else {
      longPressTimer = 0;
    }
    if (longPressTimer >= LONG_PRESS_TIME / TASK_IMU_FREQ) {
      longPressTimer = 0;
      setLedColor(LED_STATUS, RED);
      TinyUSBDevice.detach();
      utils_format("MONITOR");
      vTaskDelay(500);
      yield();
      usb_persist_restart(RESTART_NO_PERSIST);
    }

    if (xQueueReceive(hmiQueue, &(led), (TickType_t)0) == pdPASS) {
      if (led.type == LED_STATUS) {
        nextLeds[0].type = LED_STATUS;
        nextLeds[0].color = led.color;
        nextLeds[0].mode = led.mode;
      }
      if (led.type == LED_CAN) {
        nextLeds[1].type = LED_CAN;
        nextLeds[1].color = led.color;
        nextLeds[1].mode = led.mode;
      }
    }

    if (updateLedFlag) {
      leds[0] = nextLeds[0];
      leds[1] = nextLeds[1];
    }
    updateLedFlag = false;

    for (int i = 0; i < sizeof(leds) / sizeof(leds[0]); i++) {
      switch (leds[i].mode) {
        case LED_OFF:
          setLedColor(leds[i].type, NONE);
          break;
        case LED_ON:
          setLedColor(leds[i].type, leds[i].color);
          break;
        case LED_BLINK:
          setLedColor(leds[i].type, blinkFlag ? leds[i].color : NONE);
          break;
        case LED_BLINK_FAST:
          setLedColor(leds[i].type, blinkFastFlag ? leds[i].color : NONE);
          break;
        case LED_FLASH:
          setLedColor(leds[i].type, leds[i].color);
          leds[i].mode = LED_OFF;  // Flash once
          updateLedFlag = true;
          break;
        case LED_BREATH:
          setLedColor(leds[i].type, leds[i].color, gamma8[breathValue]);
          break;
        default:
          break;
      }
    }

    blinkTimer++;
    if (blinkTimer >= LED_BLINK_TIME / TASK_IMU_FREQ) {
      blinkTimer = 0;
      blinkFlag = !blinkFlag;
      updateLedFlag = true;
    }
    blinkFastTimer++;
    if (blinkFastTimer >= LED_BLINK_FAST_TIME / TASK_IMU_FREQ) {
      blinkFastTimer = 0;
      blinkFastFlag = !blinkFastFlag;
    }
    if (breathFlag) {
      breathTimer++;
      if (breathTimer >= LED_BREATH_TIME / TASK_IMU_FREQ) {
        breathFlag = false;
      }
    } else {
      breathTimer--;
      if (breathTimer == 0) {
        breathFlag = true;
      }
    }
    breathValue = (uint8_t)((int32_t)breathTimer * 255 / (LED_BREATH_TIME / TASK_IMU_FREQ));

    vTaskDelayUntil(&task_last_tick, task_freq);
  }
}

void hmi_setLed(const led_t led) { xQueueSend(hmiQueue, (void *)&led, (TickType_t)0); }

static void setLedColor(led_type_e type, led_color_e color, uint8_t brightness) {
  int red = 256 - ((color == RED) || (color == YELLOW) || (color == MAGENTA) || (color == WHITE)) * (brightness);
  int green = 256 - ((color == GREEN) || (color == YELLOW) || (color == CYAN) || (color == WHITE)) * (brightness);
  int blue = 256 - ((color == BLUE) || (color == MAGENTA) || (color == CYAN) || (color == WHITE)) * (brightness);
  if (type == LED_STATUS) {
    analogWrite(STATUS_LED_RED, red);
    analogWrite(STATUS_LED_GREEN, green);
    analogWrite(STATUS_LED_BLUE, blue);
  }
  if (type == LED_CAN) {
    analogWrite(CAN_LED_RED, red);
    analogWrite(CAN_LED_GREEN, green);
    analogWrite(CAN_LED_BLUE, blue);
  }
}
