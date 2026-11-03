# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 8: vehicle-side Pi telemetry

1. **Emit a `tx` JSON record on the vehicle.** `EVENT_SERIAL_LORA_TX` is
   debug-only today, so the Pi on the S3 never sees the vehicle's own
   telemetry. Build `{"t":"tx","id",…,"v","c"}` for every transmitted packet
   and write it to UART1, not gated by `DEBUG`. Done when the Pi sees one line
   per transmission.
2. **Move the S3 to the vehicle image.** Flash `send` to `ttyUSB0` and confirm
   the Pi receives `tx` records while the Heltec peer receives the same frames.
   Done when both outputs agree on id/voltage/coolant.
3. **Document the `tx` record.** Add it to the JSON examples and explain that
   `tx` is the vehicle's sent copy and `rx` the base's received copy. Done when
   `.llm/hardware.md` matches the wire.
