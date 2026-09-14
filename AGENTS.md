# esp7600LoRa

ESP-IDF firmware pair for vehicle telemetry over LoRa, with a SIM7600
cellular path on the roadmap. Two apps share one `components/` tree:

- `send/` — vehicle node (`isVehicle = true`): reads vehicle state,
  telemetry task → `loraTXQueue` → LoRa TX task → radio.
- `recv/` — base node (`isVehicle = false`): LoRa RX task fans out to
  `displayQueue` + `serialQueue`, plus Wi-Fi STA uplink.

Both sides run the same `event_bus` model: static `EventConfig` tables
(task fn, name, stack, priority) with queue configs, started from
`app_main()` via `initLoRa()` → `initSemaphores()` → `initEventBus()`.

## Goal

Reliable low-byte-count telemetry link (currently 3-byte packets, 4 bits
reserved) with a display + serial base and a cellular backhaul to come.
Prefer small packet + queue changes over rewrites; keep send/recv sharing
`components/` rather than forking per-side drivers.

The work queue lives in `.llm/todo.md`, in build order. Work it top-down one
step at a time, on the working branch (see Repository workflow):

1. Implement the step, nothing more.
2. Prove it statically: `idf.py build` clean in every app the step touches
   (`send/`, `recv/`), no new warnings.
3. Prove it at runtime: flash and watch with `idf.py -p <port> flash
   monitor` (or `monitor` against an already-flashed board), exercise what
   the step changed, then exit the monitor. A step that builds but
   misbehaves on hardware is not done. If the done-criteria needs eyes on
   hardware, hand the user the exact flash-and-look commands and wait for
   their verdict — never declare hardware proven without evidence.
4. Re-read the topical notes under `.llm/` and update the matching file —
   but only if something is absolutely needed. `.llm/suggestions.md` is for
   agent-proposed, user-reviewed findings that may escalate to `todo.md`.
   Silence is a valid review outcome; never add noise to justify the read.
5. Only then remove the step from `.llm/todo.md`.
6. Commit in slices: the code change is one commit; every LLM-maintained
   markdown file (`.llm/todo.md`, `.llm/suggestions.md`, docs) gets its own commit.
   Markdown never shares a commit with code, and two markdown files never
   share a commit with each other.

Never remove an untested step. A step is one action with one done-criterion;
split work that spans apps, components, or the radio protocol into separate
steps. Never batch multiple steps into one change. Never check steps off —
remove them. Do not let the queue rot.

## What it is

- Two ESP-IDF apps (`send`, `recv`) sharing `components/` via
  `EXTRA_COMPONENT_DIRS ../components/`.
- Event-bus architecture: `event_bus`, `events`, `types` components define
  tasks + queues; peripherals (`lora`, `display`, `serial`, `telemetry`,
  `voltage`, `wifi_sta`, `relay`, `utils`) implement task fns.
- RadioLib-based LoRa link (`components/radiolib`), 3-byte telemetry
  packets with 4 reserved bits.
- Base-side Wi-Fi STA uplink (`wifi_sta`) for getting received telemetry
  off the board.

## What it is not

- Not Arduino-framework code. This is ESP-IDF (`idf.py`, CMake,
  `sdkconfig`). Never add `.ino` sketches or Arduino IDE steps.
- Not a single sketch. `send/` and `recv/` are separate IDF projects with
  separate `sdkconfig` files; a change to shared components must be built
  in both.
- Not a place for live credentials. Wi-Fi SSID/PSK stay out of the tree
  (see Privacy).

## Instruction precedence

```text
AGENTS.md             constitution — what the project is, how every change is made
.llm/workflow.md      process interpretation (branches, commits, phases)
.llm/*.md             domain knowledge and contracts (only as many as needed)
.llm/todo.md          authorized work queue, in build order
.llm/suggestions.md   non-authoritative findings, escalated by the user
```

A lower file never overrides a higher one; on conflict the higher file wins
and the lower one is corrected in the same session.

