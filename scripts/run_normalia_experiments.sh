#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
beams=${BEAMS:-"1000 20000 100000"}
profiles=${PROFILES:-"greedy scenario strategic wide"}
starts=${STARTS:-"trader"}
shares=${SHARES:-"75"}
max_mb=${MAX_MB:-1024}
internal_mb=${INTERNAL_MB:-768}
max_seconds=${MAX_SECONDS:-300}
result_file=${RESULT_FILE:-"$repo/benchmarks/results/normalia-$(date -u +%Y%m%dT%H%M%SZ).jsonl"}

mkdir -p "$(dirname "$result_file")"

for beam in $beams; do
    if [[ ! "$beam" =~ ^[1-9][0-9]*$ ]]; then
        echo "[!] Invalid beam width: $beam" >&2
        exit 2
    fi
    for start in $starts; do
        case "$start" in
            trader) start_args=(--starting-trader) ;;
            open) start_args=() ;;
            *)
                echo "[!] Invalid start: $start (expected trader or open)" >&2
                exit 2
                ;;
        esac
        for profile in $profiles; do
            case "$profile" in
                greedy|scenario|strategic|wide) ;;
                *)
                    echo "[!] Invalid purchase profile: $profile" >&2
                    exit 2
                    ;;
            esac

            for share in $shares; do
                if [[ ! "$share" =~ ^[0-9]+$ ]] || ((share > 100)); then
                    echo "[!] Invalid optimistic share: $share" >&2
                    exit 2
                fi
                echo "[*] Normalia unseeded experiment: beam=$beam start=$start profile=$profile optimistic=$share%" >&2
                raw_output=$(
                    MAX_MB="$max_mb" \
                    INTERNAL_MB="$internal_mb" \
                    ARRAY_LIMIT="$beam" \
                    MAX_SECONDS="$max_seconds" \
                    "$repo/scripts/run_safe.sh" \
                        --town Normalia \
                        --no-incumbent \
                        "${start_args[@]}" \
                        --purchase-profile "$profile" \
                        --optimistic-share "$share" \
                        --benchmark-json
                )
                json=$(awk '/^\{/{line=$0} END{print line}' <<<"$raw_output")
                if [[ -z "$json" ]]; then
                    echo "[!] Experiment did not produce JSON" >&2
                    exit 1
                fi
                experiment_status=complete
                if [[ "$json" == *'"search_complete":false'* ]]; then
                    experiment_status=stopped
                    echo "[!] Experiment stopped safely before completion; preserving diagnostic JSON" >&2
                fi
                json=${json%\}}
                printf '%s,"experiment_status":"%s"}\n' "$json" "$experiment_status" | tee -a "$result_file"
            done
        done
    done
done

echo "[*] Results: $result_file" >&2
