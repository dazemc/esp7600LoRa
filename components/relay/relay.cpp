#include <Arduino.h>
#include "relay.h"
#include "events.h"
#include "event_bus.h"
#include "types.h"

void initRelay() {
  for (int pin : relayPins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
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
  for (int pin : relayPins) {
    digitalWrite(pin, LOW);
    Serial.println("setting pins low");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  vTaskDelay(pdMS_TO_TICKS(1000));
  for (int pin : relayPins) {
    digitalWrite(pin, HIGH);
    Serial.println("setting pins high");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

bool togglePinState(uint8_t pin) {
  bool pinState = digitalRead(pin) == LOW;
  pinState = !pinState;
  digitalWrite(pin, pinState ? LOW : HIGH);
  return pinState;
}

bool relayState(uint8_t pin) { return digitalRead(pin) == LOW; }

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
