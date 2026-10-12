#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
binary="${BINARY:-$repo/build/Merchant_Trading_Caravan}"
max_mb="${MAX_MB:-1024}"
internal_mb="${INTERNAL_MB:-$((max_mb * 3 / 4))}"
array_limit="${ARRAY_LIMIT:-5000}"
max_seconds="${MAX_SECONDS:-180}"

for value_name in max_mb internal_mb array_limit max_seconds; do
    value=${!value_name}
    if [[ ! "$value" =~ ^[0-9]+$ ]]; then
        echo "[!] $value_name must be a non-negative integer" >&2
        exit 2
    fi
done
if ((max_mb < 256 || max_mb > 1024 || internal_mb < 128 || internal_mb >= max_mb || array_limit == 0 || max_seconds == 0)); then
    echo "[!] Require 256<=MAX_MB<=1024, INTERNAL_MB>=128 and <MAX_MB, ARRAY_LIMIT>0, and MAX_SECONDS>0" >&2
    exit 2
fi
if [[ ! -x "$binary" ]]; then
    echo "[!] Build the solver first with: '$repo/scripts/build_safe.sh'" >&2
    exit 2
fi

internal_seconds=0
if ((max_seconds > 2)); then
    internal_seconds=$((max_seconds - 2))
fi
command=("$binary" --no-warm-trees --array-limit "$array_limit" --memory-limit-mb "$internal_mb" --time-limit-seconds "$internal_seconds" "$@")

echo "[*] Safe run: hard_cap=${max_mb}MB internal_budget=${internal_mb}MB beam=${array_limit} internal_timeout=${internal_seconds}s"
cd "$repo"
LIMIT_MAX_MB="$max_mb" LIMIT_MAX_SECONDS="$max_seconds" \
    exec "$repo/scripts/run_limited.sh" "${command[@]}"
