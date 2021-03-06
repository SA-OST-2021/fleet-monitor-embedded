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

#include <Arduino.h>

#include "task_networking.h"
#include "task_hmi.h"

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "utils.h"
#include <Ethernet.h>
#include <USB.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>

#include "utils.h"

EthernetClient ethclient;
WiFiClient wificlient;

extern USBCDC Serial;

#define TASK_NETWORKING_FREQ 500
#define CONFIG_LOAD_TIMEOUT  5000

// Ethernet Definitions
byte mac[] = {0x2C, 0xF7, 0xF1, 0x08, 0x39, 0x7E};

HTTPClient client;

ConfigParser config("config.json");

// Wifi Definitions
char ssid[] = "fleet-monitor";  //  your network SSID (name)
char pass[] = "password";       // your network password

bool ethernet_connected = false;
bool wifi_connected = false;
bool network_connected = false;

bool config_loaded = false;
bool send_frame_names = false;

void check_connection_status();
void eth_init();
void wifi_init();

bool get_config_from_server();

void task_networking(void *pvParameter) {
  eth_init();
  wifi_init();

  const TickType_t task_freq = TASK_NETWORKING_FREQ;
  TickType_t task_last_tick = xTaskGetTickCount();
  TickType_t config_load_timeout = xTaskGetTickCount();
  while (1) {
    check_connection_status();
    if (!network_connected) {
      hmi_setLed(led_t{.type = LED_STATUS, .mode = LED_BREATH, .color = BLUE});
    } else if (utils_getSettings().configMode == LOCAL && !config_loaded) {
      if (config.loadFile("config.json")) {
        Serial.println("Config loading was successful.");
        config_loaded = true;
        hmi_setLed(led_t{.type = LED_STATUS, .mode = LED_ON, .color = ethernet_connected ? GREEN : YELLOW});
      } else {
        Serial.println("Local config loading failed.");
        hmi_setLed(led_t{.type = LED_STATUS, .mode = LED_ON, .color = RED});
      }
    } else if (!config_loaded) {
      if (config_load_timeout + CONFIG_LOAD_TIMEOUT < xTaskGetTickCount()) {
        Serial.println("Loading config from server");
        if (get_config_from_server()) {
          Serial.println("Config loading successful!");
          config_loaded = true;
          hmi_setLed(led_t{.type = LED_STATUS, .mode = LED_ON, .color = ethernet_connected ? GREEN : YELLOW});
        } else {
          Serial.println("Config loading failed, trying again..");
          hmi_setLed(led_t{.type = LED_STATUS, .mode = LED_ON, .color = RED});
          config_load_timeout = xTaskGetTickCount();
        }
      }
    }

    vTaskDelayUntil(&task_last_tick, task_freq);
  }
}

/**
 * @brief Initialization of Ethernet interface
 *
 */
void eth_init() {
  Ethernet.init(34);
  Ethernet.begin(mac, 1000, 1000);
}

/**
 * @brief Initialization of WiFi interface
 *
 */
void wifi_init() { WiFi.begin(utils_getSettings().ssid, utils_getSettings().password); }

/**
 * @brief Check and update the connection status of networking interface
 *
 */
