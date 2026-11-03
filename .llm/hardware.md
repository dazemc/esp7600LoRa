# Hardware and protocol notes

Domain knowledge for esp7600LoRa. Read before touching radio, packet, queue,
pin, or relay code.

## Boards and apps

- Both apps target ESP32-S3 (`CONFIG_IDF_TARGET="esp32s3"`) with separate
  `sdkconfig` files and a 7.9 MB factory app partition. The console is UART0
  (GPIO43/44) over the board's USB-UART bridge; the native USB port is not
  the console.
- `send/` (vehicle, `isVehicle = true`): telemetry → `loraTXQueue` → LoRa TX.
  Tasks: LoRa RX, LoRa TX, Display, Serial, Voltage monitor, Coolant
  temperature, Relay, Telemetry.
- `recv/` (base, `isVehicle = false`): LoRa RX → `displayQueue` +
  `serialQueue`, plus `wifi_sta` (APSTA provisioning + STA uplink, see Wi-Fi
  provisioning below). Tasks: LoRa RX, LoRa TX, Display, Serial. The Pi Zero
  W 2 + SIMCom HAT sits on UART1 as the cellular uplink; it is external to
  this tree.
- Both share `components/` via `EXTRA_COMPONENT_DIRS ../components/`.

## Pin map (ESP32-S3)

| Function | Pins |
| --- | --- |
| SX1262 | CS 8, SCK 9, MOSI 10, MISO 11, RST 12, BUSY 13, DIO1 14 |
| OLED (I2C0) | SDA 17, SCL 18, RST 21, VEXT 36 |
| MCP23017 (I2C1) | SDA 33, SCL 42, address 0x20 |
| Pi / serial commands | UART1 TX 19, RX 20, 115200 8N1 |
| Console | UART0, 115200 8N1 |

GPIO19/20 are the S3 native USB D-/D+ pins and
`CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG` is enabled, but the Pi link on
them is bench-verified; keep the native USB port unplugged while UART1 is in
use. GPIO17/18 are the UART1 defaults and are claimed by the OLED, which is
why the Pi link sits on 19/20.

## Bench peer

The second node on the bench is a Heltec WiFi LoRa 32 V2 (ESP32-D0WDQ6,
SX1276, 8 MB flash, `/dev/ttyUSB1`) running a throwaway ESP-IDF peer app.
It uses 915 MHz, sync word 0x14, and the same 5-byte frame. Cross-family
SX1262/SX1276 traffic is verified both ways: the S3 receiver sees about
−22 dBm / 11 dB SNR, and the peer sees the S3 sender at about −43 dBm /
13 dB SNR. The peer lives in `/tmp`, not in this tree.

## Flash and partitions

Both boards carry an 8 MB flash (`esptool flash_id` reports manufacturer
0xc8, device 0x4017); the old config declared 2 MB. `CONFIG_ESPTOOLPY_FLASHSIZE`
is now 8 MB and both apps use the same `partitions.csv`:

| Name | Type | Offset | Size |
| --- | --- | --- | --- |
| nvs | data/nvs | 0x9000 | 24 KiB |
| phy_init | data/phy | 0xf000 | 4 KiB |
| factory | app | 0x10000 | 0x7F0000 (7.9 MB) |

NVS and PHY offsets are unchanged, so stored Wi-Fi credentials survive the
resize. The app slot is single-slot; there is no OTA partition.

## ADC

One `adc_shared` unit (ADC1) is created in `app_main`; the voltage and
thermistor monitors share it behind `adcSemaphore`. Voltage divider on ADC1
channel 4 (`×5.6` to battery volts); thermistor on ADC1 channel 1 with a
1 kΩ fixed resistor and Steinhart-Hart conversion in °F.

## Shared components

`event_bus`, `events`, `types` (task/queue model) · `lora` · `radiolib`
(RadioLib) · `display` · `serial` · `telemetry` · `voltage` ·
`coolant_sensor` · `adc_shared` · `wifi_sta` · `relay` · `mcp23x17` ·
`utils`.

## Packet format

5 bytes on air, packed, no padding:

