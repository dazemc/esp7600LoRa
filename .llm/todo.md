# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 1: review hardening (escalated 2026-09-14)

1. **DONE (2026-09-14, flashed + proven on base).** AP provisioning REST API
   with mDNS; creds purged from tree and board NVS; STA gated on provisioned.
   Remaining: user POSTs real creds and rotates the leaked AP password.
2. **DONE (2026-09-14, flashed + proven on sender).** Relay GPIOs driven
   outputs parked HIGH with boot-time readback log; relayTask registered.
   Found + fixed en route: relayState signature mismatch (latent link
   failure), missing gpio requirement, and a display-abort reboot loop
   (OLED probe now degrades gracefully — telemetry never dies for a screen).
3. **DONE (2026-09-14, flashed + proven).** Single increment path;
   sender and receiver logs agree packet-for-packet over the air.
4. **DONE (2026-09-14, flashed + proven).** Telemetry at 2048, 1184 bytes
   high-water margin observed live (was 164).
5. **DONE (2026-09-14, flashed + proven).** LoRaTX at 4096, 2476 bytes
   high-water margin through transmit bursts (was 372).
