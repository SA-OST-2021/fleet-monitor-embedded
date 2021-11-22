// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/twai.h"
#include <USB.h>

#include "Fms.h"

#include "task_can.h"

extern USBCDC USBSerial;

static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_43, GPIO_NUM_44, TWAI_MODE_NORMAL);
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

QueueHandle_t fmsQueue;

void print_fms_frame(Fms& frame);

void task_can(void *pvParameter)
{
  USBSerial.println(twai_driver_install(&g_config, &t_config, &f_config));
  twai_start();
  USBSerial.println("CAN Initialization Done");

  Fms known_frames[100];
  uint32_t nr_known_frames = 0;
  fmsQueue = xQueueCreate( 10, sizeof(Fms));

  while(1){
    twai_message_t rx_frame;
    twai_receive(&rx_frame, portMAX_DELAY);
    Fms frame(rx_frame);
    int index;
    bool found = false;

    for(index=0; index < nr_known_frames; index++){
      if(frame == known_frames[index]){
        found = true;
        break;
      }
    }

    if(found) {
      // Check if data changed
      
      if(frame.compareData(known_frames[index]) == false){
        xQueueSend(fmsQueue,( void * ) &frame, ( TickType_t ) 0);
        //USBSerial.printf("Time since last update: %d\n", known_frames[index].getTimeSinceLastUpdate());
        //print_fms_frame(frame);
        known_frames[index] = frame;
      }
    } else {
      nr_known_frames++;
      xQueueSend(fmsQueue,( void * ) &frame, ( TickType_t ) 0);
      USBSerial.println("New frame!");
      known_frames[index] = frame;
    }
    
  }
}

void print_fms_frame(Fms& frame){
    
    char dataBuffer[17];
    for(int i = 0; i < 8; i++){
        sprintf(dataBuffer+(2*i), "%02X", frame.getData()[i]);
    }
    dataBuffer[16] = 0;
    USBSerial.printf("%d: %04X: %s\n",xTaskGetTickCount(), frame.getPgn(), dataBuffer);
}