| Byte | Bits | Field |
| --- | --- | --- |
| 0 | 0–2 | `packetId` (3 bits, wraps at 8, used for drop detection) |
| 0 | 3 | `isVehicle` |
| 0 | 4–7 | `flags` (reserved, send 0) |
| 1 | 0–1 | `ign` (OFF/ON/START) |
| 1 | 2 | `headlights` |
| 1 | 3 | `acc` |
| 1 | 4 | `runningLights` |
| 1 | 5 | `heater` |
| 1 | 6 | `glowPlugs` |
| 1 | 7 | `isWifiEnabled` |
| 2 | 0–7 | `encodedVoltage` (linear 11–16 V → 0–255) |
| 3–4 | 0–9 | `encodedCoolantTemperature` (10-bit signed two's complement °F) |
| 3–4 | 10–15 | `extraData` (reserved, send 0) |

`vehicleState.coolantTemp` holds engineering °F; only
`packetToCompactPacket()` and `unpackPacket()` convert to and from the
10-bit wire field. A one-sided packet edit breaks the link silently: a
format change lands on both sides in one step, both builds proven, and this
file updated in the same phase.

The frame may grow; the invariant is the packed byte layout, not the current
5-byte size. Every addition updates both sides, the size assert in `types.h`,
and the table above in the same step.

## Queues

All queues length 10. Vehicle RX/TX item sizes follow `EventLoRaRX` /
`EventLoRaTX`; display and serial follow `EventDisplay` / `EventSerial`.
Task stacks (`send` / `recv`): LoRa RX 4096 / 4096, LoRa TX 4096 / 2048,
Display 3096 / 4096, Serial 4096 / 3096, Relay 2048 (send only), Voltage
2048 (send only), Coolant 2048 (send only), Telemetry 2048 (send only).
Priorities: radio tasks 3, everything else 2. Growing a queue or shrinking a
stack needs a runtime-proven step, not just a clean build.

## Serial link (decision 2026-11-03)

The Pi Zero W 2 talks to the ESP32 on UART1 (TX 19, RX 20) at 115200,
leaving the UART0 console free for `idf.py monitor`. On the Pi the link is
`/dev/ttyS0`; it must not also be the serial console — mask
`serial-getty@ttyS0` and remove `console=serial0,…` from `cmdline.txt`, or
the JSON competes with a login prompt. Commands are newline-terminated ASCII:
`START`, `ACC`, `HEADLIGHTS`, `RUNNINGLIGHTS`, `HEATER`, `GLOWPLUGS`,
`ALLOFF`, `TEST`, `STATE`, and `WIFI_RESET` (base only).

Everything the ESP32 sends to the Pi is one JSON object per line:

```json
{"t":"tx","id":3,"veh":true,"ign":"ON","head":false,"acc":true,"run":false,"heat":false,"glow":false,"wifi":false,"v":12.60,"c":76}
{"t":"rx","id":3,"veh":true,"ign":"ON","head":false,"acc":true,"run":false,"heat":false,"glow":false,"wifi":false,"v":12.60,"c":76,"rssi":-65,"snr":9.5,"drop":0}
{"t":"state","v":12.60,"ign":"ON","head":false,"acc":true,"run":false,"heat":false,"glow":false,"c":76}
{"t":"pong"}
```

`tx` is the vehicle's own transmitted frame: no RSSI/SNR/drop, and `v` is
the local sensor voltage. `rx` is the base's received copy: it adds RSSI/SNR
and the drop count, and `v` is decoded from the 8-bit air field (clamped
11–16 V, so a bench input below 11 V reads as 11.0 on the receiver). `c` is
coolant °F. Both records are emitted for every frame, not gated by `DEBUG`.
Unknown commands are logged on the console only: the bench link shows TX
echoed into RX, so an unconditional reply would feed itself. Boot and debug
logs stay on the console. The link is bench-verified with the Pi.

## Debug build

`CONFIG_TELEMETRY_DEBUG` (`components/utils/Kconfig.projbuild`) compiles the
stack/queue and packet logs into the image. It is off in the committed
`sdkconfig`; set it per app for bench work. Release images drop the branches
and their strings.

## Uplink (decision 2026-11-03)

The cellular backhaul is a Pi Zero W 2 with a SIMCom HAT, external to this
tree. The ESP32 speaks to it over UART1 (TX 19, RX 20) using the command
and telemetry protocol above; no cellular code, AT parsing, or SIM
credentials live here. The base also keeps its own Wi-Fi STA path for local
telemetry.

## Relay (decision 2026-09-14, MCP23017 rewire 2026-10)

`send` drives six active-low relays through an MCP23017 on I2C1 (SDA 33,
SCL 42, address 0x20), parked HIGH = off at boot via `initRelay()`. Expander
pin → relay: IGN 8, ACC 0, HEADLIGHTS 9, RUNNING_LIGHTS 1, GLOW_PLUGS 10,
HEATER 2. `relayState()` reads the expander output register back. Glow-plug
on-time is the curve `(220 − °F) / 20` seconds, clamped to 1–12 s, with a
1 s floor above 140 °F. Display init never aborts: a missing OLED degrades to
headless, telemetry first.

## Wi-Fi provisioning (decision 2026-09-14)

The base creates the config AP only while it is unprovisioned; once
credentials exist it boots STA-only and the AP is gone. Provisioning is a
JSON REST API over `esp_http_server` — no HTML pages; a Flutter app is the
future client. `POST /api/wifi` {ssid, pass} stores creds in NVS and restarts
into STA; `GET /api/status` reports mode and link state. `WIFI_RESET` on
UART1 erases `sta_ssid`/`sta_pass` and restarts back into the setup AP.
Credentials never appear in tracked files. ESP32 is 2.4 GHz-only:
provision the 2.4 GHz SSID (`Aetheryte_2.4`, not the 5 GHz `Aetheryte` —
verified live 2026-09-14, end-to-end join + mDNS on the home LAN).
