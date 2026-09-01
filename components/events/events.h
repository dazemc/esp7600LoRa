#pragma once

#include "types.h"

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

enum EventLoRaTXType {
  EVENT_LORA_TX,
  EVENT_LORA_SEND,
};

enum EventLoRaRXType {
  EVENT_LORA_RX,
  EVENT_LORA_RECV,
};

enum EventDisplayType {
  EVENT_DISPLAY_LORA_RX,
  EVENT_DISPLAY_LORA_TX,
  EVENT_DISPLAY_LORA_TOGGLE,
  EVENT_DISPLAY_LORA_WIFI,
};

enum EventSerialType {
  EVENT_SERIAL_LORA_RX,
  EVENT_SERIAL_LORA_TX,
  EVENT_SERIAL_LORA_TOGGLE,
  EVENT_SERIAL_LORA_WIFI,
  EVENT_SERIAL_DEBUG,
};

struct EventDebug {
  char remainingStackMsg[64];
  char remainingQueueMsg[64];
};

struct EventToggle {
  EventToggleType type;
};

struct EventLoRaTX {
  EventLoRaTXType type;
  bool isVehicle;
  LoRaSend loraSend;
};

struct EventLoRaRX {
  EventLoRaRXType type;
  LoRaRecv loraRecv;
  LoRaSend loraSend;
};

struct EventDisplay {
  EventDisplayType type;
  union {
    EventLoRaTX loraTX;
    EventLoRaRX loraRX;
    EventToggle toggle;
  };
};

struct EventSerial {
  EventSerialType type;
  union {
    EventLoRaTX loraTX;
    EventLoRaRX loraRX;
    EventToggle toggle;
    EventDebug debug;
  };
};
