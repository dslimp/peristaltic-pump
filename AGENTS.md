# AGENTS.md

## Profile

- Name: `peristaltic-local-release`
- Repo: `/Users/user/Documents/peristaltic`
- Communication language: `ru`
- Communication style: `short-pragmatic`

## Git Branches

- Default branch: `master`
- Release branch pattern: `release/*`
- Remote name: `origin`
- Do not use the `codex/` prefix for branch names.
- Create feature branches without any forced prefix (for example: `feature-modular-expansion`).

## Workflow

- Default bump: `patch`
- Release prepare command: `./scripts/release/prepare_release.sh ${bump}`
- Release commit message: `release: v${version}`
- Normal commit style: `short-imperative`

## CI

- Main workflow: `.github/workflows/master-ci.yml`
- Release workflow: `.github/workflows/release-artifacts.yml`
- Primary firmware target: `esp32s3` (central board). Do not use `esp32dev`.
- Integration tests command: `cd firmware-esp32/integration && pytest -q`
- Unit tests hint: `pio test -e native`
- Hardware device base URL: `http://192.168.88.68`

## API Contract

- Mode: `flow_and_dosing_only`
- Required endpoints:
- `/api/flow`
- `/api/dosing`
- Forbidden endpoints:
- `/api/speed`
- `/api/rpm`
- `/api/bottling`

## Product Scope

- Mobile app enabled: `false`
- Mobile app is intentionally removed and must not be reintroduced unless explicitly requested.

## Git Rules

- Force push: only if user explicitly requests.
- History squash/rewrite: only if user explicitly requests.
- Prefer no history rewrite by default.
- Push target: `origin/master`
- Release pushes only to `release/*` when doing release flow.

## Repo Hygiene

- Keep `.gitignore` simple.
- Local config file: `peristaltic-local.toml`
- Local config must stay gitignored.

## Release Checklist

- Update `VERSION`.
- Prepend `CHANGELOG.md`.
- Sync firmware version constant in `firmware-esp32/src/main.cpp`.
- Run integration tests.
- Review all release-impacting changes.
- Commit and push to `release/*` to build artifacts.

## Release Review Mode

- Enabled: `true`
- Role: `reviewer`
- Strict: `true`
- Report findings first by severity.
- Verify API contract has no forbidden endpoints.
- Check changelog/version consistency.
- Check workflow trigger and artifact names.
- Scan for regressions in firmware and tests.

## Attention Points

- Do not restore removed RPM mode.
- Do not add backward compatibility unless explicitly requested.
- Do not bring back mobile app by default.
- Do not rewrite git history by default.
- During release, act as a strict reviewer.
