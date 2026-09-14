# Hardware and protocol notes

Domain knowledge for esp7600LoRa. Read before touching radio, packet, queue,
or pin code.

## Boards and apps

- `send/` (vehicle, `isVehicle = true`): telemetry → `loraTXQueue` → LoRa TX.
  Tasks: LoRa RX, LoRa TX, Display, Serial, Voltage monitor, Telemetry.
- `recv/` (base, `isVehicle = false`): LoRa RX → `displayQueue` +
  `serialQueue`, plus `wifi_sta` STA uplink. Currently also inits Wi-Fi STA
  in `app_main()`.
- Both are ESP-IDF apps sharing `components/` via
  `EXTRA_COMPONENT_DIRS ../components/`.

## Shared components

`event_bus`, `events`, `types` (task/queue model) · `lora` · `radiolib`
(RadioLib) · `display` · `serial` · `telemetry` · `voltage` · `wifi_sta` ·
`relay` · `utils`.

## Packet format

3-byte telemetry packets, 4 bits reserved (reduced from 24 bytes). Any
format change must land on both sides at once and update this file in the
same phase — a one-sided packet edit breaks the link silently.

## Queues

All queues length 10. Vehicle RX/TX item sizes follow `EventLoRaRX` /
`EventLoRaTX`; display and serial follow `EventDisplay` / `EventSerial`.
Task stacks: LoRa RX 2048–3096, LoRa TX 2048, Display 3096–4096, Serial
2048–3096, Voltage 2048, Telemetry 1024. Priorities: radio tasks 3,
everything else 2. Growing a queue or shrinking a stack needs a
runtime-proven step, not just a clean build.

## SIM7600 roadmap

Cellular backhaul is planned, not yet in the tree. When it lands it gets
its own component (never inline AT chatter in `main.cpp`) and its own
`.llm/` note update.

## Relay (decision 2026-09-14)

`send` drives 6 active-low relay GPIOs (13, 17, 22, 23, 25, 33), parked HIGH
= off at boot via `initRelay()`, with a boot-time drive/readback log.
Bench note: against an unpowered relay board the pins read back LOW
(optocoupler clamp) — that is a bench artifact, not a drive failure.
`vehicleState` seeds from the readback, so it reflects reality. Display
init never aborts: a missing OLED degrades to headless, telemetry first.

## Wi-Fi provisioning (decision 2026-09-14)

The base offers a config AP when no STA credentials are stored in NVS.
Provisioning is a JSON REST API over `esp_http_server` — no HTML pages;
a Flutter app is the future client. `POST /api/wifi` {ssid, pass} stores
creds in NVS and switches to STA; `GET /api/status` reports mode and link
state. Credentials never appear in tracked files. ESP32 is 2.4 GHz-only:
provision the 2.4 GHz SSID (`Aetheryte_2.4`, not the 5 GHz `Aetheryte` —
verified live 2026-09-14, end-to-end join + mDNS on the home LAN).
