#pragma once

#include "../../HTTPClient/src/HTTPClient.h"
#include "config_parser.h"

void task_networking(void *pvParameter);

extern HTTPClient client;

extern bool network_connected;
extern bool config_loaded;
extern ConfigParser config;