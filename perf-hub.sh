#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/out/build/release"
HOST=127.0.0.1
PORT=4111
CLIENTS=${1:-200}
DURATION=${2:-10}

echo "=== Build release version ==="
rm -rf "$BUILD_DIR"
cmake -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo "$SCRIPT_DIR"
cmake --build "$BUILD_DIR" -j"$(nproc)"

HUB_BIN="${BUILD_DIR}/PtokaX"
CFG_DIR=$(mktemp -d)
cp "$SCRIPT_DIR"/cfg/* "$CFG_DIR/" 2>/dev/null || true

echo "=== Start hub ==="
"$HUB_BIN" -c "$CFG_DIR" &
HUB_PID=$!
sleep 2

if ! ss -tln | grep -q ":${PORT} "; then
    echo "FAIL: Hub not listening on $PORT"
    kill $HUB_PID 2>/dev/null
    exit 1
fi

echo "=== Perf record ($DURATION seconds) ==="
perf record -g -p "$HUB_PID" -o perf.data -- sleep "$DURATION" &
PERF_PID=$!

echo "=== Run load test ==="
python3 "$SCRIPT_DIR/dc-loadtest.py" "$HOST" "$PORT" "$CLIENTS" 0.02

wait $PERF_PID 2>/dev/null

echo "=== Stop hub ==="
kill $HUB_PID 2>/dev/null
wait $HUB_PID 2>/dev/null

echo "=== Perf report ==="
perf report -i perf.data --stdio --no-children 2>/dev/null | head -50

rm -rf "$CFG_DIR"
echo "=== Done ==="
