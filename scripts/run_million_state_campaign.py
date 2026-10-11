#!/usr/bin/env python3
"""Resume every town's persistent beam map to a target unique-node count."""

from __future__ import annotations

import argparse
import fcntl
import json
import os
import shutil
import sqlite3
import subprocess
import sys
from collections import Counter
from pathlib import Path
from typing import Any


TOWNS: tuple[tuple[int, str, str], ...] = (
    (0, "Normalia", "caravan-search.sqlite3"),
    (1, "Fang-Mo", "fang-mo.sqlite3"),
    (2, "L'Exquise", "l-exquise.sqlite3"),
    (3, "Mahaji", "mahaji.sqlite3"),
    (4, "Tocaccialli", "tocaccialli.sqlite3"),
    (5, "Bajaar", "bajaar.sqlite3"),
    (6, "Doba Lao", "doba-lao.sqlite3"),
    (7, "Huangoz", "huangoz.sqlite3"),
    (8, "Port-Willam", "port-willam.sqlite3"),
    (9, "Moonin", "moonin.sqlite3"),
    (10, "Cornecopea", "cornecopea.sqlite3"),
    (11, "Eduming", "eduming.sqlite3"),
    (12, "Octoyashi", "octoyashi.sqlite3"),
    (13, "Kifuai", "kifuai.sqlite3"),
    (14, "Skjollird", "skjollird.sqlite3"),
    (15, "Petrinov", "petrinov.sqlite3"),
    (16, "Jilliqo", "jilliqo.sqlite3"),
    (17, "Gayawaku", "gayawaku.sqlite3"),
    (18, "Vilparino", "vilparino.sqlite3"),
    (19, "Sanctifan", "sanctifan.sqlite3"),
    (20, "Mehaz", "mehaz.sqlite3"),
    (21, "Ipartus", "ipartus.sqlite3"),
)


def positive(value: str) -> int:
    parsed = int(value)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("must be greater than zero")
    return parsed


def map_stats(path: Path) -> dict[str, Any]:
    if not path.is_file():
        return {
            "nodes": 0,
            "open": 0,
            "partial": 0,
            "solved": 0,
            "root_status": "missing",
            "best_score": None,
            "database_bytes": 0,
        }
    with sqlite3.connect(f"file:{path}?mode=ro", uri=True) as database:
        scenarios = database.execute(
            "SELECT id,root_node_id FROM scenarios ORDER BY id"
        ).fetchall()
        if len(scenarios) != 1:
            raise RuntimeError(f"expected one scenario in {path}, found {len(scenarios)}")
        scenario_id, root_id = scenarios[0]
        counts = dict(
            database.execute(
                "SELECT status,COUNT(*) FROM node_index WHERE scenario_id=? GROUP BY status",
                (scenario_id,),
            )
        )
        root_status, best_score = database.execute(
            "SELECT status,best_score FROM node_index WHERE id=?", (root_id,)
        ).fetchone()
    return {
        "nodes": sum(int(count) for count in counts.values()),
        "open": int(counts.get(0, 0)),
        "partial": int(counts.get(1, 0)),
        "solved": int(counts.get(2, 0)),
        "root_status": ("open", "partial", "solved")[int(root_status)],
        "best_score": best_score,
        "database_bytes": path.stat().st_size,
    }


def prior_sessions(path: Path) -> Counter[int]:
    counts: Counter[int] = Counter()
    if not path.is_file():
        return counts
    for line in path.read_text(encoding="utf-8").splitlines():
        try:
            record = json.loads(line)
        except json.JSONDecodeError:
            continue
        if "campaign_town_index" in record:
            counts[int(record["campaign_town_index"])] += 1
    return counts


def selected_towns(raw: list[str] | None) -> list[tuple[int, str, str]]:
    if not raw:
        return list(TOWNS)
    wanted = {value.casefold() for value in raw}
    selected = [
        town for town in TOWNS
        if str(town[0]) in wanted or town[1].casefold() in wanted
    ]
    if len(selected) != len(wanted):
        known = {str(town[0]) for town in selected} | {town[1].casefold() for town in selected}
        missing = sorted(wanted - known)
        raise SystemExit(f"unknown town selections: {', '.join(missing)}")
    return selected


