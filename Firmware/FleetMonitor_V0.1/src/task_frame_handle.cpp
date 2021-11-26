// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <ArduinoJson.h>

#include <USB.h>

#include "Fms.h"

#include "task_can.h"
#include "task_frame_handler.h"
#include "task_networking.h"

#include "ESP32Time.h"

extern USBCDC USBSerial;
ESP32Time rtc;

#define NR_OF_FRAMES 32
#define DOCUMENT_SIZE 256 * NR_OF_FRAMES

#define CONFIG_LOAD_MAX_RETRIES 5

DynamicJsonDocument doc(DOCUMENT_SIZE);

String server = "http://10.3.141.1:8080";

typedef enum {
  FILE_UNDEFINED,
  FILE_CONFIG,
  FILE_SYSTEM,
} file_type_t;

bool send_data_to_client();
bool get_file_from_server(file_type_t file_type);

void task_frame_handler(void *pvParameter) {

  while (!network_connected)
    vTaskDelay(200);

  int timeout = 0;
  USBSerial.println("Loading config from server");
  while (!get_file_from_server(FILE_CONFIG) &&
         timeout < CONFIG_LOAD_MAX_RETRIES) {
    USBSerial.println("Config loading failed, trying again..");
    vTaskDelay(10000);
  }
  if (timeout == CONFIG_LOAD_MAX_RETRIES)
    USBSerial.println("Loading failed");

  while (1) {
    Fms frame;
    if (xQueueReceive(fmsQueue, &(frame), portMAX_DELAY) == pdPASS) {
      StaticJsonDocument<256> entry;
      String epoch = "";
      epoch += String(rtc.getEpoch()) + "." + String(rtc.getMillis());
      entry["ts"] = epoch;
      char pgn[5];
      snprintf(pgn, 5, "%0X", frame.getPgn());
      pgn[4] = 0;
      entry["pgn"] = pgn;
      char dataBuffer[17];
      for (int i = 0; i < 8; i++) {
        sprintf(dataBuffer + (2 * i), "%02X", frame.getData()[i]);
      }
      dataBuffer[16] = 0;
      entry["data"] = dataBuffer;
      doc.add(entry);
      entry.clear();
      // USBSerial.println(doc.size());
      if (doc.size() >= NR_OF_FRAMES) {
        if (network_connected) {
          USBSerial.println("Data sent!");
          send_data_to_client();
        }
        doc.clear();
      }
    }
  }
}

bool send_data_to_client() {
  static bool time_set = false;
  int status;
  String postData;
  DynamicJsonDocument response(256);
  serializeJson(doc, postData);
  client.setURL(server);
  status = client.POST(postData);

  USBSerial.print("Response Code: ");
  USBSerial.println(status);
  if (status != 200)
    return false;

  // Check if we already have the time
  if (time_set == true)
    return true;

  deserializeJson(response, client.getStream());
  int d, m, y, H, M, S;
  int matches = sscanf(response["Date"].as<const char *>(), "%d %d %d %d:%d:%d",
                       &d, &m, &y, &H, &M, &S);
  if (matches == 6) {
    rtc.setTime(S, M, H, d, m, y);
    time_set = true;
  }

  return true;
}

bool get_file_from_server(file_type_t file_type) {
  if (file_type == FILE_CONFIG) {
    client.setURL("http://10.3.141.1:8080/config.json");
  } else if (file_type == FILE_SYSTEM) {
    client.setURL("http://10.3.141.1:8080/system.json");
  } else {
    return false;
  }
  int statusCode = client.GET();
  if (statusCode != 200)
    return false;
  USBSerial.print("Status code: ");
  USBSerial.println(statusCode);
  // client.getStream()

  USBSerial.println(client.getString());

  return true;
}
