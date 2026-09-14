#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "mdns.h"
#include "cJSON.h"
#include "wifi_sta.h"

static const char *TAG = "wifi_sta";
static int s_retry_num = 0;
static bool s_sta_connected = false;
static bool s_provisioned = false;
static esp_netif_ip_info_t s_sta_ip = {};
static httpd_handle_t s_server = nullptr;

static bool loadCreds(char *ssid, size_t ssidLen, char *pass,
                       size_t passLen) {
  nvs_handle_t h;
  if (nvs_open("esp7600", NVS_READONLY, &h) != ESP_OK) {
    return false;
  }
  size_t sLen = ssidLen, pLen = passLen;
  bool ok = nvs_get_str(h, "sta_ssid", ssid, &sLen) == ESP_OK &&
            nvs_get_str(h, "sta_pass", pass, &pLen) == ESP_OK && sLen > 1;
  nvs_close(h);
  return ok;
}

static esp_err_t statusHandler(httpd_req_t *req) {
  char ssid[33] = "";
  char pass[65];
  bool provisioned = loadCreds(ssid, sizeof(ssid), pass, sizeof(pass));
  char ipStr[16] = "";
  if (s_sta_connected) {
    esp_ip4addr_ntoa(&s_sta_ip.ip, ipStr, sizeof(ipStr));
  }
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "mode", s_sta_connected ? "sta" : "setup");
  cJSON_AddBoolToObject(root, "sta_connected", s_sta_connected);
  cJSON_AddStringToObject(root, "sta_ssid", provisioned ? ssid : "");
  cJSON_AddBoolToObject(root, "provisioned", provisioned);
  cJSON_AddStringToObject(root, "ip", ipStr);
  char *body = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, body ? body : "{}");
  cJSON_free(body);
  return ESP_OK;
}

static esp_err_t wifiPostHandler(httpd_req_t *req) {
  if (req->content_len <= 0 || req->content_len > 512) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad length");
    return ESP_FAIL;
  }
  char *buf = (char *)malloc(req->content_len + 1);
  if (!buf) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  int read = httpd_req_recv(req, buf, req->content_len);
  if (read <= 0) {
    free(buf);
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  buf[read] = '\0';

  char ssid[33] = "";
  char pass[65] = "";
  cJSON *root = cJSON_Parse(buf);
  free(buf);
  if (!root) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad json");
    return ESP_FAIL;
  }
  const cJSON *jSsid = cJSON_GetObjectItem(root, "ssid");
  const cJSON *jPass = cJSON_GetObjectItem(root, "pass");
  if (cJSON_IsString(jSsid)) {
    strncpy(ssid, jSsid->valuestring, sizeof(ssid) - 1);
  }
  if (cJSON_IsString(jPass)) {
    strncpy(pass, jPass->valuestring, sizeof(pass) - 1);
  }
  cJSON_Delete(root);
  if (ssid[0] == '\0' || strlen(pass) < 8) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                        "need ssid and pass >= 8 chars");
    return ESP_FAIL;
  }

  nvs_handle_t h;
  if (nvs_open("esp7600", NVS_READWRITE, &h) != ESP_OK ||
      nvs_set_str(h, "sta_ssid", ssid) != ESP_OK ||
      nvs_set_str(h, "sta_pass", pass) != ESP_OK ||
      nvs_commit(h) != ESP_OK) {
    nvs_close(h);
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  nvs_close(h);

  ESP_LOGI(TAG, "provisioned, restarting to join %s", ssid);
  httpd_resp_sendstr(req, "{\"ok\":true,\"restarting\":true}");
  vTaskDelay(pdMS_TO_TICKS(500));
  esp_restart();
  return ESP_OK;
}

static void startMdns(void) {
  if (mdns_init() != ESP_OK) {
    ESP_LOGE(TAG, "mdns init failed");
    return;
  }
  mdns_hostname_set("esp7600lora");
  mdns_instance_name_set("esp7600LoRa base");
  mdns_service_add("esp7600LoRa", "_http", "_tcp", 80, nullptr, 0);
  mdns_service_txt_item_set("_http", "_tcp", "board", "recv");
  ESP_LOGI(TAG, "mDNS up: esp7600lora.local");
}

