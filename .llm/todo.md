# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 7: close the provisioning AP

1. **Add `WIFI_RESET` on UART1.** Erase only `sta_ssid`/`sta_pass` from the
   `esp7600` NVS namespace, keep the purge marker, and restart. Register the
   handler from `recv/main` so `wifi_sta` stays out of the vehicle image. Done
   when the Pi sends `WIFI_RESET` and the base reboots into the AP.
2. **Document the provisioning behavior.** Update `.llm/hardware.md`: the AP
   exists only while unprovisioned, the provisioned base is STA-only, and
   `WIFI_RESET` restores the AP. Done when the doc matches the code.
