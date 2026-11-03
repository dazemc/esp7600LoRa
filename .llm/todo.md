# Todo

Authorized work queue, in build order. Work top-down, one step at a time.
Remove steps as they land — never check them off.

## Phase 4: init ordering and packet truth

1. **Stop initializing the relay twice.** `app_main()` and `relayTask()` both
   call `initRelay()`, so boot logs "MCP23017 initialized" twice and the
   second pass re-runs the I2C setup. Let `initRelay()` set a `relayReady`
   flag, remove the task-local call, and have the task exit if the boot init
   failed. Done when a normal boot logs the expander once and a failed init
   still leaves the relay task idle.
