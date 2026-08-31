#include <Arduino.h>
#include "lora.h"
#include "events.h"
#include "event_bus.h"

void initLoRa() {
  SPI.begin(SCK, MISO, MOSI, CS);
  LoRa.setPins(CS, RESET, DID0);

  if (!LoRa.begin(FREQ)) {
    Serial.println("LoRa failed");
    return;
  }
  Serial.println("LoRa started");
}

void sendLoRaTask(void *arg) {

  EventLoRa event = {};
  while (true) {
    if (xQueueReceive(loraTXQueue, &event, portMAX_DELAY)) {
      switch (event.type) {
      case EVENT_LORA_SEND:
      case EVENT_LORA_RX:
      case EVENT_LORA_RECV:
        break;
      case EVENT_LORA_TX: {
        LoRaSend packet{};
        packet.vehicle = event.vehicle;
        packet.wifi = false;
        LoRa.beginPacket();
        LoRa.write((uint8_t *)&packet, sizeof(packet));
        LoRa.endPacket();
        // String str = String(event.voltageData.battery, 2);
        // snprintf(event.loraSend.voltage, sizeof(event.loraSend.voltage),
        // "%.2f", event.voltageData.battery);
        event.type = EVENT_LORA_SEND;
        event.loraSend = packet;
        xQueueSend(displayQueue, &event, portMAX_DELAY);
        xQueueSend(serialQueue, &event, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
        break;
      }
      }
    }
  }
}
