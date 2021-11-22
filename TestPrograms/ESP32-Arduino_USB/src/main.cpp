#include <Arduino.h>
#include "USB.h"

#define LED 5

USBCDC USBSerial;

void setup()
{
  pinMode(LED, OUTPUT);
  
  USB.enableDFU();
  USB.productName("ESP32S2-USB");
  USB.begin();
  USBSerial.begin();
}

void loop()
{
  digitalWrite(LED, HIGH);
  delay(200);
  digitalWrite(LED, LOW);
  delay(200);

  USBSerial.println("Test");
}
