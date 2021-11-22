#pragma once

#include <ArduinoHttpClient.h>

void task_networking(void *pvParameter);

extern HttpClient client;

extern bool network_connected;