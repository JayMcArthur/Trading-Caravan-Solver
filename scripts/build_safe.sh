#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_dir=${BUILD_DIR:-"$repo/build"}
build_type=${BUILD_TYPE:-Release}
max_seconds=${MAX_SECONDS:-600}

if [[ ! "$max_seconds" =~ ^[1-9][0-9]*$ ]]; then
    echo "[!] MAX_SECONDS must be a positive integer" >&2
    exit 2
fi

cmake -S "$repo" -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_type"
LIMIT_MAX_MB=${BUILD_MAX_MB:-1024} \
LIMIT_MAX_SECONDS="$max_seconds" \
    "$repo/scripts/run_limited.sh" cmake --build "$build_dir" --parallel 1
