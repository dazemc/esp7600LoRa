#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "events.h"
#include "event_bus.h"
#include "types.h"
#include "coolant_sensor.h"
#include "serial.h"
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cmath>

static constexpr float VCC = 3.3f;
static constexpr float R_FIXED = 1000.0f;
static constexpr int ADC_SAMPLE_COUNT = 32;

// Steinhart-Hart coefficients from calibration points:
// 57.6°F → 386.0 Ω
// 126.0°F → 99.5 Ω
// 195.0°F → 27.7 Ω
static constexpr float STEINHART_A = 2.1854771452e-03f;
static constexpr float STEINHART_B = 1.4835849337e-04f;
static constexpr float STEINHART_C = 1.9441476833e-06f;

static adc_oneshot_unit_handle_t adc_handle;
static adc_oneshot_unit_init_cfg_t init_config{};
static adc_cali_handle_t cali_handle;
static const char *TAG = "thermistor";

static void initThermistorMonitor() {
  init_config.unit_id = ADC_UNIT;
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

  adc_oneshot_chan_cfg_t chan_config = {
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH_12,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, THERMISTOR_ADC_CHANNEL,
                                             &chan_config));

  cali_handle = NULL;
  adc_cali_curve_fitting_config_t cali_config = {
      .unit_id = ADC_UNIT,
      .chan = THERMISTOR_ADC_CHANNEL,
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH_12,
  };
  ESP_ERROR_CHECK(
      adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle));
}

static int readAveragedADC() {
  int64_t raw_sum = 0;
  for (int i = 0; i < ADC_SAMPLE_COUNT; i++) {
    int raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, THERMISTOR_ADC_CHANNEL, &raw));
    raw_sum += raw;
    vTaskDelay(pdMS_TO_TICKS(2));
  }
  return static_cast<int>(raw_sum / ADC_SAMPLE_COUNT);
}

static float thermistorResistance(float voltage) {
  if (voltage <= 0.0f || voltage >= VCC) {
    ESP_LOGE(TAG, "Invalid ADC voltage: %.3f V", voltage);
    return 0.0f;
  }
  return R_FIXED * voltage / (VCC - voltage);
}

static float thermistorTemperature(float resistance) {
  if (resistance <= 0.0f) {
    return -999.0f;
  }

  const float lnR = logf(resistance);
  const float invT =
      STEINHART_A + STEINHART_B * lnR + STEINHART_C * lnR * lnR * lnR;
  const float tempK = 1.0f / invT;

  // Kelvin → Fahrenheit
  return (tempK - 273.15f) * 9.0f / 5.0f + 32.0f;
}

void thermistorMonitorTask(void *arg) {
  initThermistorMonitor();

  while (true) {
    const int raw = readAveragedADC();
    int voltage_mv;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &voltage_mv));

    const float adc_voltage = voltage_mv / 1000.0f;
    const float resistance = thermistorResistance(adc_voltage);
    const float temperature = thermistorTemperature(resistance);

    EventSerial event{};
    event.type = EVENT_SERIAL_IN;
    std::snprintf(event.serialOut.line, sizeof(event.serialOut.line),
                  "Temperature: %.2f F\n"
                  "Raw Average: %d\n"
                  "ADC: %.3f V\n"
                  "Resistance: %.2f Ohms\n",
                  temperature, raw, adc_voltage, resistance);
    xQueueSend(serialQueue, &event, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
