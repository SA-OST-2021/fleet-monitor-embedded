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
#include "USB.h"
#include "SdFat.h"
#include "system_parser.h"

#define CLEAR_TERMINAL "\033[2J\033[1;1H"

extern USBCDC Serial;
extern FatFileSystem fatfs;
extern SystemParser systemParser;

struct Settings {
  const char* ssid;
  const char* password;
  const char* hostIp;
  int16_t hostPort;
  ConnectionType connectionType;
  ConfigMode configMode;
  bool overwriteFile;
};

bool utils_init(const char* labelName, bool forceFormat = false);
bool utils_systemConfig(const char* fileName);
bool utils_startMsc(void);
bool utils_format(const char* labelName);
bool utils_isUpdated(bool clearFlag = true);
bool utils_updateEfuse(void);
bool utils_usbState(void);
Settings& utils_getSettings(void);
String& utils_getServerAddress(void);