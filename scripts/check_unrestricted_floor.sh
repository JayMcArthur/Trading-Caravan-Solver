#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
town=${1:-Sanctifan}
expected=${2:-51130}
beam=${BEAM:-5000}
max_mb=${MAX_MB:-768}
internal_mb=${INTERNAL_MB:-640}
max_seconds=${MAX_SECONDS:-60}
profile=${PROFILE:-greedy}

if [[ ! "$expected" =~ ^[0-9]+$ ]]; then
    echo "[!] Expected floor must be a non-negative integer" >&2
    exit 2
fi

raw_output=$(
    MAX_MB="$max_mb" \
    INTERNAL_MB="$internal_mb" \
    ARRAY_LIMIT="$beam" \
    MAX_SECONDS="$max_seconds" \
    "$repo/scripts/run_safe.sh" \
        --town "$town" \
        --no-incumbent \
        --purchase-profile "$profile" \
        --optimistic-share 25 \
        --benchmark-json
)
json=$(awk '/^\{/{line=$0} END{print line}' <<<"$raw_output")
score=$(awk 'match($0, /"best_score":[0-9]+/) {print substr($0, RSTART + 13, RLENGTH - 13)}' <<<"$json")

if [[ -z "$score" ]]; then
    echo "[FAIL] $town unrestricted search produced no terminal score" >&2
    printf '%s\n' "$json"
    exit 1
fi

printf '%s\n' "$json"
if ((score < expected)); then
    echo "[FAIL] $town unrestricted score $score is below verified floor $expected" >&2
    exit 1
fi

echo "[PASS] $town unrestricted score $score reaches verified floor $expected" >&2
