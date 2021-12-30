/*
 * Fleet-Monitor Software
 * Copyright (C) 2021 Institute of Networked Solutions OST
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

extern USBCDC Serial;
ESP32Time rtc;

#define FRAME_SIZE    256
#define DOCUMENT_SIZE 256 * 32

#define CONFIG_LOAD_MAX_RETRIES 5

DynamicJsonDocument doc(DOCUMENT_SIZE);

bool send_data_to_client();
int start_time = 0;

void task_frame_handler(void *pvParameter) {
  // Wait until we have network, config and can initialized

  while (!network_connected && !config_loaded && !can_connected) vTaskDelay(1000);

  while (1) {
    Fms frame;
    if (xQueueReceive(fmsQueue, &(frame), portMAX_DELAY) == pdPASS) {
      StaticJsonDocument<256> entry;
      String epoch = "";
	  float t = (float)start_time + (float)frame.getLastUpdate()/1000.0f;
      epoch += String(t, 3);
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

/**
 * @brief Transmit data to the server
 *
 * @return true is returned on success
 * @return false is returned if the request timed out or bad status code
 */
bool send_data_to_client() {
  static bool time_set = false;
  int status;
  String postData;
  DynamicJsonDocument response(256);
  serializeJson(doc, postData);
  client.setURL(utils_getServerAddress());
  status = client.POST(postData);

  if (status != 200) {
    Serial.print("Request timed out: ");
    Serial.println(status);
    return false;
  }

  // Check the response for the configReload flag
  deserializeJson(response, client.getStream());
  if (response["ConfigReload"].as<bool>() == true && utils_getSettings().configMode == REMOTE) {
    config_loaded = false;
  }

  // Synchronize time if not set already
  if (time_set == true) return true;
  int d, m, y, H, M, S;
  int matches = sscanf(response["Date"].as<const char *>(), "%d %d %d %d:%d:%d", &y, &m, &d, &H, &M, &S);
  if (matches == 6) {
    rtc.setTime(S, M, H, d, m, y);
    time_set = true;
	start_time = rtc.getEpoch();
  }

  return true;
}