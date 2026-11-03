# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 7: close the provisioning AP

1. **Document the provisioning behavior.** Update `.llm/hardware.md`: the AP
   exists only while unprovisioned, the provisioned base is STA-only, and
   `WIFI_RESET` restores the AP. Done when the doc matches the code.
