#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "types.h"

extern QueueHandle_t loraTXQueue;
extern QueueHandle_t loraRXQueue;
extern QueueHandle_t displayQueue;
extern QueueHandle_t serialQueue;
extern QueueHandle_t relayQueue;

extern SemaphoreHandle_t telemetrySemaphore;

extern VehicleState vehicleState;

void initEventBus();
void initTasks();
