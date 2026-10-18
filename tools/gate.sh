#!/usr/bin/env bash
# Floor gate: the candidate must beat bots/starter-cpp AND bots/starter-python on every map,
# from both seats, with zero faults (either side) and no errored matches.
#
#   tools/gate.sh BOT [--jobs N] [--maps all|a,b] [--out DIR] [--strict-fuel]
#
# --strict-fuel additionally requires zero fuel suspensions for the candidate.
# Exit 0 only if the gate passes. Prints both aggregates and leaves the full arena output
# under replays/gate-<bot>-<time>/{starter-cpp,starter-python}/.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export PATH="$ROOT/.venv/bin:$PATH"
BOT="${1:-}"; shift || true
[ -n "$BOT" ] || { echo "usage: $0 BOT [--jobs N] [--maps all|a,b] [--out DIR] [--strict-fuel]" >&2; exit 4; }
JOBS=8; MAPS=all; OUT=""; STRICT=0
while [ $# -gt 0 ]; do
  case "$1" in
    --jobs) JOBS="$2"; shift 2;;
    --maps) MAPS="$2"; shift 2;;
    --out) OUT="$2"; shift 2;;
    --strict-fuel) STRICT=1; shift;;
    *) echo "gate: unknown option $1" >&2; exit 4;;
  esac
done
NAME="$(basename "$BOT")"
OUT="${OUT:-$ROOT/replays/gate-$NAME-$(date +%Y%m%d-%H%M%S)}"
mkdir -p "$OUT"

cambc build "$BOT" --release >"$OUT/build.log" 2>&1 || { tail -20 "$OUT/build.log"; echo "gate: build failed for $BOT" >&2; exit 4; }

FAIL=0
BOT_ABS="$(cd "$BOT" && pwd)"
for OPP in starter-cpp starter-python; do
  echo "=== $NAME vs $OPP ==="
  if [ "$BOT_ABS" = "$ROOT/bots/$OPP" ]; then echo "skipped: candidate is $OPP itself"; echo; continue; fi
  # Candidate already built; --no-build would skip the opponent too, so let arena build both
  # (a no-op rebuild of the candidate costs well under a second).
  python3 "$ROOT/tools/arena.py" "$BOT" "$ROOT/bots/$OPP" --maps "$MAPS" --sides both --jobs "$JOBS" --out "$OUT/$OPP" \
    > "$OUT/$OPP.log" 2>&1
  RC=$?
  # Print just the aggregate block (everything after the per-match table).
  awk '/^wrote /{exit} found{print} /^$/{blank++} blank>=2{found=1}' "$OUT/$OPP.log"
  VERDICT=$(python3 - "$OUT/$OPP/summary.json" "$STRICT" <<'EOF'
import json, sys
try:
    s = json.load(open(sys.argv[1]))["summary"]
except Exception as e:
    print(f"FAIL (no summary: {e})"); sys.exit(0)
problems = []
if s["wins"] != s["games"]: problems.append(f"won {s['wins']}/{s['games']}")
if s["errors"]: problems.append(f"{s['errors']} errored")
if s["faults_a"] or s["faults_b"]: problems.append(f"faults {s['faults_a']}/{s['faults_b']}")
if sys.argv[2] == "1" and s["suspensions_a"]: problems.append(f"{s['suspensions_a']} suspensions")
print("PASS" if not problems else "FAIL (" + ", ".join(problems) + ")")
EOF
)
  echo "verdict vs $OPP: $VERDICT (arena exit $RC, details in $OUT/$OPP.log)"
  [ "$VERDICT" = "PASS" ] || FAIL=1
  echo
done
if [ "$FAIL" = 0 ]; then echo "GATE PASSED: $NAME beats both starters on every map from both seats with zero faults"; else echo "GATE FAILED for $NAME" >&2; fi
exit $FAIL
