#pragma once

#include "freertos/FreeRTOS.h"
#include "types.h"

#define BAUD 115200

void serialTask(void *arg);

const char *ignitionToString(Ignition ign);
extern QueueHandle_t serialQueue;
