#pragma once
#include "events.h"

void telemetryTask(void *arg);

extern SemaphoreHandle_t telemetrySemaphore;
