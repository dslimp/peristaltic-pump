# Changelog

All notable changes to this project are documented in this file.

## [0.2.8] - 2026-02-27

- Reworked Growth schedule generation to enforce a global pause between all active channels (no cross-group overlap).
- Extracted Growth schedule logic into a dedicated `growth-schedule.js` module and integrated it in Web UI.
- Added frontend/integration coverage for Growth schedule spacing, motor remap behavior, and release workflow chip target validation.
- Restructured Calibration/Settings tab into logical subpanels (system, expansion, aliases, calibration, security, OTA).
- Fixed release flashing manual command to use `esp32s3` in generated artifact docs.

## [0.2.7] - 2026-02-26

- Growth Program schedule now applies time offsets between channels (no simultaneous dosing).
- Added configurable pause between channels in Growth tab (`Pause between channels`, default `10` min).
- Applied pause sequencing to both nutrient channels and pH channels in generated schedule.

## [0.2.6] - 2026-02-26

- Fixed Growth Program schedule creation when fewer than 5 pump channels are active.
- Added channel-to-motor remapping for growth schedule generation on reduced hardware configs.
- Added live hardware API tests (real device, no emulator): settings roundtrip, schedule add/edit/delete, firmware release list endpoint.
- Updated README with command to run live hardware tests.

## [0.2.5] - 2026-02-26

- Switched main firmware build/release target to `esp32s3` only (removed `esp32dev` target from build flows).
- Updated CI and release workflows to produce ESP32-S3 artifacts and flashing scripts.
- Updated local CI helper script to build `esp32s3`.
- Updated README and AGENTS config to document ESP32-S3-only target.
- Fixed GitHub release list parsing for device OTA by reducing release list size and increasing JSON buffers.
- Improved OTA firmware failure diagnostics with compatibility hint for wrong chip image.

## [0.2.4] - 2026-02-26

- Fixed OTA filesystem update flow for GitHub Release assets that respond with HTTP redirects (302).
- Replaced OTA download path with redirect-aware HTTP stream update for both `littlefs.bin` and `firmware.bin`.
- Improved OTA error payload details for filesystem/firmware update failures.

## [0.2.3] - 2026-02-26

- Added Growth Program tab workflow for hydroponics with schedule generation from plant phase and water volume.
- Added JSON import/export for growth programs with schema versioning (`peristaltic.growth-programs`, v1).
- Added migration support for legacy growth profile formats during JSON import.
- Added default catalog of 10 Aquatica TriPart-based programs (popular crops).
- Added settings toggle to enable/disable pH regulation in generated growth schedules.
- Added API settings field `phRegulationEnabled` with persistence and integration test coverage.
- Added documentation for growth programs JSON format and migration behavior.

## [0.2.2] - 2026-02-26

- Merge pull request #5 from dslimp/release/0.2.1
- Merge pull request #3 from dslimp/release/0.2.0
- release: v0.2.1
- ci: fix YAML indentation in release artifacts workflow
- build: fix release prepare script for semver suffixes
- release: prepare v0.2.0 (i18n UI/OLED, schedule weekdays, HA docs)
- Merge pull request #2 from dslimp/release/0.1.0-alpha
- Include filesystem and full flash bundle in CI artifacts
- Merge pull request #1 from dslimp/release/0.1.0-alpha
- release: v0.1.0-alpha
- Simplify gitignore and ignore local agent config
- Add versioning, changelog flow, and release branch artifacts
- Initial commit

## [0.2.1] - 2026-02-25

- Added modular expansion architecture: central controller + extension board support.
- Added expansion firmware target and implementation for ESP32-S3 extension module (up to 4 DRV8825 motors).
- Added working master<->expansion protocol over I2C with autodiscovery.
- Added multi-motor support in Web UI and API (base motor + expansion motors).
- Added per-schedule motor selection, weekday mask, and schedule entry naming.
- Added editing of existing schedule entries in Web UI.
- Added per-motor aliases in settings and UI display (replacing generic "Motor N" labels).
- Added NTP timezone offset selection in settings and unified schedule time handling with selected offset.
- Updated RU UI terminology: "Поток дозировки (L/h)" -> "Дозировка (L/h)" and kept JSON term unlocalized as "JSON".
- Updated docs and release workflows, including explicit firmware/filesystem flashing instructions and release artifact workflow fixes.
- Extended integration tests and kept native unit tests green for updated API/motor behavior.

## [Unreleased]

- Internal release maintenance.

## [0.2.0] - 2026-02-25

- Added dual-language Web UI (English/Russian) with persisted language preference.
- Added dual-language OLED status output (EN/RU toggle via UI preferences).
- Added current device time in top status chips.
- Extended schedule entries with weekday selection (Mon-Sun).
- Fixed schedule trigger behavior to skip execution if pump is busy at trigger time.
- Embedded full Home Assistant setup guide directly into Web UI tab.
- Added standalone Home Assistant guide document:
  `docs/HOME_ASSISTANT.md`.

## [0.1.0-alpha] - 2026-02-24

- Initial ESP32 release baseline.
