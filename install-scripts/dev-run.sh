#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
XREADER_BIN="$BUILD_DIR/shell/xreader"
XREADERD_BIN="$BUILD_DIR/shell/xreaderd"
SCHEMA_XML="$ROOT_DIR/data/org.x.reader.gschema.xml"
SCHEMA_DIR="$BUILD_DIR/data"
BACKENDS_DIR="$BUILD_DIR/backend"
LOG_DIR="${LOG_DIR:-$BUILD_DIR/dev-run}"

if [[ "${1:-}" != "--inner" ]]; then
  mkdir -p "$LOG_DIR"
  session_timestamp="$(date +%Y%m%d-%H%M%S)-$$"
  session_log="$LOG_DIR/session-$session_timestamp.log"
  if [[ "${DEV_RUN_VERBOSE:-0}" == "1" ]]; then
    exec dbus-run-session -- "$0" --inner "$@"
  fi
  echo "dev-run: session log -> $session_log"
  exec dbus-run-session -- "$0" --inner "$@" 2>"$session_log"
fi
shift

if [[ ! -x "$XREADER_BIN" ]]; then
  echo "ERROR: binary not found: $XREADER_BIN" >&2
  exit 1
fi

if [[ ! -x "$XREADERD_BIN" ]]; then
  echo "ERROR: daemon not found: $XREADERD_BIN" >&2
  exit 1
fi

mkdir -p "$LOG_DIR"
mkdir -p "$SCHEMA_DIR"

if [[ -f "$SCHEMA_XML" ]]; then
  cp -f "$SCHEMA_XML" "$SCHEMA_DIR/"
  glib-compile-schemas "$SCHEMA_DIR"
fi

export XREADER_BACKENDS_DIR="$BACKENDS_DIR"
export GSETTINGS_SCHEMA_DIR="$SCHEMA_DIR"
export GIO_USE_VFS="${GIO_USE_VFS:-local}"
export GTK_USE_PORTAL="${GTK_USE_PORTAL:-0}"

timestamp="$(date +%Y%m%d-%H%M%S)-$$"
daemon_log="$LOG_DIR/xreaderd-$timestamp.log"
xreader_log="$LOG_DIR/xreader-$timestamp.log"

cleanup() {
  if [[ -n "${daemon_pid:-}" ]] && kill -0 "$daemon_pid" 2>/dev/null; then
    kill -TERM "$daemon_pid" 2>/dev/null || true
    wait "$daemon_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

"$XREADERD_BIN" >"$daemon_log" 2>&1 &
daemon_pid=$!

for _ in $(seq 1 30); do
  if ! kill -0 "$daemon_pid" 2>/dev/null; then
    echo "ERROR: xreaderd exited during startup. Log: $daemon_log" >&2
    exit 1
  fi

  if gdbus call \
    --session \
    --dest org.freedesktop.DBus \
    --object-path /org/freedesktop/DBus \
    --method org.freedesktop.DBus.NameHasOwner \
    org.x.reader.Daemon 2>/dev/null | grep -q "true"; then
    break
  fi
  sleep 0.1
done

if ! gdbus call \
  --session \
  --dest org.freedesktop.DBus \
  --object-path /org/freedesktop/DBus \
  --method org.freedesktop.DBus.NameHasOwner \
  org.x.reader.Daemon 2>/dev/null | grep -q "true"; then
  echo "ERROR: org.x.reader.Daemon is not registered. Log: $daemon_log" >&2
  exit 1
fi

echo "dev-run: daemon log -> $daemon_log"
echo "dev-run: app log    -> $xreader_log"

if [[ $# -eq 0 ]]; then
  "$XREADER_BIN" 2>"$xreader_log"
else
  "$XREADER_BIN" "$@" 2>"$xreader_log"
fi
