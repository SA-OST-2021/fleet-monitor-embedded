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
  const char* getPassword(void);
  const char* getHostIp(void);
  int16_t getHostPort(void);
  ConnectionType getConnectionType(void);
  ConfigMode getConfigMode(void);
  bool getBootloaderMode(bool clearFlag = true);

 private:
  StaticJsonDocument<MAX_SYSTEM_FILE_SIZE> doc;
  const char* filePath;

  bool saveFile(const char* path = NULL);
};