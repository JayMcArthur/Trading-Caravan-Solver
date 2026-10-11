#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
towns=${TOWNS:-"0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21"}
policies=${POLICIES:-"balanced exploit optimistic sell-aware diversity"}
beam=${BEAM:-5000}
profile=${PROFILE:-greedy}
max_mb=${MAX_MB:-768}
internal_mb=${INTERNAL_MB:-640}
max_seconds=${MAX_SECONDS:-180}
resume=${RESUME:-1}
result_file=${RESULT_FILE:-"$repo/benchmarks/results/all-town-pruning-b${beam}-$(date -u +%Y%m%dT%H%M%SZ).jsonl"}

case "$profile" in
    greedy|scenario|strategic|wide) ;;
    *) echo "[!] Invalid purchase profile: $profile" >&2; exit 2 ;;
esac

if [[ ! "$beam" =~ ^[1-9][0-9]*$ ]]; then
    echo "[!] BEAM must be a positive integer" >&2
    exit 2
fi
if [[ ! "$max_mb" =~ ^[1-9][0-9]*$ ]] || ((max_mb > 1024)); then
    echo "[!] MAX_MB must be an integer from 1 through 1024" >&2
    exit 2
fi
if [[ ! "$internal_mb" =~ ^[1-9][0-9]*$ ]] || ((internal_mb > max_mb)); then
    echo "[!] INTERNAL_MB must be a positive integer no greater than MAX_MB" >&2
    exit 2
fi
if [[ "$resume" != 0 && "$resume" != 1 ]]; then
    echo "[!] RESUME must be 0 or 1" >&2
    exit 2
fi

mkdir -p "$(dirname "$result_file")"

for policy in $policies; do
    case "$policy" in
        balanced)
            policy_args=(--optimistic-share 25)
            ;;
        exploit)
            policy_args=(--optimistic-share 0)
            ;;
        optimistic)
            policy_args=(--optimistic-share 50)
            ;;
        sell-aware)
            policy_args=(--optimistic-share 25 --experimental-sell-aware-beam)
            ;;
        diversity)
            policy_args=(--optimistic-share 25 --diversity-share 25)
            ;;
        *)
            echo "[!] Invalid pruning policy: $policy" >&2
            exit 2
            ;;
    esac

    for town in $towns; do
        if [[ "$resume" == 1 && -f "$result_file" ]] &&
            grep -Fq "\"experiment_town_index\":$town,\"experiment_policy\":\"$policy\"" "$result_file"; then
            echo "[*] Skipping completed run: town=$town policy=$policy" >&2
            continue
        fi

        echo "[*] All-town pruning experiment: town=$town policy=$policy beam=$beam profile=$profile start=open" >&2
        raw_output=$(
            MAX_MB="$max_mb" \
            INTERNAL_MB="$internal_mb" \
            ARRAY_LIMIT="$beam" \
            MAX_SECONDS="$max_seconds" \
            "$repo/scripts/run_safe.sh" \
                --town "$town" \
                --no-incumbent \
                --purchase-profile "$profile" \
                "${policy_args[@]}" \
                --benchmark-json
        )
        json=$(awk '/^\{/{line=$0} END{print line}' <<<"$raw_output")
        if [[ -z "$json" ]]; then
            echo "[!] Experiment did not produce JSON: town=$town policy=$policy" >&2
            exit 1
        fi

        experiment_status=complete
        if [[ "$json" == *'"search_complete":false'* ]]; then
            experiment_status=stopped
            echo "[!] Run stopped safely; preserving its diagnostic JSON" >&2
        fi

        json=${json%\}}
        result=$(printf '%s,"experiment_town_index":%s,"experiment_policy":"%s","experiment_beam":%s,"experiment_start":"open","experiment_status":"%s"}' \
            "$json" "$town" "$policy" "$beam" "$experiment_status")
        printf '%s\n' "$result" >> "$result_file"

        score=$(awk 'match($0, /"best_score":[0-9]+/) {print substr($0, RSTART + 13, RLENGTH - 13)}' <<<"$result")
        elapsed=$(awk 'match($0, /"elapsed_seconds":[0-9.]+/) {print substr($0, RSTART + 18, RLENGTH - 18)}' <<<"$result")
        echo "[+] Finished: town=$town policy=$policy score=$score seconds=$elapsed status=$experiment_status" >&2
    done
done

echo "[*] Results: $result_file" >&2
