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

#include "config_parser.h"
#include "utils.h"
#include "USB.h"

ConfigParser::ConfigParser(const char* path) : filePath(path) {}

/**
 * @brief Load a FMS configuration file
 *
 * @param path is the path of the file
 * @return true on success
 * @return false on error
 */
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

/**
 * @brief Load a FMS configuration from a string
 *
 * @param client is the stream of the data
 * @param save is true when local config file should be overwritten
 * @return true on success
 * @return false on error
 */
bool ConfigParser::loadString(Client& client, bool save) {
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    USBSerial.printf("Failed to read file, using default configuration: %d\n", error);
    return false;
  }
  if (save) {
    USBSerial.println("Overwrite local config file");
    return saveFile();
  }
  return true;
}

/**
 * @brief Get the name of the PGN
 *
 * @param pgn is the PGN to get the name from
 * @return const char* of PGN
 */
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

/**
 * @brief Get the filter method of the PGN
 *
 * @param pgn is the PGN to get the filter from
 * @return FilterType
 */
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

/**
 * @brief Get the package interval
 *
 * @param pgn is the PGN to get the interval from
 * @return int32_t in ms
 */
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

/**
 * @brief Get if a PGN is enabled
 *
 * @param pgn is the PGN to check
 * @return true if enabled
 * @return false if diabled
 */
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

/**
 * @brief Get if frame names need to be transmitted
 *
 * @return true
 * @return false
 */
bool ConfigParser::sendFrameName(void) {
  if (doc.containsKey("framename")) {
    return doc["framename"].as<bool>();
  }
  return false;
}

/**
 * @brief Save the current loaded FMS config
 *
 * @param path is the path to save it to
 * @return true on success
 * @return false on error
 */
bool ConfigParser::saveFile(const char* path) {
  if (path != NULL) {
    filePath = path;
  }
  if (fatfs.exists(filePath)) {
    if (!fatfs.remove(filePath)) {
      USBSerial.println("Could not remove file");
      return false;
    }
  }
  File file = fatfs.open(filePath, FILE_WRITE);
  if (!file) {
    USBSerial.println("open file failed");
    return false;
  }
  if (serializeJson(doc, file) == 0) {
    file.close();
    USBSerial.println("Failed to write to file");
    return false;
  }
  file.close();
  return true;
}
