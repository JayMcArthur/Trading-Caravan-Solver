# Light record search — 2026-10-09

Objective: find and independently replay a score above the current saved town
record while using substantially less memory than the previous deep sweep.
Run one native process at a time. Polishing uses a 384 MB hard cap, 288 MB
internal budget, and 600-second maximum; subsequent beam experiments must stay
at or below a 512 MB hard cap.

## Plan

1. Completed: inspect current worktree, records, deep-campaign guidance, and
   live processes. No native campaign was running. Previous deep sweep peaked
   at 668 MB and found no improvements.
2. Completed: safe build and sequential polishing of 19 towns. All lanes exited
   successfully, and the screen stopped when Moonin improved by 57 points.
3. Superseded: alternate ranking/suffix searches are unnecessary to meet the
   objective because the polishing screen found a new record.
4. Completed: strict independent replay and direct `Game` replay both pass
   for Moonin 37,584. Native fixture, `Best.txt`, current campaign table, and
   README are updated. Bounded full-suite verification passed.
5. Completed: residual audit confirms matching saved paths, complete lane
   measurements, resource caps, passing checks, and no live native processes.

## Result

**Moonin: 37,584, up 57 from 37,527.** The winning run evaluated 1,000,000
candidate routes, accepted 57,839 legal candidates, and completed 13 rounds
before reaching its evaluation budget. It exited 0 in 2.393 seconds (including
wrapper startup), with peak RSS 183,108 KiB = 178.8 MiB. No lane timed out or
hit its memory limit. Evaluated candidates are the processed-state count for
this concrete route search; these are not symbolic beam expansions.

Moonin can call Merchant at any time. Moving Magic Cleppsydra from before the
day-10 Camel to immediately before the day-13 Sell preserves the extra day and
leaves its 100-gold price available during the day-12 Buy. The first cargo
basket changes from 8 Spice / 9 Pottery / 1 Silk / 2 Jewelry to
15 Pottery / 1 Silk / 2 Jewelry. That uses 80 more gold and sells for 118 more,
adding 38 gold before the final 50% Interest action, hence 57 final points.

The entire screen evaluated 18,784,602 routes in 40.640 seconds, peaking at
182.5 MiB. Processes were sequential, each bounded externally at 384 MB,
compared with the previous deep campaign's 1,024 MB cap and 668 MB peak.
This is a bounded discovery, not a proof of global optimality.

## Reproduction

```bash
BUILD_MAX_MB=768 ./scripts/build_safe.sh
MAX_MB=384 INTERNAL_MB=288 MAX_SECONDS=595 ./scripts/run_safe.sh \
  --town Moonin --polish-record --polish-beam 256 \
  --polish-rounds 24 --polish-evaluations 1000000
MAX_MB=384 ./scripts/run_tests_safe.sh moonin-record
```

The discovery used the previous 37,527 fixture, whose Merchant was before the
day-10 Camel and whose day-12 basket was `[2, 8, 9, 0, 1, 2]`.
The fixture now contains the improved route, so subsequent runs start from
37,584. The complete replayable winning path is:

```text
1: Trader
2: Buy [12, 0, 0, 0, 0, 0]
3: Trader
4: Camel
5: Buy [26, 0, 0, 0, 0, 0]
6: Trader
7: Trader
8: Camel
Witch - Midas was a Trader
9: Buy [43, 0, 0, 0, 0, 0]
10: Camel
11: Camel
12: Buy [2, 0, 15, 0, 1, 2]
Merch - Magic Cleppsydra
13: Sell
14: Buy [20, 0, 0, 0, 0, 9]
15: Sell
16: Interest
```

## Lane measurements

Every lane used beam 256, at most 24 rounds and 1,000,000 evaluations, with
the same caps and timeout. RSS below is MiB; seconds include wrapper startup.
All scores other than Moonin match the pre-campaign record.

| Town | Score | Evaluated | Seconds | Peak RSS MiB | Status |
| --- | ---: | ---: | ---: | ---: | --- |
| Mehaz | 48,738 | 1,000,000 | 1.720 | 161.4 | completed |
| Tocaccialli | 49,634 | 1,000,000 | 1.962 | 174.2 | completed |
| Gayawaku | 50,105 | 1,000,000 | 2.116 | 167.3 | completed |
| L'Exquise | 56,817 | 1,000,000 | 3.374 | 174.8 | completed |
| Petrinov | 45,578 | 784,602 | 1.596 | 122.8 | completed |
| Eduming | 81,321 | 1,000,000 | 2.026 | 162.8 | completed |
| Bajaar | 56,147 | 1,000,000 | 1.995 | 163.4 | completed |
| Port-Willam | 42,296 | 1,000,000 | 2.017 | 179.4 | completed |
| Skjollird | 47,065 | 1,000,000 | 2.037 | 178.1 | completed |
| Cornecopea | 43,881 | 1,000,000 | 1.895 | 162.5 | completed |
| Fang-Mo | 37,569 | 1,000,000 | 2.042 | 175.9 | completed |
| Jilliqo | 37,819 | 1,000,000 | 2.351 | 175.9 | completed |
| Octoyashi | 40,056 | 1,000,000 | 1.965 | 182.5 | completed |
| Kifuai | 37,547 | 1,000,000 | 2.021 | 177.8 | completed |
| Ipartus | 37,734 | 1,000,000 | 2.055 | 175.0 | completed |
| Doba Lao | 45,543 | 1,000,000 | 2.310 | 160.4 | completed |
| Mahaji | 37,589 | 1,000,000 | 2.470 | 179.1 | completed |
| Vilparino | 37,569 | 1,000,000 | 2.295 | 177.9 | completed |
| Moonin | **37,584** | 1,000,000 | 2.393 | 178.8 | completed |

Raw commands, environment, actions, exit codes, and measurements are in
`benchmarks/results/light-record-20261009/summary.jsonl`. Each lane also has a
full `.log` action path and `.time` resource report in that directory.

## Residual audit plan

1. Completed: full native suite passed through `MAX_MB=512 MAX_SECONDS=180
   ./scripts/run_tests_safe.sh`. Assertions remain enabled in Release via
   `-UNDEBUG`. Moonin's test executes strict legality validation and a separate
   direct `Game` replay, confirming day 16, 37,309 gold and 37,584 final points.
2. Completed: saved `Best.txt` actions match the original winning log and this
   report exactly. The native fixture matches and passes the focused replay
   test under a 384 MB cap. Current score tables show 37,584; historical
   campaign documents retain their original measured values.
3. Completed: all 19 rows have full action logs, measurements, exact commands,
   caps and successful exit codes. No solver/compiler remains running.

Final checks: safe build passed under a 768 MB cap with compiler serialization;
focused replay passed under 384 MB; full native suite passed under 512 MB;
shell syntax checks passed for all bounded execution/build wrappers; artifact
audit passed. Logs are `build.log`, `moonin-record-test.log`, `full-tests.log`,
and `audit.log` in the raw artifact directory. No required residual work remains
for the requested new-record search.
