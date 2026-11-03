#pragma once
#include "sdkconfig.h"
#include "stdint.h"
#include "stdio.h"
#include "types.h"

#ifdef CONFIG_TELEMETRY_DEBUG
constexpr bool DEBUG = true;
#else
constexpr bool DEBUG = false;
#endif

extern bool isVehicle;

void debugRemainingStackSize(const char *taskName, char *loc,
                             const size_t remaining);

void debugRemainingQueue(const char *queueName, char *loc,
                         const uint8_t remaining);

uint8_t incrementPacketId(uint8_t &packetIdTX);

LoRaCompactPacket packetToCompactPacket(LoRaPacket loraPacket,
                                        uint8_t packetId);

LoRaPacket unpackPacket(LoRaCompactPacket packet);
