#pragma once

#include "freertos/FreeRTOS.h"

#define BAUD 115200

void printSerialTask(void *arg);

extern QueueHandle_t serialQueue;
