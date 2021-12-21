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

#define MAX_CONFIG_FILE_SIZE 6 * 1024

enum FilterType { NO_FILTER, ON_CHANGE, MAX_INTERVAL };

class ConfigParser {
 public:
  ConfigParser(const char* path);
  bool loadFile(const char* path);
  bool loadString(Client& client, bool save = true);
  const char* getName(uint16_t pgn);
  FilterType getFilter(uint16_t pgn);
  int32_t getInterval(uint16_t pgn);
  bool isEnabled(uint16_t pgn);
  bool sendFrameName(void);

 private:
  StaticJsonDocument<MAX_CONFIG_FILE_SIZE> doc;
  const char* filePath;

  bool saveFile(const char* path = NULL);
};