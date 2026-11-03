#include "event_bus.h"
#include "events.h"

VehicleState vehicleState{};

void initEventBus(const EventConfig *events, size_t count) {
  // Queues first: a task can run before the next table entry is processed, so
  // it must never see a later queue as null.
  for (size_t i = 0; i < count; i++) {
    const QueueConfig *queueConfig = events[i].queueConfig;
    if (queueConfig == nullptr) {
      continue;
    }

    *queueConfig->handle =
        xQueueCreate(queueConfig->length, queueConfig->itemSize);
    if (*queueConfig->handle == nullptr) {
      printf("\033[1;31mERROR: Failed to create event queue\033[0m\n\n");
      abort();
    }
  }

  for (size_t i = 0; i < count; i++) {
    const TaskConfig taskConfig = events[i].taskConfig;
    if (xTaskCreate(taskConfig.function, taskConfig.name,
                    taskConfig.stackSize, taskConfig.arg, taskConfig.priority,
                    taskConfig.handle) != pdPASS) {
      printf("\033[1;31mERROR: Failed to create task %s\033[0m\n\n",
             taskConfig.name);
      abort();
    }
  }
}
void initSemaphores(const SemaphoreConfig *semaphores, size_t count) {
  for (size_t i = 0; i < count; i++) {
    SemaphoreConfig semaphoreConfig = semaphores[i];
    *semaphoreConfig.handle = xSemaphoreCreateBinary();
    if (*semaphoreConfig.handle == nullptr) {
      printf("\033[1;31mERROR: Failed to create semaphore\033[0m\n\n");
      abort();
    }
  }
}
