# Suggestions

Non-authoritative findings. One entry per issue, ≤5 lines each. Entries must
be absolutely needed (prevent a future mistake, unblock work, or record a
decision + reason). The user escalates entries to `.llm/todo.md`.

## 2026-09-14 code review (live-link verified, both boards up)

1. **Live Wi-Fi credentials are committed to a public repo.**
   `components/wifi_sta/wifi_sta.h` defines `SSID "Aetheryte"` and the real
   PSK in clear, pushed to github.com/dazemc/esp7600LoRa. Rotate the PSK,
   scrub history, move creds to Kconfig.projbuild or an untracked header.
2. **Packet ID on air is off by one from the logged ID.**
   `buildPacket()` increments global `packetIdTX`, then `packetToCompactPacket()`
   takes it by value and increments the copy — the transmitted ID is always
   header + 1. Drop-detection still works, but sender logs disagree with the
   receiver. Increment once (pass by reference or remove the second call).
3. **Telemetry task stack nearly exhausted (164 bytes remaining, observed live).**
   1024-byte stack with `ESP_LOGE` float formatting in the call chain is one
   bad day from overflow. Raise to 2048 in `send/main/main.cpp`.
4. **LoRaTX task stack thin (372 bytes remaining, observed live).**
   Synchronous `radio.transmit()` runs inside this 2048-byte task. Raise to
   4096 to match the display task before packet work grows.
5. **Wi-Fi gives up forever after 10 retries.** `wifi_sta.cpp` logs "giving up"
   and never retries — an unattended base stays offline until a manual reboot.
   Add a periodic reconnect (timer with backoff) instead of a dead end.
6. **Radio init failure spins forever.** `initLoRa()` loops `delay(1000)` on
   `radio.begin()` failure; under the task watchdog this becomes a silent
   reset loop. Restart with backoff or fail distinctly instead.
7. **Base-side test-transmit path is live dead code.** The non-vehicle branch
   of `sendLoRaTask()` builds a hardcoded packet; nothing fires it today, but
   any future `EVENT_LORA_TX` on the base puts junk on air. Gate behind
   `DEBUG` or delete it.
8. **`initRelay()` is never called and `relayTask` is in neither event table.**
   Relay GPIOs boot as floating inputs and `vehicleState` relay fields sit at
   zero-init — on a remote-start vehicle node, drive the pins or drop the
   component before it surprises someone.
