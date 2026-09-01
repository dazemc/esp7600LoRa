#pragma once
#include "stdint.h"
#include "stdio.h"

extern const bool DEBUG;

void debugRemainingStackSize(const char *taskName, char *loc,
                             const size_t remaining);

void debugRemainingQueue(const char *queueName, char *loc,
                         const uint8_t remaining);
