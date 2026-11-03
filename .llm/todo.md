# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 7: close the provisioning AP

1. **Make Wi-Fi mode follow provisioning.** `initWiFi()` always creates the
   AP netif and sets `WIFI_MODE_APSTA`. Create the AP only when no credentials
   exist; otherwise create the STA netif and set `WIFI_MODE_STA`. The HTTP
   server and mDNS still start in both modes. Done when an unprovisioned base
   boots AP-only and the forced STA branch boots STA-only.
2. **Add `WIFI_RESET` on UART1.** Erase only `sta_ssid`/`sta_pass` from the
   `esp7600` NVS namespace, keep the purge marker, and restart. Register the
   handler from `recv/main` so `wifi_sta` stays out of the vehicle image. Done
   when the Pi sends `WIFI_RESET` and the base reboots into the AP.
3. **Document the provisioning behavior.** Update `.llm/hardware.md`: the AP
   exists only while unprovisioned, the provisioned base is STA-only, and
   `WIFI_RESET` restores the AP. Done when the doc matches the code.
