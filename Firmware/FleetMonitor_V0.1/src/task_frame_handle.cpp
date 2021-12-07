// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <ArduinoJson.h>

#include <USB.h>

#include "Fms.h"
#include "utils.h"

#include "task_can.h"
#include "task_frame_handler.h"
#include "task_networking.h"
#include "task_hmi.h"

#include "ESP32Time.h"

extern USBCDC USBSerial;
ESP32Time rtc;

#define FRAME_SIZE    256
#define DOCUMENT_SIZE 256 * 32

#define CONFIG_LOAD_MAX_RETRIES 5

DynamicJsonDocument doc(DOCUMENT_SIZE);

bool send_data_to_client();

void task_frame_handler(void *pvParameter) {
  // Wait until we have network, config and can initialized

  while (!network_connected && !config_loaded && !can_connected) vTaskDelay(1000);

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

      if (config.sendFrameName()) {
        entry["name"] = config.getName(frame.getPgn());
      }

      doc.add(entry);
      entry.clear();
      if (doc.memoryUsage() + FRAME_SIZE >= DOCUMENT_SIZE) {
        if (network_connected) {
          if (send_data_to_client()) {
            hmi_setLed(led_t{.type = LED_STATUS, .mode = LED_ON, .color = ethernet_connected ? GREEN : YELLOW});
          } else {
            hmi_setLed(led_t{.type = LED_STATUS, .mode = LED_ON, .color = MAGENTA});
          }
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
  client.setURL(utils_getServerAddress());
  status = client.POST(postData);

  if (status != 200) {
    USBSerial.print("Request timed out: ");
    USBSerial.println(status);
    return false;
  }
  // TODO update config on change
  // Check if we already have the time

  deserializeJson(response, client.getStream());
  if (response["ConfigReload"].as<bool>() == true && utils_getSettings().configMode == REMOTE) {
    config_loaded = false;
  }
  if (time_set == true) return true;
  int d, m, y, H, M, S;
  int matches = sscanf(response["Date"].as<const char *>(), "%d %d %d %d:%d:%d", &y, &m, &d, &H, &M, &S);
  if (matches == 6) {
    rtc.setTime(S, M, H, d, m, y);
    time_set = true;
  }

  return true;
}