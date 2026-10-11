#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
towns=${TOWNS:-"0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21"}
beams=${BEAMS:-"1000"}
profiles=${PROFILES:-"greedy"}
shares=${SHARES:-"25"}
starts=${STARTS:-"trader"}
beam_scorers=${BEAM_SCORERS:-"legacy"}
max_mb=${MAX_MB:-768}
internal_mb=${INTERNAL_MB:-512}
max_seconds=${MAX_SECONDS:-90}
result_file=${RESULT_FILE:-"$repo/benchmarks/results/classical-$(date -u +%Y%m%dT%H%M%SZ).jsonl"}

mkdir -p "$(dirname "$result_file")"

for beam in $beams; do
    for profile in $profiles; do
        case "$profile" in
            greedy|scenario|strategic|wide) ;;
            *) echo "[!] Invalid purchase profile: $profile" >&2; exit 2 ;;
        esac
        for share in $shares; do
            if [[ ! "$share" =~ ^[0-9]+$ ]] || ((share > 100)); then
                echo "[!] Invalid optimistic share: $share" >&2
                exit 2
            fi
            for start in $starts; do
                case "$start" in
                    trader) start_args=(--starting-trader) ;;
                    open) start_args=() ;;
                    *) echo "[!] Invalid start: $start" >&2; exit 2 ;;
                esac
                for beam_scorer in $beam_scorers; do
                        case "$beam_scorer" in
                            legacy) beam_scorer_args=() ;;
                            sell-aware) beam_scorer_args=(--experimental-sell-aware-beam) ;;
                            *) echo "[!] Invalid beam scorer: $beam_scorer" >&2; exit 2 ;;
                        esac
                        for town in $towns; do
                            echo "[*] Classical experiment: town=$town beam=$beam profile=$profile share=$share start=$start beam_scorer=$beam_scorer" >&2
                            raw_output=$(
                                MAX_MB="$max_mb" \
                                INTERNAL_MB="$internal_mb" \
                                ARRAY_LIMIT="$beam" \
                                MAX_SECONDS="$max_seconds" \
                                "$repo/scripts/run_safe.sh" \
                                    --town "$town" \
                                    --no-incumbent \
                                    --purchase-profile "$profile" \
                                    --optimistic-share "$share" \
                                    "${beam_scorer_args[@]}" \
                                    "${start_args[@]}" \
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
                            printf '%s,"experiment_beam":%s,"experiment_start":"%s","experiment_beam_scorer":"%s","experiment_status":"%s"}\n' \
                                "$json" "$beam" "$start" "$beam_scorer" "$experiment_status" | tee -a "$result_file"
                        done
                    done
            done
        done
    done
done

echo "[*] Results: $result_file" >&2
