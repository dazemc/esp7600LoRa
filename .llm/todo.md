# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 3: coolant path and base correctness

1. **Clamp `voltageToU8()` in release builds.** Both clamps are guarded by
   `&& DEBUG` today, so a release build casts out-of-range floats to `uint8_t`
   (undefined below 11 V, wraps above 16 V). Clamp unconditionally; keep only
   the `ESP_LOGE` lines behind `DEBUG`. Done when 10 V encodes 0, 17 V encodes
   255, and both apps build clean.
2. **Make relay commands safe on the base.** recv links `relay.cpp` through
   `serial` but never creates `relayQueue`, so `START`/`HEADLIGHTS`/`ALLOFF`
   reach `xQueueSend(nullptr, …)` and trip the FreeRTOS assert. Guard the push
   and log "ignored on base". Done when a command sent to a recv build is
   rejected without a reboot.
3. **Gate the base test-transmit branch.** The non-vehicle branch of
   `sendLoRaTask()` builds a hardcoded packet (headlights on, zero voltage)
   and sends it on any TX event. Put the branch behind `DEBUG` so a release
   base never transmits. Done when both apps build clean and review shows the
   branch unreachable with `DEBUG` off.
4. **Fail radio init without blocking the app.** `initLoRa()` spins forever on
   `radio.begin()` failure, and it runs before `initEventBus()`, so a missing
   radio also kills relay, serial, and telemetry. Return status, log once, and
   start the bus anyway; radio tasks idle/log instead of spinning. Done when a
   board with no radio still boots the relay and serial tasks, and both apps
   build clean.
5. **Surface coolant on the base.** Add `COOLANT: n F` to the OLED RX screen
   and to the debug RX serial dump. Done when the base reading matches the
   sender's `STATE` value.
6. **Pin the packed frame size at compile time.** Add
   `static_assert(sizeof(LoRaCompactPacket) == 5)` and `offsetof` checks for
   the non-bitfield byte fields, so padding or an uncoordinated growth attempt
   fails the build. The frame may grow; the packed layout is the invariant,
   the size is not. Done when a deliberate field addition fails to compile
   until the assert is bumped and both apps build clean.
7. **Grow the base app partition.** The factory partition is 1 MiB at 0x10000
    while the 2 MB flash config leaves ~960 KiB unused, and recv is at
    951 KiB. Read the real flash ID, add a custom partition CSV (factory
    0x1F0000), and flash both apps. Done when `idf.py size` on recv reports the
    new limit, both apps boot, and `.llm/hardware.md` records the table.
8. **Treat a failed thermistor as a fault.** An open or shorted sensor yields
    -999 °F, clamps to -512 °F, and drives a 12 s glow plus an impossible
    `STATE` value. Detect the out-of-range resistance/voltage, log a fault,
    and use the documented max-glow fallback. Done when an unplugged sensor
    logs the fault and still glows 12 s.
9. **Abort the start when glow fails.** `ignSequence()` discards
    `glowPlugSequence()` and `runningLightsSequence()` results. Propagate the
    glow error and return before the IGN toggle; log a running-light failure
    but continue. Done when a forced glow error prevents the IGN toggle and a
    forced light error still starts.
10. **Make ALLOFF fail closed.** `allOffSequence()` returns only the last
    pin's error and clears every state flag even when a write failed. Attempt
    all pins, keep the first failure, log each stuck pin, and update flags only
    on confirmed writes. Done when an I2C fault during ALLOFF leaves the
    failed relay's flag set and the command reports failure.
11. **Bound the radio tasks' UI queue waits.** `displayLoRa()`,
    `serialLoRa()`, and the recv fan-out use `portMAX_DELAY`; a slow OLED or
    serial consumer stalls the radio. Use a zero/short timeout, drop the UI
    event when full, and count it. Done when pausing the display task under
    load does not stall RX/TX and the drop is logged.
12. **Make DEBUG compile-time.** `DEBUG` is a mutable global, so release
    images keep every debug branch and string. Move it to a Kconfig or
    `constexpr` switch per app. Done when recv's release size drops in
    `idf.py size` and the DEBUG build still emits the same lines.
13. **Expose ACC and GLOWPLUGS on the serial link.** Add the two command
    strings mapped to the existing `EVENT_TOGGLE_ACC` /
    `EVENT_TOGGLE_GLOWPLUGS`. Done when both drive the expander and log on
    UART1.
