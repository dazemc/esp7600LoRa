#pragma once

#include "adc_cali_schemes.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "hal/adc_types.h"
#include <freertos/FreeRTOS.h>
#include <stdio.h>

#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_4
#define ADC_ATTEN ADC_ATTEN_DB_12

void voltageMonitorTask(void *arg);
uint8_t voltageToU8(float voltage);
float u8ToVoltage(uint8_t u8ToVoltage);
