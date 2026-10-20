#pragma once

#include "freertos/FreeRTOS.h"
#include "types.h"

#define BAUD 115200
#define TXD_PIN (GPIO_NUM_1)
#define RXD_PIN (GPIO_NUM_3)
#define UART_PORT UART_NUM_0
#define BUF_SIZE (1024)
#define MAX_LINE_LENGTH 128

void serialTask(void *arg);

const char *ignitionToString(Ignition ign);
extern QueueHandle_t serialQueue;
