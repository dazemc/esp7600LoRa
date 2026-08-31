#pragma once

#include "driver/i2c_master.h"
#include <Arduino.h>
#include <ssd1306.h>

#define OLED_SDA GPIO_NUM_4
#define OLED_SCL GPIO_NUM_15
#define OLED_RST GPIO_NUM_16
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

void displayTask(void *arg);

struct DisplayData {
  ssd1306_handle_t disp;
  char message[128];
  int OledX;
  int OledY;
};

extern DisplayData displayData;
extern QueueHandle_t displayQueue;
