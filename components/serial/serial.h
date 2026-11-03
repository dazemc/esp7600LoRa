#pragma once

#include "freertos/FreeRTOS.h"
#include "types.h"

#define BAUD 115200
#define TXD_PIN (GPIO_NUM_19)
#define RXD_PIN (GPIO_NUM_20)
#define UART_PORT UART_NUM_1
#define BUF_SIZE (1024)
#define MAX_LINE_LENGTH 128

void serialTask(void *arg);
void setWiFiResetHandler(void (*handler)());

const char *ignitionToString(Ignition ign);
extern QueueHandle_t serialQueue;
