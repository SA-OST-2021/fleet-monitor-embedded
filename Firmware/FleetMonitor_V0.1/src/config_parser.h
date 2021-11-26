#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#define MAX_CONFIG_FILE_SIZE     6*1024

enum FilterType {NO_FILTER, ON_CHANGE, MAX_INTERVAL};

class ConfigParser
{
  public:
    ConfigParser(void);
    bool loadFile(const char* path);
    bool loadString(Client& client, bool saveFile = true);
    bool saveFile(const char* path, const String& data);
    const char* getName(uint16_t pgn);
    FilterType getFilter(uint16_t pgn);
    int32_t getInterval(uint16_t pgn);
    bool isEnabled(uint16_t pgn);
    bool sendFrameName(void);

  private:
    StaticJsonDocument<MAX_CONFIG_FILE_SIZE> doc;
    const char* filePath;
};