# Million-state persistent campaign — 2026-09-04

## Objective

Build a separate persistent search graph for every Classical town until it
contains at least 1,000,000 unique semantic states. If a town's root becomes
fully solved below that threshold, record the smaller exhaustive graph instead.
Inspect every graph for score improvements and replay-verify any candidate
before changing `Best.txt`.

## Search configuration

- Mapped beam order with width 2,000. Width 500 is reserved for later
  diversity passes after the initial graph-growth target.
- Strategic purchase profile and 5% continuation-lane retention.
- Open Merchant and Witch timing and options.
- No seeded incumbent and no automatic refinement.
- 100,000 atomic expansion budget per resumable process. The initial 25,000
  sessions completed in about one minute, so this larger checkpoint remains
  comfortably inside the time cap while amortizing queue rescans.
- 600-second wall limit, 1,024 MB hard memory cap, and 768 MB internal budget.
- One SQLite file per town under `search_maps/`.
- A 5 GiB free-space floor checked before every session.
- Append-only session data in
  `benchmarks/results/million-state-campaign.jsonl`.

Run or resume the complete campaign with:

```bash
scripts/run_million_state_campaign.py
```

Run it as a detached resumable supervisor with:

```bash
scripts/start_million_state_campaign.sh
```

Its PID is recorded in `runtime/million-state-campaign.pid` and its concise
checkpoint log is `benchmarks/results/million-state-campaign.log`. Starting a
second detached supervisor is rejected while that PID is alive. The runner
also holds `search_maps/.million-state-campaign.lock`, inherited by each solver
child, so a replacement runner cannot overlap an orphaned child.

Inspect progress without launching native code:

```bash
scripts/run_million_state_campaign.py --status
```

Limit a recovery run to selected towns with, for example:

```bash
scripts/run_million_state_campaign.py --towns Port-Willam Normalia
```

## Starting inventory

The machine has 804 GB free. Existing maps are:

| Town | Unique states | Current mapped best |
| --- | ---: | ---: |
| Port-Willam | 279,382 | 42,296 |
| Normalia | 52,518 | 37,478 |
| Remaining 20 towns | 0 each | — |

The original format projected to roughly 28–44 GB for all 22 maps. The compact
lossless format stores each semantic state once, stores provenance separately,
compresses both payloads, and removes redundant SQLite rowid trees. A migrated
Port-Willam copy fell from 362 MB to 148 MB with identical graph counts and
top-three routes. At that measured density, 22 million states project to about
12 GB. Actual database bytes and disk space are checked throughout the
campaign.

## Trace-allocation incident and recovery

An interrupted foreground supervisor was followed by a detached restart. Two
writers could therefore overlap briefly, and the original process-local
`MAX(trace_id)+1` allocator gave both the same trace ID. Fang-Mo stopped on the
resulting uniqueness violation at 771,322 committed states. The database
passed SQLite integrity and graph-invariant verification.

The allocator now obtains the current maximum only after `BEGIN IMMEDIATE`, so
SQLite serializes allocation across writers. A deterministic two-writer
regression test reproduced the original uniqueness violation before the fix
and passes after it. A 1,000-expansion production replay then advanced Fang-Mo
to 773,813 states with dense trace IDs and clean verification. The inherited
campaign lock prevents the operational overlap as well.

## Status

Campaign running in the detached supervisor. Normalia has reached 1,023,163
states (929,766 solved) and independently found 37,527. Fang-Mo resumed from
773,813 states with a 37,569 lower bound. The append-only JSONL file is
authoritative for session timings, memory peaks, widths, scores, and node-count
deltas. This report will be updated at town milestones and after the final
integrity/replay audit.
