# Suggestions

Non-authoritative findings. One entry per issue, ≤5 lines each. Entries must
be absolutely needed (prevent a future mistake, unblock work, or record a
decision + reason). The user escalates entries to `.llm/todo.md`.

## 2026-09-14 code review (live-link verified, both boards up)
(escalated to `.llm/todo.md` phase 1: items 1, 8, 2, 3, 4 — remaining open below)

1. **Escalated to todo phase 1.**
2. **Escalated to todo phase 1.**
3. **Escalated to todo phase 1.**
4. **Escalated to todo phase 1.**
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
8. **Escalated to todo phase 1.**
