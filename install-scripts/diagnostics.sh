#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
PDF_INPUT="${1:-$ROOT_DIR/test/test-links.pdf}"
OUT_DIR="${OUT_DIR:-$BUILD_DIR/hop-diagnostics}"
mkdir -p "$OUT_DIR"

XREADER_BIN="$BUILD_DIR/shell/xreader"
SCHEMA_XML="$ROOT_DIR/data/org.x.reader.gschema.xml"
SCHEMA_DIR="$BUILD_DIR/data"
BACKENDS_DIR="$BUILD_DIR/backend"

if [[ ! -x "$XREADER_BIN" ]]; then
  echo "ERROR: binary not found: $XREADER_BIN" >&2
  exit 1
fi

if [[ ! -f "$PDF_INPUT" ]]; then
  echo "ERROR: input file not found: $PDF_INPUT" >&2
  exit 1
fi

if [[ -f "$SCHEMA_XML" ]]; then
  cp -f "$SCHEMA_XML" "$SCHEMA_DIR/"
  glib-compile-schemas "$SCHEMA_DIR"
fi

export XREADER_BACKENDS_DIR="$BACKENDS_DIR"
export GSETTINGS_SCHEMA_DIR="$SCHEMA_DIR"

timestamp="$(date +%Y%m%d-%H%M%S)"
report="$OUT_DIR/report-$timestamp.txt"
run_out="$OUT_DIR/run-$timestamp.out"
run_err="$OUT_DIR/run-$timestamp.err"
smoke_out="$OUT_DIR/smoke-$timestamp.out"
smoke_err="$OUT_DIR/smoke-$timestamp.err"

{
  echo "timestamp=$timestamp"
  echo "root_dir=$ROOT_DIR"
  echo "build_dir=$BUILD_DIR"
  echo "xreader_bin=$XREADER_BIN"
  echo "pdf_input=$PDF_INPUT"
  echo
  echo "[timing_help_mode]"
} > "$report"

help_times=()
for _ in 1 2 3 4 5; do
  t="$(
    /usr/bin/time -f "%e" "$XREADER_BIN" --help 2>&1 >/dev/null </dev/null
  )"
  help_times+=("$t")
done

{
  echo "help_runs=${#help_times[@]}"
  echo "help_times_s=$(IFS=,; echo "${help_times[*]}")"
} >> "$report"

echo >> "$report"
echo "[smoke_test]" >> "$report"
set +e
G_MESSAGES_DEBUG=all timeout 3s "$XREADER_BIN" "$PDF_INPUT" >"$run_out" 2>"$run_err"
run_status=$?
set -e
echo "run_status=$run_status" >> "$report"
echo "run_status_note=124 means process stayed alive during timeout window" >> "$report"

set +e
timeout 3s "$XREADER_BIN" "$PDF_INPUT" >"$smoke_out" 2>"$smoke_err"
smoke_status=$?
set -e
echo "smoke_status=$smoke_status" >> "$report"

echo >> "$report"
echo "[timing_from_logs]" >> "$report"
python3 - "$run_out" >> "$report" <<'PY'
import re
import sys

text = open(sys.argv[1], encoding="utf-8", errors="ignore").read().splitlines()
ts_re = re.compile(r'(\d{2}):(\d{2}):(\d{2})\.(\d{3})')
base = None
default_hit = None
webkit_hit = None

for ln in text:
    m = ts_re.search(ln)
    if not m:
        continue
    t = ((int(m.group(1)) * 60 + int(m.group(2))) * 60 + int(m.group(3))) * 1000 + int(m.group(4))
    if base is None:
        base = t
    if "/org/x/reader/default/" in ln and default_hit is None:
        default_hit = t
    if "WebKitWebProcess" in ln and webkit_hit is None:
        webkit_hit = t

def fmt(v):
    return "NA" if v is None else str(v)

print(f"first_default_delta_ms={fmt(None if base is None or default_hit is None else default_hit - base)}")
print(f"first_webkit_delta_ms={fmt(None if base is None or webkit_hit is None else webkit_hit - base)}")
PY

echo >> "$report"
echo "[process_snapshot]" >> "$report"
"$XREADER_BIN" "$PDF_INPUT" >"$OUT_DIR/idle-$timestamp.out" 2>"$OUT_DIR/idle-$timestamp.err" &
pid=$!
sleep 2
children="$(pgrep -P "$pid" || true)"
{
  echo "pid=$pid"
  ps -o pid,ppid,pcpu,rss,pmem,comm -p "$pid" ${children:+-p $children}
  rss_total="$(ps -o rss= -p "$pid" ${children:+-p $children} | awk '{s+=$1} END{print s+0}')"
  echo "rss_total_kb=$rss_total"
} >> "$report"
kill -TERM "$pid" 2>/dev/null || true
wait "$pid" 2>/dev/null || true

echo >> "$report"
echo "[error_counters]" >> "$report"
warn_count="$(grep -E -c 'WARNING|warning' "$run_err" || true)"
crit_count="$(grep -E -c 'CRITICAL|critical' "$run_err" || true)"
err_count="$(grep -E -c 'ERROR|Error registering document' "$run_err" || true)"
echo "warnings=$warn_count" >> "$report"
echo "criticals=$crit_count" >> "$report"
echo "errors=$err_count" >> "$report"

echo >> "$report"
echo "[disk]" >> "$report"
du -sh "$ROOT_DIR" "$BUILD_DIR" 2>/dev/null >> "$report"
du -sh "$HOME/.cache/xreader" "$HOME/.local/share/xreader" "$HOME/.config/xreader" 2>/dev/null >> "$report" || true

echo >> "$report"
echo "[versions]" >> "$report"
{
  echo "meson=$(meson --version)"
  echo "ninja=$(ninja --version)"
  echo "gcc=$(gcc --version | head -n 1)"
  echo "gtk=$(pkg-config --modversion gtk+-3.0 2>/dev/null || echo NA)"
  echo "glib=$(pkg-config --modversion glib-2.0 2>/dev/null || echo NA)"
  echo "poppler_glib=$(pkg-config --modversion poppler-glib 2>/dev/null || echo NA)"
  echo "webkit2gtk=$(pkg-config --modversion webkit2gtk-4.1 2>/dev/null || echo NA)"
} >> "$report"

echo "report=$report"
echo "run_out=$run_out"
echo "run_err=$run_err"
echo "smoke_out=$smoke_out"
echo "smoke_err=$smoke_err"