def write_record(path: Path, record: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8") as output:
        output.write(json.dumps(record, separators=(",", ":")) + "\n")
        output.flush()
        os.fsync(output.fileno())


def main() -> int:
    repo = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target-nodes", type=positive, default=1_000_000)
    parser.add_argument("--session-node-budget", type=positive, default=100_000)
    parser.add_argument("--widths", type=positive, nargs="+", default=[2_000])
    parser.add_argument("--max-seconds", type=positive, default=600)
    parser.add_argument("--minimum-free-gb", type=positive, default=5)
    parser.add_argument("--merchant-option")
    parser.add_argument("--towns", nargs="+")
    parser.add_argument("--maps-dir", type=Path, default=repo / "search_maps")
    parser.add_argument(
        "--results",
        type=Path,
        default=repo / "benchmarks/results/million-state-campaign.jsonl",
    )
    parser.add_argument("--status", action="store_true")
    parser.add_argument("--max-sessions", type=positive)
    parser.add_argument("--sessions-per-town", type=positive)
    args = parser.parse_args()

    if args.max_seconds > 600:
        raise SystemExit("--max-seconds may not exceed the 600-second campaign limit")
    towns = selected_towns(args.towns)
    args.maps_dir.mkdir(parents=True, exist_ok=True)

    if args.status:
        for town_id, town_name, filename in towns:
            stats = map_stats(args.maps_dir / filename)
            print(json.dumps({"town_index": town_id, "town": town_name, **stats}))
        return 0

    lock_path = args.maps_dir / ".million-state-campaign.lock"
    campaign_lock = lock_path.open("a+")
    try:
        fcntl.flock(campaign_lock.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError as error:
        raise RuntimeError(
            f"another campaign or inherited solver still holds {lock_path}"
        ) from error
    sessions = prior_sessions(args.results)

    completed_sessions = 0
    for town_id, town_name, filename in towns:
        map_path = args.maps_dir / filename
        town_sessions = 0
        while True:
            if args.sessions_per_town is not None and town_sessions >= args.sessions_per_town:
                print(
                    f"[*] Session cap satisfied: town={town_name} sessions={town_sessions}",
                    flush=True,
                )
                break
            before = map_stats(map_path)
            if before["nodes"] >= args.target_nodes or before["root_status"] == "solved":
                print(
                    f"[*] Target satisfied: town={town_name} nodes={before['nodes']} "
                    f"root={before['root_status']} best={before['best_score']}",
                    flush=True,
                )
                break
            if args.max_sessions is not None and completed_sessions >= args.max_sessions:
                return 0
            free_bytes = shutil.disk_usage(args.maps_dir).free
            if free_bytes < args.minimum_free_gb * 1024**3:
                raise RuntimeError(
                    f"campaign stopped with only {free_bytes / 1024**3:.1f} GiB free"
                )

            session_number = sessions[town_id] + 1
            width = args.widths[(session_number - 1) % len(args.widths)]
            print(
                f"[*] Million-state campaign: town={town_name} session={session_number} "
                f"nodes={before['nodes']} target={args.target_nodes} width={width} "
                f"merchant={args.merchant_option or 'any'}",
                flush=True,
            )
            environment = os.environ.copy()
            environment.update(
                {
                    "ARRAY_LIMIT": str(width),
                    "MAX_MB": "1024",
                    "INTERNAL_MB": "768",
                    "MAX_SECONDS": str(args.max_seconds),
                }
            )
            command = [
                str(repo / "scripts/run_safe.sh"),
                "--town", str(town_id),
                "--no-incumbent",
                "--no-auto-refine",
                "--search-map", str(map_path),
                "--search-node-budget", str(args.session_node_budget),
                "--search-map-order", "beam",
                "--benchmark-json",
            ]
            if args.merchant_option is not None:
                command.extend(("--merchant-option", args.merchant_option))
            process = subprocess.Popen(
                command,
                cwd=repo,
                env=environment,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                pass_fds=(campaign_lock.fileno(),),
            )
            benchmark: dict[str, Any] | None = None
            assert process.stdout is not None
            for line in process.stdout:
                if line.startswith("{"):
                    try:
                        benchmark = json.loads(line)
                        continue
                    except json.JSONDecodeError:
                        pass
                print(line, end="", flush=True)
            return_code = process.wait()
            after = map_stats(map_path)
            if benchmark is None:
                raise RuntimeError(f"{town_name} session produced no benchmark JSON")
            record = {
                **benchmark,
                "campaign_town_index": town_id,
                "campaign_session": session_number,
                "campaign_width": width,
                "campaign_merchant_option": args.merchant_option or "any",
                "campaign_target_nodes": args.target_nodes,
                "campaign_nodes_before": before["nodes"],
                "campaign_nodes_after": after["nodes"],
                "campaign_root_status": after["root_status"],
                "campaign_database_bytes": after["database_bytes"],
                "campaign_process_status": return_code,
            }
            write_record(args.results, record)
            if return_code != 0:
                raise RuntimeError(f"{town_name} session failed with status {return_code}")
            search_progress = any(
                after[field] != before[field]
                for field in ("nodes", "open", "partial", "solved", "root_status")
            )
            if not search_progress:
                raise RuntimeError(f"{town_name} session made no persistent progress")
            sessions[town_id] += 1
            completed_sessions += 1
            town_sessions += 1
            print(
                f"[+] Saved: town={town_name} session={session_number} width={width} "
                f"nodes={after['nodes']} solved={after['solved']} best={after['best_score']}",
                flush=True,
            )

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("[!] Campaign interrupted; committed SQLite sessions remain resumable", file=sys.stderr)
        raise SystemExit(130)
