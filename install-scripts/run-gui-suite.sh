#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
XREADER_BIN="$BUILD_DIR/shell/xreader"
XREADERD_BIN="$BUILD_DIR/shell/xreaderd"
TIMESTAMP="$(date +%Y%m%d-%H%M%S)"
OUT_DIR_DEFAULT="$BUILD_DIR/gui-suite/$TIMESTAMP"
OUT_DIR="${OUT_DIR:-$OUT_DIR_DEFAULT}"
SUMMARY_FILE="$OUT_DIR/summary.txt"

mkdir -p "$OUT_DIR"

if [[ ! -x "$XREADER_BIN" ]]; then
  echo "ERROR: xreader binary not found: $XREADER_BIN" >&2
  exit 1
fi

if [[ ! -x "$XREADERD_BIN" ]]; then
  echo "ERROR: xreaderd binary not found: $XREADERD_BIN" >&2
  exit 1
fi

write_summary() {
  printf '%s\n' "$1" | tee -a "$SUMMARY_FILE"
}

sanitize_name() {
  printf '%s' "$1" | tr -cs 'A-Za-z0-9._-' '_'
}

run_probe_atspi() {
  local launcher="$1"
  dbus-run-session -- bash -lc "
    set -e
    '$launcher' --launch-immediately >/dev/null 2>&1 &
    pid=\$!
    sleep 0.3
    gdbus call --session \
      --dest org.freedesktop.DBus \
      --object-path /org/freedesktop/DBus \
      --method org.freedesktop.DBus.NameHasOwner org.a11y.Bus
    kill -TERM \$pid >/dev/null 2>&1 || true
    wait \$pid 2>/dev/null || true
  "
}

run_probe_daemon() {
  local daemon_bin="$1"
  dbus-run-session -- bash -lc "
    set -e
    '$daemon_bin' --gapplication-service >/dev/null 2>&1 &
    pid=\$!
    sleep 0.4
    gdbus call --session \
      --dest org.freedesktop.DBus \
      --object-path /org/freedesktop/DBus \
      --method org.freedesktop.DBus.NameHasOwner org.x.reader.Daemon
    kill -TERM \$pid >/dev/null 2>&1 || true
    wait \$pid 2>/dev/null || true
  "
}

{
  echo "timestamp=$TIMESTAMP"
  echo "root_dir=$ROOT_DIR"
  echo "build_dir=$BUILD_DIR"
  echo "out_dir=$OUT_DIR"
  echo "xreader_bin=$XREADER_BIN"
  echo "xreaderd_bin=$XREADERD_BIN"
  echo
  echo "[env]"
  echo "DISPLAY=${DISPLAY:-NA}"
  echo "WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-NA}"
  echo "DBUS_SESSION_BUS_ADDRESS=${DBUS_SESSION_BUS_ADDRESS:-NA}"
  echo "XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-NA}"
  echo
  echo "[tools]"
  echo "meson=$(command -v meson || echo NA)"
  echo "gdbus=$(command -v gdbus || echo NA)"
  echo "dbus-run-session=$(command -v dbus-run-session || echo NA)"
  echo "at-spi-bus-launcher=$(command -v at-spi-bus-launcher || echo /usr/libexec/at-spi-bus-launcher)"
  echo "xvfb-run=$(command -v xvfb-run || echo NA)"
} > "$SUMMARY_FILE"

write_summary ""
write_summary "[probes]"

if [[ -n "${DBUS_SESSION_BUS_ADDRESS:-}" ]]; then
  if gdbus call --session --dest org.freedesktop.DBus \
      --object-path /org/freedesktop/DBus \
      --method org.freedesktop.DBus.NameHasOwner org.a11y.Bus \
      >>"$SUMMARY_FILE" 2>&1; then
    write_summary "current_session_a11y_bus=OK"
  else
    write_summary "current_session_a11y_bus=FAIL"
  fi
else
  write_summary "current_session_a11y_bus=NO_SESSION_BUS"
fi

ATSPI_LAUNCHER="/usr/libexec/at-spi-bus-launcher"
if [[ ! -x "$ATSPI_LAUNCHER" ]]; then
  ATSPI_LAUNCHER="$(command -v at-spi-bus-launcher || true)"
fi

if [[ -n "$ATSPI_LAUNCHER" && -x "$ATSPI_LAUNCHER" ]]; then
  if run_probe_atspi "$ATSPI_LAUNCHER" >>"$SUMMARY_FILE" 2>&1; then
    write_summary "isolated_probe_atspi=OK"
  else
    write_summary "isolated_probe_atspi=FAIL"
  fi
else
  write_summary "isolated_probe_atspi=LAUNCHER_NOT_FOUND"
fi

if run_probe_daemon "$XREADERD_BIN" >>"$SUMMARY_FILE" 2>&1; then
  write_summary "isolated_probe_xreaderd=OK"
else
  write_summary "isolated_probe_xreaderd=FAIL"
fi

declare -a tests=()
if [[ $# -gt 0 ]]; then
  tests=("$@")
else
  mapfile -t tests < <(meson test -C "$BUILD_DIR" --list | sed '/^$/d')
fi

if [[ ${#tests[@]} -eq 0 ]]; then
  write_summary ""
  write_summary "ERROR: no GUI tests discovered"
  exit 1
fi

write_summary ""
write_summary "[suite]"
write_summary "tests_total=${#tests[@]}"

pass_count=0
fail_count=0
declare -a failed_tests=()

for test_name in "${tests[@]}"; do
  safe_name="$(sanitize_name "$test_name")"
  test_log="$OUT_DIR/$safe_name.log"
  start_s="$(date +%s)"

  if meson test -C "$BUILD_DIR" --no-rebuild "$test_name" --print-errorlogs >"$test_log" 2>&1; then
    status="PASS"
    pass_count=$((pass_count + 1))
  else
    status="FAIL"
    fail_count=$((fail_count + 1))
    failed_tests+=("$test_name")
  fi

  elapsed_s=$(( $(date +%s) - start_s ))
  write_summary "test=$test_name status=$status elapsed_s=$elapsed_s log=$test_log"
done

write_summary ""
write_summary "[result]"
write_summary "pass=$pass_count"
write_summary "fail=$fail_count"

if [[ $fail_count -gt 0 ]]; then
  write_summary "failed_tests=$(IFS=,; echo "${failed_tests[*]}")"
  for failed in "${failed_tests[@]}"; do
    safe_name="$(sanitize_name "$failed")"
    test_log="$OUT_DIR/$safe_name.log"
    write_summary ""
    write_summary "[failure:$failed]"
    grep -E "FAIL|ERROR|Traceback|ServiceUnknown|AT-SPI|Couldn't connect|No such file|exit status" \
      "$test_log" | tail -n 30 >>"$SUMMARY_FILE" || true
  done
fi

echo
echo "GUI suite finished."
echo "summary: $SUMMARY_FILE"

if [[ $fail_count -gt 0 ]]; then
  exit 1
fi
