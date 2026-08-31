#pragma once

#include <WiFi.h>

void initWiFi(char *ssid, char *psk);
void initWiFiAP(const char *ssid, const char *psk);
void webServerAPTask(void *arg);
void webServerLocalTask(void *arg);
