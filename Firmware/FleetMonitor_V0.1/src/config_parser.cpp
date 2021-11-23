#include "config_parser.h"
#include "SdFat.h"
#include "USB.h"

extern USBCDC USBSerial;

ConfigParser::ConfigParser(void)
{
  //doc = DynamicJsonDocument(4096);
}

bool ConfigParser::loadFile(const char* path)
{
  File file;
  if(!file.open(path))
  {
    USBSerial.println("open root failed");
    return false;
  }

  DeserializationError error = deserializeJson(doc, file);
  if (error)
  {
    USBSerial.println("Failed to read file, using default configuration");
    return false;
  }

  

  return true;
}

bool ConfigParser::loadString(const String& data)
{

}

bool ConfigParser::saveFile(const char* path, const String& data)
{

}

String ConfigParser::getName(const char* pgn)
{

}

FilterType ConfigParser::getFilter(const char* pgn)
{

}

int32_t ConfigParser::getInterval(const char* pgn)
{

}

bool ConfigParser::isEnabled(const char* pgn)
{

}