# esp7600LoRa

Firmware and Pi-side uplink for a vehicle telemetry link over LoRa.

- **Vehicle (`send/`)** — ESP32-S3 reads battery voltage, coolant temperature,
  and relay state; packs a 5-byte LoRa frame; sends a JSON copy to the Pi over
  UART1.
- **Base (`recv/`)** — ESP32-S3 receives the frames, shows them on an SSD1306
  OLED, writes JSON to UART1, and offers Wi-Fi provisioning.
- **Pi Zero W 2 + SIMCom HAT** — the cellular backhaul. It receives the JSONL
  telemetry, logs it persistently, and can forward it.

Both firmware apps are ESP-IDF projects (not Arduino) and share one
`components/` tree.

## Architecture

```text
vehicle sensors ──► telemetry task ──► loraTXQueue ──► SX1262 ──► air
       │                                                   │
       └── UART1 JSON `tx` ──► Pi Zero W 2 ──► cellular    │
                                                           ▼
                                                    SX1262 (base)
                                                           │
                                      ┌────────────────────┴───────────┐
                                      ▼                                ▼
                                 SSD1306 OLED                 UART1 JSON `rx`
                                                                       │
                                                                       ▼
                                                                   Pi (optional)
```

The radio link is half-duplex. The Pi is the internet path; LoRa only carries
small telemetry and command records.

## Current status

Implemented and bench-verified:

- ESP32-S3 target for both apps, 8 MB flash, 7.9 MB app partition.
- 5-byte packed telemetry frame with a 10-bit signed coolant field.
- SX1262 radio, non-fatal init, RSSI/SNR, packet-ID gap counter.
- MCP23017 relay expander; glow-plug curve; fail-closed ALLOFF; start abort.
- SSD1306 display; base RX screen with voltage, coolant, RSSI, SNR.
- UART1 JSON for `tx`, `rx`, `state`, and `pong`.
- Base Wi-Fi provisioning: AP only while unprovisioned, STA-only after,
  `WIFI_RESET` over UART1.
- Pi persistent JSONL logger with daily rotation and retention.
- Temporary Heltec V2 (ESP32/SX1276) bench peer for two-node tests.

Not done yet:

- `recv/` is not ported to the Heltec/ESP32. The Heltec is a bench peer only.
- No general internet over LoRa; the Pi owns the internet path.
- No start confirmation from RPM/alternator (deferred until that hardware).

## Hardware

### Vehicle node (S3)

| Function | Pins |
| --- | --- |
| SX1262 | CS 8, SCK 9, MOSI 10, MISO 11, RST 12, BUSY 13, DIO1 14 |
| OLED (I2C0) | SDA 17, SCL 18, RST 21, VEXT 36 |
| MCP23017 (I2C1) | SDA 33, SCL 42, address `0x20` |
| Pi / serial | UART1 TX 19, RX 20, 115200 8N1 |
| Console | UART0 (GPIO43/44), 115200 |

- Battery divider on ADC1 channel 4 (`×5.6`).
- Thermistor on ADC1 channel 1, 1 kΩ fixed, Steinhart-Hart, °F.
- Relay expander outputs are active-low and parked HIGH (off) at boot.

Expander pin → relay: IGN 8, ACC 0, HEADLIGHTS 9, RUNNING_LIGHTS 1,
GLOW_PLUGS 10, HEATER 2.

### Base node

The supported target is the same ESP32-S3/SX1262 pin map. The **Heltec WiFi
LoRa 32 V2** is supported only as a bench peer, not by `recv/`:

| Function | Heltec V2 pins |
| --- | --- |
| SX1276 | SCK 5, MISO 19, MOSI 27, CS 18, RST 14, DIO0 26 |
| OLED (SSD1306) | SDA 4, SCL 15, RST 16, VEXT 21 |

The peer uses 915 MHz, sync word `0x14`, and the same 5-byte frame. The
SX1262 and SX1276 interoperate on the bench (about −22 dBm at the S3 and
−43 dBm at the peer).

### Pi Zero W 2

| Function | Connection |
| --- | --- |
| ESP32 link | `/dev/ttyS0`, 115200 8N1 |
| Cellular | SIMCom HAT (external to this repo) |

