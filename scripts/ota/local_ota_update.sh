#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
FW_DIR="$ROOT_DIR/firmware-esp32"
BUILD_DIR="$FW_DIR/.pio/build/esp32s3"

DEVICE_URL=""
HOST_IP=""
PORT="18080"
AUTH=""
NO_BUILD="0"

usage() {
  cat <<'EOF'
Usage:
  ./scripts/ota/local_ota_update.sh --device http://192.168.88.68 [options]

Options:
  --device <url|ip>   Device base URL or IP (required)
  --host <ip>         Host IP visible from ESP32 (default: auto-detect)
  --port <port>       Local HTTP server port (default: 18080)
  --auth <user:pass>  Basic auth for device API (optional)
  --no-build          Skip PlatformIO build step
  -h, --help          Show this help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --device)
      DEVICE_URL="${2:-}"
      shift 2
      ;;
    --host)
      HOST_IP="${2:-}"
      shift 2
      ;;
    --port)
      PORT="${2:-}"
      shift 2
      ;;
    --auth)
      AUTH="${2:-}"
      shift 2
      ;;
    --no-build)
      NO_BUILD="1"
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if [[ -z "$DEVICE_URL" ]]; then
  echo "Missing --device" >&2
  usage
  exit 1
fi

if [[ "$DEVICE_URL" != http://* && "$DEVICE_URL" != https://* ]]; then
  DEVICE_URL="http://${DEVICE_URL}"
fi
DEVICE_URL="${DEVICE_URL%/}"

if [[ -z "$HOST_IP" ]]; then
  if command -v ipconfig >/dev/null 2>&1; then
    HOST_IP="$(ipconfig getifaddr en0 2>/dev/null || true)"
    if [[ -z "$HOST_IP" ]]; then
      HOST_IP="$(ipconfig getifaddr en1 2>/dev/null || true)"
    fi
  fi
fi
if [[ -z "$HOST_IP" ]]; then
  echo "Cannot auto-detect host IP. Please pass --host <ip>." >&2
  exit 1
fi

if [[ "$NO_BUILD" != "1" ]]; then
  echo "[1/4] Build firmware and filesystem..."
  (cd "$FW_DIR" && pio run -e esp32s3 && pio run -e esp32s3 -t buildfs)
fi

if [[ ! -f "$BUILD_DIR/firmware.bin" || ! -f "$BUILD_DIR/littlefs.bin" ]]; then
  echo "Build artifacts not found in $BUILD_DIR" >&2
  exit 1
fi

TMP_DIR="$(mktemp -d)"
SERVER_PID=""
cleanup() {
  if [[ -n "$SERVER_PID" ]]; then
    kill "$SERVER_PID" >/dev/null 2>&1 || true
  fi
  rm -rf "$TMP_DIR"
}
trap cleanup EXIT

cp "$BUILD_DIR/firmware.bin" "$TMP_DIR/firmware.bin"
cp "$BUILD_DIR/littlefs.bin" "$TMP_DIR/littlefs.bin"

echo "[2/4] Start local artifact server on ${HOST_IP}:${PORT}..."
python3 -m http.server "$PORT" --bind 0.0.0.0 --directory "$TMP_DIR" >/tmp/peristaltic-local-ota.log 2>&1 &
SERVER_PID="$!"
sleep 1

FW_URL="http://${HOST_IP}:${PORT}/firmware.bin"
FS_URL="http://${HOST_IP}:${PORT}/littlefs.bin"

echo "[3/4] Trigger OTA update on ${DEVICE_URL}..."
PAYLOAD="$(cat <<EOF
{"mode":"url","url":"${FW_URL}","filesystemUrl":"${FS_URL}","assetName":"firmware.bin","filesystemAssetName":"littlefs.bin"}
EOF
)"

CURL_OPTS=(-sS --max-time 180 -H "Content-Type: application/json" -X POST "${DEVICE_URL}/api/firmware/update" -d "$PAYLOAD")
if [[ -n "$AUTH" ]]; then
  CURL_OPTS=(-u "$AUTH" "${CURL_OPTS[@]}")
fi
RESPONSE="$(curl "${CURL_OPTS[@]}")"
echo "$RESPONSE"
if [[ "$RESPONSE" == *"\"error\""* ]]; then
  echo "OTA request failed" >&2
  exit 1
fi

echo "[4/4] Waiting for device reboot..."
for _ in $(seq 1 60); do
  STATE_OPTS=(-sS --max-time 4 "${DEVICE_URL}/api/state")
  if [[ -n "$AUTH" ]]; then
    STATE_OPTS=(-u "$AUTH" "${STATE_OPTS[@]}")
  fi
  STATE="$(curl "${STATE_OPTS[@]}" || true)"
  if [[ -n "$STATE" ]]; then
    echo "$STATE"
    echo "Local OTA update completed."
    exit 0
  fi
  sleep 2
done

echo "Device did not come back in time." >&2
exit 1