## Repository workflow

- `main` is the stable branch. Exactly one working branch, `working`, exists
  beside it and carries the code steps of the phase at the top of
  `.llm/todo.md` that still has steps. No other branches exist (no per-phase
  `feature/*` branches).
- Code lands on `working`; markdown lands on `main`. `AGENTS.md`, every file
  under `.llm/`, and docs pages commit directly on `main`, immediately, one
  file per commit, and are pushed. After each markdown commit, merge `main`
  back into `working` so the tree keeps reading current docs.
- Every step is one action with one done-criterion; split work that spans
  apps, components, or the packet format into separate steps. Never batch
  multiple steps into one change.
- Commit every finished TODO step as a slice: one code commit on `working`,
  then one commit per touched markdown file on `main` (switch to `main`,
  commit, push, switch back, merge `main` into `working`). A step is
  finished only when it is implemented, built (`idf.py build` clean in all
  touched apps), proven on hardware, and removed from `.llm/todo.md`.
  Markdown never shares a commit with code or with another markdown file.
- On `main`, `git add` only the intended file — build artifacts (`build/`,
  `managed_components/`, `.ccls-cache/`, `compile_commands.json`,
  `sdkconfig.old`) belong to `.gitignore` and stay dirty; never
  `git commit -a` on `main`. Follow `scope: summary` imperative.
- Never start a new phase without the user's explicit go-ahead in chat: no
  branch, no first step, until asked. Merging a finished phase likewise
  waits for confirmation.
- When a phase's steps are all landed and removed, merge `main` into
  `working` first so the PR carries code only, then `gh pr create --base
  main --head working` and `gh pr merge --merge` (never squash — squash
  destroys the sliced history). Pull `main`, then `git reset --hard main`
  on `working` and force-push (the one sanctioned force-push) so the next
  phase starts clean.
- Commits use the contributor's configured Git identity. Follow
  `scope: summary` in the imperative.
- After every change, review the touched code for misses (packet-format
  traps, queue-depth issues, stack sizes, power draw, hardware gaps) and
  append anything that meets the bar to `.llm/suggestions.md`; findings
  never live only in the transcript.
- After each phase is merged, walk every open suggestion with the user and
  settle its decision — keep, condense, move, escalate, or dismiss — before
  the next phase starts.
- Networked Git/GitHub commands (`fetch`, `push`, `gh`) run outside any
  sandbox; sandboxed credential or network failures are not authoritative.

## Build

```sh
cd send && idf.py build
cd recv && idf.py build
idf.py -p /dev/ttyUSB0 flash monitor   # flash + watch; exit monitor when done
```

A first build requires network access (component manager fetches
`dependencies.lock` entries into `managed_components/`); subsequent builds
reuse the cache. Both apps must build after any shared-component change.

## Privacy

Secrets stay out of the tree by name only: Wi-Fi SSID/PSK, SIM PIN/APN
credentials, server URLs with tokens. `recv/main/main.cpp` already carries
commented-out SSID/PSK placeholders — they stay commented, and live values
are passed at flash/monitor time or via untracked local config, never
committed. Never read, print, summarize, or commit a live credential.

## Safety

Never flash a board without the user's explicit confirmation of which port
and which app (`send` vs `recv`) — flashing the wrong image onto a deployed
node is the live-system break here. Never leave a monitor session holding
the serial port when done. Keep `sdkconfig` changes deliberate: a radio-pin
or partition-table edit bricks the link until both sides agree, so matching
`send`/`recv` changes land as one step with both builds proven.

## File map

```text
AGENTS.md             constitution (this file)
.llm/workflow.md      branch/commit/phase interpretation
.llm/hardware.md      boards, radios, pins, packet format, queues
.llm/todo.md          currently authorized work, in build order
.llm/suggestions.md   non-authoritative observations
send/                 vehicle firmware (IDF app)
recv/                 base firmware (IDF app)
components/           shared IDF components (event_bus, lora, display, ...)
```
