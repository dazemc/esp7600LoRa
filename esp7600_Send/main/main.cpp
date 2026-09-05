#include "event_bus.h"
#include "events.h"
#include "types.h"
#include "lora.h"
#include "display.h"
#include "serial.h"
#include "telemetry.h"
#include "voltage.h"

static const QueueConfig recvLoRaRXQueueConf{
    .handle = &loraRXQueue, .length = 10, .itemSize = sizeof(EventLoRaRX)};

static const QueueConfig sendLoRaTXQueueConf{
    .handle = &loraTXQueue, .length = 10, .itemSize = sizeof(EventLoRaTX)};

static const QueueConfig displayQueueConf{
    .handle = &displayQueue, .length = 10, .itemSize = sizeof(EventDisplay)};

static const QueueConfig serialQueueConf{
    .handle = &serialQueue, .length = 10, .itemSize = sizeof(EventSerial)};

static const EventConfig events[]{
    {.taskConfig = {recvLoRaTask, "LoRa RX", 2048, NULL, 3, NULL},
     .queueConfig = &recvLoRaRXQueueConf},
    {.taskConfig = {sendLoRaTask, "LoRa TX", 2048, NULL, 3, NULL},
     .queueConfig = &sendLoRaTXQueueConf},
    {.taskConfig = {displayTask, "Display", 3096, NULL, 2, NULL},
     .queueConfig = &displayQueueConf},
    {.taskConfig = {serialTask, "Serial", 2048, NULL, 2, NULL},
     .queueConfig = &serialQueueConf},
    {.taskConfig = {voltageMonitorTask, "Voltage", 2048, NULL, 2, NULL},
     .queueConfig = nullptr},
    {.taskConfig = {telemetryTask, "Telemetry", 1024, NULL, 2, NULL},
     .queueConfig = nullptr},
};

static const SemaphoreConfig semaphores[]{{&telemetrySemaphore}};

extern "C" void app_main() {
  isVehicle = true;
  initLoRa();
  initSemaphores(semaphores, sizeof(semaphores) / sizeof(semaphores[0]));
  initEventBus(events, sizeof(events) / sizeof(events[0]));
}
