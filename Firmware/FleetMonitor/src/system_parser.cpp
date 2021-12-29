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

#include "system_parser.h"
#include "utils.h"
#include "USB.h"

SystemParser::SystemParser(void) {}

/**
 * @brief Load a system configuration file
 *
 * @param path is the path of the file
 * @return true on success
 * @return false on error
 */
bool SystemParser::loadFile(const char* path) {
  filePath = path;
  File file = fatfs.open(filePath);

  if (!file) {
    Serial.println("open file failed");
    return false;
  }

  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    file.close();
    Serial.printf("Failed to read file, using default configuration: %d\n", error);
    return false;
  }

  file.close();
  return true;
}

/**
 * @brief Get the name of the access point
 *
 * @return const char* with the name
 */
const char* SystemParser::getSsid(void) {
  if (doc.containsKey("ssid")) {
    return doc["ssid"].as<const char*>();
  }
  return "";
}

/**
 * @brief get the password of the access point
 *
 * @param clearEntry is used to define if the password should be deleted from the file
 * @return const char* with the password
 */
const char* SystemParser::getPassword(bool clearEntry) {
  if (doc.containsKey("password")) {
    const char* password = doc["password"].as<const char*>();
    if (clearEntry) {
      doc["password"] = "";
      saveFile();
    }
    return password;
  }
  return "";
}

/**
 * @brief Get the host ip address
 *
 * @return const char* with the ip address
 */
const char* SystemParser::getHostIp(void) {
  if (doc.containsKey("host_ip")) {
    return doc["host_ip"].as<const char*>();
  }
  return "";
}

/**
 * @brief Get the host port
 *
 * @return int16_t with the port
 */
int16_t SystemParser::getHostPort(void) {
  if (doc.containsKey("host_port")) {
    return doc["host_port"].as<int>();
  }
  return -1;
}

/**
 * @brief Get the how to connect with the netword
 *
 * @return ConnectionType
 */
ConnectionType SystemParser::getConnectionType(void) {
  if (doc.containsKey("connection")) {
    if (strcmp(doc["connection"].as<const char*>(), "auto") == 0) return AUTO;
    if (strcmp(doc["connection"].as<const char*>(), "wlan") == 0) return WLAN;
    if (strcmp(doc["connection"].as<const char*>(), "lan") == 0) return LAN;
  }
  return (ConnectionType)-1;
}

/**
 * @brief Get where the config should be read from
 *
 * @return ConfigMode
 */
ConfigMode SystemParser::getConfigMode(void) {
  if (doc.containsKey("config")) {
    if (strcmp(doc["config"].as<const char*>(), "remote") == 0) return REMOTE;
    if (strcmp(doc["config"].as<const char*>(), "local") == 0) return LOCAL;
  }
  return (ConfigMode)-1;
}

/**
 * @brief Get if the local configuration should be overwritten with the remote one
 *
 * @return true
 * @return false
 */
bool SystemParser::getOverwriteState(void) {
  if (doc.containsKey("overwrite_file")) {
    return doc["overwrite_file"].as<bool>();
  }
  return false;
}

/**
 * @brief Get if we need to boot into DFU mode
 *
 * @param clearFlag is used to define if the flag should be reset from the file
 * @return true
 * @return false
 */
bool SystemParser::getBootloaderMode(bool clearFlag) {
  bool bootloader = false;
  if (doc.containsKey("bootloader")) {
    bootloader = doc["bootloader"].as<bool>();
    if (clearFlag) {
      doc["bootloader"] = false;
      saveFile();
    }
  }
  return bootloader;
}

/**
 * @brief Save the current loaded system config as a file
 *
 * @param path to location
 * @return true on success
 * @return false on error
 */
bool SystemParser::saveFile(const char* path) {
  if (path != NULL) {
    filePath = path;
  }
  if (fatfs.exists(filePath)) {
    if (!fatfs.remove(filePath)) {
      Serial.println("Could not remove file");
      return false;
    }
  }
  File file = fatfs.open(filePath, FILE_WRITE);
  if (!file) {
    Serial.println("open file failed");
    return false;
  }
  if (serializeJson(doc, file) == 0) {
    file.close();
    Serial.println("Failed to write to file");
    return false;
  }
  file.close();
  return true;
}