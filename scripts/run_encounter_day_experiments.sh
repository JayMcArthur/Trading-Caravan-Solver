#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
towns=${TOWNS:-"Bajaar Port-Willam Petrinov Gayawaku"}
merchant_days=${MERCHANT_DAYS:-"5 6 7"}
witch_days=${WITCH_DAYS:-"9 10 11"}
beam=${BEAM:-500}
max_mb=${MAX_MB:-768}
internal_mb=${INTERNAL_MB:-512}
max_seconds=${MAX_SECONDS:-90}
result_file=${RESULT_FILE:-"$repo/benchmarks/results/encounter-days-$(date -u +%Y%m%dT%H%M%SZ).jsonl"}

if [[ ! "$beam" =~ ^[1-9][0-9]*$ ]]; then
    echo "[!] BEAM must be a positive integer" >&2
    exit 2
fi

mkdir -p "$(dirname "$result_file")"

for town in $towns; do
    case "$town" in
        Bajaar) merchant_option="Sturdy Saddle" ;;
        Port-Willam|Gayawaku) merchant_option="Magic Cleppsydra" ;;
        Petrinov) merchant_option="Blue Treasure" ;;
        *)
            echo "[!] No isolated Merchant choice configured for town: $town" >&2
            exit 2
            ;;
    esac

    for merchant_day in $merchant_days; do
        for witch_day in $witch_days; do
            echo "[*] Encounter scenario: town=$town merchant_day=$merchant_day witch_day=$witch_day beam=$beam" >&2
            raw_output=$(
                MAX_MB="$max_mb" \
                INTERNAL_MB="$internal_mb" \
                ARRAY_LIMIT="$beam" \
                MAX_SECONDS="$max_seconds" \
                "$repo/scripts/run_safe.sh" \
                    --town "$town" \
                    --merchant-day "$merchant_day" \
                    --witch-day "$witch_day" \
                    --merchant-option "$merchant_option" \
                    --witch-option "Midas was a Trader" \
                    --no-incumbent \
                    --benchmark-json
            )
            json=$(awk '/^\{/{line=$0} END{print line}' <<<"$raw_output")
            if [[ -z "$json" ]]; then
                echo "[!] Scenario did not produce JSON" >&2
                exit 1
            fi
            experiment_status=complete
            if [[ "$json" == *'"search_complete":false'* ]]; then
                experiment_status=stopped
                echo "[!] Scenario stopped safely; preserving diagnostic JSON" >&2
            fi
            json=${json%\}}
            printf '%s,"experiment_status":"%s"}\n' "$json" "$experiment_status" | tee -a "$result_file"
        done
    done
done

echo "[*] Results: $result_file" >&2
