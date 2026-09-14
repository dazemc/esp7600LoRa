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
5. **Covered by phase-1 wifi rework (STA retry still caps at 10, but the setup
   AP stays up so the node is never stranded — re-POST /api/wifi).**
6. **Escalated to todo phase 2 (2026-09-14 walk-through).**
7. **Escalated to todo phase 2 (2026-09-14 walk-through).**
8. **Escalated to todo phase 1.**
9. **Upstream chill-sam/ssd1306 1.1.2 keeps esp_driver_gpio in PRIV_REQUIRES,
   breaking IDF 6 builds; we carry a one-line workaround (gpio in display's
   REQUIRES).** Drop the workaround when upstream fixes it.
10. **Lesson 2026-09-14: flashing preserves NVS, so the Wi-Fi driver can
    rejoin a stale network even with no creds in firmware.** Any credential
    rotation must wipe or overwrite board NVS (we purge once via marker);
    never assume a reflash alone clears stored secrets.
