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

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#define MAX_SYSTEM_FILE_SIZE 1 * 1024

enum ConnectionType { AUTO, LAN, WLAN };
enum ConfigMode { LOCAL, REMOTE };

class SystemParser {
 public:
  SystemParser(void);
  bool loadFile(const char* path);
  const char* getSsid(void);
  const char* getPassword(bool clearEntry = true);
  const char* getHostIp(void);
  int16_t getHostPort(void);
  ConnectionType getConnectionType(void);
  ConfigMode getConfigMode(void);
  bool getOverwriteState(void);
  bool getBootloaderMode(bool clearFlag = true);

 private:
  StaticJsonDocument<MAX_SYSTEM_FILE_SIZE> doc;
  const char* filePath;

  bool saveFile(const char* path = NULL);
};