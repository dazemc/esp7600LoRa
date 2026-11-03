#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

inline constexpr long LORA_FREQ = 915E6;
inline constexpr int LORA_CS = 8;
inline constexpr int LORA_SCK = 9;
inline constexpr int LORA_MOSI = 10;
inline constexpr int LORA_MISO = 11;
inline constexpr int LORA_RST = 12;
// inline constexpr int LORA_DIO0 = 26;
inline constexpr int LORA_BUSY = 13;
inline constexpr int LORA_DIO1 = 14;

esp_err_t initLoRa();
void sendLoRaTask(void *arg);
void recvLoRaTask(void *arg);

extern QueueHandle_t loraTXQueue;
extern QueueHandle_t loraRXQueue;
