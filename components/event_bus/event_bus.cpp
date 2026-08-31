#include "event_bus.h"
#include "events.h"
#include "Arduino.h"

VehicleState vehicleState{};

void initEventBus(const EventConfig *events, size_t count) {
  for (size_t i = 0; i < count; i++) {
    const QueueConfig *queueConfig = events[i].queueConfig;
    const TaskConfig taskConfig = events[i].taskConfig;

    if (queueConfig != nullptr) {
      *queueConfig->handle =
          xQueueCreate(queueConfig->length, queueConfig->itemSize);
      if (*queueConfig->handle == nullptr) {
        Serial.println("Failed to create event queue");
        abort();
      }
    }

    xTaskCreate(taskConfig.function, taskConfig.name, taskConfig.stackSize,
                taskConfig.arg, taskConfig.priority, taskConfig.handle);
  }
}
void initSemaphores(const SemaphoreConfig *semaphores, size_t count) {
  for (size_t i = 0; i < count; i++) {
    SemaphoreConfig semaphoreConfig = semaphores[i];
    *semaphoreConfig.handle = xSemaphoreCreateBinary();
  }
}
