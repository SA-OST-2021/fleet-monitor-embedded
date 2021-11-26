#include "config_parser.h"
#include "utils.h"
#include "USB.h"

extern USBCDC USBSerial;
extern FatFileSystem fatfs;

ConfigParser::ConfigParser(void) {}

bool ConfigParser::loadFile(const char* path) {
  filePath = path;
  File file = fatfs.open(filePath);

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

bool ConfigParser::loadString(Client& client, bool saveFile) {
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    USBSerial.printf("Failed to read file, using default configuration: %d\n", error);
    return false;
  }
  return true;
}

bool ConfigParser::saveFile(const char* path, const String& data) { return true; }

const char* ConfigParser::getName(uint16_t pgn) {
  char pgnStr[5];
  snprintf(pgnStr, sizeof(pgnStr), "%04X", pgn);
  for (JsonVariant value : doc["frames"].as<JsonArray>()) {
    if (strncmp(value["pgn"].as<const char*>(), pgnStr, 4) == 0) {
      return value["name"].as<const char*>();
    }
  }
  return "Unknown frame";
}

FilterType ConfigParser::getFilter(uint16_t pgn) {
  char pgnStr[5];
  snprintf(pgnStr, sizeof(pgnStr), "%04X", pgn);
  for (JsonVariant value : doc["frames"].as<JsonArray>()) {
    if (strncmp(value["pgn"].as<const char*>(), pgnStr, 4) == 0) {
      if (strcmp(value["filter"].as<const char*>(), "nofilter") == 0) return NO_FILTER;
      if (strcmp(value["filter"].as<const char*>(), "change") == 0) return ON_CHANGE;
      if (strcmp(value["filter"].as<const char*>(), "interval") == 0) return MAX_INTERVAL;
      return (FilterType)-1;
    }
  }
  return (FilterType)-1;
}

int32_t ConfigParser::getInterval(uint16_t pgn) {
  char pgnStr[5];
  snprintf(pgnStr, sizeof(pgnStr), "%04X", pgn);
  for (JsonVariant value : doc["frames"].as<JsonArray>()) {
    if (strncmp(value["pgn"].as<const char*>(), pgnStr, 4) == 0) {
      if (value.containsKey("time")) {
        return value["time"].as<int>();
      }
    }
  }
  return -1;
}

bool ConfigParser::isEnabled(uint16_t pgn) {
  char pgnStr[5];
  snprintf(pgnStr, sizeof(pgnStr), "%04X", pgn);
  for (JsonVariant value : doc["frames"].as<JsonArray>()) {
    if (strncmp(value["pgn"].as<const char*>(), pgnStr, 4) == 0) {
      return value["active"].as<bool>();
    }
  }
  if (doc.containsKey("unknownframes")) {
    return doc["unknownframes"].as<bool>();
  }
  return false;
}

bool ConfigParser::sendFrameName(void) {
  if (doc.containsKey("framename")) {
    return doc["framename"].as<bool>();
  }
  return false;
}