#include "config_parser.h"
#include "SdFat.h"
#include "USB.h"

extern USBCDC USBSerial;

ConfigParser::ConfigParser(void)
{

}

bool ConfigParser::loadFile(const char* path)
{
  File file;
  if(!file.open(path))
  {
    USBSerial.println("open file failed");
    return false;
  }

  DeserializationError error = deserializeJson(doc, file);
  if (error)
  {
    file.close();
    USBSerial.printf("Failed to read file, using default configuration: %d\n", error);
    return false;
  }

  file.close();
  return true;
}

bool ConfigParser::loadString(const String& data)
{

}

bool ConfigParser::saveFile(const char* path, const String& data)
{

}

const char* ConfigParser::getName(const char* pgn)
{
  for (JsonVariant value : doc["frames"].as<JsonArray>())
  {
    if(strncmp(value["pgn"].as<const char*>(), pgn, 4) == 0)
    {
      return value["name"].as<const char*>();
    }
  }
  return "";
}

FilterType ConfigParser::getFilter(const char* pgn)
{
  for (JsonVariant value : doc["frames"].as<JsonArray>())
  {
    if(strncmp(value["pgn"].as<const char*>(), pgn, 4) == 0)
    {
      if(strcmp(value["filter"].as<const char*>(), "nofilter") == 0) return NO_FILTER;
      if(strcmp(value["filter"].as<const char*>(), "change") == 0) return ON_CHANGE;
      if(strcmp(value["filter"].as<const char*>(), "interval") == 0) return MAX_INTERVAL;      
      return (FilterType) -1;
    }
  }
  return (FilterType) -1;
}

int32_t ConfigParser::getInterval(const char* pgn)
{
  for (JsonVariant value : doc["frames"].as<JsonArray>())
  {
    if(strncmp(value["pgn"].as<const char*>(), pgn, 4) == 0)
    {
      if(value.containsKey("time"))
      {
        return value["time"].as<int>();
      }
    }
  }
  return -1;
}

bool ConfigParser::isEnabled(const char* pgn)
{
  for (JsonVariant value : doc["frames"].as<JsonArray>())
  {
    if(strncmp(value["pgn"].as<const char*>(), pgn, 4) == 0)
    {
      return value["active"].as<bool>();
    }
  }
  return false;
}