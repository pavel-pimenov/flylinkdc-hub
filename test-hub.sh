#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TIMEOUT=15
TEST_RESULT=0
HUB_PID=""
SAN_LOG=""
DOCKER_WAS_RUNNING=0
SKIP_DOCKER=0
DOCKER_MODE=0
DOCKER_TEST_SERVICE="ptokax-test"
DOCKER_TEST_CONTAINER="ptokax-hub-test"
DOCKER_TEST_DC_PORT=41110
DOCKER_TEST_JSON_PORT=8889

usage() {
    echo "Usage: $0 [OPTIONS] [BUILD_TYPE]"
    echo ""
    echo "Options:"
    echo "  --docker        Use Docker test service instead of local hub"
    echo "  --skip-docker   Don't stop/start Docker containers (test on non-default ports)"
    echo "  --help          Show this help"
    echo ""
    echo "BUILD_TYPE: Debug (default), Release, RelWithDebInfo"
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --docker) DOCKER_MODE=1; shift ;;
        --skip-docker) SKIP_DOCKER=1; shift ;;
        --help|-h) usage ;;
        *) break ;;
    esac
done

BUILD_TYPE="${1:-Debug}"
BUILD_DIR="${SCRIPT_DIR}/out/build/${BUILD_TYPE}"

cleanup() {
    if [ "$DOCKER_MODE" -eq 1 ]; then
        echo "Stopping Docker test service..."
        cd "$SCRIPT_DIR" && docker compose -f docker-compose.test.yml down 2>/dev/null || true
        echo "Cleaning up test artifacts..."
        rm -f "$SCRIPT_DIR/cfg-test/"*.pxb "$SCRIPT_DIR/cfg-test/"*.sqlite* "$SCRIPT_DIR/cfg-test/"*.db 2>/dev/null || true
        rm -rf "$SCRIPT_DIR/logs-test/"* 2>/dev/null || true
    fi
    if [ -n "$HUB_PID" ] && kill -0 "$HUB_PID" 2>/dev/null; then
        kill "$HUB_PID" 2>/dev/null || true
        wait "$HUB_PID" 2>/dev/null || true
    fi
    if [ -n "$SAN_LOG" ] && [ -f "$SAN_LOG" ]; then
        echo "Sanitizer log: $SAN_LOG"
        if grep -q "ERROR:" "$SAN_LOG" 2>/dev/null; then
            echo "=== SANITIZER ERRORS FOUND ==="
            cat "$SAN_LOG"
        fi
    fi
    # Restart Docker if it was running and verify hub starts
    if [ "$DOCKER_WAS_RUNNING" -eq 1 ] && [ "$SKIP_DOCKER" -eq 0 ] && [ "$DOCKER_MODE" -eq 0 ]; then
        echo "Restarting Docker containers..."
        cd "$SCRIPT_DIR" && docker compose up -d ptokax 2>/dev/null
        echo "Waiting for hub to start in Docker..."
        DOCKER_OK=0
        for i in $(seq 1 15); do
            if docker compose logs ptokax --tail 1 2>/dev/null | grep -qE '\[PROTO\]|\[QUEUE\]|\[CMDS\]|\[EXTJSON\]' 2>/dev/null; then
                DOCKER_OK=1
                break
            fi
            sleep 1
        done
        if [ "$DOCKER_OK" -eq 1 ]; then
            echo "OK: Docker hub is running and accepting connections"
        else
            echo "FAIL: Docker hub did not start properly after test cleanup"
            docker compose logs ptokax --tail 10 2>/dev/null || true
        fi
    fi
}
trap cleanup EXIT

# Stop Docker if running to free ports for local test
if [ "$DOCKER_MODE" -eq 0 ]; then
    if [ "$SKIP_DOCKER" -eq 0 ]; then
        if docker compose -f "$SCRIPT_DIR/docker-compose.yml" ps ptokax 2>/dev/null | grep -q "Up"; then
            DOCKER_WAS_RUNNING=1
            echo "Stopping Docker containers to free ports..."
            cd "$SCRIPT_DIR" && docker compose stop ptokax 2>/dev/null
            sleep 1
        fi
    else
        echo "Skipping Docker stop (--skip-docker)"
    fi
else
    echo "Using Docker test service (--docker)"
fi

echo "=== [0.5/5] CP1251 encoding check ==="
CORRUPTED_FILES=""
while IFS= read -r -d '' f; do
    if python3 -c "
import sys
data = open(sys.argv[1], 'rb').read()
sys.exit(0 if b'\xef\xbf\xbd' in data else 1)
" "$f" 2>/dev/null; then
        CORRUPTED_FILES="${CORRUPTED_FILES}  $f\n"
    fi
