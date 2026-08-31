#include "telemetry.h"
#include "types.h"
#include "events.h"
#include "event_bus.h"

const char *ignitionToString(Ignition ign) {
  switch (ign) {
  case OFF:
    return "OFF";
  case ON:
    return "ON";
  case START:
    return "START";
  default:
    return "UNKNOWN";
  }
}

void telemetryTask(void *arg) {
  while (true) {
    while (true) {
      if (xSemaphoreTake(telemetrySemaphore, portMAX_DELAY)) {
        EventLoRa txEvent;
        txEvent.type = EVENT_LORA_TX;
        txEvent.vehicle = vehicleState;

        xQueueSend(loraTXQueue, &txEvent, portMAX_DELAY);
      }
    }
  }
}
