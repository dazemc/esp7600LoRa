#include "serial.h"
#include "events.h"
#include "event_bus.h"
#include "utils.h"
#include "relay.h"
#include "driver/gpio.h"
#include "driver/uart.h"

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
  uart_config_t uart_config = {};
  uart_config.baud_rate = BAUD;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.source_clk = UART_SCLK_DEFAULT;

  ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE,
                               UART_PIN_NO_CHANGE));

  vTaskDelay(pdMS_TO_TICKS(100));
}

static void readBuffer() {
  static char line_buffer[MAX_LINE_LENGTH];
  static size_t line_pos = 0;

  size_t length = 0;
  uart_get_buffered_data_len(UART_PORT, &length);

  if (length > 0) {
    uint8_t byte_received;

    for (size_t i = 0; i < length; i++) {
      if (uart_read_bytes(UART_PORT, &byte_received, 1, 0) > 0) {

        if (byte_received == '\n' || byte_received == '\r') {
          if (line_pos > 0) {
            line_buffer[line_pos] = '\0';

            EventSerial eventSerialIn{};
            eventSerialIn.type = EVENT_SERIAL_IN;
            strncpy(eventSerialIn.serialIn.line, line_buffer,
                    sizeof(eventSerialIn.serialIn.line) - 1);
            eventSerialIn.serialIn
                .line[sizeof(eventSerialIn.serialIn.line) - 1] =
                '\0'; // Force null termination

            xQueueSend(serialQueue, &eventSerialIn, pdMS_TO_TICKS(10));
            // Reset buffer for the next message
            line_pos = 0;
          }
        } else if (line_pos < (MAX_LINE_LENGTH - 1)) {
          line_buffer[line_pos++] = (char)byte_received;
        } else {
          line_pos = 0;
        }
      }
    }
  }
}

void serialTask(void *arg) {
  initSerial();
  EventSerial event;

  while (true) {
    readBuffer();
    if (xQueueReceive(serialQueue, &event, pdMS_TO_TICKS(10))) {
      switch (event.type) {
      case EVENT_SERIAL_LORA_TX: {
        if (DEBUG) {
          printf("TX sent: ");
          printf("PacketId: %d\n", event.loraTX.loraPacket.header.packetId);
          printf("Voltage: %f\n",
                 event.loraTX.loraPacket.vehicle.voltageData.battery);
          printf("ACC: %d\n", event.loraTX.loraPacket.vehicle.acc);
          printf("IGN: ");
          printf("%s\n", ignitionToString(event.loraTX.loraPacket.vehicle.ign));
          printf("HEADLIGHTS: %d\n",
                 event.loraTX.loraPacket.vehicle.headlights);
          printf("RUNNING_LIGHTS: %d\n",
                 event.loraTX.loraPacket.vehicle.runningLights);
          printf("GLOWPLUGS: %d\n", event.loraTX.loraPacket.vehicle.glowPlugs);
          printf("HEATER: %d\n", event.loraTX.loraPacket.vehicle.heater);
          printf("\n");
        }
        break;
      }
      case EVENT_SERIAL_LORA_RX:
        if (DEBUG) {
          printf("RX recv: ");
          printf("PacketId: %d\n", event.loraRX.loraPacket.header.packetId);
          printf("Voltage: %f\n",
                 event.loraRX.loraPacket.vehicle.voltageData.battery);
          printf("ACC: %d\n", event.loraRX.loraPacket.vehicle.acc);
          printf("IGN: ");
          printf("%s\n", ignitionToString(event.loraRX.loraPacket.vehicle.ign));
          printf("HEADLIGHTS: %d\n",
                 event.loraRX.loraPacket.vehicle.headlights);
          printf("RUNNING_LIGHTS: %d\n",
                 event.loraRX.loraPacket.vehicle.runningLights);
          printf("GLOWPLUGS: %d\n", event.loraRX.loraPacket.vehicle.glowPlugs);
          printf("HEATER: %d\n", event.loraRX.loraPacket.vehicle.heater);
          printf("\n");
        }
        break;
      case EVENT_SERIAL_LORA_WIFI:
      case EVENT_SERIAL_LORA_TOGGLE:
        break;
      case EVENT_SERIAL_DEBUG: {
        if (DEBUG) {
          UBaseType_t remaining = uxTaskGetStackHighWaterMark(NULL);
          size_t remainingBytes = remaining * sizeof(StackType_t);
          printf("%s", event.debug.remainingStackMsg);
          if (event.debug.remainingQueueMsg[0] != '\0') {
            printf("%s", event.debug.remainingQueueMsg);
          }
          printf("\033[1;34mDEBUG: serial stack size remaining: %zu "
                 "bytes\033[0m\n",
                 remainingBytes);
          printf("\033[1;34mDEBUG: serial queue size: %d\033[0m\n",
                 uxQueueMessagesWaiting(serialQueue));
        }
        break;
      }
      case EVENT_SERIAL_IN: {

        EventToggle eventToggle{};
        char *msg = event.serialIn.line;
        if (!strcmp("START", msg)) {
          if (DEBUG) {
            printf("\033[1;34mDEBUG: Command received: '%s'\033[0m\n", msg);
          }
          eventToggle.type = EVENT_TOGGLE_IGN;
          xQueueSend(relayQueue, &eventToggle, portMAX_DELAY);
        } else if (!strcmp("HEADLIGHTS", msg)) {
          eventToggle.type = EVENT_TOGGLE_HEADLIGHTS;
          if (DEBUG) {
            printf("\033[1;34mDEBUG: Command received: '%s'\033[0m\n", msg);
          }
          xQueueSend(relayQueue, &eventToggle, portMAX_DELAY);
        } else if (!strcmp("RUNNINGLIGHTS", msg)) {
          eventToggle.type = EVENT_TOGGLE_RUNNINGLIGHTS;
          if (DEBUG) {
            printf("\033[1;34mDEBUG: Command received: '%s'\033[0m\n", msg);
          }
          xQueueSend(relayQueue, &eventToggle, portMAX_DELAY);
        } else if (!strcmp("HEATER", msg)) {
          eventToggle.type = EVENT_TOGGLE_HEATER;
          if (DEBUG) {
            printf("\033[1;34mDEBUG: Command received: '%s'\033[0m\n", msg);
          }
          xQueueSend(relayQueue, &eventToggle, portMAX_DELAY);
        } else if (!strcmp("ALLOFF", msg)) {
          eventToggle.type = EVENT_TOGGLE_ALL_OFF;
          if (DEBUG) {
            printf("\033[1;34mDEBUG: Command received: '%s'\033[0m\n", msg);
          }
          xQueueSend(relayQueue, &eventToggle, portMAX_DELAY);
        } else {
          if (DEBUG) {
            printf("\033[1;34mDEBUG: Unparsed serial message received: "
                   "%s\033[0m\n",
                   msg);
          }
        }
        break;
      }
      }
    }
  }
}
