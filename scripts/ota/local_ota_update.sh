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
  --host <ip>         Host IP visible from ESP32 (optional override)
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

if [[ "$NO_BUILD" != "1" ]]; then
  echo "[1/6] Build firmware and filesystem..."
  (cd "$FW_DIR" && pio run -e esp32s3 && pio run -e esp32s3 -t buildfs)
fi

if [[ ! -f "$BUILD_DIR/firmware.bin" || ! -f "$BUILD_DIR/littlefs.bin" ]]; then
  echo "Build artifacts not found in $BUILD_DIR" >&2
  exit 1
fi

curl_opts=(-sS --max-time 8)
if [[ -n "$AUTH" ]]; then
  curl_opts=(-u "$AUTH" "${curl_opts[@]}")
fi

wait_for_reboot() {
  echo "[5/6] Wait for device reboot..."
  for _ in $(seq 1 60); do
    STATE="$(curl "${curl_opts[@]}" --max-time 4 "${DEVICE_URL}/api/state" || true)"
    if [[ -n "$STATE" ]]; then
      echo "$STATE"
      echo "[6/6] Local OTA update completed."
      return 0
    fi
    sleep 2
  done
  return 1
}

echo "[2/6] Try direct local upload mode (no host HTTP server)..."
FS_UPLOAD_RESPONSE="$(curl "${curl_opts[@]}" --max-time 240 -X POST -F "file=@${BUILD_DIR}/littlefs.bin;type=application/octet-stream" "${DEVICE_URL}/api/firmware/upload/filesystem" || true)"
if [[ "$FS_UPLOAD_RESPONSE" == *"\"ok\":true"* ]]; then
  echo "$FS_UPLOAD_RESPONSE"
  FW_UPLOAD_RESPONSE="$(curl "${curl_opts[@]}" --max-time 240 -X POST -F "file=@${BUILD_DIR}/firmware.bin;type=application/octet-stream" "${DEVICE_URL}/api/firmware/upload/firmware" || true)"
  echo "$FW_UPLOAD_RESPONSE"
  if [[ "$FW_UPLOAD_RESPONSE" == *"\"ok\":true"* ]]; then
    if wait_for_reboot; then
      exit 0
    fi
    echo "Device did not come back in time." >&2
    exit 1
  fi
fi

if [[ "$FS_UPLOAD_RESPONSE" == *"404"* || "$FS_UPLOAD_RESPONSE" == *"Not Found"* ]]; then
  echo "Direct upload endpoint is unavailable on device firmware. Fallback to URL mode."
else
  echo "Direct upload mode did not complete, fallback to URL mode."
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

echo "[3/6] Start local artifact server on 0.0.0.0:${PORT}..."
python3 -m http.server "$PORT" --bind 0.0.0.0 --directory "$TMP_DIR" >/tmp/peristaltic-local-ota.log 2>&1 &
SERVER_PID="$!"
sleep 1

add_candidate() {
  local candidate="$1"
  if [[ -z "$candidate" ]]; then return; fi
  for existing in "${HOST_CANDIDATES[@]-}"; do
    if [[ "$existing" == "$candidate" ]]; then return; fi
  done
  HOST_CANDIDATES+=("$candidate")
}

extract_device_host() {
  local raw="${DEVICE_URL#http://}"
  raw="${raw#https://}"
  raw="${raw%%/*}"
  echo "${raw%%:*}"
}

declare -a HOST_CANDIDATES=()
if [[ -n "$HOST_IP" ]]; then
  add_candidate "$HOST_IP"
fi

DEVICE_HOST="$(extract_device_host)"
if command -v route >/dev/null 2>&1 && command -v ipconfig >/dev/null 2>&1; then
  IFACE="$(route -n get "$DEVICE_HOST" 2>/dev/null | awk '/interface:/{print $2; exit}' || true)"
  if [[ -n "$IFACE" ]]; then
    IFACE_IP="$(ipconfig getifaddr "$IFACE" 2>/dev/null || true)"
    add_candidate "$IFACE_IP"
  fi
  add_candidate "$(ipconfig getifaddr en0 2>/dev/null || true)"
  add_candidate "$(ipconfig getifaddr en1 2>/dev/null || true)"
fi

if [[ ${#HOST_CANDIDATES[@]} -eq 0 ]]; then
  echo "Cannot auto-detect host IP. Pass --host <ip>." >&2
  exit 1
fi

probe_url() {
  local url="$1"
  curl "${curl_opts[@]}" --get --data-urlencode "url=${url}" "${DEVICE_URL}/api/firmware/probe" || true
}

PROBE_SUPPORTED="1"
SELECTED_HOST=""
FW_URL=""
FS_URL=""

echo "[4/6] Resolve host IP reachable from ESP32..."
for candidate in "${HOST_CANDIDATES[@]}"; do
  test_fw="http://${candidate}:${PORT}/firmware.bin"
  test_fs="http://${candidate}:${PORT}/littlefs.bin"

  if ! curl -fsS --max-time 5 "$test_fw" -o /dev/null; then
    continue
  fi
  if ! curl -fsS --max-time 5 "$test_fs" -o /dev/null; then
    continue
  fi

  probe_fw="$(probe_url "$test_fw")"
  probe_fs="$(probe_url "$test_fs")"
  if [[ "$probe_fw" == *"\"ok\":true"* && "$probe_fs" == *"\"ok\":true"* ]]; then
    SELECTED_HOST="$candidate"
    FW_URL="$test_fw"
    FS_URL="$test_fs"
    break
  fi

  if [[ "$probe_fw" == *"404"* || "$probe_fw" == *"Not Found"* || "$probe_fw" == *"\"error\":\"url query arg is required\""* ]]; then
    PROBE_SUPPORTED="0"
    SELECTED_HOST="$candidate"
    FW_URL="$test_fw"
    FS_URL="$test_fs"
    break
  fi
done

if [[ -z "$SELECTED_HOST" ]]; then
  echo "No reachable host IP candidate for local OTA. Checked: ${HOST_CANDIDATES[*]}" >&2
  echo "Server log tail:" >&2
  tail -n 50 /tmp/peristaltic-local-ota.log >&2 || true
  exit 1
fi

if [[ "$PROBE_SUPPORTED" == "1" ]]; then
  echo "Selected host IP: ${SELECTED_HOST} (device probe passed)"
else
  echo "Selected host IP: ${SELECTED_HOST} (probe endpoint unavailable, using compatibility mode)"
fi

echo "[4/6] Trigger OTA update on ${DEVICE_URL}..."
PAYLOAD="$(cat <<EOF
{"mode":"url","url":"${FW_URL}","filesystemUrl":"${FS_URL}","assetName":"firmware.bin","filesystemAssetName":"littlefs.bin"}
EOF
)"

RESPONSE="$(curl "${curl_opts[@]}" -H "Content-Type: application/json" -X POST "${DEVICE_URL}/api/firmware/update" -d "$PAYLOAD")"
echo "$RESPONSE"
if [[ "$RESPONSE" == *"\"error\""* ]]; then
  echo "OTA request failed. Server log tail:" >&2
  tail -n 80 /tmp/peristaltic-local-ota.log >&2 || true
  exit 1
fi

if wait_for_reboot; then
  exit 0
fi
echo "Device did not come back in time." >&2
exit 1
