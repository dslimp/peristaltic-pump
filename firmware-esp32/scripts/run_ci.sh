#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE_NAME="peristaltic-fw-ci"
REBUILD=0

if [[ "${1:-}" == "--rebuild" ]]; then
  REBUILD=1
fi

ensure_image() {
  if [[ "$REBUILD" -eq 1 ]]; then
    docker build -t "$IMAGE_NAME" "$ROOT_DIR"
    return
  fi

  if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
    docker build -t "$IMAGE_NAME" "$ROOT_DIR"
  else
    echo "Using cached Docker image: $IMAGE_NAME"
  fi
}
ensure_image

run_with_retry() {
  local cmd="$1"
  local attempts=3
  local attempt=1

  until docker run --rm -v "$ROOT_DIR":/work -w /work "$IMAGE_NAME" bash -lc "$cmd"; do
    if [[ $attempt -ge $attempts ]]; then
      echo "Command failed after $attempts attempts: $cmd" >&2
      return 1
    fi
    echo "Retry $attempt/$attempts for: $cmd" >&2
    attempt=$((attempt + 1))
  done
}

run_with_retry "pio run -e esp32s3"
run_with_retry "pio test -e native"
run_with_retry "cd integration && pytest -q"
