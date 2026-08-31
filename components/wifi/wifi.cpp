#include "wifi.h"
#include <WebServer.h>

static WiFiServer serverAP(80);
static WebServer server(80);

void handleLoRaCheck() {
  // webserverQueue, emit event to voltage and then send it back to a handler
  String json = "{\"status\":\"success\",\"voltage\":42}";
  server.send(200, "application/json", json);
}

void initWiFi(char *ssid, char *psk) {
  WiFi.begin(ssid, psk);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(300));
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    server.on("/lora/check", HTTP_GET, handleLoRaCheck);
    server.begin();
    Serial.println("REST server started");
  } else
    abort();
}

void initWiFiAP(const char *ssid, const char *psk) {
  WiFi.softAP(ssid, psk);
  Serial.print("AP Mode started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  serverAP.begin();
}

void webServerAPTask(void *arg) {
  while (true) {
    WiFiClient client = serverAP.accept();
    if (client) {
      Serial.println("Client Connected");
      while (client.connected()) {
        if (client.connected()) {
          Serial.write(client.read());
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

void webServerLocalTask(void *arg) {
  while (true) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}
