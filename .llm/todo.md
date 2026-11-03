# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 6: review fixes

1. **Buffer UART1 TX.** `uart_driver_install()` uses a zero TX buffer, so
   every RX JSON waits up to 100 ms for `tx_done`. Install a ring buffer and
   drop the wait. Done when the peer's RX JSON still arrives and commands
   still answer.
2. **Block the RX task on DIO1.** The RX task polls every 5 ms; notify it
    from the DIO1 ISR and wait on the notification (with a slow safety
    timeout). Done when the peer link still receives with no polling loop.
3. **Remove the dead coolant serial event.** The coolant task queues a
    4-line `EVENT_SERIAL_OUT` every second that is dropped outside `DEBUG`.
    Remove it. Done when both apps build.
4. **Remove dead code.** Drop the unused `Telemetry` union, `onReceive`,
    `relayCycleTest`/`relayPinState`, the relay test functions, and the
    unused event enum values. Done when both apps build.
