# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 6: review fixes

1. **Remove the dead coolant serial event.** The coolant task queues a
    4-line `EVENT_SERIAL_OUT` every second that is dropped outside `DEBUG`.
    Remove it. Done when both apps build.
2. **Remove dead code.** Drop the unused `Telemetry` union, `onReceive`,
    `relayCycleTest`/`relayPinState`, the relay test functions, and the
    unused event enum values. Done when both apps build.
