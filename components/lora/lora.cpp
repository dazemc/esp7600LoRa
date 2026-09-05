#undef ARDUINO
#include <RadioLib.h>
#include <hal/ESP-IDF/EspHal.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "lora.h"
#include "events.h"
#include "event_bus.h"
#include "display.h"
#include "serial.h"
#include "utils.h"
#include "voltage.h"

QueueHandle_t loraTXQueue = nullptr;
QueueHandle_t loraRXQueue = nullptr;
bool isVehicle{};

static SemaphoreHandle_t radioMutex;

static EspHal *hal = new EspHal(LORA_SCK, LORA_MISO, LORA_MOSI);
static SX1276 radio = new Module(hal, LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1);

static uint8_t packetIdTX = 0;
static uint8_t lastPacketIdRX;
static bool havePacketRX = false;

static const char *TAG = "lora";

volatile bool receivedFlag = false;

void setFlag(void) { receivedFlag = true; }

void initLoRa() {
  ESP_LOGI(TAG, "[SX1276] Initializing ...");
  radioMutex = xSemaphoreCreateMutex();

  int state = radio.begin(915.0);

  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGE(TAG, "failed, code %d", state);
    while (true) {
      hal->delay(1000);
    }
  }

  radio.setSyncWord(0x14);
  radio.setDio0Action(setFlag, hal->GpioInterruptRising);
  radio.startReceive();

  ESP_LOGI(TAG, "success!");
}

void displayLoRa(const LoRaPacket &packet) {
  EventDisplay displayEvent = {};
  displayEvent.type = EVENT_DISPLAY_LORA_TX;
  displayEvent.loraTX.loraPacket = packet;
  xQueueSend(displayQueue, &displayEvent, portMAX_DELAY);
}

void serialLoRa(const LoRaPacket &packet) {
  EventSerial serialEvent = {};
  serialEvent.type = EVENT_SERIAL_LORA_TX;
  serialEvent.loraTX.loraPacket = packet;
  xQueueSend(serialQueue, &serialEvent, portMAX_DELAY);
}

void sendPacketDuplex(const LoRaPacket &packet) {
  xSemaphoreTake(radioMutex, portMAX_DELAY);
  receivedFlag = false;
  radio.clearDio0Action();

  LoRaCompactPacket compactPacket = packetToCompactPacket(packet, packetIdTX);
  int state = radio.transmit((uint8_t *)&compactPacket, sizeof(compactPacket));
  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGE(TAG, "transmit failed, code %d", state);
  }

  radio.setDio0Action(setFlag, hal->GpioInterruptRising);
  radio.startReceive();

  xSemaphoreGive(radioMutex);

  displayLoRa(packet);
  serialLoRa(packet);
}

void buildPacket(EventLoRaTX event) {
  LoRaPacket packet{};
  packet.header.packetId = incrementPacketId(packetIdTX);
  packet.vehicle = event.loraPacket.vehicle;
  packet.isWifi = false;
  packet.isVehicle = isVehicle;
  sendPacketDuplex(packet);
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
        } else {
          // test packet
          LoRaPacket packet{};
          packet.header.packetId = incrementPacketId(packetIdTX);
          packet.vehicle = {
              .voltageData = {},
              .ign = OFF,
              .headlights = true,
              .acc = false,
              .runningLights = false,
              .heater = false,
              .glowPlugs = false,
          };
          sendPacketDuplex(packet);
        }
        break;
      }
      }

      if (DEBUG) {
        EventSerial dbg{};
        dbg.type = EVENT_SERIAL_DEBUG;
        UBaseType_t remaining = uxTaskGetStackHighWaterMark(NULL);
        debugRemainingStackSize("LoRaTX", dbg.debug.remainingStackMsg,
                                remaining);
        debugRemainingQueue("LoRaTX", dbg.debug.remainingQueueMsg,
                            uxQueueMessagesWaiting(loraTXQueue));
        xQueueSend(serialQueue, &dbg, portMAX_DELAY);
      }
    }
  }
}

void checkPacketId(uint8_t packetId) {
  if (havePacketRX) {
    uint8_t expected = (lastPacketIdRX + 1) & 0x07;
    if (packetId != expected) {
      printf("\033[1;33mWARNING: dropped packet(s) expected %d but got "
             "%d\033[0m\n",
             expected, packetId);
    }
    lastPacketIdRX = packetId;
  } else {
    havePacketRX = true;
    lastPacketIdRX = packetId;
  }
}

void recvLoRaTask(void *arg) {
  EventDisplay displayEvent{};
  EventSerial serialEvent{};

  while (true) {
    if (receivedFlag) {
      receivedFlag = false;
      xSemaphoreTake(radioMutex, portMAX_DELAY);

      uint8_t buffer[sizeof(LoRaCompactPacket)];
      int state = radio.readData(buffer, sizeof(buffer));

      if (state == RADIOLIB_ERR_NONE) {
        size_t len = radio.getPacketLength();

        if (len != sizeof(LoRaCompactPacket)) {
          printf("Invalid packet size: %zu\n", len);
        } else {
          LoRaCompactPacket packet{};
          memcpy(&packet, buffer, sizeof(packet));

          checkPacketId(packet.packetId);

          displayEvent.type = EVENT_DISPLAY_LORA_RX;
          LoRaPacket unpackedPacket = unpackPacket(packet);
          unpackedPacket.vehicle.voltageData.battery =
              u8ToVoltage(unpackedPacket.vehicle.voltageData.encodedVoltage);
          displayEvent.loraRX.loraPacket = unpackedPacket;

          serialEvent.type = EVENT_SERIAL_LORA_RX;
          serialEvent.loraRX.loraPacket = unpackedPacket;

          xQueueSend(serialQueue, &serialEvent, portMAX_DELAY);
          xQueueSend(displayQueue, &displayEvent, portMAX_DELAY);
        }
      } else {
        ESP_LOGE(TAG, "readData failed, code %d", state);
      }

      radio.startReceive();
      xSemaphoreGive(radioMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}
