# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 6: review fixes

1. **Initialize `ignState`.** `ignSequence()` leaves `ignState`
   uninitialized for an out-of-range `Ignition`; default to OFF and log. Done
   when both apps build.
2. **Slim `LoRaRawPacket`.** The 256-byte `data` array and `length` are never
   read but ride in every display/serial RX queue item. Keep only `rssi`/`snr`.
   Done when the RX JSON and display still carry the same values on the peer
   link.
3. **Buffer UART1 TX.** `uart_driver_install()` uses a zero TX buffer, so
   every RX JSON waits up to 100 ms for `tx_done`. Install a ring buffer and
   drop the wait. Done when the peer's RX JSON still arrives and commands
   still answer.
4. **Block the RX task on DIO1.** The RX task polls every 5 ms; notify it
    from the DIO1 ISR and wait on the notification (with a slow safety
    timeout). Done when the peer link still receives with no polling loop.
5. **Remove the dead coolant serial event.** The coolant task queues a
    4-line `EVENT_SERIAL_OUT` every second that is dropped outside `DEBUG`.
    Remove it. Done when both apps build.
6. **Remove dead code.** Drop the unused `Telemetry` union, `onReceive`,
    `relayCycleTest`/`relayPinState`, the relay test functions, and the
    unused event enum values. Done when both apps build.
