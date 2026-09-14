# esp7600LoRa repository workflow

This file interprets `AGENTS.md` (see its Instruction precedence section) and
never overrides it; on a conflict the constitution wins and this file is
corrected.

- `main` is the stable branch. Exactly one working branch, `working`, exists
  beside it and carries the code steps of the phase at the top of
  `.llm/todo.md` that still has steps. No other branches exist (no per-phase
  `feature/*` branches).
- Code lands on `working`; markdown lands on `main`. `AGENTS.md`, every file
  under `.llm/`, and docs pages commit directly on `main`, immediately, one
  file per commit, and are pushed. After each markdown commit, merge `main`
  back into `working` so the tree keeps reading current docs.
- Every step is one action with one done-criterion; split work that spans
  apps (`send/`, `recv/`), shared components, or the packet format into
  separate steps. Never batch multiple steps into one change.
- Commit every finished TODO step as a slice: one code commit on `working`,
  then one commit per touched markdown file on `main` (switch to `main`,
  commit, push, switch back, merge `main` into `working`). A step is
  finished only when it is implemented, built (`idf.py build` clean in all
  touched apps), proven on hardware, and removed from `.llm/todo.md`.
  Markdown never shares a commit with code or with another markdown file.
- On `main`, `git add` only the intended file — build outputs (`build/`,
  `managed_components/`, `.ccls-cache/`, `compile_commands.json`,
  `sdkconfig.old`) stay dirty per `.gitignore`; never `git commit -a` on
  `main`. Follow `scope: summary` imperative. Before slicing, `git reset`
  to unstage everything, then `git add` per area so cross-area edits never
  batch into one commit.
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
  `scope: summary` imperative.
- Keep both apps building. Any shared-component change is verified with
  `idf.py build` in `send/` and in `recv/` before pushing.
- After every change, review the touched code for misses (packet-format
  traps, queue depths, task stack sizes, power draw, pin conflicts) and
  append anything that meets the bar to `.llm/suggestions.md`; findings
  never live only in the transcript.
- After each phase is merged, walk every open suggestion with the user and
  settle its decision — keep, condense, move, escalate, or dismiss — before
  the next phase starts.
- Networked Git/GitHub commands (`fetch`, `push`, `gh`) run outside any
  sandbox; sandboxed credential or network failures are not authoritative.

## Flash discipline

Flash commands name the port and the app explicitly
(`idf.py -p /dev/ttyUSB0 flash monitor` from `send/` or `recv/`). Confirm
both with the user before flashing. Never flash `send/` onto the base
board or `recv/` onto the vehicle board. Release the serial port when done.

## opencode

Headless opencode runs drive implementation through this agent (the AFK
workflow): `opencode serve` on localhost, then `opencode run --attach`
with `--auto`, a scoped task file, and hard constraints (do not flash
hardware, do not commit unless told which slice). A run is done when it
exits and `git status` shows only the intended files.
