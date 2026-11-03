# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 3: coolant path and base correctness

1. **Store coolant in °F in `vehicleState.coolantTemp`.** The thermistor task
   writes the packed 10-bit value today, so relay sees 1014 for −10 °F and
   `STATE` reports 1014. Store the clamped engineering value instead; packing
   belongs at the wire boundary. Done when a cold bench reading (substituted
   resistor) logs as a negative °F in `STATE` and takes the cold branch in
   `glowPlugSequence()`.
2. **Own the 10-bit coolant conversion in `utils.cpp`.** `packetToCompactPacket()`
   never sets `encodedCoolantTemperature` and `unpackPacket()` never reads it,
   so the air frame always carries 0 °F and the sign-extension block in
   `recvLoRaTask()` is dead code. Pack and sign-extend in the two conversion
   functions, delete the `lora.cpp` block. Done when −40/−10/32/140/200 °F
   round-trip exactly on the base and both apps build clean.
3. **Clamp glow-plug on-time to 1–12 s.** `(220 - temp) / 20` yields 0 s at
   201–219 °F and goes negative above 220, so the plugs toggle off instantly.
   Clamp the computed seconds, keep the 140 °F override, and drop the raw
   `printf("Reading temp...")`. Done when 0/100/200/230 °F log 11/6/1/1 s on
   the bench.
4. **Clamp `voltageToU8()` in release builds.** Both clamps are guarded by
   `&& DEBUG` today, so a release build casts out-of-range floats to `uint8_t`
   (undefined below 11 V, wraps above 16 V). Clamp unconditionally; keep only
   the `ESP_LOGE` lines behind `DEBUG`. Done when 10 V encodes 0, 17 V encodes
   255, and both apps build clean.
5. **Make relay commands safe on the base.** recv links `relay.cpp` through
   `serial` but never creates `relayQueue`, so `START`/`HEADLIGHTS`/`ALLOFF`
   reach `xQueueSend(nullptr, …)` and trip the FreeRTOS assert. Guard the push
   and log "ignored on base". Done when a command sent to a recv build is
   rejected without a reboot.
6. **Gate the base test-transmit branch.** The non-vehicle branch of
   `sendLoRaTask()` builds a hardcoded packet (headlights on, zero voltage)
   and sends it on any TX event. Put the branch behind `DEBUG` so a release
   base never transmits. Done when both apps build clean and review shows the
   branch unreachable with `DEBUG` off.
7. **Fail radio init without blocking the app.** `initLoRa()` spins forever on
   `radio.begin()` failure, and it runs before `initEventBus()`, so a missing
   radio also kills relay, serial, and telemetry. Return status, log once, and
   start the bus anyway; radio tasks idle/log instead of spinning. Done when a
   board with no radio still boots the relay and serial tasks, and both apps
   build clean.
8. **Surface coolant on the base.** Add `COOLANT: n F` to the OLED RX screen
   and to the debug RX serial dump. Done when the base reading matches the
   sender's `STATE` value.
