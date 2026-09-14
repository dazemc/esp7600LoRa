#pragma once

// Wi-Fi provisioning (see .llm/hardware.md): the base runs AP+STA. The setup
// AP is always up; STA connects when credentials exist in NVS. Credentials
// arrive over the REST API only — never commit them.
#define MAX_RETRY 10
#define SETUP_AP_SSID "esp7600LoRa-setup"

void initWiFi(void);
