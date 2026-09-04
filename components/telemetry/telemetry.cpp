#include "telemetry.h"
#include "types.h"
#include "events.h"
#include "event_bus.h"
#include "lora.h"
#include "serial.h"
#include "utils.h"

SemaphoreHandle_t telemetrySemaphore = nullptr;

void telemetryTask(void *arg) {
  while (true) {
    if (xSemaphoreTake(telemetrySemaphore, portMAX_DELAY)) {
      EventLoRaTX txEvent;
      txEvent.type = EVENT_LORA_TX;
      txEvent.loraPacket.vehicle = vehicleState;

      xQueueSend(loraTXQueue, &txEvent, portMAX_DELAY);
      if (DEBUG) {
        EventSerial event{};
        event.type = EVENT_SERIAL_DEBUG;
        UBaseType_t remaining = uxTaskGetStackHighWaterMark(NULL);
        debugRemainingStackSize("telemetry", event.debug.remainingStackMsg,
                                remaining);
        xQueueSend(serialQueue, &event, portMAX_DELAY);
      }
    }
  }
}