void check_connection_status() {
  static EthernetHardwareStatus eth_harware_status = EthernetNoHardware;
  static EthernetLinkStatus eth_link_status = Unknown;
  static IPAddress eth_ip(0, 0, 0, 0);

  static wl_status_t wifi_status = WL_NO_SHIELD;
  static IPAddress wifi_ip(0, 0, 0, 0);

  static TickType_t wifi_disconnect_time = xTaskGetTickCount();

  static bool ethernet_already_connected = false;
  static bool wifi_already_connected = false;

  static int timeout = 0;

  /** Ethernet stuff **/
  IPAddress no_ip(0, 0, 0, 0);

  // Diagnostics Print, only on change
  /*if (eth_harware_status != Ethernet.hardwareStatus() || eth_link_status != Ethernet.linkStatus() ||
      eth_ip != Ethernet.localIP()) {
    eth_harware_status = Ethernet.hardwareStatus();
    eth_link_status = Ethernet.linkStatus();
    eth_ip = Ethernet.localIP();
    Serial.print("[ETH] Hardware Status: ");
    Serial.println(eth_harware_status == EthernetW5500 ? "UP" : "DOWN");
    Serial.print("[ETH] Link Status: ");
    Serial.println(eth_link_status == LinkON ? "UP" : "DOWN");
    Serial.print("[ETH] IP Adress: ");
    Serial.println(eth_ip);

    if (Ethernet.hardwareStatus() != EthernetW5500) {
      Serial.println("[ETH] No Hardware Found!");
    }

    if (Ethernet.localIP() == no_ip && Ethernet.linkStatus() == LinkON) {
      Serial.println("[ETH] Waiting for IP..");
    }
  }*/

  // Check for Hardware connection
  if (Ethernet.hardwareStatus() != EthernetW5500) {
    Ethernet.begin(mac, 1000, 1000);
  }
  // Check for Link Connection
  else if (Ethernet.linkStatus() != LinkON) {
    // Ethernet.setLocalIP(no_ip);
  }

  // Check both Link and IP
  if (Ethernet.localIP() == no_ip && Ethernet.linkStatus() == LinkON && (timeout + 10000) < xTaskGetTickCount()) {
    timeout = xTaskGetTickCount();
    Ethernet.begin(mac, 1000, 1000);
  }

  ethernet_connected = ((Ethernet.localIP() != no_ip) && (Ethernet.linkStatus() == LinkON));

  /** WiFi Stuff **/
  // Diagnostics Print, only on change
  /*if (wifi_status != WiFi.status() || wifi_ip != WiFi.localIP()) {
    wifi_status = WiFi.status();
    wifi_ip = WiFi.localIP();
    Serial.print("[WiFi] Status: ");
    Serial.println(wifi_status);
    Serial.print("[WiFi] IP Adress: ");
    Serial.println(wifi_ip);

    if (WiFi.status() == WL_NO_SHIELD) {
      Serial.println("[WiFi] Init failed, trying again..");
    } else if (WiFi.status() == WL_DISCONNECTED) {
      Serial.println("[WiFi] No connection");
    } else if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi] Waiting for Connection..");
    }
  }*/

  if (WiFi.status() != WL_CONNECTED) {
    if (WiFi.status() == WL_NO_SHIELD) {
      WiFi.begin(ssid, pass);
    } else {
      if ((wifi_disconnect_time + 60000) < xTaskGetTickCount()) {
        WiFi.begin(ssid, pass);
        wifi_disconnect_time = xTaskGetTickCount();
      }
    }
  }

  wifi_connected = ((WiFi.localIP() != no_ip) && (WiFi.status() == WL_CONNECTED));

  if (ethernet_connected == false) ethernet_already_connected = false;
  if (wifi_connected == false) wifi_already_connected = false;

  // Prioritize Ethernet over WIFI
  if (ethernet_connected && !ethernet_already_connected) {
    ethernet_already_connected = true;
    wifi_already_connected = false;
    client.begin(ethclient, "http://");
    Serial.println("Connected over Ethernet");
  } else if (wifi_connected && !ethernet_connected && !wifi_already_connected) {
    wifi_already_connected = true;
    client.begin(wificlient, "http://");
    Serial.println("Connected over WiFi");
  }

  network_connected = wifi_connected | ethernet_connected;
}

/**
 * @brief Get the config from server object
 *
 * @return true on success
 * @return false if the config was not received from the server or error in parsing
 */
bool get_config_from_server() {
  client.setURL(utils_getServerAddress() + "/config.json");

  int statusCode = client.GET();
  if (statusCode != 200) return false;
  Serial.print("Status code: ");
  Serial.println(statusCode);

  // TODO: investigate system lockup when saving file locally
  return config.loadString(client.getStream(), utils_getSettings().overwriteFile);
}
