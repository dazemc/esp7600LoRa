# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 5: Pi telemetry link (JSON)

1. **Emit one JSON line per RX packet on UART1.** Build the `{"t":"rx",…}`
   record from the RX event (id, vehicle, flags, volts, coolant, RSSI, SNR,
   drop) and write it to UART1, not gated by `DEBUG`. Done when a serial
   terminal on the Pi side sees one line per received packet.
2. **Make every command reply JSON.** Replace the `TEST`/`STATE` text replies
   with `{"t":"pong"}` and `{"t":"state",…}`; unknown commands reply
   `{"t":"err","msg":…}`. Done when each reply is valid JSON and
   newline-terminated.
3. **Show link stats on the base OLED.** Add the last frame's RSSI/SNR to the
   RX screen. Done when the display matches the JSON line.
