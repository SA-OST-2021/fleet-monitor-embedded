#include "system_parser.h"
#include "utils.h"
#include "USB.h"

extern USBCDC USBSerial;
extern FatFileSystem fatfs;

SystemParser::SystemParser(void) {}

bool SystemParser::loadFile(const char* path) {
  filePath = path;

  File file = fatfs.open(filePath, (oflag_t)FILE_WRITE);
  file.println("Hello World!");
  file.close();

  file = fatfs.open(filePath);

  if (!file) {
    USBSerial.println("open file failed");
    return false;
  }

  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    file.close();
    USBSerial.printf("Failed to read file, using default configuration: %d\n", error);
    return false;
  }

  file.close();
  return true;
}

const char* SystemParser::getSsid(void) {
  if (doc.containsKey("ssid")) {
    return doc["ssid"].as<const char*>();
  }
  return "";
}

const char* SystemParser::getPassword(void) {
  if (doc.containsKey("password")) {
    return doc["password"].as<const char*>();
  }
  return "";
}

const char* SystemParser::getHostIp(void) {
  if (doc.containsKey("host_ip")) {
    return doc["host_ip"].as<const char*>();
  }
  return "";
}

int16_t SystemParser::getHostPort(void) {
  if (doc.containsKey("host_port")) {
    return doc["host_port"].as<int>();
  }
  return -1;
}

ConnectionType SystemParser::getConnectionType(void) {
  if (doc.containsKey("connection")) {
    if (strcmp(doc["connection"].as<const char*>(), "auto") == 0) return AUTO;
    if (strcmp(doc["connection"].as<const char*>(), "wlan") == 0) return WLAN;
    if (strcmp(doc["connection"].as<const char*>(), "lan") == 0) return LAN;
  }
  return (ConnectionType)-1;
}

ConfigMode SystemParser::getConfigMode(void) {
  if (doc.containsKey("config")) {
    if (strcmp(doc["config"].as<const char*>(), "remote") == 0) return REMOTE;
    if (strcmp(doc["config"].as<const char*>(), "local") == 0) return LOCAL;
  }
  return (ConfigMode)-1;
}

bool SystemParser::getBootloaderMode(bool clearFlag) {
  bool bootloader = false;
  if (doc.containsKey("bootloader")) {
    bootloader = doc["bootloader"].as<bool>();
    USBSerial.printf("check flag: %d\n", clearFlag);
    if (clearFlag) {
      doc["bootloader"] = false;
      USBSerial.println("Going to save file");
      saveFile("test.json");
    }
  }
  return bootloader;
}

bool SystemParser::saveFile(const char* path) {
  if (path != NULL) {
    filePath = path;
  }
  File file = fatfs.open(filePath, (oflag_t)FILE_WRITE);
  if (!file) {
    USBSerial.println("open file failed");
    return false;
  }
  USBSerial.println("File opened");
  if (serializeJson(doc, file) == 0) {
    file.close();
    USBSerial.println("Failed to write to file");
    return false;
  }
  USBSerial.println("close file");
  file.close();
  return true;
}