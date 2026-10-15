#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
binary=${BINARY:-"$repo/build/Merchant_Trading_Caravan_Tests"}

if (($# == 1)) && [[ "$1" == "safety-limits" ]]; then
    cd "$repo"
    LIMIT_MAX_MB=${MAX_MB:-768} \
    LIMIT_MAX_SECONDS=${MAX_SECONDS:-180} \
        exec "$repo/scripts/run_limited.sh" python3 "$repo/tests/NativeJobSafetyTests.py"
fi

if (($# == 1)) && [[ "$1" == "cache-conversion" ]]; then
    cd "$repo"
    LIMIT_MAX_MB=${MAX_MB:-768} \
    LIMIT_MAX_SECONDS=${MAX_SECONDS:-180} \
        exec "$repo/scripts/run_limited.sh" python3 -m unittest discover -s tests -p CacheConversionTests.py -v
fi

if [[ ! -x "$binary" ]]; then
    echo "[!] Build tests first with: $repo/scripts/build_safe.sh" >&2
    exit 2
fi

cd "$repo"
if (($# == 1)) && [[ "$1" == "prefix-timing" ]]; then
    LIMIT_MAX_MB=${MAX_MB:-768} \
    LIMIT_MAX_SECONDS=${MAX_SECONDS:-180} \
        exec "$repo/scripts/run_limited.sh" python3 "$repo/tests/PrefixTimingTests.py" "$binary"
fi
LIMIT_MAX_MB=${MAX_MB:-768} \
LIMIT_MAX_SECONDS=${MAX_SECONDS:-180} \
    exec "$repo/scripts/run_limited.sh" "$binary" "$@"
