#pragma once

#include "freertos/FreeRTOS.h"

inline constexpr long LORA_FREQ = 915E6;
inline constexpr int LORA_CS = 18;
inline constexpr int LORA_SCK = 5;
inline constexpr int LORA_MOSI = 27;
inline constexpr int LORA_MISO = 19;
inline constexpr int LORA_RST = 14;
inline constexpr int LORA_DIO0 = 26;
inline constexpr int LORA_DIO1 = 35;

void initLoRa();
void sendLoRaTask(void *arg);
void recvLoRaTask(void *arg);
void onReceive(int packetSize);

extern QueueHandle_t loraTXQueue;
extern QueueHandle_t loraRXQueue;
extern bool isVehicle;