done < <(find "$SCRIPT_DIR/scripts" \( -name '*.lua' -o -name '*.lu' \) -print0)
while IFS= read -r -d '' f; do
    if python3 -c "
import sys
data = open(sys.argv[1], 'rb').read()
sys.exit(0 if b'\xef\xbf\xbd' in data else 1)
" "$f" 2>/dev/null; then
        CORRUPTED_FILES="${CORRUPTED_FILES}  $f\n"
    fi
done < <(find "$SCRIPT_DIR" -maxdepth 1 \( -name 'Settings.pxt' -o -name 'Motd.txt' \) -print0 2>/dev/null)
if [ -n "$CORRUPTED_FILES" ]; then
    echo "FAIL: CP1251 encoding corrupted (contains UTF-8 replacement characters U+FFFD):"
    echo -e "$CORRUPTED_FILES"
    echo "These files must be restored from git: git show ce8c92e:<file> > <file>"
    exit 1
else
    LUA_COUNT=$(find "$SCRIPT_DIR/scripts" -name '*.lua' -o -name '*.lu' | wc -l)
    echo "OK: All $LUA_COUNT Lua scripts have valid CP1251 encoding"
fi

echo "=== [1/5] Lua syntax check ==="
LUA_ERRS=$(find "$SCRIPT_DIR/scripts" \( -name '*.lua' -o -name '*.lu' \) -print0 2>/dev/null | xargs -0 -r luac5.4 -p 2>&1)
if [ -z "$LUA_ERRS" ]; then
    LUA_COUNT=$(find "$SCRIPT_DIR/scripts" -name '*.lua' -o -name '*.lu' | wc -l)
    echo "OK: All $LUA_COUNT Lua scripts pass syntax check"
else
    echo "FAIL: Lua syntax errors found:"
    echo "$LUA_ERRS"
    exit 1
fi

echo "=== [1.5/5] luacheck static analysis ==="
LUACHECK_OUT=$(luacheck "$SCRIPT_DIR/scripts" --no-color -q --config "$SCRIPT_DIR/.luacheckrc" 2>&1 || true)
    LUACHECK_COUNT=$(echo "$LUACHECK_OUT" | grep -c "^    " || true)
LUACHECK_ERRORS=$(echo "$LUACHECK_OUT" | grep -c "^Total:.* [1-9][0-9]* errors" || true)
if [ "$LUACHECK_ERRORS" -eq 0 ]; then
    echo "OK: $LUACHECK_COUNT warnings, 0 errors"
else
    echo "WARN: luacheck found issues:"
    echo "$LUACHECK_OUT" | grep -E "^(Total:|    )" | head -20
    if [ "$LUACHECK_ERRORS" -gt 0 ]; then
        echo "FAIL: luacheck found $LUACHECK_ERRORS errors"
        exit 1
    fi
fi

if [ "$DOCKER_MODE" -eq 1 ]; then
    # In --docker mode, skip local build (Docker image provides the hub)
    echo "SKIP: Build (using Docker image for hub testing)"
    echo "SKIP: cppcheck (run legacy mode for full static analysis)"
    echo "SKIP: Unit tests (run legacy mode for unit tests)"
