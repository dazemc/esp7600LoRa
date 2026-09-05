#pragma once

#include <freertos/FreeRTOS.h>
#include "driver/gpio.h"

// 13, 17, 22, 23, 25, 33
#define IGN GPIO_NUM_25
#define ACC GPIO_NUM_22
#define HEADLIGHTS GPIO_NUM_17
#define RUNNING_LIGHTS GPIO_NUM_23
#define GLOW_PLUGS GPIO_NUM_33
#define HEATER GPIO_NUM_13

inline constexpr gpio_num_t relayPins[] = {
    IGN, ACC, HEADLIGHTS, RUNNING_LIGHTS, GLOW_PLUGS, HEATER,
};

void relayTask(void *arg);
void initRelay();
void relayCycleTest();
bool relayState(uint8_t pin);

extern QueueHandle_t relayQueue;
