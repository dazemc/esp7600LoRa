#pragma once

#include "stdint.h"

struct VoltageData {
  float raw;
  float adc;
  float battery;
};

struct LoRaPacket {
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
    LoRaPacket loraRecv;
    VehicleState vehicle;
    LoRaSend loraSend;
  };
};

enum EventWiFiType {
  EVENT_WIFI,
};

enum EventToggleType {
  EVENT_TOGGLE_HEADLIGHTS,
  EVENT_TOGGLE_ACC,
  EVENT_TOGGLE_RUNNINGLIGHTS,
  EVENT_TOGGLE_HEATER,
  EVENT_TOGGLE_GLOWPLUGS,
  EVENT_TOGGLE_IGN,
};

enum EventLoRaType {
  EVENT_LORA_TX,
  EVENT_LORA_RX,
  EVENT_LORA_SEND,
  EVENT_LORA_RECV,
};
