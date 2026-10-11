#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
towns=${TOWNS:-"0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21"}
shares=${SHARES:-"0 2 5"}
beam=${BEAM:-50000}
suffix_limit=${SUFFIX_LIMIT:-50000}
batch_size=${BATCH_SIZE:-5000}
split_day=${SPLIT_DAY:-12}
max_mb=${MAX_MB:-1024}
internal_mb=${INTERNAL_MB:-768}
max_seconds=${MAX_SECONDS:-602}
resume=${RESUME:-1}
result_file=${RESULT_FILE:-"$repo/benchmarks/results/deep-all-town-$(date -u +%Y%m%dT%H%M%SZ).jsonl"}

positive_integer() {
    [[ "$1" =~ ^[1-9][0-9]*$ ]]
}

if ! positive_integer "$beam" || ! positive_integer "$suffix_limit" ||
    ! positive_integer "$batch_size" || ! positive_integer "$split_day" ||
    ! positive_integer "$max_seconds"; then
    echo "[!] Beam, suffix, batch, split-day, and time settings must be positive integers" >&2
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
for share in $shares; do
    if [[ ! "$share" =~ ^[0-9]+$ ]] || ((share > 100)); then
        echo "[!] Continuation lane shares must be integers from 0 through 100" >&2
        exit 2
    fi
done
for town in $towns; do
    if [[ ! "$town" =~ ^[0-9]+$ ]] || ((town > 21)); then
        echo "[!] Town indices must be integers from 0 through 21" >&2
        exit 2
    fi
done

mkdir -p "$(dirname "$result_file")"

for town in $towns; do
    for share in $shares; do
        marker="\"experiment_town_index\":$town,\"experiment_continuation_lane_share\":$share"
        if [[ "$resume" == 1 && -f "$result_file" ]] && grep -Fq "$marker" "$result_file"; then
            echo "[*] Skipping recorded run: town=$town continuation_share=$share%" >&2
            continue
        fi

        echo "[*] Deep campaign: town=$town continuation_share=$share% beam=$beam suffix=$suffix_limit" >&2
        raw_output=$(
            MAX_MB="$max_mb" \
            INTERNAL_MB="$internal_mb" \
            ARRAY_LIMIT="$beam" \
            MAX_SECONDS="$max_seconds" \
            "$repo/scripts/run_safe.sh" \
                --town "$town" \
                --no-auto-refine \
                --hybrid-batch-size "$batch_size" \
                --hybrid-suffix-limit "$suffix_limit" \
                --hybrid-split-day "$split_day" \
                --hybrid-skeleton-order structure-interleave \
                --continuation-lane-share "$share" \
                --benchmark-json
        )
        json=$(awk '/^\{/{line=$0} END{print line}' <<<"$raw_output")
        if [[ -z "$json" ]]; then
            echo "[!] Run did not produce JSON: town=$town continuation_share=$share%" >&2
            exit 1
        fi

        experiment_status=complete
        if [[ "$json" == *'"search_complete":false'* ]]; then
            experiment_status=stopped
            echo "[!] Run stopped safely; preserving its diagnostic JSON" >&2
        fi

        json=${json%\}}
        result=$(printf '%s,"experiment_town_index":%s,"experiment_continuation_lane_share":%s,"experiment_beam":%s,"experiment_suffix_limit":%s,"experiment_status":"%s"}' \
            "$json" "$town" "$share" "$beam" "$suffix_limit" "$experiment_status")
        printf '%s\n' "$result" >> "$result_file"

        score=$(awk 'match($0, /"best_score":[0-9]+/) {print substr($0, RSTART + 13, RLENGTH - 13)}' <<<"$result")
        elapsed=$(awk 'match($0, /"elapsed_seconds":[0-9.]+/) {print substr($0, RSTART + 18, RLENGTH - 18)}' <<<"$result")
        rss=$(awk 'match($0, /"peak_rss_mb":[0-9]+/) {print substr($0, RSTART + 14, RLENGTH - 14)}' <<<"$result")
        echo "[+] Finished: town=$town continuation_share=$share% score=$score seconds=$elapsed peak_rss_mb=$rss status=$experiment_status" >&2
    done
done

echo "[*] Results: $result_file" >&2
