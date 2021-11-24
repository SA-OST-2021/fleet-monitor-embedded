#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#define MAX_FILE_SIZE     6*1024

enum FilterType {NO_FILTER, ON_CHANGE, MAX_INTERVAL};

class ConfigParser
{
  public:
    ConfigParser(void);
    bool loadFile(const char* path);
    bool loadString(const String& data);
    bool saveFile(const char* path, const String& data);
    const char* getName(const char* pgn);
    FilterType getFilter(const char* pgn);
    int32_t getInterval(const char* pgn);
    bool isEnabled(const char* pgn);
    bool sendFrameName(void);

  private:
    StaticJsonDocument<MAX_FILE_SIZE> doc;
};