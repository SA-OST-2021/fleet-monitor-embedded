#pragma once

extern QueueHandle_t fmsQueue;

void task_can(void *pvParameter);

extern bool can_connected;
