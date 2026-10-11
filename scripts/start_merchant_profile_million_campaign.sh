#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
pid_file="$repo/runtime/merchant-profile-million-campaign.pid"
log_file="$repo/benchmarks/results/merchant-profile-million-campaign.log"

mkdir -p "$repo/runtime" "$repo/benchmarks/results"

if [[ -f "$pid_file" ]]; then
    existing_pid=$(sed -n '1p' "$pid_file")
    if [[ "$existing_pid" =~ ^[1-9][0-9]*$ ]] && kill -0 "$existing_pid" 2>/dev/null; then
        echo "[!] Merchant-profile campaign is already running as PID $existing_pid" >&2
        exit 2
    fi
fi

nohup setsid "$repo/scripts/run_merchant_profile_million_campaign.sh" "$@" \
    >>"$log_file" 2>&1 </dev/null &
campaign_pid=$!
printf '%s\n' "$campaign_pid" >"$pid_file"

echo "[*] Merchant-profile million-state campaign started as PID $campaign_pid"
echo "[*] Log: $log_file"
