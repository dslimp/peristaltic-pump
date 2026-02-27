# Peristaltic Pump (ESP32-S3)

ESP32-S3 firmware for a DIY Nema-based peristaltic pump.

## Main features

- Pump control by flow (L/h)
- Fixed-volume dosing mode
- Calibration for CW/CCW directions
- Smooth ramp-up/ramp-down
- Runtime and pumped-volume counters
- Wi-Fi provisioning via AP portal
- Embedded Web UI (`http://<esp32s3-ip>/`)
- Growth Program builder (plant + fertilizer type -> dosing profile)
- Home Assistant integration tab in Web UI
- HTTP JSON API for external control
- Modular expansion over RS-485 (autodiscovery, up to 4 extra motors)

## API endpoints

- `GET /api/state`
- `POST /api/start`
- `POST /api/stop`
- `POST /api/flow` body `{ "litersPerHour": 6.0, "reverse": false }`
- `POST /api/dosing` body `{ "volumeMl": 500, "reverse": false }`
- `GET /api/settings`
- `POST /api/settings`
- `POST /api/calibration/run`
- `POST /api/calibration/apply`
- `GET /api/wifi`
- `POST /api/wifi/reset`
- `GET /api/firmware/config`
- `POST /api/firmware/config`
- `GET /api/firmware/releases`
- `POST /api/firmware/update`
  - GitHub release mode: `{ "mode": "latest" }` or `{ "mode": "tag", "tag": "v0.2.8" }`
  - Local URL mode: `{ "mode": "url", "url": "http://<host>/firmware.bin", "filesystemUrl": "http://<host>/littlefs.bin" }`

## Build and flash

Do not run plain `pio run -t upload` in this project, because it can try all environments.
Always specify `-e` and (recommended) explicit `--upload-port`.

List serial ports:

```bash
pio device list
```

Central board firmware:

```bash
cd firmware-esp32
pio run -e esp32s3 -t upload --upload-port /dev/cu.wchusbserialXXXX
pio device monitor -p /dev/cu.wchusbserialXXXX -b 115200
```

Expansion board firmware (RS-485):

```bash
cd firmware-esp32
pio run -e esp32s3-expansion -t upload --upload-port /dev/cu.wchusbserialYYYY
pio device monitor -p /dev/cu.wchusbserialYYYY -b 115200
```

Protocol details:
- `docs/EXPANSION_RS485_PROTOCOL.md`

## Web UI files

- `firmware-esp32/data/index.html`
- `firmware-esp32/data/styles.css`
- `firmware-esp32/data/app.js`

After UI changes:

```bash
cd firmware-esp32
pio run -e esp32s3 -t uploadfs --upload-port /dev/cu.wchusbserialXXXX
```

`uploadfs` is only for the central firmware (`esp32s3`) where Web UI is served.

## CI and tests

GitHub Actions (for `master`) runs:

- Firmware build: `pio run -e esp32s3`
- Unit tests: `pio test -e native`
- Integration tests: `pytest -q`

Local CI run in Docker:

```bash
cd firmware-esp32
./scripts/run_ci.sh
```

Live hardware API tests (real device, no emulator):

```bash
cd firmware-esp32/integration
PERISTALTIC_HW_BASE="http://192.168.88.68" pytest -q tests/test_api_hardware_live.py
```

Local OTA update without creating GitHub release:

```bash
./scripts/ota/local_ota_update.sh --device http://192.168.88.68 --host 192.168.88.26
```

This command:
- builds `firmware.bin` and `littlefs.bin`
- serves them from your machine over HTTP
- triggers device OTA with `mode=url`
- waits until the device reboots and returns `/api/state`

## Versioning and release branches

- Current project version is stored in `VERSION`
- Release notes are stored in `CHANGELOG.md`
- Release artifact builds are triggered by pushing to `release/*` branches

Prepare next version locally:

```bash
./scripts/release/prepare_release.sh patch
# or: minor / major
```

This updates:

- `VERSION`
- `CHANGELOG.md`
- firmware version constant in `firmware-esp32/src/main.cpp`

Release artifacts for `release/*` branches include full flashing bundle
(`bootloader.bin`, `partitions.bin`, `firmware.bin`, `littlefs.bin`, and flash scripts).

Flashing guide:
- `docs/FLASHING_RELEASE.md`

Home Assistant guide:
- `docs/HOME_ASSISTANT.md`

Growth Program docs:
- `docs/GROWTH_PROGRAMS_JSON.md`
- `docs/GROWTH_PROGRAM_SOURCES.md`
