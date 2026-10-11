#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
towns=${TOWNS:-"0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21"}
beam=${BEAM:-500}
continuation_share=${CONTINUATION_SHARE:-5}
max_mb=${MAX_MB:-1024}
internal_mb=${INTERNAL_MB:-768}
max_seconds=${MAX_SECONDS:-120}
resume=${RESUME:-1}
result_file=${RESULT_FILE:-"$repo/benchmarks/results/all-town-rebaseline-b${beam}-$(date -u +%Y%m%dT%H%M%SZ).jsonl"}

positive_integer() {
    [[ "$1" =~ ^[1-9][0-9]*$ ]]
}

if ! positive_integer "$beam" || ! positive_integer "$max_seconds"; then
    echo "[!] BEAM and MAX_SECONDS must be positive integers" >&2
    exit 2
fi
if [[ ! "$continuation_share" =~ ^[0-9]+$ ]] || ((continuation_share > 100)); then
    echo "[!] CONTINUATION_SHARE must be an integer from 0 through 100" >&2
    exit 2
fi
if ! positive_integer "$max_mb" || ((max_mb > 1024)); then
    echo "[!] MAX_MB must be an integer from 1 through 1024" >&2
    exit 2
fi
if ! positive_integer "$internal_mb" || ((internal_mb >= max_mb)); then
    echo "[!] INTERNAL_MB must be positive and smaller than MAX_MB" >&2
    exit 2
fi
if [[ "$resume" != 0 && "$resume" != 1 ]]; then
    echo "[!] RESUME must be 0 or 1" >&2
    exit 2
fi
for town in $towns; do
    if [[ ! "$town" =~ ^[0-9]+$ ]] || ((town > 21)); then
        echo "[!] Town indices must be integers from 0 through 21" >&2
        exit 2
    fi
done

mkdir -p "$(dirname "$result_file")"

for town in $towns; do
    marker="\"experiment_town_index\":$town,\"experiment_continuation_lane_share\":$continuation_share"
    if [[ "$resume" == 1 && -f "$result_file" ]] && grep -Fq "$marker" "$result_file"; then
        echo "[*] Skipping recorded run: town=$town continuation_share=$continuation_share%" >&2
        continue
    fi

    echo "[*] Rebaseline: town=$town beam=$beam continuation_share=$continuation_share%" >&2
    raw_output=$(
        MAX_MB="$max_mb" \
        INTERNAL_MB="$internal_mb" \
        ARRAY_LIMIT="$beam" \
        MAX_SECONDS="$max_seconds" \
        "$repo/scripts/run_safe.sh" \
            --town "$town" \
            --no-incumbent \
            --no-auto-refine \
            --continuation-lane-share "$continuation_share" \
            --benchmark-json
    )
    json=$(awk '/^\{/{line=$0} END{print line}' <<<"$raw_output")
    if [[ -z "$json" ]]; then
        echo "[!] Run did not produce JSON: town=$town" >&2
        exit 1
    fi

    experiment_status=complete
    if [[ "$json" == *'"search_complete":false'* ]]; then
        experiment_status=stopped
        echo "[!] Run stopped safely; preserving its diagnostic JSON" >&2
    fi

    json=${json%\}}
    result=$(printf '%s,"experiment_town_index":%s,"experiment_continuation_lane_share":%s,"experiment_beam":%s,"experiment_start":"open","experiment_status":"%s"}' \
        "$json" "$town" "$continuation_share" "$beam" "$experiment_status")
    printf '%s\n' "$result" >> "$result_file"

    score=$(awk 'match($0, /"best_score":[0-9]+/) {print substr($0, RSTART + 13, RLENGTH - 13)}' <<<"$result")
    elapsed=$(awk 'match($0, /"elapsed_seconds":[0-9.]+/) {print substr($0, RSTART + 18, RLENGTH - 18)}' <<<"$result")
    rss=$(awk 'match($0, /"peak_rss_mb":[0-9]+/) {print substr($0, RSTART + 14, RLENGTH - 14)}' <<<"$result")
    echo "[+] Finished: town=$town score=$score seconds=$elapsed peak_rss_mb=$rss status=$experiment_status" >&2
done

echo "[*] Results: $result_file" >&2
