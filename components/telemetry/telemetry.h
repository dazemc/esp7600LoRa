#pragma once
#include "events.h"

void telemetryTask(void *arg);
const char *ignitionToString(Ignition ign);

extern SemaphoreHandle_t telemetrySemaphore;
