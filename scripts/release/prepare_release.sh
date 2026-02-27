#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
VERSION_FILE="$ROOT_DIR/VERSION"
CHANGELOG_FILE="$ROOT_DIR/CHANGELOG.md"
FW_MAIN="$ROOT_DIR/firmware-esp32/src/main.cpp"

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <major|minor|patch>" >&2
  exit 1
fi

BUMP_TYPE="$1"
CURRENT_VERSION="$(tr -d ' \n' < "$VERSION_FILE")"

if [[ ! "$CURRENT_VERSION" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)(-.+)?$ ]]; then
  echo "Invalid VERSION format: $CURRENT_VERSION" >&2
  exit 1
fi

MAJOR="${BASH_REMATCH[1]}"
MINOR="${BASH_REMATCH[2]}"
PATCH="${BASH_REMATCH[3]}"
SUFFIX="${BASH_REMATCH[4]:-}"

case "$BUMP_TYPE" in
  major)
    MAJOR=$((MAJOR + 1))
    MINOR=0
    PATCH=0
    ;;
  minor)
    MINOR=$((MINOR + 1))
    PATCH=0
    ;;
  patch)
    PATCH=$((PATCH + 1))
    ;;
  *)
    echo "Unknown bump type: $BUMP_TYPE (use major|minor|patch)" >&2
    exit 1
    ;;
esac

NEW_VERSION="${MAJOR}.${MINOR}.${PATCH}${SUFFIX}"
TODAY="$(date +%Y-%m-%d)"

echo "$NEW_VERSION" > "$VERSION_FILE"

if [[ -n "$(git -C "$ROOT_DIR" tag -l "v${CURRENT_VERSION}")" ]]; then
  COMMITS="$(git -C "$ROOT_DIR" log "v${CURRENT_VERSION}..HEAD" --pretty=format:'- %s' || true)"
else
  COMMITS="$(git -C "$ROOT_DIR" log -n 20 --pretty=format:'- %s' || true)"
fi

if [[ -z "${COMMITS// }" ]]; then
  COMMITS="- Internal release maintenance."
fi

TMP_CHANGELOG="$(mktemp)"
{
  echo "# Changelog"
  echo
  echo "All notable changes to this project are documented in this file."
  echo
  echo "## [${NEW_VERSION}] - ${TODAY}"
  echo
  echo "$COMMITS"
  echo
  if [[ -f "$CHANGELOG_FILE" ]]; then
    sed '1,4d' "$CHANGELOG_FILE"
  fi
} > "$TMP_CHANGELOG"
mv "$TMP_CHANGELOG" "$CHANGELOG_FILE"

if [[ -f "$FW_MAIN" ]]; then
  sed -i.bak -E "s/constexpr char kFirmwareVersion\\[\\] = \\\"[^\"]+-esp32\\\";/constexpr char kFirmwareVersion[] = \\\"${NEW_VERSION}-esp32\\\";/" "$FW_MAIN"
  rm -f "$FW_MAIN.bak"
fi

echo "Release prepared: $CURRENT_VERSION -> $NEW_VERSION"
echo "Updated: VERSION, CHANGELOG.md, firmware version constant"
