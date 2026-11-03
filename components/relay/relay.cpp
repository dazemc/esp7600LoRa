// TODO!!!! 16 channel relay so the opposing relay for each output will be used
// as an input, this way I can update the state of the vehicle based on these
// input states

//  I.E when I am manually starting the vehicle but still want the
//  glowPlugSequence to operate without going through the whole ignSequence as
//  this will replace the glowplug controller and use the coolant temp sensor to
//  determine toggle time. I should also check the resistance values on each of
//  the plugs to prevent quick cycling with the same toggle time, like on cold
//  days where crank might just need to be re-run but glowplugs are still hot.
// This means I will need to build an 8 way voltage div. I am wondering if I can
// build one volt div and switch between plugs with transistors or something....
// Either way going to need another gpio expander, might as well get two more
// for future telemetry.

//  Also add a time limit on the running duration while the key is not present.
//  This means instead of a running check like I will need for the ign/glowplugs
//  I will use the key position to determine that state. This will be the same
//  indicator for the "manual" glowPlugSequence, the relay should turn off if
//  the actual physical switch is used. Still want a fall back for completely
//  cutting power to the relay channels so I will need a dedicated powersupply
//  that I can operate with the pi and esp32.

// add timining advance and fuel pump toggle, this is completely not needed for
// normal operation but will help in maintenance and diagnostics I could also
// fix the blower motor flap with a stepper motor...
#include "relay.h"
#include "driver/gpio.h"
#include "events.h"
#include "event_bus.h"
#include "types.h"
#include "telemetry.h"
#include "mcp23x17.h"
#include <algorithm>

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

esp_err_t togglePinState(uint8_t pin, bool *newState) {
  bool currentState;
  esp_err_t err = relayState(pin, &currentState);

  if (err != ESP_OK) {
    return err;
  }

  // Currently ON -> write HIGH; currently OFF -> write LOW.
  err = mcp23x17_set_level(&mcp, pin, currentState ? 1 : 0);

  if (err != ESP_OK) {
    return err;
  }

  *newState = !currentState;
  return ESP_OK;
}

esp_err_t relayState(uint8_t pin, bool *state) {
  uint32_t level = 1;
  esp_err_t err = mcp23x17_get_level(&mcp, pin, &level);
  if (err != ESP_OK) {
    return err;
  }
  *state = (level == 0);
  return ESP_OK;
}

esp_err_t allOffSequence() {
  esp_err_t err = ESP_FAIL;
  for (uint8_t pin : relayPins) {
    err = mcp23x17_set_level(&mcp, pin, 1);
    if (err != ESP_OK) {
      printf("Pin: %d toggle failed: %s\n", pin, esp_err_to_name(err));
    }
  }
  if (err != ESP_OK) {
    return err;
  }
  vehicleState.acc = false;
  vehicleState.glowPlugs = false;
  vehicleState.ign = OFF;
  vehicleState.headlights = false;
  vehicleState.runningLights = false;
  vehicleState.heater = false;
  return ESP_OK;
}

// One °F step per 20 °F below 220 °F; a warm engine gets one second.
static int glowPlugSeconds(int16_t coolantF) {
  if (coolantF > 140) {
    return 1;
  }
  return std::clamp((220 - coolantF) / 20, 1, 12);
}

esp_err_t glowPlugSequence() {
  esp_err_t err = relayState(GLOW_PLUGS, &vehicleState.glowPlugs);
  if (err != ESP_OK) {
    printf("GLOW_PLUGS read state failed: %s\n", esp_err_to_name(err));
    return err;
  }
  if (vehicleState.glowPlugs) {
    err = togglePinState(GLOW_PLUGS, &vehicleState.glowPlugs);
    if (err != ESP_OK) {
      printf("GLOW_PLUGS toggle failed: %s\n", esp_err_to_name(err));
      return err;
    }
  }

  const int toggleTime = glowPlugSeconds(vehicleState.coolantTemp);

  err = togglePinState(GLOW_PLUGS, &vehicleState.glowPlugs);

  if (err != ESP_OK) {
    printf("GLOW_PLUGS toggle failed: %s\n", esp_err_to_name(err));
    return err;
  }

  vTaskDelay(pdMS_TO_TICKS(toggleTime * 1000));

  err = togglePinState(GLOW_PLUGS, &vehicleState.glowPlugs);
  if (err != ESP_OK) {
    printf("GLOW_PLUGS toggle failed: %s\n", esp_err_to_name(err));
    return err;
  }
  return ESP_OK;
}

