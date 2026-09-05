#include "relay.h"
#include "driver/gpio.h"
#include "events.h"
#include "event_bus.h"
#include "types.h"
#include "telemetry.h"

QueueHandle_t relayQueue = nullptr;

void initRelay() {
  for (gpio_num_t pin : relayPins) {
    // pinMode(pin, OUTPUT);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    // digitalWrite(pin, HIGH);
    gpio_set_level(pin, 1);
  }
  // TODO get a gpio expander (MCP23017) as I will need to sense relay state
  // when vehicle is operated. This will do for remote start and monitoring.

  // init vehicle state pins to off on boot, this will be changed when I get the
  // gpio expansion board and can read the state directly
  vehicleState.acc = relayState(ACC);
  vehicleState.headlights = relayState(HEADLIGHTS);
  vehicleState.runningLights = relayState(RUNNING_LIGHTS);
  vehicleState.heater = relayState(HEATER);
  vehicleState.glowPlugs = relayState(GLOW_PLUGS);
  vehicleState.ign = OFF;
}

void relayCycleTest() {
  for (gpio_num_t pin : relayPins) {
    gpio_set_level(pin, 0);
    printf("setting pins low\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  vTaskDelay(pdMS_TO_TICKS(1000));
  for (gpio_num_t pin : relayPins) {
    gpio_set_level(pin, 1);
    printf("setting pins high\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

bool togglePinState(gpio_num_t pin) {
  bool pinState = gpio_get_level(pin);
  pinState = !pinState;
  gpio_set_level(pin, pinState ? 0 : 1);
  return pinState;
}

bool relayState(gpio_num_t pin) { return gpio_get_level(pin) == 0; }

void relayTask(void *arg) {
  // This will recieve state from LoRa or Serial via Raspberry Pi 4g modem, then
  // send update the same way it came.
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
