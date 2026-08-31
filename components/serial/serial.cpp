#include "serial.h"
#include "events.h"
#include "event_bus.h"
#include "types.h"
#include "telemetry.h"
#include <Arduino.h>

QueueHandle_t serialQueue = nullptr;

static void initSerial() {
  Serial.begin(BAUD);
  vTaskDelay(pdMS_TO_TICKS(100));
  Serial.println("Serial started");
}

void printSerialTask(void *arg) {
  initSerial();
  EventSerial event;

  while (true) {
    if (xQueueReceive(serialQueue, &event, portMAX_DELAY)) {
      switch (event.type) {
      case EVENT_SERIAL_LORA_TX: {
        Serial.print("Tx sent;\nVoltage: ");
        Serial.println(event.loraTX.vehicle.voltageData.battery);
        Serial.printf("ACC: %d\n", event.loraTX.vehicle.acc);
        Serial.print("IGN: ");
        Serial.println(ignitionToString(event.loraTX.vehicle.ign));
        Serial.printf("HEADLIGHTS: %d\n", event.loraTX.vehicle.headlights);
        Serial.printf("RUNNING_LIGHTS: %d\n",
                      event.loraTX.vehicle.runningLights);
        Serial.printf("GLOWPLUGS: %d\n", event.loraTX.vehicle.glowPlugs);
        Serial.printf("HEATER: %d\n", event.loraTX.vehicle.heater);
        break;
      }
      case EVENT_SERIAL_LORA_RX:
        Serial.printf("RX recievd:\nVOLTAGE: %.2f\nADC: %.2f\nRAW: %.2f\n",
                      event.loraRX.vehicle.voltageData.battery,
                      event.loraRX.vehicle.voltageData.adc,
                      event.loraRX.vehicle.voltageData.raw);
        break;
      case EVENT_SERIAL_LORA_WIFI:
      case EVENT_SERIAL_LORA_TOGGLE:
        break;
      }
    }
  }
}
