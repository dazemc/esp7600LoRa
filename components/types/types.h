#pragma once

#include "stdint.h"
#include <cstddef>
#include "freertos/FreeRTOS.h"
#include "hal/adc_types.h"
#include "esp_adc/adc_oneshot.h"

struct TaskConfig {
  TaskFunction_t function;
  const char *name;
  uint32_t stackSize;
  void *arg;
  UBaseType_t priority;
  TaskHandle_t *handle;
};

struct QueueConfig {
  QueueHandle_t *handle;
  size_t length;
  size_t itemSize;
};

struct AdcConfig {
  adc_oneshot_unit_handle_t handle;
  adc_unit_t adcUnit;
};

struct SemaphoreConfig {
  SemaphoreHandle_t *handle;
};

struct EventConfig {
  const TaskConfig taskConfig;
  const QueueConfig *queueConfig = nullptr;
};

struct VoltageData {
  float raw;
  float adc;
  float battery;
  uint8_t encodedVoltage;
};

struct LoRaRawPacket {
  int rssi;
  float snr;
};

enum Ignition {
  OFF,
  ON,
  START,
};

struct VehicleState {
  VoltageData voltageData;
  Ignition ign;
  bool headlights;
  bool acc;
  bool runningLights;
  bool heater;
  bool glowPlugs;
  int16_t coolantTemp; // engineering °F; packed to 10 bits on air
} __attribute__((packed));

extern VehicleState vehicleState;

// this is really just to check for dropped packets so 8 is enough
struct Header {
  uint8_t packetId : 3;
} __attribute__((packed));

struct LoRaPacket {
  Header header;
  VehicleState vehicle;
  bool isWifi;
  bool isVehicle;
} __attribute__((packed));

struct LoRaCompactPacket {
  // Byte 0
  uint8_t packetId : 3;
  uint8_t isVehicle : 1;
  uint8_t flags : 4;

  // Byte 1
  uint8_t ign : 2;
  uint8_t headlights : 1;
  uint8_t acc : 1;
  uint8_t runningLights : 1;
  uint8_t heater : 1;
  uint8_t glowPlugs : 1;
  uint8_t isWifiEnabled : 1;

  // Byte 2
  uint8_t encodedVoltage;

  // Bytes 3-4
  uint16_t encodedCoolantTemperature : 10;
  uint16_t extraData : 6;
} __attribute__((packed));

// The frame may grow, but it must never gain padding. Bump the size when a
// field is added and update `.llm/hardware.md` in the same step.
static_assert(sizeof(LoRaCompactPacket) == 5,
              "LoRaCompactPacket changed size; update both sides and the docs");
static_assert(offsetof(LoRaCompactPacket, encodedVoltage) == 2,
              "LoRaCompactPacket gained padding before byte 2");


