#include "relay.h"
#include "driver/gpio.h"
#include "events.h"
#include "event_bus.h"
#include "types.h"
#include "telemetry.h"
#include "mcp23x17.h"

QueueHandle_t relayQueue = nullptr;
static mcp23x17_t mcp;

esp_err_t initRelay() {
  ESP_ERROR_CHECK(i2cdev_init());

  esp_err_t err =
      mcp23x17_init_desc(&mcp, MCP_ADDR, I2C_PORT, I2C_SDA, I2C_SCL);

  if (err != ESP_OK) {
    printf("MCP descriptor init failed: %s\n", esp_err_to_name(err));
    return err;
  }

  mcp.cfg.master.clk_speed = 100000;

  for (uint8_t pin = 0; pin < 16; ++pin) {
    // Preload the output HIGH (relay OFF) before enabling output mode.
    err = mcp23x17_set_level(&mcp, pin, 1);
    if (err != ESP_OK) {
      printf("GPIO %u HIGH setup failed: %s\n", pin, esp_err_to_name(err));
      return err;
    }

    err = mcp23x17_set_mode(&mcp, pin, MCP23X17_GPIO_OUTPUT);
    if (err != ESP_OK) {
      printf("GPIO %u setup failed: %s\n", pin, esp_err_to_name(err));
      return err;
    }
  }

  printf("MCP23017 initialized; all relays OFF.\n");
  return ESP_OK;
}

void testMcp23017Pins() {
  while (1) {
    for (uint8_t pin = 0; pin < 16; ++pin) {
      printf("GPIO %u ON\n", pin);

      ESP_ERROR_CHECK(mcp23x17_set_level(&mcp, pin, 0));
      vTaskDelay(pdMS_TO_TICKS(1000));

      ESP_ERROR_CHECK(mcp23x17_set_level(&mcp, pin, 1));
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

// void initRelay() {
//   for (gpio_num_t pin : relayPins) {
//     // pinMode(pin, OUTPUT);
//     gpio_set_direction(pin, GPIO_MODE_OUTPUT);
//     // digitalWrite(pin, HIGH);
//     gpio_set_level(pin, 1);
//     printf("relay: pin %d driven HIGH, readback %d\n", pin,
//            gpio_get_level(pin));
//   }
//   // TODO get a gpio expander (MCP23017) as I will need to sense relay state
//   // when vehicle is operated. This will do for remote start and monitoring.
//
//   // init vehicle state pins to off on boot, this will be changed when I get
//   the
//   // gpio expansion board and can read the state directly
//   vehicleState.acc = relayState(ACC);
//   vehicleState.headlights = relayState(HEADLIGHTS);
//   vehicleState.runningLights = relayState(RUNNING_LIGHTS);
//   vehicleState.heater = relayState(HEATER);
//   vehicleState.glowPlugs = relayState(GLOW_PLUGS);
//   vehicleState.ign = OFF;
// }

void testAssignedPins() {
  while (1) {
    for (uint8_t pin : relayPins) {
      mcp23x17_set_level(&mcp, pin, 0);
      printf("setting pins low\n");
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    for (uint8_t pin : relayPins) {
      mcp23x17_set_level(&mcp, pin, 1);
      printf("setting pins high\n");
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

bool togglePinState(uint8_t pin) {
  uint32_t level;
  // bool pinState = gpio_get_level(pin);
  mcp23x17_get_level(&mcp, pin, &level);
  bool pinState = (level == 1);
  pinState = !pinState;
  mcp23x17_set_level(&mcp, pin, pinState ? 0 : 1);
  return pinState;
}

bool relayState(gpio_num_t pin) { return gpio_get_level(pin) == 0; }

void relayTask(void *arg) {
  // This will recieve state from LoRa or Serial via Raspberry Pi 4g modem, then
  // send update the same way it came.
  if (initRelay() != ESP_OK) {
    return;
  }
  EventToggle event;
  while (true) {
    if (xQueueReceive(relayQueue, &event, portMAX_DELAY)) {
      switch (event.type) {
      case EVENT_TOGGLE_ACC:
        vehicleState.acc = togglePinState(ACC);
        xSemaphoreGive(telemetrySemaphore);
        break;
      case EVENT_TOGGLE_IGN:
        // const time for now (2-3s), change when I build detection... maybe
        // alternator current or rpm
        // TODO
        break;
      case EVENT_TOGGLE_HEATER:
        vehicleState.heater = togglePinState(HEATER);
        xSemaphoreGive(telemetrySemaphore);
        break;
      case EVENT_TOGGLE_GLOWPLUGS:
        // TODO const time for now (3-5s), change when I get coolant temp sensor
        break;
      case EVENT_TOGGLE_HEADLIGHTS:
        vehicleState.headlights = togglePinState(HEADLIGHTS);
        xSemaphoreGive(telemetrySemaphore);
        break;
      case EVENT_TOGGLE_RUNNINGLIGHTS:
        vehicleState.runningLights = togglePinState(RUNNING_LIGHTS);
        xSemaphoreGive(telemetrySemaphore);
        break;
      }
    }
  }
}
