#include "utils.h"
#include "stdint.h"
#include "stddef.h"
#include "stdio.h"
#include "freertos/FreeRTOS.h"

const bool DEBUG = true;

void debugRemainingStackSize(const char *taskName, char *loc,
                             const size_t remaining) {
  size_t remainingBytes = remaining * sizeof(StackType_t);
  snprintf(loc, 64,
           "\033[1;34mDEBUG: %s stack size remaining: %zu bytes\033[0m\n",
           taskName, remainingBytes);
}

void debugRemainingQueue(const char *queueName, char *loc,
                         const uint8_t remaining) {
  snprintf(loc, 64, "\033[1;34mDEBUG: %s queue remaining: %d\033[0m\n",
           queueName, remaining);
}
