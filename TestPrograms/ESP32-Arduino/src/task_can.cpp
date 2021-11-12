// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/twai.h"
#include <USB.h>

#include "task_can.h"

extern USBCDC USBSerial;

static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_43, GPIO_NUM_44, TWAI_MODE_LISTEN_ONLY);
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

void task_can(void *pvParameter)
{
  USBSerial.println(twai_driver_install(&g_config, &t_config, &f_config));
  twai_start();
  USBSerial.println("CAN Initialization Done");
  while(1){
    twai_message_t rx_msg;
    twai_receive(&rx_msg, portMAX_DELAY);
    USBSerial.print(xTaskGetTickCount());
    USBSerial.print(": ");
    USBSerial.print(rx_msg.identifier, HEX);
    USBSerial.print(": ");
    for (int i = 0; i < 8; i++) {
        USBSerial.print(rx_msg.data[i], HEX);
        USBSerial.print(" ");
    }
    USBSerial.println();
  }
}