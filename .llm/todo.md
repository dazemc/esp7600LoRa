# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 1: review hardening (escalated 2026-09-14)

1. **Rotate Wi-Fi PSK and move creds out of the tree.** Done when the live
   PSK is rotated on the AP, no credential string remains in any tracked
   file, and creds arrive via Kconfig.projbuild or an untracked header.
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
