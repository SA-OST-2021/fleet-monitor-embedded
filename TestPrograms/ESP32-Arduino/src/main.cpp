#include <Arduino.h>


#define LED 5

void setup() {
  pinMode(LED, OUTPUT);// put your setup code here, to run once:
}

void loop() {
  digitalWrite(LED, HIGH);
  delay(200);
  digitalWrite(LED, LOW);
  delay(200);
}