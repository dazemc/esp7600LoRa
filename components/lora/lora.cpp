#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "LoRa.h"
#include "lora.h"
#include "events.h"
#include "event_bus.h"
#include "display.h"
#include "serial.h"
#include "utils.h"

QueueHandle_t loraTXQueue = nullptr;
QueueHandle_t loraRXQueue = nullptr;

void initLoRa() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa failed");
    return;
  }
  Serial.println("LoRa started");
}

void sendLoRaTask(void *arg) {
  EventLoRaTX event = {};
  while (true) {
    if (xQueueReceive(loraTXQueue, &event, portMAX_DELAY)) {
      switch (event.type) {
      case EVENT_LORA_SEND:
        break;
      case EVENT_LORA_TX: {
        EventSerial serialEvent = {};
        EventDisplay displayEvent = {};
        LoRaSend packet{};
        packet.vehicle = event.vehicle;
        packet.wifi = false;
        LoRa.beginPacket();
        LoRa.write((uint8_t *)&packet, sizeof(packet));
        LoRa.endPacket();
        // String str = String(event.voltageData.battery, 2);
        // snprintf(event.loraSend.voltage, sizeof(event.loraSend.voltage),
        // "%.2f", event.voltageData.battery);
        displayEvent.type = EVENT_DISPLAY_LORA_TX;
        displayEvent.loraTX.loraSend = packet;
        serialEvent.type = EVENT_SERIAL_LORA_TX;
        serialEvent.loraTX.loraSend = packet;
        // event.loraSend = packet;
        xQueueSend(displayQueue, &displayEvent, portMAX_DELAY);
        xQueueSend(serialQueue, &serialEvent, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
        break;
      }
      }
      if (DEBUG) {
        EventSerial event{};
        event.type = EVENT_SERIAL_DEBUG;
        UBaseType_t remaining = uxTaskGetStackHighWaterMark(NULL);
        debugRemainingStackSize("LoRaTX", event.debug.remainingStackMsg,
                                remaining);
        debugRemainingQueue("LoRaTX", event.debug.remainingQueueMsg,
                            uxQueueMessagesWaiting(loraTXQueue));
        xQueueSend(serialQueue, &event, portMAX_DELAY);
      }
    }
  }
}

void recvLoRaTask(void *arg) {
  EventLoRaRX event{};
  EventDisplay displayEvent{};
  EventSerial serialEvent{};
  LoRa.onReceive(onReceive);
  LoRa.receive();
  while (true) {
    if (xQueueReceive(loraRXQueue, &event, portMAX_DELAY)) {
      switch (event.type) {
      case EVENT_LORA_RECV:
        break;
      case EVENT_LORA_RX: {
        if (event.loraRecv.length != sizeof(LoRaSend)) {
          Serial.printf("Invalid packet size: %d\n", event.loraRecv.length);
          continue;
        }

        LoRaRecv packet{};

        memcpy(&packet, event.loraRecv.data, sizeof(packet));
        displayEvent.type = EVENT_DISPLAY_LORA_RX;
        displayEvent.loraRX.loraRecv = packet;
        serialEvent.type = EVENT_SERIAL_LORA_RX;
        serialEvent.loraRX.loraRecv = packet;
        xQueueSend(serialQueue, &serialEvent, portMAX_DELAY);
        xQueueSend(displayQueue, &displayEvent, portMAX_DELAY);
      }
      }
      if (DEBUG) {
        EventSerial event{};
        event.type = EVENT_SERIAL_DEBUG;
        UBaseType_t remaining = uxTaskGetStackHighWaterMark(NULL);
        debugRemainingStackSize("LoRaRX", event.debug.remainingStackMsg,
                                remaining);
        debugRemainingQueue("LoRaRX", event.debug.remainingQueueMsg,
                            uxQueueMessagesWaiting(loraRXQueue));
        xQueueSend(serialQueue, &event, portMAX_DELAY);
      }
    }
  }
}

void onReceive(int packetSize) {
  if (packetSize) {
    EventLoRaRX event{};
    event.type = EVENT_LORA_RX;
    event.loraRecv.length = 0;
    event.loraRecv.rssi = LoRa.packetRssi();
    event.loraRecv.snr = LoRa.packetSnr();

    while (LoRa.available() &&
           event.loraRecv.length < sizeof(event.loraRecv.data)) {
      event.loraRecv.data[event.loraRecv.length++] = LoRa.read();
    }
    xQueueSend(loraRXQueue, &event, 0);
  }
}
