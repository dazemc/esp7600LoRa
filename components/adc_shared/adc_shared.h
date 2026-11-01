#pragma once
#include "events.h"

#define ADC_UNIT ADC_UNIT_1

void initSharedAdc();

extern SemaphoreHandle_t adcSemaphore;
extern AdcConfig adcConfig;
