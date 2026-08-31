#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "events.h"
#include "event_bus.h"
#include "types.h"
#include "voltage.h"
#include "telemetry.h"

static adc_oneshot_unit_handle_t adc_handle;
static adc_oneshot_unit_init_cfg_t init_config{};
static adc_cali_handle_t cali_handle;

static void initVoltageMonitor() {
  init_config.unit_id = ADC_UNIT;

  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

  adc_oneshot_chan_cfg_t chan_config = {
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH_12,
  };

  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config));

  cali_handle = NULL;

  adc_cali_line_fitting_config_t cali_config = {.unit_id = ADC_UNIT,
                                                .atten = ADC_ATTEN,
                                                .bitwidth = ADC_BITWIDTH_12,
                                                .default_vref = 1100};

  ESP_ERROR_CHECK(
      adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle));
}
void voltageMonitorTask(void *arg) {
  initVoltageMonitor();
  while (true) {
    int raw;
    int voltage_mv;

    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw));
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &voltage_mv));

    float adc_voltage = voltage_mv / 1000.0f;
    float battery_voltage = adc_voltage * 5.6f;
    vehicleState.voltageData.raw = raw;
    vehicleState.voltageData.adc = adc_voltage;
    vehicleState.voltageData.battery = battery_voltage;
    xSemaphoreGive(telemetrySemaphore);

    // printf("Raw: %d\nADC: %.3f V\nBattery: %.2f V\n", raw, adc_voltage,
    // battery_voltage);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
