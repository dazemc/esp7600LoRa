#include "utils.h"
#include "stdint.h"
#include "stddef.h"
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "types.h"
#include "esp_log.h"

const bool DEBUG = true;
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
                                        uint8_t packetIdTX) {

  LoRaCompactPacket packet{};
  packet.packetId = incrementPacketId(packetIdTX);
  packet.isVehicle = isVehicle;
  packet.ign = loraPacket.vehicle.ign;
  packet.headlights = loraPacket.vehicle.headlights;
  packet.acc = loraPacket.vehicle.acc;
  packet.runningLights = loraPacket.vehicle.runningLights;
  packet.heater = loraPacket.vehicle.heater;
  packet.glowPlugs = loraPacket.vehicle.glowPlugs;
  packet.isWifiEnabled = false;
  packet.encodedVoltage = loraPacket.vehicle.voltageData.encodedVoltage;
  return packet;
}

LoRaPacket unpackPacket(LoRaCompactPacket packet) {
  LoRaPacket unpacked{};

  unpacked.header.packetId = packet.packetId;
  unpacked.isVehicle = packet.isVehicle;

  unpacked.vehicle.ign = unpackIgnition(packet.ign);
  unpacked.vehicle.headlights = packet.headlights;
  unpacked.vehicle.acc = packet.acc;
  unpacked.vehicle.runningLights = packet.runningLights;
  unpacked.vehicle.heater = packet.heater;
  unpacked.vehicle.glowPlugs = packet.glowPlugs;

  unpacked.vehicle.voltageData.encodedVoltage = packet.encodedVoltage;

  return unpacked;
}
