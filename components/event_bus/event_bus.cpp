#include "event_bus.h"
#include "display.h"
#include "lora.h"
#include "voltage.h"
#include "serial.h"
#include "telemetry.h"
#include "relay.h"

QueueHandle_t loraTXQueue = nullptr;
QueueHandle_t loraRXQueue = nullptr;
QueueHandle_t displayQueue = nullptr;
QueueHandle_t serialQueue = nullptr;
QueueHandle_t relayQueue = nullptr;

SemaphoreHandle_t telemetrySemaphore = nullptr;

VehicleState vehicleState{};

void initEventBus() {
  loraTXQueue = xQueueCreate(10, sizeof(EventLoRa));
  loraRXQueue = xQueueCreate(10, sizeof(EventLoRa));
  displayQueue = xQueueCreate(10, sizeof(EventLoRa));
  serialQueue = xQueueCreate(10, sizeof(EventLoRa));
  relayQueue = xQueueCreate(10, sizeof(EventToggle));
  telemetrySemaphore = xSemaphoreCreateBinary();
  if (loraTXQueue == nullptr || loraRXQueue == nullptr ||
      displayQueue == nullptr || serialQueue == nullptr ||
      telemetrySemaphore == nullptr || relayQueue == nullptr) {
    Serial.println("Failed to create event queue or semaphore");
    abort();
  }
  initTasks();
}

void initTasks() {
  xTaskCreate(displayTask, "display", 4096, NULL, 2, NULL);
  xTaskCreate(sendLoRaTask, "LoRa Send", 4096, NULL, 6, NULL);
  xTaskCreate(printSerialTask, "serial", 4096, NULL, 2, NULL);
  xTaskCreate(voltageMonitorTask, "voltage", 4096, NULL, 6, NULL);
  xTaskCreate(telemetryTask, "telemetry", 4096, NULL, 6, NULL);
  xTaskCreate(relayTask, "relay", 4096, NULL, 7, NULL);
}