Wiring is crossed: Pi TX (GPIO14) → ESP32 RX (GPIO20), Pi RX (GPIO15) ←
ESP32 TX (GPIO19). Use 3.3 V levels.

## Packet format

5 bytes on air, `__attribute__((packed))`, no padding:

| Byte | Bits | Field |
| --- | --- | --- |
| 0 | 0–2 | `packetId` (3 bits, wraps at 8) |
| 0 | 3 | `isVehicle` |
| 0 | 4–7 | `flags` (reserved, send 0) |
| 1 | 0–1 | `ign` (OFF/ON/START) |
| 1 | 2 | `headlights` |
| 1 | 3 | `acc` |
| 1 | 4 | `runningLights` |
| 1 | 5 | `heater` |
| 1 | 6 | `glowPlugs` |
| 1 | 7 | `isWifiEnabled` |
| 2 | 0–7 | `encodedVoltage` (11–16 V → 0–255) |
| 3–4 | 0–9 | `encodedCoolantTemperature` (10-bit signed two's complement °F) |
| 3–4 | 10–15 | `extraData` (reserved, send 0) |

`vehicleState.coolantTemp` is engineering °F; only
`packetToCompactPacket()`/`unpackPacket()` pack the 10-bit field. A
`static_assert` in `components/types/types.h` pins the size and byte offset.
**Both sides must be flashed together after a packet change.**

## Prerequisites

- ESP-IDF **v6.0.2** (the version this tree is built with).
- Python 3 and the IDF Python environment (`idf.py`).
- Network access on the first build: the component manager fetches
  `chill-sam/ssd1306`, `espressif/mdns`, `espressif/cjson`, and
  `esp-idf-lib` into `managed_components/`.
- Two USB-UART ports for a two-node bench (`/dev/ttyUSB0`, `/dev/ttyUSB1`).
- On the Pi: `python3-serial` (`sudo apt install python3-serial`), `gzip`.

```sh
source /opt/esp-idf/export.sh
idf.py --version          # expect v6.0.2
```

## Build

```sh
cd send && idf.py build
cd ../recv && idf.py build
```

Both apps target **ESP32-S3**. The 8 MB flash and 7.9 MB factory partition
come from `send/partitions.csv` and `recv/partitions.csv` plus
`CONFIG_ESPTOOLPY_FLASHSIZE="8MB"` and `CONFIG_PARTITION_TABLE_CUSTOM=y`.
NVS and PHY offsets are unchanged, so Wi-Fi credentials survive a resize.

Debug logs are a compile-time switch, off in the committed `sdkconfig`:

```sh
cd recv
idf.py menuconfig     # esp7600LoRa → Enable telemetry debug logs
# or set CONFIG_TELEMETRY_DEBUG=y in sdkconfig
```

## Flash

```sh
# vehicle
cd send && idf.py -p /dev/ttyUSB0 flash

# base
cd ../recv && idf.py -p /dev/ttyUSB1 flash

# watch (UART0 console)
idf.py -p /dev/ttyUSB0 monitor
```

Never flash `send/` onto the base board or `recv/` onto the vehicle board.
Release the serial port when done. If a board does not enter download mode
automatically, hold BOOT while resetting.

## Wi-Fi provisioning (base)

The base boots **AP-only while unprovisioned**:

- SSID `esp7600LoRa-setup`, open, at `192.168.4.1`.
- `GET /api/status` reports mode, link state, SSID, and IP.
- `POST /api/wifi` with `{"ssid":"...","pass":"..."}` stores credentials in
  NVS and restarts. The passphrase must be 8–63 characters.
- ESP32 is 2.4 GHz-only; provision the 2.4 GHz SSID.

Once credentials exist the base boots **STA-only** and the setup AP is gone.
To reprovision, send `WIFI_RESET` on UART1 (see below); it erases only
`sta_ssid`/`sta_pass` and restarts into the AP.

Credentials never live in this repository.

## UART1 / Pi link

The ESP32 writes one JSON object per line at 115200 8N1. The ESP32's UART0
console is separate.

### Records

```json
{"t":"tx","id":3,"veh":true,"ign":"ON","head":false,"acc":true,"run":false,"heat":false,"glow":false,"wifi":false,"v":12.60,"c":76}
{"t":"rx","id":3,"veh":true,"ign":"ON","head":false,"acc":true,"run":false,"heat":false,"glow":false,"wifi":false,"v":12.60,"c":76,"rssi":-65,"snr":9.5,"drop":0}
{"t":"state","v":12.60,"ign":"ON","head":false,"acc":true,"run":false,"heat":false,"glow":false,"c":76}
{"t":"pong"}
```

- `tx` — the vehicle's own transmitted frame. No RSSI/SNR/drop; `v` is the
  local sensor voltage.
- `rx` — the base's received copy. Adds RSSI (dBm), SNR (dB), and the count
  of unexpected gaps (duplicates, old/reordered frames, and one half-duplex
  TX collision are excluded); `v` is decoded from the 8-bit air field
  (clamped 11–16 V, so a bench input below 11 V reads as 11.0 on the
  receiver).
- `c` is coolant °F. `tx`/`rx` are emitted for every frame, not gated by
  `DEBUG`.

Unknown input is not answered: the bench link shows TX echoed into RX, so an
unconditional reply would feed itself.

### Commands

Newline-terminated ASCII: `START`, `ACC`, `HEADLIGHTS`, `RUNNINGLIGHTS`,
`HEATER`, `GLOWPLUGS`, `ALLOFF`, `TEST`, `STATE`, and `WIFI_RESET` (base
only).

Relay commands are ignored on the base (no relay queue); `WIFI_RESET` is
unavailable on the vehicle (no Wi-Fi).

### Pi serial setup

The Pi's `/dev/ttyS0` must not also be the serial console:

```sh
sudo systemctl disable --now serial-getty@ttyS0
sudo systemctl mask serial-getty@ttyS0
sudo sed -i 's/console=serial0,115200 //' /boot/firmware/cmdline.txt
sudo reboot
```

Verify:

```sh
systemctl is-active serial-getty@ttyS0    # inactive
systemctl is-enabled serial-getty@ttyS0   # masked
grep -o 'console=[^ ]*' /boot/firmware/cmdline.txt   # console=tty1
ls -l /dev/ttyS0                          # root:dialout
```

Add the login user to `dialout` if needed. Set the port once:

```sh
stty -F /dev/ttyS0 115200 raw -echo
timeout 5 cat /dev/ttyS0                  # only while the logger is stopped
```

## Persistent logging on the Pi

The logger service `esp7600-log` reads `/dev/ttyS0`, validates each JSON line,
adds a Pi timestamp, and appends to `/var/log/esp7600/telemetry-YYYY-MM-DD.jsonl`.

- Flush every 5 s, `fsync` every 60 s (bounded loss on power cut).
- Daily rotation, previous day gzipped, 14-day / 200 MB cap.
- `Restart=always`, starts after `dev-ttyS0.device`, runs as `daze:dialout`.
- The service is the **only** reader; a shell `cat` would steal bytes.

Create `/usr/local/bin/esp7600-log.py`:

```python
#!/usr/bin/env python3
import json
import os
import signal
import subprocess
import sys
import time
from datetime import datetime, timedelta
from pathlib import Path

import serial

PORT = os.environ.get("ESP7600_PORT", "/dev/ttyS0")
BAUD = int(os.environ.get("ESP7600_BAUD", "115200"))
LOG_DIR = Path(os.environ.get("ESP7600_LOG_DIR", "/var/log/esp7600"))
KEEP_DAYS = int(os.environ.get("ESP7600_KEEP_DAYS", "14"))
MAX_TOTAL_BYTES = int(os.environ.get("ESP7600_MAX_MB", "200")) * 1024 * 1024
FLUSH_SECONDS = 5
FSYNC_SECONDS = 60

running = True
out = None
current_day = None


def stop(signum, frame):
    global running
    running = False


def prune():
    cutoff = datetime.now() - timedelta(days=KEEP_DAYS)
    files = sorted(LOG_DIR.glob("telemetry-*.jsonl.gz"),
                   key=lambda p: p.stat().st_mtime)
    total = sum(p.stat().st_size for p in LOG_DIR.glob("telemetry-*.jsonl*"))
    for p in files:
        try:
            day = datetime.strptime(p.name[10:20], "%Y-%m-%d")
        except ValueError:
            continue
        if day >= cutoff and total <= MAX_TOTAL_BYTES:
            continue
        try:
            size = p.stat().st_size
            p.unlink()
            total -= size
        except FileNotFoundError:
            pass


def open_day(day):
    global out, current_day
    if out is not None:
        out.flush()
        os.fsync(out.fileno())
        out.close()
    current_day = day
    path = LOG_DIR / f"telemetry-{day}.jsonl"
    out = open(path, "a", buffering=1)
    for p in LOG_DIR.glob("telemetry-*.jsonl"):
        if p.name != path.name:
            subprocess.Popen(["gzip", "-f", str(p)])
    prune()


def main():
    global running
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)

    ser = serial.Serial(PORT, BAUD, timeout=1)
    open_day(datetime.now().strftime("%Y-%m-%d"))
    last_flush = last_fsync = time.monotonic()

    while running:
        raw = ser.readline()
        now = datetime.now()
        day = now.strftime("%Y-%m-%d")
        if day != current_day:
            open_day(day)

        if raw:
            line = raw.decode("utf-8", "replace").strip()
            if line.startswith("{"):
                try:
                    obj = json.loads(line)
                except json.JSONDecodeError:
                    obj = None
                if isinstance(obj, dict):
                    obj["ts"] = now.astimezone().isoformat(timespec="seconds")
                    out.write(json.dumps(obj, separators=(",", ":")) + "\n")

        t = time.monotonic()
        if t - last_flush >= FLUSH_SECONDS:
            out.flush()
            last_flush = t
        if t - last_fsync >= FSYNC_SECONDS:
            out.flush()
            os.fsync(out.fileno())
            last_fsync = t

    if out is not None:
        out.flush()
        os.fsync(out.fileno())
        out.close()
    ser.close()


if __name__ == "__main__":
    main()
```

Create `/etc/systemd/system/esp7600-log.service`:

```ini
[Unit]
Description=esp7600 UART JSON logger
After=dev-ttyS0.device
Requires=dev-ttyS0.device
StartLimitIntervalSec=0

[Service]
Type=simple
User=daze
Group=dialout
ExecStart=/usr/bin/python3 /usr/local/bin/esp7600-log.py
Restart=always
RestartSec=2
NoNewPrivileges=true
PrivateTmp=true
ProtectHome=true
ProtectSystem=full
ReadWritePaths=/var/log/esp7600

[Install]
WantedBy=multi-user.target
```

Install and verify:

```sh
sudo install -m 755 esp7600-log.py /usr/local/bin/esp7600-log.py
sudo install -m 644 esp7600-log.service /etc/systemd/system/esp7600-log.service
sudo install -d -o daze -g dialout -m 0750 /var/log/esp7600
sudo systemctl daemon-reload
sudo systemctl enable --now esp7600-log

systemctl is-active esp7600-log
systemctl is-enabled esp7600-log
tail -n 3 /var/log/esp7600/telemetry-*.jsonl
```

Logs are JSONL with a trailing `"ts"`:

```json
{"t":"tx","id":5,"veh":true,"ign":"OFF","acc":false,"v":3.54,"c":76,"ts":"2026-11-03T05:41:13-08:00"}
```

Retention can be changed with `ESP7600_KEEP_DAYS` / `ESP7600_MAX_MB` in the
unit's environment. The Pi has no RTC; timestamps are correct after NTP
syncs.

## Bench peer (Heltec V2)

The Heltec V2 is a throwaway ESP-IDF peer app, not part of `send/` or
`recv/`. It uses RadioLib's `SX1276` class, 915 MHz, sync word `0x14`, and the
same 5-byte frame. It transmits a known frame every ~2 s and prints any frame
it receives:

```text
I (501) peer: tx id=0 state=0
I (1316) peer: rx len=5 rssi=-42.0 snr=13.0 first=08
```

It is useful for checking the link without a second S3. To rebuild the peer,
create a small ESP-IDF project with `EXTRA_COMPONENT_DIRS` pointing at this
repo's `components/`, `COMPONENTS main radiolib ssd1306`, and the Heltec pins
above. The current temporary project lives in `/tmp/opencode/heltec_peer`.

## Relay, coolant, voltage

- `initRelay()` parks all relays HIGH (off) and sets `relayReady`. A failed
  I2C init leaves the relay task idle; radio and serial keep running.
- Glow-plug on-time is `(220 − °F) / 20` seconds, clamped 1–12 s, with a 1 s
  floor above 140 °F.
- An open/short thermistor is a fault: it logs once and uses the `-512 °F`
  max-glow fallback.
- `ignSequence()` aborts before the starter if the glow plugs fail; a
  running-light failure only warns.
- `allOffSequence()` clears a state flag only after that relay's write
  succeeds, and reports the first failure.
- `voltageToU8()` clamps to 11–16 V in release builds.

## Debug and stack sizes

`CONFIG_TELEMETRY_DEBUG` compiles the debug branches and strings into the
image; release builds drop them. The debug build prints stack/queue high-water
marks on the console.

Task stacks (`send` / `recv`): LoRa RX 4096 / 4096, LoRa TX 4096 / 2048,
Display 3096 / 4096, Serial 4096 / 3096, Relay 2048 (send only), Voltage 2048
(send only), Coolant 2048 (send only), Telemetry 2048 (send only).
The 4096-byte RX stack is required: the RX task carries two large event
structs and the radio call chain.

## Troubleshooting

**No JSON on the Pi**
- Is `/dev/ttyS0` still the serial console? Mask the getty and remove
  `console=serial0` from `cmdline.txt`.
- Is the logger running? It is the only reader; stop it before `cat`.
- Is the login user in `dialout`? Check `ls -l /dev/ttyS0`.
- Are TX/RX crossed? Pi TX → ESP32 RX (GPIO20), Pi RX ← ESP32 TX (GPIO19).
- Baud mismatch: 115200 8N1 on both sides.

**Base AP is not visible**
- A provisioned base is STA-only. Send `WIFI_RESET` on UART1 and reboot.
- The AP is 2.4 GHz-only and open (`esp7600LoRa-setup`).

**Board reboots or the screen flashes**
- Check for `rst:` / `Guru` lines on UART0. A stack overflow showed up this
  way once; the RX stacks are now 4096.
- Decode a backtrace with
  `xtensa-esp32s3-elf-addr2line -pfiaC -e build/<app>.elf <addresses>`.

**Relay init failed**
- Check MCP23017 power, `0x20` addressing, SDA 33 / SCL 42, and 4.7 kΩ
  pull-ups. `relayTask` idles rather than aborting; telemetry continues.

**Battery reads ~3.4 V on the bench**
- No battery is connected; the ADC input is near 0.6 V. The air field clamps
  to 11.0 V, so the receiver shows 11.0 while the `tx` record shows the real
  local voltage.

**Flash size/partition errors**
- The boards are 8 MB. Confirm `CONFIG_ESPTOOLPY_FLASHSIZE="8MB"`,
  `CONFIG_PARTITION_TABLE_CUSTOM=y`, and `partitions.csv` in the app.

## Repository layout

```text
AGENTS.md              constitution and workflow
README.md              this file
.llm/                  workflow, hardware/protocol notes, todo, suggestions
send/                  vehicle app (ESP-IDF, ESP32-S3)
recv/                  base app (ESP-IDF, ESP32-S3)
components/            shared components
  event_bus, events, types
  lora, radiolib, telemetry
  serial, display, relay, voltage, coolant_sensor, adc_shared
  wifi_sta, utils
send/partitions.csv    same 8 MB layout, 0x7F0000 factory
recv/partitions.csv
```

Each app keeps its own `sdkconfig`; a shared-component change must be built in
both apps. Work follows `AGENTS.md`: one step at a time, build both touched
apps, prove on hardware, remove the step, commit code and markdown separately.

## Privacy and safety

- Wi-Fi and cellular credentials never live in this tree. Provision over the
  setup AP or NVS at runtime.
- Do not flash a deployed vehicle node with the base image or vice versa.
- Release the serial port when a monitor exits.
- The setup AP is open while unprovisioned; provision promptly and use
  `WIFI_RESET` for reprovisioning.
