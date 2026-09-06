#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "wifi_sta.h"

static const char *TAG = "wifi_sta";
static int s_retry_num = 0;

static void wifiEventHandler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START:
      ESP_LOGI(TAG, "WiFi started, connecting...");
      ESP_ERROR_CHECK(esp_wifi_connect());
      break;

    case WIFI_EVENT_STA_CONNECTED:
      ESP_LOGI(TAG, "Connected to AP");
      s_retry_num = 0;
      break;

    case WIFI_EVENT_STA_DISCONNECTED: {
      wifi_event_sta_disconnected_t *disc =
          (wifi_event_sta_disconnected_t *)event_data;
      ESP_LOGW(TAG, "Disconnected, reason: %d", disc->reason);

      if (s_retry_num < MAX_RETRY) {
        ESP_ERROR_CHECK(esp_wifi_connect());
        s_retry_num++;
        ESP_LOGI(TAG, "Retry %d/%d", s_retry_num, MAX_RETRY);
      } else {
        ESP_LOGE(TAG, "Max retries reached, giving up");
      }
      break;
    }

    default:
      break;
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
  }
}

void initWiFi(void) {
  // NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  }

  // Netif + event loop
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  // WiFi driver
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // Config
  wifi_config_t wifi_config = {};
  strncpy((char *)wifi_config.sta.ssid, SSID, sizeof(wifi_config.sta.ssid));
  strncpy((char *)wifi_config.sta.password, PSK,
          sizeof(wifi_config.sta.password));
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

  // Event handlers
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifiEventHandler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &wifiEventHandler, NULL));

  // Start
  ESP_ERROR_CHECK(esp_wifi_start());
}
