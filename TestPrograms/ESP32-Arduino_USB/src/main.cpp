#include <Arduino.h>
#include "USB.h"
#include "esp32-hal-tinyusb.h"

#define LED 5

USBCDC USBSerial;

void setup()
{
  pinMode(LED, OUTPUT);
  
  //USB.enableDFU();
  //USB.productName("ESP32S2-USB");
  //USB.begin();
  //USBSerial.begin();

  for(int i = 10; i > 0; i--){
    Serial.println(i);
    delay(1000);
  }
  usb_persist_restart(RESTART_BOOTLOADER);
}

void loop()
{
  digitalWrite(LED, HIGH);
  delay(200);
  digitalWrite(LED, LOW);
  delay(200);

  USBSerial.println("Test");
}
