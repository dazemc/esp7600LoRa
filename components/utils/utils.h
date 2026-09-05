#pragma once
#include "stdint.h"
#include "stdio.h"
#include "types.h"

extern const bool DEBUG;
extern bool isVehicle;

void debugRemainingStackSize(const char *taskName, char *loc,
                             const size_t remaining);

void debugRemainingQueue(const char *queueName, char *loc,
                         const uint8_t remaining);

uint8_t incrementPacketId(uint8_t &packetIdTX);

LoRaCompactPacket packetToCompactPacket(LoRaPacket loraPacket,
                                        uint8_t packetIdTX);

LoRaPacket unpackPacket(LoRaCompactPacket packet);
