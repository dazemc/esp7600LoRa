#include "types.h"

extern VehicleState vehicleState;

void initEventBus(const EventConfig *events, size_t count);
void initSemaphores(const SemaphoreConfig *semaphores, size_t count);
