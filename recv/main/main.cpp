#include "event_bus.h"
#include "events.h"
#include "types.h"
#include "lora.h"
#include "serial.h"
#include "display.h"
#include "freertos/FreeRTOS.h"
#include "utils.h"
#include "wifi_sta.h"

// static const char AP_SSID[5] = "sage";
// static const char AP_PSK[9] = "minotaur";
// static char SSID[14] = "Aetheryte_2.4";
// static char PSK[13] = "blackchocobo";

static const QueueConfig recvLoRaQueueConf{
    .handle = &loraRXQueue, .length = 10, .itemSize = sizeof(EventLoRaRX)};

static const QueueConfig sendLoRaQueueConf{
    .handle = &loraTXQueue, .length = 10, .itemSize = sizeof(EventLoRaTX)};

static const QueueConfig displayQueueConf{
    .handle = &displayQueue, .length = 10, .itemSize = sizeof(EventDisplay)};

static const QueueConfig serialQueueConf{
    .handle = &serialQueue, .length = 10, .itemSize = sizeof(EventSerial)};

static const EventConfig events[]{
    {.taskConfig = {recvLoRaTask, "LoRa RX", 3096, NULL, 3, NULL},
     .queueConfig = &recvLoRaQueueConf},
    {.taskConfig = {sendLoRaTask, "LoRa TX", 2048, NULL, 3, NULL},
     .queueConfig = &sendLoRaQueueConf},
    {.taskConfig = {displayTask, "Display", 4096, NULL, 2, NULL},
     .queueConfig = &displayQueueConf},
    {.taskConfig = {serialTask, "Serial", 3096, NULL, 2, NULL},
     .queueConfig = &serialQueueConf},
};

static const SemaphoreConfig semaphores[]{
    // {&telemetrySemaphore}
};

extern "C" void app_main() {
  isVehicle = false;
  initLoRa();
  initSemaphores(semaphores, sizeof(semaphores) / sizeof(semaphores[0]));
  initEventBus(events, sizeof(events) / sizeof(events[0]));
  initWiFi();
  // initWiFiAP(AP_SSID, AP_PSK);
  // while (true) {
  //   EventLoRaTX event{};
  //   event.type = EVENT_LORA_TX;
  //   xQueueSend(loraTXQueue, &event, portMAX_DELAY);
  //   vTaskDelay(pdMS_TO_TICKS(3000));
  // }
}
