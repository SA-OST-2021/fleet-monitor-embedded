#pragma once

#include <Arduino.h>
#include "USB.h"
#include "SdFat.h"

extern USBCDC USBSerial;
extern FatFileSystem fatfs;

bool utils_init(const char* labelName, bool forceFormat = false);
bool utils_format(const char* labelName);
bool utils_isUpdated(bool clearFlag = true);
