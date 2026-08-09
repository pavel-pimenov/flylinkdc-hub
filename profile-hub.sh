#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/out/build/profile"
HOST=127.0.0.1
PORT=4111
CLIENTS=${1:-100}
RAMP=${2:-0.05}

echo "=== Build profile version ==="
rm -rf "$BUILD_DIR"
cmake -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Profile "$SCRIPT_DIR"
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

echo "=== Run load test ($CLIENTS clients) ==="
python3 "$SCRIPT_DIR/dc-loadtest.py" "$HOST" "$PORT" "$CLIENTS" "$RAMP"

echo "=== Stop hub ==="
kill $HUB_PID 2>/dev/null
wait $HUB_PID 2>/dev/null

echo "=== Generate gprof report ==="
cd "$BUILD_DIR"
if [ -f gmon.out ]; then
    gprof PtokaX gmon.out > profile.txt
    echo "Profile saved to: $BUILD_DIR/profile.txt"
    echo ""
    echo "=== Top 20 hotspots ==="
    head -30 profile.txt
else
    echo "No gmon.out found"
fi

rm -rf "$CFG_DIR"
echo "=== Done ==="
