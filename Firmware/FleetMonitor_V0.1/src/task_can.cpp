// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/twai.h"
#include <USB.h>

#include "Fms.h"

#include "task_can.h"
#include "task_networking.h"
#include "hmi_task.h"

extern USBCDC USBSerial;

static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_43, GPIO_NUM_44, TWAI_MODE_NORMAL);
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

QueueHandle_t fmsQueue;

bool can_connected = false;

void print_fms_frame(Fms &frame);

void task_can(void *pvParameter) {
  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    hmi_setLed(led_t{.type = LED_CAN, .mode = LED_BLINK, .color = RED});
    USBSerial.println("Installing CAN Driver failed!");
    while (1)
      ;
  }
  if (twai_start() != ESP_OK) {
    hmi_setLed(led_t{.type = LED_CAN, .mode = LED_BLINK, .color = RED});
    USBSerial.println("Starting CAN failed!");
    while (1)
      ;
  }
  USBSerial.println("CAN Initialization Done");

  hmi_setLed(led_t{.type = LED_CAN, .mode = LED_BLINK, .color = BLUE});

  Fms known_frames[100];
  uint32_t nr_known_frames = 0;
  fmsQueue = xQueueCreate(10, sizeof(Fms));
  // Wait until we have network and configinitialized
  while (!network_connected && !config_loaded) vTaskDelay(1000);

  while (1) {
    twai_message_t rx_frame;
    if (twai_receive(&rx_frame, 1000) != ESP_OK) {
      can_connected = false;
      hmi_setLed(led_t{.type = LED_CAN, .mode = LED_BLINK, .color = BLUE});
    } else {
      hmi_setLed(led_t{.type = LED_CAN, .mode = LED_BLINK, .color = GREEN});
      can_connected = true;

      Fms frame(rx_frame);

      // Only do something with the frame if enabled
      if (config.isEnabled(frame.getPgn()) == true) {
        int index;
        bool found = false;

        // Check if we have previously seen the frame
        for (index = 0; index < nr_known_frames; index++) {
          if (frame == known_frames[index]) {
            found = true;
            break;
          }
        }

        // If we have previously seen the frame
        if (found) {
          // Check the filter type
          switch (config.getFilter(frame.getPgn())) {
            // Always transmit the frame
            case NO_FILTER:
              xQueueSend(fmsQueue, (void *)&frame, (TickType_t)0);
              break;
            // Only transmit if data inside the frame changed
            case ON_CHANGE:
              if (frame.compareData(known_frames[index]) == false) {
                xQueueSend(fmsQueue, (void *)&frame, (TickType_t)0);
                known_frames[index] = frame;
              }
              break;
            // Transmit with a fixed maximum update rate
            case MAX_INTERVAL:
              if ((known_frames[index].getTimeSinceLastUpdate() + config.getInterval(frame.getPgn())) <
                  xTaskGetTickCount()) {
                xQueueSend(fmsQueue, (void *)&frame, (TickType_t)0);
                known_frames[index] = frame;
              }
              break;
            default:
              break;
          }

        } else {
          // New frame found
          xQueueSend(fmsQueue, (void *)&frame, (TickType_t)0);
          nr_known_frames++;
          known_frames[index] = frame;
        }
      }
    }
  }
}

void print_fms_frame(Fms &frame) {
  char dataBuffer[17];
  for (int i = 0; i < 8; i++) {
    sprintf(dataBuffer + (2 * i), "%02X", frame.getData()[i]);
  }
  dataBuffer[16] = 0;
  USBSerial.printf("%d: %04X: %s\n", xTaskGetTickCount(), frame.getPgn(), dataBuffer);
}
