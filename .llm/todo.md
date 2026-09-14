# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 1: review hardening (escalated 2026-09-14)

1. **Provision Wi-Fi via AP-mode REST API, purge hardcoded creds.** Done when
   no credential string remains in any tracked file, the base boots a config
   AP when no STA creds are stored, STA creds are set over a JSON REST API
   (no HTML — a Flutter app is the future client) and persisted in NVS, and
   the base connects as STA from NVS. (User rotates the real-world AP
   password out of band — the old one sat in a public repo.)
2. **Wire up or drop the relay component.** Done when relay GPIOs are driven
   outputs with `initRelay()` called and `relayTask` registered, or the
   component is removed — no floating relay inputs on the vehicle node.
3. **Fix packet ID off-by-one.** Done when the transmitted ID equals the
   logged/displayed ID (single increment path), proven by matching sender
   and receiver logs over several packets.
4. **Raise telemetry task stack to 2048.** Done when `send/` builds clean
   and live logs show comfortable high-water margin.
5. **Raise LoRaTX task stack to 4096.** Done when `send/` builds clean and
   live logs show comfortable high-water margin through transmit bursts.
