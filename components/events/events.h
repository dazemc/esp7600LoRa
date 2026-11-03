#pragma once

#include "types.h"

enum EventToggleType {
  EVENT_TOGGLE_HEADLIGHTS,
  EVENT_TOGGLE_ACC,
  EVENT_TOGGLE_RUNNINGLIGHTS,
  EVENT_TOGGLE_HEATER,
  EVENT_TOGGLE_GLOWPLUGS,
  EVENT_TOGGLE_IGN,
  EVENT_TOGGLE_ALL_OFF,
};

enum EventLoRaTXType {
  EVENT_LORA_TX,
};

enum EventDisplayType {
  EVENT_DISPLAY_LORA_RX,
  EVENT_DISPLAY_LORA_TX,
};

enum EventSerialType {
  EVENT_SERIAL_LORA_RX,
  EVENT_SERIAL_LORA_TX,
  EVENT_SERIAL_DEBUG,
  EVENT_SERIAL_IN,
  EVENT_SERIAL_OUT,
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
  LoRaPacket loraPacket;
};

struct EventLoRaRX {
  LoRaRawPacket loraRawPacket;
  LoRaPacket loraPacket;
  uint32_t droppedPackets;
};

struct EventDisplay {
  EventDisplayType type;
  union {
    EventLoRaTX loraTX;
    EventLoRaRX loraRX;
  };
};

struct EventSerialIn {
  char line[128];
};

struct EventSerialOut {
  char line[128];
};

struct EventSerial {
  EventSerialType type;
  union {
    EventLoRaTX loraTX;
    EventLoRaRX loraRX;
    EventDebug debug;
    EventSerialIn serialIn;
    EventSerialOut serialOut;
  };
};
