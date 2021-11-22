// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <ArduinoJson.h>


#include <USB.h>

#include "Fms.h"

#include "task_frame_handler.h"
#include "task_can.h"
#include "task_networking.h"

extern USBCDC USBSerial;

DynamicJsonDocument doc(2048);

bool send_data_to_client();

void task_frame_handler(void *pvParameter) {
  vTaskDelay(1000);
  while(1){
      Fms frame;
      if( xQueueReceive( fmsQueue, &(frame),portMAX_DELAY ) == pdPASS ){
            StaticJsonDocument<128> entry;
            entry["ts"] = xTaskGetTickCount();
            char pgn[5];
            snprintf(pgn, 5, "%0X",frame.getPgn());
            pgn[4] = 0;
            entry["pgn"] = pgn;
            char dataBuffer[17];
            for(int i = 0; i < 8; i++){
                sprintf(dataBuffer+(2*i), "%02X", frame.getData()[i]);
            }
            dataBuffer[16] = 0;
            entry["data"]=dataBuffer;
            doc.add(entry);
            entry.clear();
            USBSerial.println(doc.size());
            if(doc.size() >= 16){
              if(network_connected){
                send_data_to_client();
                serializeJson(doc, USBSerial);
              }
              doc.clear();
            }            
      }
  }
}

bool send_data_to_client() {

  String contentType = "application/x-www-form-urlencoded";
  String postData;
  serializeJson(doc, postData);
  int httpResponseCode = client.post("/", contentType, postData);

  USBSerial.print("Response Code: ");
  USBSerial.println(httpResponseCode);
  return true;
}

