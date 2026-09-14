# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 2: failure-path hardening (escalated 2026-09-14 walk-through)

1. **Fail radio init distinctly instead of spinning.** Done when a
   `radio.begin()` failure restarts with backoff or halts with a clear
   diagnostic — no silent watchdog reset loop — proven by build plus
   code-path review (hardware fault injection optional).
2. **Gate or remove the base test-transmit path.** Done when the
   non-vehicle branch of `sendLoRaTask()` can no longer put a hardcoded
   packet on air outside `DEBUG`, proven by build plus review.
