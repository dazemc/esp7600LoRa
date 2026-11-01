#include "adc_shared.h"

SemaphoreHandle_t adcSemaphore = nullptr;
adc_oneshot_unit_handle_t shared_adc_handle = nullptr;
AdcConfig adcConfig{};

void initSharedAdc() {
  adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = ADC_UNIT,
      .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };

  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &shared_adc_handle));

  adcSemaphore = xSemaphoreCreateMutex();
  assert(adcSemaphore != nullptr);
  adcConfig.handle = shared_adc_handle;
  adcConfig.adcUnit = ADC_UNIT;
}
