#include "utils.h"
#include "stdint.h"
#include "stddef.h"
#include "stdio.h"
#include <algorithm>
#include "freertos/FreeRTOS.h"
#include "types.h"
#include "esp_log.h"

const char *TAG = "utils";

uint8_t incrementPacketId(uint8_t &packetId) {
  packetId = (packetId + 1) & 0x07;
  return packetId;
}

void debugRemainingStackSize(const char *taskName, char *loc,
                             const size_t remaining) {
  size_t remainingBytes = remaining * sizeof(StackType_t);
  snprintf(loc, 64,
           "\033[1;34mDEBUG: %s stack size remaining: %zu bytes\033[0m\n",
           taskName, remainingBytes);
}

void debugRemainingQueue(const char *queueName, char *loc,
                         const uint8_t remaining) {
  snprintf(loc, 64, "\033[1;34mDEBUG: %s queue remaining: %d\033[0m\n",
           queueName, remaining);
}

// 10-bit two's-complement °F on air; the state itself is engineering °F.
static uint16_t packCoolantTemperature(int16_t tempF) {
  const int16_t clamped = std::clamp<int16_t>(tempF, -512, 511);
  return static_cast<uint16_t>(clamped) & 0x03FF;
}

static int16_t unpackCoolantTemperature(uint16_t raw) {
  raw &= 0x03FF;
  return (raw & 0x0200) ? static_cast<int16_t>(raw | 0xFC00)
                        : static_cast<int16_t>(raw);
}

Ignition unpackIgnition(uint8_t ign) {
  switch (ign) {
  case (0):
    return OFF;
  case (1):
    return ON;
  case (2):
    return START;
  default:
    ESP_LOGE(TAG, "Unrecognized Ignition enum value %d \n", ign);
    return OFF;
  }
}

LoRaCompactPacket packetToCompactPacket(LoRaPacket loraPacket,
                                        uint8_t packetId) {

  LoRaCompactPacket packet{};
  // Caller owns the increment (buildPacket / test branch set header + global
  // once); the air ID must equal the logged header ID.
  packet.packetId = packetId;
  packet.isVehicle = loraPacket.isVehicle;
  packet.ign = loraPacket.vehicle.ign;
  packet.headlights = loraPacket.vehicle.headlights;
  packet.acc = loraPacket.vehicle.acc;
  packet.runningLights = loraPacket.vehicle.runningLights;
  packet.heater = loraPacket.vehicle.heater;
  packet.glowPlugs = loraPacket.vehicle.glowPlugs;
  packet.isWifiEnabled = false;
  packet.encodedVoltage = loraPacket.vehicle.voltageData.encodedVoltage;
  packet.encodedCoolantTemperature =
      packCoolantTemperature(loraPacket.vehicle.coolantTemp);
  return packet;
}

LoRaPacket unpackPacket(LoRaCompactPacket packet) {
  LoRaPacket unpacked{};

  unpacked.header.packetId = packet.packetId;
  unpacked.isVehicle = packet.isVehicle;
  unpacked.isWifi = packet.isWifiEnabled;

  unpacked.vehicle.ign = unpackIgnition(packet.ign);
  unpacked.vehicle.headlights = packet.headlights;
  unpacked.vehicle.acc = packet.acc;
  unpacked.vehicle.runningLights = packet.runningLights;
  unpacked.vehicle.heater = packet.heater;
  unpacked.vehicle.glowPlugs = packet.glowPlugs;

  unpacked.vehicle.voltageData.encodedVoltage = packet.encodedVoltage;
  unpacked.vehicle.coolantTemp =
      unpackCoolantTemperature(packet.encodedCoolantTemperature);

  return unpacked;
}
