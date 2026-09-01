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
bool isVehicle{};
static uint8_t packetId = 0;
static uint8_t lastPacketId;
static bool havePacket = false;

void initLoRa() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa failed");
    return;
  }
  Serial.println("LoRa started");
}

void displayLoRa(LoRaSend packet) {
  EventDisplay displayEvent = {};
  displayEvent.type = EVENT_DISPLAY_LORA_TX;
  displayEvent.loraTX.loraSend = packet;
  xQueueSend(displayQueue, &displayEvent, portMAX_DELAY);
}

void serialLoRa(LoRaSend packet) {
  EventSerial serialEvent = {};
  serialEvent.type = EVENT_SERIAL_LORA_TX;
  serialEvent.loraTX.loraSend = packet;
  xQueueSend(serialQueue, &serialEvent, portMAX_DELAY);
}

void sendPacket(LoRaSend packet) {
  LoRa.beginPacket();
  LoRa.write((uint8_t *)&packet, sizeof(packet));
  LoRa.endPacket();
  LoRa.receive();
  displayLoRa(packet);
  serialLoRa(packet);
}

uint8_t incrementPacketId() {
  packetId = (packetId + 1) & 0x07;
  return packetId;
}

void buildPacket(EventLoRaTX event) {
  LoRaSend packet{};
  packet.header.packetId = incrementPacketId();
  packet.vehicle = event.loraSend.vehicle;
  packet.isWifi = false;
  packet.isVehicle = isVehicle;
  sendPacket(packet);
}

void sendLoRaTask(void *arg) {
  EventLoRaTX event = {};
  while (true) {
    if (xQueueReceive(loraTXQueue, &event, portMAX_DELAY)) {
      switch (event.type) {
      case EVENT_LORA_SEND:
        break;
      case EVENT_LORA_TX: {
        if (isVehicle) {
          buildPacket(event);
          break;
        } else {
          // test
          LoRaSend packet{};
          packet.header.packetId = incrementPacketId();
          packet.vehicle = {
              .voltageData = {},
              .ign = OFF,
              .headlights = true,
              .acc = false,
              .runningLights = false,
              .heater = false,
              .glowPlugs = false,
          };
          event.loraSend = packet;
          // buildPacket(event);
          sendPacket(packet);
          break;
        }
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

void checkPacketId(uint8_t packetId) {
  if (havePacket) {
    uint8_t expected = (lastPacketId + 1) & 0x07;
    if (packetId != expected) {
      Serial.printf("\033[1;33mWARNING: dropped packet(s) expected %d but got "
                    "%d\033[0m\n",
                    expected, packetId);
    }
    lastPacketId = packetId;
  } else {
    // so I don't continously set the bool
    havePacket = true;
    lastPacketId = packetId;
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

        LoRaSend packet{};

        memcpy(&packet, event.loraRecv.data, sizeof(packet));
        checkPacketId(packet.header.packetId);
        displayEvent.type = EVENT_DISPLAY_LORA_RX;
        displayEvent.loraRX.loraSend = packet;
        serialEvent.type = EVENT_SERIAL_LORA_RX;
        serialEvent.loraRX.loraSend = packet;
        xQueueSend(serialQueue, &serialEvent, portMAX_DELAY);
        xQueueSend(displayQueue, &displayEvent, portMAX_DELAY);
        break;
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
