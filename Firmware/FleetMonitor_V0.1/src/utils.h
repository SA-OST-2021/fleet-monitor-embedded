#pragma once

#include <Arduino.h>
#include "USB.h"
#include "SdFat.h"
#include "system_parser.h"

#define CLEAR_TERMINAL "\033[2J\033[1;1H"

extern USBCDC USBSerial;
extern FatFileSystem fatfs;
extern SystemParser systemParser;

bool utils_init(const char* labelName, bool forceFormat = false);
bool utils_systemConfig(const char* fileName);
bool utils_format(const char* labelName);
bool utils_isUpdated(bool clearFlag = true);
bool utils_updateEfuse(void);