esp_err_t accSequence() {
  if (!vehicleState.acc) {
    esp_err_t err = togglePinState(ACC, &vehicleState.acc);
    if (err != ESP_OK) {
      printf("ACC toggle failed: %s\n", esp_err_to_name(err));
      return err;
    }
  }
  return ESP_OK;
}

esp_err_t runningLightsSequence() {
  esp_err_t err = togglePinState(RUNNING_LIGHTS, &vehicleState.runningLights);
  if (err != ESP_OK) {
    printf("RUNNING_LIGHTS toggle failed: %s\n", esp_err_to_name(err));
    return err;
  }
  return ESP_OK;
}

esp_err_t headLightsSequence() {
  esp_err_t err = togglePinState(HEADLIGHTS, &vehicleState.headlights);
  if (err != ESP_OK) {
    printf("HEADLIGHTS toggle failed: %s\n", esp_err_to_name(err));
    return err;
  }
  return ESP_OK;
}

esp_err_t heaterSequence() {
  esp_err_t err = togglePinState(HEATER, &vehicleState.heater);
  if (err != ESP_OK) {
    printf("HEATER toggle failed: %s\n", esp_err_to_name(err));
    return err;
  }
  return ESP_OK;
}

esp_err_t ignSequence() {
  bool ignState;
  esp_err_t err = accSequence();
  if (err != ESP_OK) {
    return err;
  }

  // Running lights are not a start prerequisite; report and continue.
  err = runningLightsSequence();
  if (err != ESP_OK) {
    printf("RUNNING_LIGHTS failed: %s\n", esp_err_to_name(err));
  }
  switch (vehicleState.ign) {
  case OFF:
    ignState = false;
    break;
  case START:
    ignState = true;
    break;
  case ON:
    printf("Vehicle already started!");
    break;
  }
  if (vehicleState.ign == ON) {
    return ESP_FAIL;
  }

  err = glowPlugSequence();
  if (err != ESP_OK) {
    printf("GLOW_PLUGS failed; aborting start: %s\n", esp_err_to_name(err));
    return err;
  }

  err = togglePinState(IGN, &ignState);
  if (err != ESP_OK) {
    printf("IGN first toggle failed: %s\n", esp_err_to_name(err));
    return err;
  }
  vehicleState.ign = START;

  // need to check alternator and/or rpm
  vTaskDelay(pdMS_TO_TICKS(2000));

  err = togglePinState(IGN, &ignState);
  if (err != ESP_OK) {
    printf("IGN second toggle failed: %s\n", esp_err_to_name(err));
    return err;
  }
  vehicleState.ign = ON;
  return ESP_OK;
}

void relayTask(void *arg) {
  // Receive relay events from LoRa or Serial via the Raspberry Pi/modem,
  // then notify telemetry when a relay state changes.

  if (initRelay() != ESP_OK) {
    printf("Relay initialization failed.\n");
    vTaskDelete(nullptr);
    return;
  }

  EventToggle event;

  while (true) {
    // break on critical systems, treat the rest as warnings
    if (xQueueReceive(relayQueue, &event, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    esp_err_t err = ESP_OK;

    switch (event.type) {

    case EVENT_TOGGLE_ACC: {
      err = accSequence();
      if (err != ESP_OK) {
        break;
      }
      xSemaphoreGive(telemetrySemaphore);
      break;
    }

    case EVENT_TOGGLE_IGN: {
      printf("DEBUG: IGN event received\n");

      err = ignSequence();

      if (err != ESP_OK) {
        printf("DEBUG: ignSequence failed: %s\n", esp_err_to_name(err));
        break;
      }

      printf("DEBUG: ignSequence completed\n");
      xSemaphoreGive(telemetrySemaphore);
      break;
    }

    case EVENT_TOGGLE_HEATER:
      heaterSequence();
      xSemaphoreGive(telemetrySemaphore);
      break;

    case EVENT_TOGGLE_GLOWPLUGS:
      glowPlugSequence();
      xSemaphoreGive(telemetrySemaphore);
      break;

    case EVENT_TOGGLE_HEADLIGHTS:
      headLightsSequence();
      xSemaphoreGive(telemetrySemaphore);
      break;

    case EVENT_TOGGLE_RUNNINGLIGHTS:
      runningLightsSequence();
      xSemaphoreGive(telemetrySemaphore);
      break;

    case EVENT_TOGGLE_ALL_OFF: {
      esp_err_t err = allOffSequence();
      if (err != ESP_OK) {
        printf("ERROR: could not toggle all relays\n");
      }
      xSemaphoreGive(telemetrySemaphore);
      break;
    }
    default:
      printf("Unknown relay event: %d\n", (int)event.type);
      break;
    }
  }
}
