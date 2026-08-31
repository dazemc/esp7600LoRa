#pragma once

#include <Arduino.h>

// 13, 17, 22, 23, 25, 33
#define IGN 25
#define ACC 22
#define HEADLIGHTS 17
#define RUNNING_LIGHTS 23
#define GLOW_PLUGS 33
#define HEATER 13

inline constexpr int relayPins[] = {
    IGN, ACC, HEADLIGHTS, RUNNING_LIGHTS, GLOW_PLUGS, HEATER,
};

void relayTask(void *arg);
void initRelay();
void relayCycleTest();
bool relayState(uint8_t pin);
