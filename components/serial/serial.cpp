#include "serial.h"
#include "events.h"
#include "event_bus.h"
#include "utils.h"

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

// static void initSerial() {
//   Serial.begin(BAUD);
//   vTaskDelay(pdMS_TO_TICKS(100));
//   Serial.println("Serial started");
// }

void serialTask(void *arg) {
  // initSerial();
  EventSerial event;

  while (true) {
    if (xQueueReceive(serialQueue, &event, portMAX_DELAY)) {
      switch (event.type) {
      case EVENT_SERIAL_LORA_TX: {
        printf("TX sent: ");
        printf("PacketId: %d\n", event.loraTX.loraPacket.header.packetId);
        printf("Voltage: %f\n",
               event.loraTX.loraPacket.vehicle.voltageData.battery);
        printf("ACC: %d\n", event.loraTX.loraPacket.vehicle.acc);
        printf("IGN: ");
        printf("%s\n", ignitionToString(event.loraTX.loraPacket.vehicle.ign));
        printf("HEADLIGHTS: %d\n", event.loraTX.loraPacket.vehicle.headlights);
        printf("RUNNING_LIGHTS: %d\n",
               event.loraTX.loraPacket.vehicle.runningLights);
        printf("GLOWPLUGS: %d\n", event.loraTX.loraPacket.vehicle.glowPlugs);
        printf("HEATER: %d\n", event.loraTX.loraPacket.vehicle.heater);
        printf("\n");
        break;
      }
      case EVENT_SERIAL_LORA_RX:
        printf("RX recv: ");
        printf("PacketId: %d\n", event.loraRX.loraPacket.header.packetId);
        printf("Voltage: %f\n",
               event.loraRX.loraPacket.vehicle.voltageData.battery);
        printf("ACC: %d\n", event.loraRX.loraPacket.vehicle.acc);
        printf("IGN: ");
        printf("%s\n", ignitionToString(event.loraRX.loraPacket.vehicle.ign));
        printf("HEADLIGHTS: %d\n", event.loraRX.loraPacket.vehicle.headlights);
        printf("RUNNING_LIGHTS: %d\n",
               event.loraRX.loraPacket.vehicle.runningLights);
        printf("GLOWPLUGS: %d\n", event.loraRX.loraPacket.vehicle.glowPlugs);
        printf("HEATER: %d\n", event.loraRX.loraPacket.vehicle.heater);
        printf("\n");
        break;
      case EVENT_SERIAL_LORA_WIFI:
      case EVENT_SERIAL_LORA_TOGGLE:
        break;
      case EVENT_SERIAL_DEBUG:
        UBaseType_t remaining = uxTaskGetStackHighWaterMark(NULL);
        size_t remainingBytes = remaining * sizeof(StackType_t);
        printf("%s", event.debug.remainingStackMsg);
        if (event.debug.remainingQueueMsg[0] != '\0') {
          printf("%s", event.debug.remainingQueueMsg);
        }
        printf(
            "\033[1;34mDEBUG: serial stack size remaining: %zu bytes\033[0m\n",
            remainingBytes);
        printf("\033[1;34mDEBUG: serial queue size: %d\033[0m\n",
               uxQueueMessagesWaiting(serialQueue));
        break;
      }
    }
  }
}
