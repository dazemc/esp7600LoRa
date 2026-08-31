#pragma once

#include "stdint.h"
#include "freertos/FreeRTOS.h"

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
};

struct LoRaRecv {
  uint8_t data[256];
  int length;
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
} __attribute__((packed));

extern VehicleState vehicleState;

struct LoRaSend {
  VehicleState vehicle;
  bool wifi;
} __attribute__((packed));

struct Telemetry {
  union {
    LoRaRecv loraRecv;
    VehicleState vehicle;
    LoRaSend loraSend;
  };
};
