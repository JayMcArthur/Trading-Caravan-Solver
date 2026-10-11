#!/usr/bin/env python3
"""Inspect and maintain a persistent Trading Caravan exact-search graph."""

from __future__ import annotations

import argparse
import json
import sqlite3
from pathlib import Path
from typing import Any


STATUS_NAMES = {0: "open", 1: "partial", 2: "solved"}


def connect(path: Path) -> sqlite3.Connection:
    if not path.is_file():
        raise SystemExit(f"search map does not exist: {path}")
    database = sqlite3.connect(f"file:{path}?mode=ro", uri=True)
    database.row_factory = sqlite3.Row
    return database


def file_bytes(path: Path) -> int:
    return sum(
        candidate.stat().st_size
        for candidate in (path, Path(f"{path}-wal"), Path(f"{path}-shm"))
        if candidate.exists()
    )


def scenario_rows(database: sqlite3.Connection) -> list[dict[str, Any]]:
    return [
        dict(row)
        for row in database.execute(
            "SELECT id,label,root_node_id,created_at FROM scenarios ORDER BY id"
        )
    ]


def scenario_stats(
    database: sqlite3.Connection, scenario_id: int
) -> dict[str, Any]:
    scenario = database.execute(
        "SELECT id,label,root_node_id,created_at FROM scenarios WHERE id=?",
        (scenario_id,),
    ).fetchone()
    if scenario is None:
        raise SystemExit(f"unknown scenario id: {scenario_id}")
    counts = {"open": 0, "partial": 0, "solved": 0}
    total = 0
    for row in database.execute(
        "SELECT status,COUNT(*) AS count FROM node_index "
        "WHERE scenario_id=? GROUP BY status",
        (scenario_id,),
    ):
        count = int(row["count"])
        total += count
        counts[STATUS_NAMES[int(row["status"])]] = count
    root = database.execute(
        "SELECT status,best_score,child_count,solved_child_count "
        "FROM node_index WHERE id=?",
        (scenario["root_node_id"],),
    ).fetchone()
    edges = database.execute(
        "SELECT COUNT(*) FROM edges e JOIN node_index i ON i.id=e.parent_id "
        "WHERE i.scenario_id=?",
        (scenario_id,),
    ).fetchone()[0]
    results = database.execute(
        "SELECT COUNT(*) FROM node_results r JOIN node_index i ON i.id=r.node_id "
        "WHERE i.scenario_id=?",
        (scenario_id,),
    ).fetchone()[0]
    traces = database.execute(
        "SELECT COUNT(*) FROM traces WHERE scenario_id=?", (scenario_id,)
    ).fetchone()[0]
    return {
        "id": scenario_id,
        "label": scenario["label"],
        "created_at": scenario["created_at"],
        "root_status": STATUS_NAMES[int(root["status"])],
        "root_best_score": root["best_score"],
        "root_children": int(root["child_count"]),
        "root_solved_children": int(root["solved_child_count"]),
        "nodes": total,
        **counts,
        "edges": int(edges),
        "terminal_results": int(results),
        "trace_links": int(traces),
    }


def format_action(row: sqlite3.Row) -> str:
    action = int(row["action"])
    option = int(row["option"])
    if action == 0:
        quantities = ", ".join(
            str(int(row[name]))
            for name in ("food", "spice", "pottery", "marble", "silk", "jewelry")
        )
        return f"Buy [{quantities}]"
    if action == 1:
        return "Sell"
    if action == 2:
        return "Trader" if option == 0 else "Camel"
    if action == 3:
        return "Interest"
    if action == 4:
        return f"Merchant option {option}"
    if action == 5:
        return f"Witch option {option}"
    if action == 6:
        return f"Disaster option {option}"
    if action == 7:
        return "Wait"
    return f"Unknown action {action} option {option}"


def reverse_route(
    database: sqlite3.Connection, scenario_id: int, terminal_trace: int
) -> list[str]:
    actions: list[str] = []
    trace_id = terminal_trace
    while trace_id:
        row = database.execute(
            "SELECT parent_id,action,food,spice,pottery,marble,silk,jewelry,option "
            "FROM traces WHERE scenario_id=? AND trace_id=?",
            (scenario_id, trace_id),
        ).fetchone()
        if row is None:
            raise SystemExit(f"missing trace {trace_id} in scenario {scenario_id}")
        actions.append(format_action(row))
        trace_id = int(row["parent_id"])
    actions.reverse()
    return actions


def best_routes(
    database: sqlite3.Connection, scenario_id: int, limit: int
) -> list[dict[str, Any]]:
    rows = database.execute(
        "SELECT r.score,r.trace_id FROM node_results r "
        "JOIN node_index i ON i.id=r.node_id WHERE i.scenario_id=? "
        "ORDER BY r.score DESC,r.trace_id LIMIT ?",
        (scenario_id, limit),
    )
    return [
        {
            "score": int(row["score"]),
            "terminal_trace": int(row["trace_id"]),
            "actions": reverse_route(database, scenario_id, int(row["trace_id"])),
        }
        for row in rows
    ]


def verify(database: sqlite3.Connection, scenario_id: int) -> list[str]:
    errors: list[str] = []
    for row in database.execute(
        "SELECT i.id,i.status,i.child_count,i.solved_child_count,"
        "COUNT(e.child_id) AS actual_children,"
        "COALESCE(SUM(c.status=2),0) AS actual_solved "
        "FROM node_index i LEFT JOIN edges e ON e.parent_id=i.id "
        "LEFT JOIN node_index c ON c.id=e.child_id "
        "WHERE i.scenario_id=? GROUP BY i.id",
        (scenario_id,),
    ):
        node_id = int(row["id"])
        status = int(row["status"])
        actual_children = int(row["actual_children"])
        actual_solved = int(row["actual_solved"])
        if status != 0 and (
            int(row["child_count"]) != actual_children
            or int(row["solved_child_count"]) != actual_solved
        ):
            errors.append(f"node {node_id}: cached child counts differ from edges")
        should_be_solved = status != 0 and actual_children == actual_solved
        if (status == 2) != should_be_solved:
            errors.append(f"node {node_id}: solved status disagrees with descendants")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("database", type=Path)
    parser.add_argument("command", choices=("scenarios", "stats", "best", "verify", "export"))
    parser.add_argument("--scenario", type=int)
    parser.add_argument("--limit", type=int, default=3)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    with connect(args.database) as database:
        if args.command == "scenarios":
            payload: Any = scenario_rows(database)
        else:
            scenarios = scenario_rows(database)
            scenario_id = args.scenario
            if scenario_id is None:
                if len(scenarios) != 1:
                    raise SystemExit("--scenario is required when the map contains multiple scenarios")
                scenario_id = int(scenarios[0]["id"])
            if args.command == "stats":
                payload = scenario_stats(database, scenario_id)
                payload["database_bytes"] = file_bytes(args.database)
            elif args.command == "best":
                payload = best_routes(database, scenario_id, args.limit)
            elif args.command == "verify":
                errors = verify(database, scenario_id)
                payload = {"ok": not errors, "errors": errors}
            else:
                payload = {
                    "stats": scenario_stats(database, scenario_id),
                    "best": best_routes(database, scenario_id, args.limit),
                }

    rendered = json.dumps(payload, indent=2)
    if args.output:
        args.output.write_text(rendered + "\n", encoding="utf-8")
    else:
        print(rendered)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
