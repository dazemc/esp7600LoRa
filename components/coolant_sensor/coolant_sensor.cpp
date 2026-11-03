#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "types.h"
#include "coolant_sensor.h"
#include "adc_shared.h"
#include <cstdint>
#include <cmath>
#include <algorithm>

static constexpr float VCC = 3.3f;
static constexpr float R_FIXED = 1000.0f;
static constexpr int ADC_SAMPLE_COUNT = 32;
// Sentinel that drives the relay's maximum glow time (12 s).
static constexpr int16_t COOLANT_FAULT_F = -512;

// Steinhart-Hart coefficients
static constexpr float STEINHART_A = 2.1854771452e-03f;
static constexpr float STEINHART_B = 1.4835849337e-04f;
static constexpr float STEINHART_C = 1.9441476833e-06f;

static adc_cali_handle_t cali_handle = nullptr;
static const char *TAG = "thermistor";

// Only configures the channel + calibration. Does NOT create the unit.
static void initThermistorMonitor(adc_oneshot_unit_handle_t handle,
                                  adc_unit_t unit) {
  adc_oneshot_chan_cfg_t chan_config = {
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH_12,
  };
  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(handle, THERMISTOR_ADC_CHANNEL, &chan_config));

  adc_cali_curve_fitting_config_t cali_config = {
      .unit_id = unit,
      .chan = THERMISTOR_ADC_CHANNEL,
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH_12,
  };
  ESP_ERROR_CHECK(
      adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle));
}

static int readAveragedADC(adc_oneshot_unit_handle_t handle) {
  int64_t raw_sum = 0;
  for (int i = 0; i < ADC_SAMPLE_COUNT; i++) {
    int raw;
    xSemaphoreTake(adcSemaphore, portMAX_DELAY);
    ESP_ERROR_CHECK(adc_oneshot_read(handle, THERMISTOR_ADC_CHANNEL, &raw));
    xSemaphoreGive(adcSemaphore);
    raw_sum += raw;
    vTaskDelay(pdMS_TO_TICKS(2));
  }
  return static_cast<int>(raw_sum / ADC_SAMPLE_COUNT);
}

static float thermistorResistance(float voltage) {
  if (voltage <= 0.0f || voltage >= VCC) {
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
  AdcConfig *cfg = static_cast<AdcConfig *>(arg);
  adc_oneshot_unit_handle_t adc_handle = cfg->handle;

  initThermistorMonitor(adc_handle,
                        cfg->adcUnit); // or hard-code ADC_UNIT_1 if you prefer

  while (true) {
    const int raw = readAveragedADC(adc_handle);
    int voltage_mv;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &voltage_mv));

    const float adc_voltage = voltage_mv / 1000.0f;
    const float resistance = thermistorResistance(adc_voltage);

    static bool faultLogged = false;
    int16_t coolantF;
    if (resistance <= 0.0f) {
      if (!faultLogged) {
        ESP_LOGE(TAG, "coolant sensor fault at %.3f V; max-glow fallback",
                 adc_voltage);
        faultLogged = true;
      }
      coolantF = COOLANT_FAULT_F;
    } else {
      if (faultLogged) {
        ESP_LOGW(TAG, "coolant sensor recovered");
        faultLogged = false;
      }
      const float temperature = thermistorTemperature(resistance);
      // Engineering value in °F; only the wire conversion packs 10 bits.
      coolantF = static_cast<int16_t>(std::lroundf(temperature));
      coolantF = std::clamp<int16_t>(coolantF, -512, 511);
    }

    vehicleState.coolantTemp = coolantF;

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
