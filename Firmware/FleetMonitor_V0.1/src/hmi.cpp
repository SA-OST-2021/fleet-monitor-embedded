#include "hmi.h"

void hmi_init(void)
{
    digitalWrite(CAN_LED_RED, 1);
    digitalWrite(CAN_LED_GREEN, 1);
    digitalWrite(CAN_LED_BLUE, 1);
    digitalWrite(STATUS_LED_RED, 1);
    digitalWrite(STATUS_LED_GREEN, 1);
    digitalWrite(STATUS_LED_BLUE, 1);

    pinMode(CAN_LED_RED, OUTPUT);       
    pinMode(CAN_LED_GREEN, OUTPUT);     
    pinMode(CAN_LED_BLUE, OUTPUT);      
    pinMode(STATUS_LED_RED, OUTPUT);    
    pinMode(STATUS_LED_GREEN, OUTPUT);  
    pinMode(STATUS_LED_BLUE, OUTPUT);   
}