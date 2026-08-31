#pragma once

#include "types.h"

struct EventToggle {
  EventToggleType type;
};

struct EventLoRa {
  EventLoRaType type;
  union {
    LoRaPacket loraRecv;
    VehicleState vehicle;
    LoRaSend loraSend;
  };
};
