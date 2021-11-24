#pragma once

#include "../../HTTPClient/src/HTTPClient.h"

void task_networking(void *pvParameter);

extern HTTPClient client;

extern bool network_connected;