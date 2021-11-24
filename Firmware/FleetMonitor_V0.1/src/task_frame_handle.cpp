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

#define DOCUMENT_SIZE 8192
#define NR_OF_FRAMES 32

DynamicJsonDocument doc(DOCUMENT_SIZE);

typedef enum{
  FILE_UNDEFINED,
  FILE_CONFIG,
  FILE_SYSTEM,
} file_type_t;


bool send_data_to_client();
bool get_file_from_server(file_type_t file_type);

void task_frame_handler(void *pvParameter){
  while(1){
      vTaskDelay(10000);
    get_file_from_server(FILE_CONFIG);
  }
  

  while(1){
      Fms frame;
      if( xQueueReceive( fmsQueue, &(frame),portMAX_DELAY ) == pdPASS ){
        StaticJsonDocument<256> entry;
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
        //USBSerial.println(doc.size());
        if(doc.size() >= NR_OF_FRAMES){
          if(network_connected){
            USBSerial.println("Data sent!");
            send_data_to_client();
          }
          doc.clear();
        }            
      }
  }
}

bool send_data_to_client(){
  int status;
  String postData;
  serializeJson(doc, postData);
  client.setURL("http://10.3.141.1:8080");
  status = client.POST(postData);
  String response = client.getString();

  USBSerial.print("Response Code: ");
  USBSerial.println(status);
  USBSerial.print("Response: ");
  USBSerial.println(response);

  
  return true;
}

bool get_file_from_server(file_type_t file_type){
  if(file_type == FILE_CONFIG){
    client.setURL("http://10.3.141.1:8080/config.json");
  } else if(file_type == FILE_SYSTEM){
    client.setURL("http://10.3.141.1:8080/system.json");
  } else {
    return false;
  }
  int statusCode = client.GET();
  
  //vTaskDelay(250);
  USBSerial.print("Status code: ");
  USBSerial.println(statusCode);

  USBSerial.println(client.getString());

  return true;
}

