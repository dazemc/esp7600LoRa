# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 5: Pi telemetry link (JSON)

1. **Make every command reply JSON.** Replace the `TEST`/`STATE` text replies
   with `{"t":"pong"}` and `{"t":"state",…}`; unknown commands reply
   `{"t":"err","msg":…}`. Done when each reply is valid JSON and
   newline-terminated.
2. **Show link stats on the base OLED.** Add the last frame's RSSI/SNR to the
   RX screen. Done when the display matches the JSON line.
