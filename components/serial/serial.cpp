#include "serial.h"
#include "events.h"
#include "event_bus.h"
#include "utils.h"
#include "relay.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "types.h"

QueueHandle_t serialQueue = nullptr;
static uint8_t serialBuffer[BUF_SIZE];

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

static const char *jsonBool(bool value) { return value ? "true" : "false"; }

static void writeUartLine(const char *line) {
  uart_write_bytes(UART_PORT, line, strlen(line));
  uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100));
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

static void queueRelayCommand(EventToggleType type) {
  if (relayQueue == nullptr) {
    printf("Relay command ignored: no relay queue on this node\n");
    return;
  }
  EventToggle event{};
  event.type = type;
  if (xQueueSend(relayQueue, &event, portMAX_DELAY) != pdTRUE) {
    printf("Relay queue full; command dropped\n");
  }
}

static void readBuffer() {
  static char line_buffer[MAX_LINE_LENGTH];
  static size_t line_pos = 0;

  int length =
      uart_read_bytes(UART_PORT, serialBuffer, sizeof(serialBuffer), 0);

  if (length <= 0) {
    return;
  }

  for (int i = 0; i < length; i++) {
    uint8_t b = serialBuffer[i];

    if (b == '\r' || b == '\n') {
      if (line_pos == 0) {
        continue;
      }

      line_buffer[line_pos] = '\0';

      EventSerial incoming{};
      incoming.type = EVENT_SERIAL_IN;

      strncpy(incoming.serialIn.line, line_buffer,
              sizeof(incoming.serialIn.line) - 1);

      incoming.serialIn.line[sizeof(incoming.serialIn.line) - 1] = '\0';

      if (xQueueSend(serialQueue, &incoming, 0) != pdTRUE) {
        printf("ERROR: Serial queue full; dropping command\n");
      }

      line_pos = 0;
    } else if (line_pos < sizeof(line_buffer) - 1) {
      line_buffer[line_pos++] = static_cast<char>(b);
    } else {
      printf("ERROR: Serial command too long\n");
      line_pos = 0;
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
      case EVENT_SERIAL_LORA_RX: {
        const EventLoRaRX &rx = event.loraRX;
        const LoRaPacket &p = rx.loraPacket;
        char json[320];
        int len = snprintf(
            json, sizeof(json),
            "{\"t\":\"rx\",\"id\":%u,\"veh\":%s,\"ign\":\"%s\",\"head\":%s,"
            "\"acc\":%s,\"run\":%s,\"heat\":%s,\"glow\":%s,\"wifi\":%s,"
            "\"v\":%.2f,\"c\":%d,\"rssi\":%d,\"snr\":%.1f,\"drop\":%u}\n",
            static_cast<unsigned>(p.header.packetId), jsonBool(p.isVehicle),
            ignitionToString(p.vehicle.ign), jsonBool(p.vehicle.headlights),
            jsonBool(p.vehicle.acc), jsonBool(p.vehicle.runningLights),
            jsonBool(p.vehicle.heater), jsonBool(p.vehicle.glowPlugs),
            jsonBool(p.isWifi),
            static_cast<double>(p.vehicle.voltageData.battery),
            static_cast<int>(p.vehicle.coolantTemp), rx.loraRawPacket.rssi,
            static_cast<double>(rx.loraRawPacket.snr),
            static_cast<unsigned>(rx.droppedPackets));
        if (len > 0 && len < static_cast<int>(sizeof(json))) {
          writeUartLine(json);
        }
        if (DEBUG) {
          printf("%s", json);
        }
        break;
      }
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
        char *msg = event.serialIn.line;

        if (!strcmp("START", msg)) {
          if (DEBUG) {
            printf("\033[1;34mDEBUG: Command received: '%s'\033[0m\n", msg);
          }
          queueRelayCommand(EVENT_TOGGLE_IGN);
        } else if (!strcmp("HEADLIGHTS", msg)) {
          if (DEBUG) {
            printf("\033[1;34mDEBUG: Command received: '%s'\033[0m\n", msg);
          }
          queueRelayCommand(EVENT_TOGGLE_HEADLIGHTS);
        } else if (!strcmp("RUNNINGLIGHTS", msg)) {
          if (DEBUG) {
            printf("\033[1;34mDEBUG: Command received: '%s'\033[0m\n", msg);
          }
          queueRelayCommand(EVENT_TOGGLE_RUNNINGLIGHTS);
        } else if (!strcmp("HEATER", msg)) {
          if (DEBUG) {
            printf("\033[1;34mDEBUG: Command received: '%s'\033[0m\n", msg);
          }
          queueRelayCommand(EVENT_TOGGLE_HEATER);
        } else if (!strcmp("ACC", msg)) {
          if (DEBUG) {
            printf("\033[1;34mDEBUG: Command received: '%s'\033[0m\n", msg);
          }
          queueRelayCommand(EVENT_TOGGLE_ACC);
        } else if (!strcmp("GLOWPLUGS", msg)) {
          if (DEBUG) {
            printf("\033[1;34mDEBUG: Command received: '%s'\033[0m\n", msg);
          }
          queueRelayCommand(EVENT_TOGGLE_GLOWPLUGS);
        } else if (!strcmp("ALLOFF", msg)) {
          if (DEBUG) {
            printf("\033[1;34mDEBUG: Command received: '%s'\033[0m\n", msg);
          }
          queueRelayCommand(EVENT_TOGGLE_ALL_OFF);
        } else if (!strcmp("TEST", msg)) {

          printf("TEST message received\n");

          constexpr char response[] = "HEARD FROM ESP32\n";

          int written =
              uart_write_bytes(UART_PORT, response, sizeof(response) - 1);

          printf("UART response queued: %d bytes\n", written);

          uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100));
        } else if (!strcmp("STATE", msg)) {

          char response[256];

          int len =
              snprintf(response, sizeof(response),
                       "Aux Battery: %.2f\n"
                       "IGN: %s\n"
                       "Headlights: %s\n"
                       "ACC: %s\n"
                       "Running Lights: %s\n"
                       "Heater: %s\n"
                       "Glow Plugs: %s\n"
                       "Coolant Temp: %d\n",
                       static_cast<double>(vehicleState.voltageData.battery),
                       ignitionToString(vehicleState.ign),
                       vehicleState.headlights ? "ON" : "OFF",
                       vehicleState.acc ? "ON" : "OFF",
                       vehicleState.runningLights ? "ON" : "OFF",
                       vehicleState.heater ? "ON" : "OFF",
                       vehicleState.glowPlugs ? "ON" : "OFF",
                       static_cast<int>(vehicleState.coolantTemp));

          if (len > 0 && len < sizeof(response)) {
            uart_write_bytes(UART_PORT, response, len);
            uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100));
          }

        }

        else {
          if (DEBUG) {
            printf("\033[1;34mDEBUG: Unparsed serial message received: "
                   "%s\033[0m\n",
                   msg);
          }
        }
        break;
      }
      case EVENT_SERIAL_OUT: {
        char *msg = event.serialIn.line;
        if (DEBUG) {
          printf("\033[1;34mDEBUG: "
                 "%s\033[0m\n",
                 msg);
        }
        break;
      }
      }
    }
  }
}
