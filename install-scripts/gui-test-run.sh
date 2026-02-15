#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ "${1:-}" != "--inner" ]]; then
  exec dbus-run-session -- "$0" --inner "$@"
fi
shift

if [[ $# -lt 2 ]]; then
  echo "usage: $0 <test-script-name|path> <xreader-bin>" >&2
  exit 2
fi

if [[ "${GUI_TEST_XVFB:-0}" != "1" && "${GUI_TEST_USE_XVFB:-1}" = "1" ]]; then
  if command -v xvfb-run >/dev/null 2>&1; then
    exec env GUI_TEST_XVFB=1 xvfb-run -a -s "-screen 0 1920x1080x24" "$0" --inner "$@"
  fi
fi

TEST_ARG="$1"
XREADER_BIN="$2"
XREADERD_BIN="$(dirname "$XREADER_BIN")/xreaderd"
BUILD_DIR="$(cd "$(dirname "$XREADER_BIN")/.." && pwd)"
SCHEMA_XML="$ROOT_DIR/data/org.x.reader.gschema.xml"
SCHEMA_DIR="$BUILD_DIR/data"
BACKENDS_DIR="$BUILD_DIR/backend"
LAUNCHER_DIR="$BUILD_DIR/test-launchers"
LAUNCHER_SCRIPT="$LAUNCHER_DIR/xreader-test-launch.sh"

if [[ "$TEST_ARG" = /* ]]; then
  TEST_SCRIPT="$TEST_ARG"
else
  TEST_SCRIPT="$ROOT_DIR/test/$TEST_ARG"
fi

if [[ ! -x "$TEST_SCRIPT" ]]; then
  echo "ERROR: test script not executable: $TEST_SCRIPT" >&2
  exit 1
fi

if [[ ! -x "$XREADER_BIN" ]]; then
  echo "ERROR: binary not found: $XREADER_BIN" >&2
  exit 1
fi

if [[ ! -x "$XREADERD_BIN" ]]; then
  echo "ERROR: daemon binary not found: $XREADERD_BIN" >&2
  exit 1
fi

mkdir -p "$SCHEMA_DIR"
if [[ -f "$SCHEMA_XML" ]]; then
  cp -f "$SCHEMA_XML" "$SCHEMA_DIR/"
  glib-compile-schemas "$SCHEMA_DIR"
fi

export XREADER_BACKENDS_DIR="$BACKENDS_DIR"
export GSETTINGS_SCHEMA_DIR="$SCHEMA_DIR"
export GIO_USE_VFS="${GIO_USE_VFS:-local}"
export GTK_USE_PORTAL="${GTK_USE_PORTAL:-0}"
export NO_AT_BRIDGE="${NO_AT_BRIDGE:-0}"
export GTK_MODULES="${GTK_MODULES:-gail:atk-bridge}"
export GVFS_DISABLE_FUSE="${GVFS_DISABLE_FUSE:-1}"
export LANG="${LANG:-C}"

mkdir -p "$LAUNCHER_DIR"
cat >"$LAUNCHER_SCRIPT" <<EOF
#!/usr/bin/env bash
set -euo pipefail
export XREADER_BACKENDS_DIR="$BACKENDS_DIR"
export GSETTINGS_SCHEMA_DIR="$SCHEMA_DIR"
export GIO_USE_VFS="${GIO_USE_VFS:-local}"
export GTK_USE_PORTAL="${GTK_USE_PORTAL:-0}"
export GVFS_DISABLE_FUSE="${GVFS_DISABLE_FUSE:-1}"

if ! gdbus call \\
  --session \\
  --dest org.freedesktop.DBus \\
  --object-path /org/freedesktop/DBus \\
  --method org.freedesktop.DBus.NameHasOwner \\
  org.x.reader.Daemon 2>/dev/null | grep -q "true"; then
  "$XREADERD_BIN" --gapplication-service >/dev/null 2>&1 &
  for _ in \$(seq 1 40); do
    if gdbus call \\
      --session \\
      --dest org.freedesktop.DBus \\
      --object-path /org/freedesktop/DBus \\
      --method org.freedesktop.DBus.NameHasOwner \\
      org.x.reader.Daemon 2>/dev/null | grep -q "true"; then
      break
    fi
    sleep 0.1
  done
fi

if ! gdbus call \\
  --session \\
  --dest org.freedesktop.DBus \\
  --object-path /org/freedesktop/DBus \\
  --method org.freedesktop.DBus.NameHasOwner \\
  org.x.reader.Daemon 2>/dev/null | grep -q "true"; then
  echo "ERROR: org.x.reader.Daemon not available on session bus" >&2
  exit 1
fi

exec "$XREADER_BIN" "\$@"
EOF
chmod +x "$LAUNCHER_SCRIPT"

ATSPI_LAUNCHER="/usr/libexec/at-spi-bus-launcher"
if [[ ! -x "$ATSPI_LAUNCHER" ]]; then
  ATSPI_LAUNCHER="$(command -v at-spi-bus-launcher || true)"
fi

cleanup() {
  if [[ -n "${atspi_pid:-}" ]] && kill -0 "$atspi_pid" 2>/dev/null; then
    kill -TERM "$atspi_pid" 2>/dev/null || true
    wait "$atspi_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

if [[ -n "$ATSPI_LAUNCHER" ]]; then
  "$ATSPI_LAUNCHER" --launch-immediately >/dev/null 2>&1 &
  atspi_pid=$!
fi

set +e
"$TEST_SCRIPT" "$LAUNCHER_SCRIPT"
status=$?
set -e

exit "$status"
