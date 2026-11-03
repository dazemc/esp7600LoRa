# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 3: coolant path and base correctness

1. **Treat a failed thermistor as a fault.** An open or shorted sensor yields
    -999 °F, clamps to -512 °F, and drives a 12 s glow plus an impossible
    `STATE` value. Detect the out-of-range resistance/voltage, log a fault,
    and use the documented max-glow fallback. Done when an unplugged sensor
    logs the fault and still glows 12 s.
2. **Abort the start when glow fails.** `ignSequence()` discards
    `glowPlugSequence()` and `runningLightsSequence()` results. Propagate the
    glow error and return before the IGN toggle; log a running-light failure
    but continue. Done when a forced glow error prevents the IGN toggle and a
    forced light error still starts.
3. **Make ALLOFF fail closed.** `allOffSequence()` returns only the last
    pin's error and clears every state flag even when a write failed. Attempt
    all pins, keep the first failure, log each stuck pin, and update flags only
    on confirmed writes. Done when an I2C fault during ALLOFF leaves the
    failed relay's flag set and the command reports failure.
4. **Bound the radio tasks' UI queue waits.** `displayLoRa()`,
    `serialLoRa()`, and the recv fan-out use `portMAX_DELAY`; a slow OLED or
    serial consumer stalls the radio. Use a zero/short timeout, drop the UI
    event when full, and count it. Done when pausing the display task under
    load does not stall RX/TX and the drop is logged.
5. **Make DEBUG compile-time.** `DEBUG` is a mutable global, so release
    images keep every debug branch and string. Move it to a Kconfig or
    `constexpr` switch per app. Done when recv's release size drops in
    `idf.py size` and the DEBUG build still emits the same lines.
6. **Expose ACC and GLOWPLUGS on the serial link.** Add the two command
    strings mapped to the existing `EVENT_TOGGLE_ACC` /
    `EVENT_TOGGLE_GLOWPLUGS`. Done when both drive the expander and log on
    UART1.