else
    echo "=== [2/5] Building PtokaX (${BUILD_TYPE}) ==="
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cmake -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ${CMAKE_PREFIX_PATH:+-DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH"} "$SCRIPT_DIR" >/dev/null
    cmake --build "$BUILD_DIR" -j"$(nproc)" 2>&1 | tail -3
    cmake --build "$BUILD_DIR" --target test_utility -j"$(nproc)" 2>&1 | tail -3

    HUB_BIN="${BUILD_DIR}/PtokaX"
    if [ ! -f "$HUB_BIN" ]; then
        echo "FAIL: Binary not found at $HUB_BIN"
        exit 1
    fi
    echo "OK: Binary built"

    echo "=== [2.5/5] cppcheck static analysis ==="
    if command -v cppcheck &>/dev/null; then
        CPPCHECK_OUT=$(cppcheck --project="$BUILD_DIR/compile_commands.json" \
            --suppressions-list=core/.cppcheck-suppressions \
            --suppress=useInitializationList \
            --suppress=unusedStructMember \
            --suppress=preprocessorErrorDirective \
            --max-ctu-depth=0 \
            --check-level=normal \
            --inline-suppr \
            --suppress="*:skein/*" \
            --suppress="*:sqlite/*" \
            --suppress="*:_deps/googletest-src/*" \
            --suppress="*:_deps/googlemock-src/*" \
            2>&1 | grep -v "^Checking " | grep -v "^$" | grep -v "[0-9]*/[0-9]* files checked") || true
        if [ -z "$CPPCHECK_OUT" ]; then
            echo "OK: cppcheck found no issues"
        else
            echo "$CPPCHECK_OUT"
            echo "FAIL: cppcheck found issues"
            TEST_RESULT=1
        fi
    else
        echo "SKIP: cppcheck not installed"
    fi

    echo "=== [2.75/5] Unit tests (GoogleTest) ==="
    if [ -f "$BUILD_DIR/test_utility" ]; then
        TEST_OUT=$("$BUILD_DIR/test_utility" 2>&1) || true
        if echo "$TEST_OUT" | grep -q "PASSED"; then
            TEST_COUNT=$(echo "$TEST_OUT" | grep -oP '\d+ tests? from' | head -1 | grep -oP '\d+')
            echo "OK: All ${TEST_COUNT} unit tests passed"
        else
            echo "$TEST_OUT"
            echo "FAIL: Unit tests failed"
            TEST_RESULT=1
        fi
    else
        echo "SKIP: test_utility binary not found (run cmake first)"
    fi
fi

if [ "$DOCKER_MODE" -eq 1 ]; then
    # --- Docker mode: use test service ---
    echo "=== [3/5] Building and starting Docker test service ==="
    cd "$SCRIPT_DIR"
    bash gen-build-number.sh
    docker compose -f docker-compose.test.yml build "$DOCKER_TEST_SERVICE" 2>&1 | tail -3
    docker compose -f docker-compose.test.yml up -d "$DOCKER_TEST_SERVICE" 2>/dev/null

    echo "Waiting for test hub to become healthy..."
    STARTED=0
    for i in $(seq 1 30); do
        if docker compose -f docker-compose.test.yml ps "$DOCKER_TEST_SERVICE" 2>/dev/null | grep -q "(healthy)"; then
            STARTED=1
            break
        fi
        sleep 1
    done

    if [ "$STARTED" -eq 0 ]; then
        echo "FAIL: Test hub did not become healthy within 30s"
        docker compose -f docker-compose.test.yml logs "$DOCKER_TEST_SERVICE" --tail 20 2>/dev/null || true
        TEST_RESULT=1
    else
        echo "OK: Test hub is healthy"
    fi

    if [ "$TEST_RESULT" -eq 0 ]; then
        echo "=== [4/5] TCP connection test ==="
        if timeout 5 bash -c "exec 3<>/dev/tcp/127.0.0.1/${DOCKER_TEST_DC_PORT}; exec 3>&-" 2>/dev/null; then
            echo "OK: TCP connection to port $DOCKER_TEST_DC_PORT succeeded"
        else
            echo "FAIL: Cannot connect to hub on port $DOCKER_TEST_DC_PORT"
            docker compose -f docker-compose.test.yml logs "$DOCKER_TEST_SERVICE" --tail 10 2>/dev/null || true
            TEST_RESULT=1
        fi

        echo "=== [4.5/5] DC client login + concurrent \$Lock test ==="
        if HUB_PORT="$DOCKER_TEST_DC_PORT" HUB_HOST=127.0.0.1 python3 "${SCRIPT_DIR}/test-dc-client.py"; then
            echo "OK: DC client test passed"
        else
            echo "FAIL: DC client test failed"
            TEST_RESULT=1
        fi

        echo "=== [4.6/5] Fuzz testing ==="
        if HUB_PORT="$DOCKER_TEST_DC_PORT" HUB_HOST=127.0.0.1 python3 "${SCRIPT_DIR}/test-fuzz.py"; then
            echo "OK: Fuzz test passed"
        else
            echo "FAIL: Fuzz test failed"
            TEST_RESULT=1
        fi

        echo "=== [5/5] Container health check ==="
        if docker ps --format '{{.Names}}' 2>/dev/null | grep -q "^${DOCKER_TEST_CONTAINER}$"; then
            echo "OK: Test container is running"
        else
            echo "FAIL: Test container is not running"
            TEST_RESULT=1
        fi

        echo "=== Sanitizer report ==="
        DOCKER_ASAN=$(docker logs "$DOCKER_TEST_CONTAINER" 2>&1 | grep -i "ERROR:" || true)
        if [ -n "$DOCKER_ASAN" ]; then
            echo "FAIL: Sanitizer found errors in Docker logs:"
            echo "$DOCKER_ASAN"
            TEST_RESULT=1
        else
            echo "OK: No sanitizer errors in Docker logs"
        fi
    fi
else
    # --- Local mode: start hub locally ---
    echo "=== [3/5] Starting hub ==="
    CFG_DIR=$(mktemp -d)
    # Seed test-friendly config so fuzzing doesn't hit IP bans
    if [ -d "$SCRIPT_DIR/cfg-test" ]; then
        mkdir -p "$CFG_DIR/cfg"
        cp "$SCRIPT_DIR/cfg-test/"* "$CFG_DIR/cfg/" 2>/dev/null || true
    fi

    if [ "$BUILD_TYPE" = "Debug" ]; then
        SAN_LOG="${SCRIPT_DIR}/asan.log"
        export ASAN_OPTIONS="log_path=${SAN_LOG}:detect_leaks=1:halt_on_error=0:log_exe_name=1"
        export UBSAN_OPTIONS="log_path=${SAN_LOG}.ubsan:print_stacktrace=1:halt_on_error=0"
        echo "  Sanitizer logs -> $SAN_LOG"
    fi

    "$HUB_BIN" -c "$CFG_DIR" >/dev/null 2>&1 &
    HUB_PID=$!

    STARTED=0
    for i in $(seq 1 $TIMEOUT); do
        if ss -tln | grep -qE ":(411|4111|37015) " 2>/dev/null; then
            STARTED=1
            break
        fi
        if ! kill -0 "$HUB_PID" 2>/dev/null; then
            echo "FAIL: Hub process exited prematurely"
            TEST_RESULT=1
            break
        fi
        sleep 1
    done

    if [ "$STARTED" -eq 0 ]; then
        echo "FAIL: Hub did not start within ${TIMEOUT}s"
        TEST_RESULT=1
    fi

    if [ "$TEST_RESULT" -eq 0 ]; then
        LISTENING_PORTS=$(ss -tln | grep -oP ':\K(411|4111|37015)' | sort -u | tr '\n' ',' | sed 's/,$//')
        echo "OK: Hub started. Listening ports: $LISTENING_PORTS"

        echo "=== [4/5] TCP connection test ==="
        TEST_PORT=$(ss -tln | grep -oP ':\K(411|4111|37015)' | head -1)
        if [ -n "$TEST_PORT" ] && timeout 5 bash -c "exec 3<>/dev/tcp/127.0.0.1/$TEST_PORT; exec 3>&-" 2>/dev/null; then
            echo "OK: TCP connection to port $TEST_PORT succeeded"
        else
            echo "FAIL: Cannot connect to hub"
            TEST_RESULT=1
        fi

        echo "=== [4.5/5] DC client login + concurrent \$Lock test ==="
        DC_PORT=$(ss -tlnp 2>/dev/null | grep "pid=${HUB_PID}," | grep -oP ':\K(4411|411|4111)' | head -1 || true)
        if [ -z "$DC_PORT" ]; then
            echo "SKIP: test hub did not bind a DC protocol port (4411/411/4111)"
        elif HUB_PORT="$DC_PORT" HUB_HOST=127.0.0.1 python3 "${SCRIPT_DIR}/test-dc-client.py"; then
            echo "OK: DC client test passed"
        else
            echo "FAIL: DC client test failed"
            TEST_RESULT=1
        fi

        echo "=== [4.6/5] Fuzz testing ==="
        if [ -z "$DC_PORT" ]; then
            echo "SKIP: test hub did not bind a DC protocol port"
        elif HUB_PORT="$DC_PORT" HUB_HOST=127.0.0.1 python3 "${SCRIPT_DIR}/test-fuzz.py"; then
            echo "OK: Fuzz test passed"
        else
            echo "FAIL: Fuzz test failed"
            TEST_RESULT=1
        fi

        echo "=== [5/5] Process health check ==="
        if kill -0 "$HUB_PID" 2>/dev/null; then
            echo "OK: Hub process is alive (PID=$HUB_PID)"
        else
            echo "FAIL: Hub process died"
            TEST_RESULT=1
        fi

        if [ "$BUILD_TYPE" = "Debug" ] && [ -f "${SAN_LOG}" ]; then
            echo "=== Sanitizer report ==="
            if grep -q "ERROR:" "${SAN_LOG}" 2>/dev/null; then
                echo "FAIL: Sanitizer found errors:"
                cat "${SAN_LOG}"
                TEST_RESULT=1
            else
                echo "OK: No sanitizer errors"
            fi
        fi
    fi

    rm -rf "$CFG_DIR"
fi

echo ""
if [ "$TEST_RESULT" -eq 0 ]; then
    echo "=== ALL TESTS PASSED ==="
else
    echo "=== TESTS FAILED ==="
fi
exit $TEST_RESULT