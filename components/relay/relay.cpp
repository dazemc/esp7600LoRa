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
  }
  if (err != ESP_OK) {
    return err;
  }
  return ESP_OK;
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
  err = togglePinState(GLOW_PLUGS, &vehicleState.glowPlugs);

  if (err != ESP_OK) {
    printf("GLOW_PLUGS toggle failed: %s\n", esp_err_to_name(err));
    return err;
  }
  vTaskDelay(pdMS_TO_TICKS(4000));
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
  runningLightsSequence();
  if (err != ESP_OK) {
    return err;
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
  glowPlugSequence();
  err = togglePinState(IGN, &ignState);
  if (err != ESP_OK) {
    printf("IGN first toggle failed: %s\n", esp_err_to_name(err));
    return err;
  }

  vTaskDelay(pdMS_TO_TICKS(2000));

  err = togglePinState(IGN, &ignState);
  if (err != ESP_OK) {
    printf("IGN second toggle failed: %s\n", esp_err_to_name(err));
    return err;
  }
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
