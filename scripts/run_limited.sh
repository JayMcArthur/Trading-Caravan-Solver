#!/usr/bin/env bash
set -euo pipefail

if (($# == 0)); then
    echo "[!] Usage: run_limited.sh <command> [args...]" >&2
    exit 2
fi

max_mb=${LIMIT_MAX_MB:-768}
max_seconds=${LIMIT_MAX_SECONDS:-180}

for value_name in max_mb max_seconds; do
    value=${!value_name}
    if [[ ! "$value" =~ ^[0-9]+$ ]]; then
        echo "[!] $value_name must be a non-negative integer" >&2
        exit 2
    fi
done
if ((max_mb < 256 || max_mb > 1024 || max_seconds == 0)); then
    echo "[!] Require 256<=LIMIT_MAX_MB<=1024 and LIMIT_MAX_SECONDS>0" >&2
    exit 2
fi
if ! command -v prlimit >/dev/null 2>&1; then
    echo "[!] prlimit is required; refusing an unbounded native run" >&2
    exit 2
fi
if ! command -v timeout >/dev/null 2>&1; then
    echo "[!] timeout is required; refusing an unbounded native run" >&2
    exit 2
fi

ulimit -c 0
as_bytes=$((max_mb * 1024 * 1024))
echo "[*] Limited process: hard_cap=${max_mb}MB timeout=${max_seconds}s" >&2
exec timeout --signal=TERM --kill-after=5s "${max_seconds}s" \
    prlimit --as="$as_bytes" --core=0 -- "$@"
