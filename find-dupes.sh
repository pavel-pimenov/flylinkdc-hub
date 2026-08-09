#!/bin/bash
# find-dupes.sh — поиск дубликатов кода (copy-paste) в core/ двумя движками.
#
# Режимы:
#   ./find-dupes.sh            # cppcheck style (duplicate/identical/redundant/knownCondition)
#   ./find-dupes.sh --cpd      # jscpd token-based clone detection (блочные дубли функций)
#   ./find-dupes.sh --xml      # cppcheck: сохранить XML-отчёт
#
# Ограничения:
#   - cppcheck 2.19 HE ловит блочные клоны целиком, только мелкие duplicate*/redundant.
#     Для copy-paste целых функций используй --cpd (jscpd).
#   - jscpd ловит и преамбулы (#include-блоки) всех .cpp как "дубликаты".
#     Скрипт автоматически отфильтровывает клоны, начинающиеся в первых 25 строках
#     обоих файлов (это заголовки, не функциональный код).
#
# Внешние либы (sqlite, prometheus-cpp, pugixml, lua, civetweb) исключены согласно AGENTS.md.
#
# Зависимости: cppcheck (install-deps.sh), node/npm (для jscpd).

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

CORE_DIR="core"
MODE="cppcheck"
XML=0
JSDCP_MIN_TOKENS=150
JSDCP_THRESHOLD=0.5

while [[ $# -gt 0 ]]; do
	case "$1" in
		--cpd) MODE="cpd" ;;
		--xml) XML=1 ;;
		--min-tokens=*) JSDCP_MIN_TOKENS="${1#*=}" ;;
		--threshold=*) JSDCP_THRESHOLD="${1#*=}" ;;
		*) echo "Unknown arg: $1" >&2; exit 2 ;;
	esac
	shift
done

# Включаемые пути (синхронно с CMakeLists.txt)
INCLUDES=(
	"-Icore"
	"-Ifly-server-test-port"
	"-Iskein/Optimized_32bit"
	"-I/usr/include/lua5.4"
	"-Iout/build/default/generated"
)

DEFINES=(
	"-DFLYLINKDC_USE_DB"
	"-DFLYLINKDC_USE_CPU_STAT"
	"-DFLYLINKDC_USE_STAT_RELOCATION"
	"-DFLYLINKDC_USE_UDP_THREAD"
	"-DFLYLINKDC_USE_VERSION"
	"-DUSE_FLYLINKDC_EXT_JSON"
	"-DSPDLOG_FMT_EXTERNAL"
	"-D_WITH_SQLITE"
	"-D_DEFAULT_SOURCE"
	"-D_GNU_SOURCE"
	"-D_POSIX_C_SOURCE=200809L"
	"-DPtokaX_VERSION=\"0.5.354\""
)

# Исключаем внешние либы и сгенерированное
SUPPRESS_DIRS=(
	"-iSQLiteCpp"
	"-iprometheus-cpp"
	"-ipugixml"
	"-ilua"
	"-icivetweb"
	"-iskein"
	"-ifly-server-test-port"
)

SUPPRESSIONS=(
	"--suppress=unusedFunction"
	"--suppress=missingIncludeSystem"
	"--suppress=unknownMacro"
	"--suppress=unmatchedSuppression"
)

mkdir -p out/analyze out/analyze/.cppcheck-build out/analyze/jscpd

if [ "$MODE" = "cpd" ]; then
	echo "[find-dupes] jscpd token-based clone detection (min-tokens=$JSDCP_MIN_TOKENS, threshold=$JSDCP_THRESHOLD)"
	echo "[find-dupes] filtering preamble clones (both starts <= 25 lines)..."
	timeout 150 npx --yes jscpd@latest "$CORE_DIR" \
		--format cpp \
		--min-tokens "$JSDCP_MIN_TOKENS" \
		--threshold "$JSDCP_THRESHOLD" \
		--ignore "**/out/**" \
		--reporters json \
		--output out/analyze/jscpd 2>/dev/null

	python3 - "$JSDCP_MIN_TOKENS" <<'PY'
import json, sys
mt = int(sys.argv[1])
try:
	d = json.load(open('out/analyze/jscpd/jscpd-report.json'))
except FileNotFoundError:
	print("no report generated"); sys.exit(0)
clones = d.get('duplicates', [])
real = []
for c in clones:
	f, g = c['firstFile'], c['secondFile']
	# отбрасываем преамбулы: оба клона начинаются в первых 25 строках
	if f['start'] <= 25 and g['start'] <= 25:
		continue
	if f['name'] == g['name'] and f['start'] == g['start']:
		continue
	real.append(c)
real.sort(key=lambda c: -c['lines'])
print(f"real (non-preamble) clones: {len(real)} / total {len(clones)}")
print(f"{'LINES':>5} {'TOKENS':>6}  FILE_A:LINE  <->  FILE_B:LINE")
print("-" * 70)
for c in real[:60]:
	f, g = c['firstFile'], c['secondFile']
	tag = "SAME-FILE" if f['name'] == g['name'] else "CROSS"
	print(f"{c['lines']:>5} {c['tokens']:>6}  {f['name']}:{f['start']}  <->  {g['name']}:{g['start']}  [{tag}]")
PY
	exit 0
fi

# --- cppcheck mode ---
if [ "$XML" -eq 1 ]; then
	OUT_FILE="out/analyze/cppcheck-dupes.xml"
	echo "[find-dupes] writing XML report to $OUT_FILE"
	cppcheck \
		--enable=style,performance,portability \
		--std=c++23 \
		--language=c++ \
		--cppcheck-build-dir=out/analyze/.cppcheck-build \
		--inline-suppr \
		"${SUPPRESSIONS[@]}" \
		"${SUPPRESS_DIRS[@]}" \
		"${INCLUDES[@]}" \
		"${DEFINES[@]}" \
		--xml --xml-version=2 \
		"$CORE_DIR" 2> "$OUT_FILE"
	echo "[find-dupes] done. Report: $OUT_FILE"
	echo "[find-dupes] duplicate/redundant-related findings:"
	grep -oE "duplicate[A-Za-z]*|redundantCopy|redundantAssignment|identicalInnerCondition|knownConditionTrueFalse" "$OUT_FILE" | sort | uniq -c
else
	cppcheck \
		--enable=style,performance,portability \
		--std=c++23 \
		--language=c++ \
		--cppcheck-build-dir=out/analyze/.cppcheck-build \
		--inline-suppr \
		"${SUPPRESSIONS[@]}" \
		"${SUPPRESS_DIRS[@]}" \
		"${INCLUDES[@]}" \
		"${DEFINES[@]}" \
		"$CORE_DIR"
fi
