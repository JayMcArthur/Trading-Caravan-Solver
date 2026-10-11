#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
session_nodes=${SESSION_NODE_BUDGET:-100000}
max_seconds=${MAX_SECONDS:-600}
minimum_free_gb=${MINIMUM_FREE_GB:-100}

deep_lanes=(
    "route-to-mahaji|Route to Mahaji"
    "sturdy-saddle|Sturdy Saddle"
    "magic-cleppsydra|Magic Cleppsydra"
    "blue-treasure|Blue Treasure"
)

screen_lanes=(
    "wooden-statuette|Wooden Statuette"
    "canvas-bag|Canvas Bag"
    "leaders-necklace|Leaders Necklace"
    "hand-of-midas|Hand of Midas"
)

run_lane() {
    local lane=$1
    local target_nodes=$2
    local sessions_per_town=${3:-}
    IFS='|' read -r slug merchant_option <<<"$lane"
    maps_dir="$repo/search_maps/merchant-profiles/$slug"
    result_file="$repo/benchmarks/results/million-state-merchant-$slug.jsonl"
    command=(
        python3 "$repo/scripts/run_million_state_campaign.py"
        --merchant-option "$merchant_option" \
        --target-nodes "$target_nodes" \
        --session-node-budget "$session_nodes" \
        --max-seconds "$max_seconds" \
        --minimum-free-gb "$minimum_free_gb" \
        --maps-dir "$maps_dir" \
        --results "$result_file"
    )
    if [[ -n "$sessions_per_town" ]]; then
        command+=(--sessions-per-town "$sessions_per_town")
    fi
    echo "[*] Merchant campaign: merchant=$merchant_option target=$target_nodes maps=$maps_dir" >&2
    "${command[@]}"
}

for lane in "${deep_lanes[@]}"; do
    run_lane "$lane" 1000000
done

for lane in "${screen_lanes[@]}"; do
    run_lane "$lane" 1000000 1
done
