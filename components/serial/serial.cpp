#include "serial.h"
#include "events.h"
#include "event_bus.h"
#include "utils.h"
#include <Arduino.h>

QueueHandle_t serialQueue = nullptr;

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

static void initSerial() {
  Serial.begin(BAUD);
  vTaskDelay(pdMS_TO_TICKS(100));
  Serial.println("Serial started");
}

void serialTask(void *arg) {
  initSerial();
  EventSerial event;

  while (true) {
    if (xQueueReceive(serialQueue, &event, portMAX_DELAY)) {
      switch (event.type) {
      case EVENT_SERIAL_LORA_TX: {
        Serial.print("TX sent: ");
        Serial.printf("PacketId: %d\n", event.loraTX.loraSend.header.packetId);
        Serial.printf("Voltage: %f\n",
                      event.loraTX.loraSend.vehicle.voltageData.battery);
        Serial.printf("ACC: %d\n", event.loraTX.loraSend.vehicle.acc);
        Serial.print("IGN: ");
        Serial.println(ignitionToString(event.loraTX.loraSend.vehicle.ign));
        Serial.printf("HEADLIGHTS: %d\n",
                      event.loraTX.loraSend.vehicle.headlights);
        Serial.printf("RUNNING_LIGHTS: %d\n",
                      event.loraTX.loraSend.vehicle.runningLights);
        Serial.printf("GLOWPLUGS: %d\n",
                      event.loraTX.loraSend.vehicle.glowPlugs);
        Serial.printf("HEATER: %d\n", event.loraTX.loraSend.vehicle.heater);
        Serial.println();
        break;
      }
      case EVENT_SERIAL_LORA_RX:
        Serial.print("RX recv: ");
        Serial.printf("PacketId: %d\n", event.loraRX.loraSend.header.packetId);
        Serial.printf("Voltage: %f\n",
                      event.loraRX.loraSend.vehicle.voltageData.battery);
        Serial.printf("ACC: %d\n", event.loraRX.loraSend.vehicle.acc);
        Serial.print("IGN: ");
        Serial.println(ignitionToString(event.loraRX.loraSend.vehicle.ign));
        Serial.printf("HEADLIGHTS: %d\n",
                      event.loraRX.loraSend.vehicle.headlights);
        Serial.printf("RUNNING_LIGHTS: %d\n",
                      event.loraRX.loraSend.vehicle.runningLights);
        Serial.printf("GLOWPLUGS: %d\n",
                      event.loraRX.loraSend.vehicle.glowPlugs);
        Serial.printf("HEATER: %d\n", event.loraRX.loraSend.vehicle.heater);
        Serial.println();
        break;
      case EVENT_SERIAL_LORA_WIFI:
      case EVENT_SERIAL_LORA_TOGGLE:
        break;
      case EVENT_SERIAL_DEBUG:
        UBaseType_t remaining = uxTaskGetStackHighWaterMark(NULL);
        size_t remainingBytes = remaining * sizeof(StackType_t);
        Serial.print(event.debug.remainingStackMsg);
        if (event.debug.remainingQueueMsg[0] != '\0') {
          Serial.print(event.debug.remainingQueueMsg);
        }
        Serial.printf(
            "\033[1;34mDEBUG: serial stack size remaining: %zu bytes\033[0m\n",
            remainingBytes);
        Serial.printf("\033[1;34mDEBUG: serial queue size: %d\033[0m\n",
                      uxQueueMessagesWaiting(serialQueue));
        break;
      }
    }
  }
}
