#include "serial.h"
#include "events.h"
#include "event_bus.h"
#include "types.h"
#include "telemetry.h"
#include <Arduino.h>

void initSerial() {
  Serial.begin(BAUD);
  vTaskDelay(pdMS_TO_TICKS(100));
  Serial.println("Serial started");
}

void printSerialTask(void *arg) {
  EventLoRa event;

  while (true) {
    if (xQueueReceive(serialQueue, &event, portMAX_DELAY)) {
      switch (event.type) {
      case EVENT_LORA_SEND: {
        Serial.print("Sending;\nVoltage: ");
        Serial.println(event.loraSend.vehicle.voltageData.battery);
        Serial.printf("ACC: %d\n", event.loraSend.vehicle.acc);
        Serial.print("IGN: ");
        Serial.println(ignitionToString(event.loraSend.vehicle.ign));
        Serial.printf("HEADLIGHTS: %d\n", event.loraSend.vehicle.headlights);
        Serial.printf("RUNNING_LIGHTS: %d\n",
                      event.loraSend.vehicle.runningLights);
        Serial.printf("GLOWPLUGS: %d\n", event.loraSend.vehicle.glowPlugs);
        Serial.printf("HEATER: %d\n", event.loraSend.vehicle.heater);
        break;
      }
      case EVENT_LORA_RECV: {
        break;
      }
      case EVENT_LORA_RX:
      case EVENT_LORA_TX: {
        break;
      }
      }
    }
  }
}