static void startHttpd(void) {
  if (s_server) {
    return;
  }
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  if (httpd_start(&s_server, &config) != ESP_OK) {
    ESP_LOGE(TAG, "httpd start failed");
    return;
  }
  httpd_uri_t statusUri = {
      .uri = "/api/status",
      .method = HTTP_GET,
      .handler = statusHandler,
  };
  httpd_uri_t wifiUri = {
      .uri = "/api/wifi",
      .method = HTTP_POST,
      .handler = wifiPostHandler,
  };
  httpd_register_uri_handler(s_server, &statusUri);
  httpd_register_uri_handler(s_server, &wifiUri);
  ESP_LOGI(TAG, "REST API up: GET /api/status, POST /api/wifi");
}

static void wifiEventHandler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START:
      if (s_provisioned) {
        ESP_LOGI(TAG, "WiFi started, connecting...");
        ESP_ERROR_CHECK(esp_wifi_connect());
      } else {
        ESP_LOGI(TAG, "WiFi started, STA idle until provisioned");
      }
      break;

    case WIFI_EVENT_STA_CONNECTED:
      ESP_LOGI(TAG, "Connected to AP");
      s_retry_num = 0;
      break;

    case WIFI_EVENT_STA_DISCONNECTED: {
      wifi_event_sta_disconnected_t *disc =
          (wifi_event_sta_disconnected_t *)event_data;
      s_sta_connected = false;
      ESP_LOGW(TAG, "Disconnected, reason: %d", disc->reason);

      if (s_retry_num < MAX_RETRY) {
        ESP_ERROR_CHECK(esp_wifi_connect());
        s_retry_num++;
        ESP_LOGI(TAG, "Retry %d/%d", s_retry_num, MAX_RETRY);
      } else {
        // The setup AP stays up, so the node is never stranded: just
        // re-provision over POST /api/wifi.
        ESP_LOGE(TAG, "STA failed, staying in setup-AP mode for reprovision");
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
    s_sta_connected = true;
    s_sta_ip = event->ip_info;
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

  // One-time purge: firmware predating provisioning may have left STA creds
  // in the driver's NVS namespace. Wipe once, then leave NVS alone.
  nvs_handle_t mh;
  uint8_t purged = 0;
  if (nvs_open("esp7600", NVS_READWRITE, &mh) == ESP_OK) {
    nvs_get_u8(mh, "nvs_purged", &purged);
    nvs_close(mh);
  }
  if (!purged) {
    ESP_LOGW(TAG, "purging NVS of pre-provisioning credentials");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
    if (nvs_open("esp7600", NVS_READWRITE, &mh) == ESP_OK) {
      nvs_set_u8(mh, "nvs_purged", 1);
      nvs_commit(mh);
      nvs_close(mh);
    }
  }

  // Netif + event loop
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  esp_netif_create_default_wifi_ap();

  // WiFi driver
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // Setup AP is always up (APSTA): provisioning + future Flutter client.
  wifi_config_t ap_config = {};
  strncpy((char *)ap_config.ap.ssid, SETUP_AP_SSID,
          sizeof(ap_config.ap.ssid) - 1);
  ap_config.ap.ssid_len = strlen(SETUP_AP_SSID);
  ap_config.ap.max_connection = 4;
  ap_config.ap.authmode = WIFI_AUTH_OPEN;
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

  // STA joins only when provisioned over the REST API.
  char ssid[33] = "";
  char pass[65] = "";
  if (loadCreds(ssid, sizeof(ssid), pass, sizeof(pass))) {
    s_provisioned = true;
    wifi_config_t sta_config = {};
    strncpy((char *)sta_config.sta.ssid, ssid, sizeof(sta_config.sta.ssid) - 1);
    strncpy((char *)sta_config.sta.password, pass,
            sizeof(sta_config.sta.password) - 1);
    sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_LOGI(TAG, "joining provisioned STA %s", ssid);
  } else {
    ESP_LOGI(TAG, "unprovisioned — join " SETUP_AP_SSID
                  " and POST /api/wifi");
  }

  // Event handlers
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifiEventHandler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &wifiEventHandler, NULL));

  // Start
  ESP_ERROR_CHECK(esp_wifi_start());
  startMdns();
  startHttpd();
}
