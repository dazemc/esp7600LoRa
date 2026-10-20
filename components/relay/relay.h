#pragma once

#include <freertos/FreeRTOS.h>
#include "driver/gpio.h"

/*
PIN | RELAY
0 -> 2
1 -> 4
2 -> 6
3 -> 8
4 -> 12
5 -> 10
6 -> 14
7 -> 16
8 -> 1
9 -> 3
10 -> 5
11 -> 7
12 -> 9
13 -> 13
14 -> 11
15 -> 15
*/
#define IGN 8            // r1
#define ACC 0            // r2
#define HEADLIGHTS 9     // r3
#define RUNNING_LIGHTS 1 // r4
#define GLOW_PLUGS 10    // r5
#define HEATER 2         // r6

#define I2C_PORT I2C_NUM_1
#define I2C_SDA GPIO_NUM_33
#define I2C_SCL GPIO_NUM_22
#define MCP_ADDR 0x20

inline constexpr uint8_t relayPins[] = {
    IGN, ACC, HEADLIGHTS, RUNNING_LIGHTS, GLOW_PLUGS, HEATER,
};

void relayTask(void *arg);
esp_err_t initRelay();
void relayCycleTest();
bool relayState(uint8_t pin);
void testMcp23017Pins();
void testAssignedPins();

extern QueueHandle_t relayQueue;
