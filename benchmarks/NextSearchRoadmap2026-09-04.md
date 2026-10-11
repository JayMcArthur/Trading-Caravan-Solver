# Search roadmap after exact deferred dominance

This is the agreed sequence after exact deferred symbolic dominance recovered
Normalia 37,527 at beam 500 and Gayawaku 50,105 at beam 5,000. The objective is
to improve coverage and ultimately prove top-three routes without relying on an
arbitrarily large beam.

## 1. Rebaseline every town

Run all 22 towns at beam 500 with an open start, no incumbent, no automatic
refinement, and 5% continuation lanes. Use beam 2,000–5,000 only for towns that
still miss their replay-verified score. Every native process must run
sequentially through `scripts/run_safe.sh` beneath the 1,024 MB hard cap.

Deliverable: a complete score/performance matrix, verified improvements, and a
short list of remaining misses.

## 2. Add exact Buy-quantity survival auditing

Extend the observational route audit beyond action types. For every saved Buy,
track whether its exact food/cargo allocation remains feasible in the deferred
model, is replaced by a provably dominating allocation, is removed by an exact
reduction, or disappears at the lossy beam.

Correctness rule: auditing must never reserve a state, alter ranking, or change
the generated-state count.

## 3. Canonicalize sold ledgers

Replace history-shaped sold ledgers with an exact current-resource envelope:

```text
exact remaining food -> maximum gold + trace provenance
```

Weighted food keeps distinct food entries. Weightless food may use proven
cross-food dominance. This should let equivalent outcomes from different Buy
and post-Sell histories deduplicate and dominate one another.

Validation: exhaustive small-market replay, unchanged verified scores, reduced
sold expansion time and state count.

## 4. Prune while generating

Introduce a deep `FrontierAccumulator` module with a small interface:

```text
insert(candidate)
finish() -> exact nondominated states
```

It should perform semantic deduplication and exact dominance as candidates
arrive, before temporary vectors grow to the periodic reduction threshold.
Lossy beam selection remains a separate final step.

Validation: identical post-reduction frontier for the same candidate multiset,
independent of insertion order; lower temporary memory and generation overhead.

## 5. Batch exact Sell evaluation

Group queries that share Buy market, Sell market, capacity, and gold-operation
shape. Evaluate the repeated maximum

```text
gold_function(spend) + cargo_sale_value
```

as a shared max-plus frontier/table rather than rescanning the same cargo rows
for each symbolic state. This is the most plausible place for matrix-style or
structure-of-arrays processing.

Validation: bit-exact agreement with the current exact-sale oracle across all
budgets, weights, Interest rounding sequences, and market-event combinations.

## 6. Generalize deferred purchase dominance

The current production proof requires identical ordered origin shapes. Build a
capability-envelope comparison that can prove every weaker origin/allocation is
covered by one or more stronger origins with different shapes.

Validation: exhaustive containment oracle first; no quadratic all-pairs scan;
separate counters for partition rejection, coverage checks, and successful
cross-shape prunes.

## 7. Add an admissible bound and memory-bounded proof search

Create a mathematically safe upper-bound module for every unfinished state.
Use it with the exact reducer and a depth-first schedule that retains only a
bounded transposition table and the best three finished routes. Prune a state
only when its bound cannot beat the current third-place result.

This is the path toward a certified top three. The current continuation and
skeleton scores remain ordering heuristics because they are not safe bounds.

Validation: small-horizon exhaustive equivalence, explicit bound-violation
assertions, less than 1 GB resident memory, and a final certificate that every
unexplored state is bounded below the third-place score.

## Implementation review: items 2-7

Implemented on 2026-09-04 after the beam-2,000 and beam-5,000 rebaseline
passes:

- **2 — exact Buy-quantity audit:** complete. The route audit now distinguishes
  exact quantity survival, a proven dominating replacement, and disappearance.
  It records each exact-reduction stage and concrete materialization without
  entering equality, hashing, ranking, or generation decisions.
- **3 — canonical sold ledger:** complete. A sold continuation is now an exact
  current `(remaining food, maximum gold, trace provenance)` envelope. It no
  longer replays the original Buy history for every later action. Weighted food
  retains exact-food rows; weightless food uses cross-food Pareto dominance.
- **4 — prune while generating:** complete. `FrontierAccumulator` receives
  candidates directly from all concrete, open-Buy, and sold-ledger expansion
  paths. Exact duplicate/dominance reduction happens on insertion. Lossy beam
  trimming remains a separate operation at the configured temporary ceiling
  and layer boundary.
- **5 — batch exact Sell:** complete for the current symbolic representation.
  A deferred Buy collects all exact food/origin Sell queries, groups compatible
  capacity rows, requests the maximum required exact cargo frontier once, and
  shares it across the group. The purchase handler's exact market/capacity
  cache shares those rows across symbolic states.
- **6 — generalized purchase dominance:** complete. Coverage maps a weaker
  origin's exact remaining-food and spend domain into a stronger origin and
  verifies the rounded gold function pointwise. The accumulator partitions by
  future context/capability before comparisons and reports partition
  rejections, coverage checks, and successful cross-shape removals separately.
- **7 — proof-search foundation:** complete, with a deliberately conservative
  first bound. `--proof-search` runs beam-free best-ordered DFS with a bounded
  exact transposition table and keeps the best three finished routes.
  `--proof-transposition-limit` bounds the table. Completion sets
  `proof_top_three_certified` only after the DFS stack is exhausted. The
  admissible bound is currently the full `int` score ceiling; it is safe but
  intentionally weak, so tightening it is the main remaining proof-performance
  job rather than a correctness prerequisite.

Validation results:

- The complete native test suite passes through `scripts/run_tests_safe.sh`.
- The exact proof search matches the existing brute-force oracle on a
  two-action small horizon, uses no beam pruning, and stays within its bounded
  transposition table.
- A two-second Normalia proof smoke test popped 56,163 states, limited the DFS
  stack to 64 states and the table to 5,000 states, stopped cleanly without a
  false certificate, and peaked at 135 MB RSS.
- Normalia beam 500 still produces 37,527. Its exact Sell calls fell from the
  pre-roadmap roughly 462,000 calls to 56,637 in the post-cutover run; peak RSS
  remained 90 MB.
- Post-cutover beam-500 spot checks produced Fang-Mo 37,569, Cornecopea 43,881,
  and Gayawaku 50,039 at 94 MB, 93 MB, and 89 MB respectively. Fang-Mo and
  Cornecopea now recover their verified scores at beam 500.
- Normalia's 18-action known route survives through the exact-quantity audit
  to a replayed finish, and enabling the audit leaves processed/generated state
  counts unchanged.

No `Best.txt` score changed in this implementation pass: every recovered score
was already represented by a replay-verified route.

## Current campaign

Step 1's beam-500 rebaseline completed on 2026-09-04. The reproducible runner
is `scripts/run_all_town_rebaseline.sh`; its raw result is
`benchmarks/results/deferred-dominance-rebaseline-b500-20260904.jsonl`.

Exact configuration:

- Open start (`--no-incumbent`).
- One pass (`--no-auto-refine`).
- Strategic purchase profile and the one-cargo-Buy-until-Sell rule.
- Beam 500 with 5% continuation lanes.
- Sequential processes through `scripts/run_safe.sh`.
- 768 MB internal budget, 1,024 MB hard cap, and 120-second per-town limit.

| ID | Town | Open search | Verified best | Gap | Processed | Generated | Seconds | Peak MB |
|---:|---|---:|---:|---:|---:|---:|---:|---:|
| 0 | Normalia | 37,527 | 37,527 | 0 | 7,646 | 29,932 | 1.585 | 86 |
| 1 | Fang-Mo | 37,527 | 37,569 | -42 | 7,709 | 31,376 | 1.581 | 98 |
| 2 | L'Exquise | 56,817 | 56,817 | 0 | 7,728 | 35,595 | 1.469 | 62 |
| 3 | Mahaji | 37,559 | 37,589 | -30 | 7,670 | 31,169 | 1.499 | 85 |
| 4 | Tocaccialli | 47,621 | 49,634 | -2,013 | 8,146 | 31,176 | 1.803 | 90 |
| 5 | Bajaar | 56,147 | 56,147 | 0 | 7,882 | 31,448 | 2.345 | 96 |
| 6 | Doba Lao | 45,543 | 45,543 | 0 | 7,646 | 28,403 | 1.089 | 58 |
| 7 | Huangoz | 37,478 | 37,527 | -49 | 7,654 | 30,606 | 1.541 | 92 |
| 8 | Port-Willam | 42,200 | 42,296 | -96 | 7,646 | 32,252 | 1.756 | 97 |
| 9 | Moonin | 37,527 | 37,527 | 0 | 9,620 | 39,210 | 1.753 | 87 |
| 10 | Cornecopea | 43,797 | 43,881 | -84 | 7,705 | 31,187 | 1.473 | 89 |
| 11 | Eduming | 81,321 | 81,321 | 0 | 7,646 | 24,995 | 0.879 | 56 |
| 12 | Octoyashi | 40,056 | 40,056 | 0 | 7,708 | 30,627 | 1.567 | 88 |
| 13 | Kifuai | 37,500 | 37,547 | -47 | 7,669 | 30,574 | 1.535 | 95 |
| 14 | Skjollird | 43,531 | 47,065 | -3,534 | 7,646 | 32,185 | 2.149 | 92 |
| 15 | Petrinov | 45,578 | 45,578 | 0 | 7,721 | 29,336 | 1.565 | 104 |
| 16 | Jilliqo | 37,789 | 37,819 | -30 | 7,677 | 31,416 | 1.731 | 97 |
| 17 | Gayawaku | 49,628 | 50,105 | -477 | 7,654 | 33,499 | 2.978 | 92 |
| 18 | Vilparino | 37,527 | 37,569 | -42 | 7,646 | 30,207 | 1.508 | 86 |
| 19 | Sanctifan | 55,445 | 55,728 | -283 | 8,843 | 37,443 | 1.744 | 89 |
| 20 | Mehaz | 48,738 | 48,738 | 0 | 7,736 | 34,444 | 1.981 | 92 |
| 21 | Ipartus | 37,734 | 37,734 | 0 | 7,646 | 30,015 | 1.737 | 91 |

Totals and conclusions:

- The open search matched 10 of 22 replay-verified scores and improved none.
- Search score total: 994,590. Verified-best total: 1,001,317. Gap: 6,727.
- It processed 172,644 states and generated 697,095 states in 37.268 solver-seconds.
- Maximum observed RSS was 104 MB; all searches completed far below both limits.
- `Best.txt` and the verified route fixtures require no changes from this pass.

Targeted continuation of step 1:

1. Beam 2,000 on the 12 misses: Fang-Mo, Mahaji, Tocaccialli, Huangoz,
   Port-Willam, Cornecopea, Kifuai, Skjollird, Jilliqo, Gayawaku, Vilparino,
   and Sanctifan.
2. Beam 5,000 only for survivors, prioritizing Skjollird (-3,534),
   Tocaccialli (-2,013), Gayawaku (-477), and Sanctifan (-283).
3. Treat a wider-beam recovery as evidence about lossy ranking, not a new
   record, unless it exceeds the current replay-verified score.

### Targeted beam-2,000 pass

The 12 beam-500 misses were rerun from open starts at beam 2,000 on 2026-09-04.
All other settings and safety limits were unchanged. The raw result is
`benchmarks/results/deferred-dominance-targeted-b2000-20260904.jsonl`.

| ID | Town | Beam 500 | Beam 2,000 | Gain | Verified best | Gap | Seconds | Peak MB |
|---:|---|---:|---:|---:|---:|---:|---:|---:|
| 1 | Fang-Mo | 37,527 | 37,569 | +42 | 37,569 | 0 | 4.420 | 103 |
| 3 | Mahaji | 37,559 | 37,589 | +30 | 37,589 | 0 | 3.855 | 95 |
| 4 | Tocaccialli | 47,621 | 49,042 | +1,421 | 49,634 | -592 | 5.359 | 116 |
| 7 | Huangoz | 37,478 | 37,527 | +49 | 37,527 | 0 | 4.418 | 101 |
| 8 | Port-Willam | 42,200 | 42,296 | +96 | 42,296 | 0 | 4.660 | 106 |
| 10 | Cornecopea | 43,797 | 43,881 | +84 | 43,881 | 0 | 3.600 | 105 |
| 13 | Kifuai | 37,500 | 37,547 | +47 | 37,547 | 0 | 4.073 | 104 |
| 14 | Skjollird | 43,531 | 47,065 | +3,534 | 47,065 | 0 | 5.715 | 109 |
| 16 | Jilliqo | 37,789 | 37,819 | +30 | 37,819 | 0 | 4.594 | 106 |
| 17 | Gayawaku | 49,628 | 50,039 | +411 | 50,105 | -66 | 9.044 | 121 |
| 18 | Vilparino | 37,527 | 37,569 | +42 | 37,569 | 0 | 3.891 | 102 |
| 19 | Sanctifan | 55,445 | 55,602 | +157 | 55,728 | -126 | 4.836 | 104 |

This pass recovered 9 of the 12 verified scores, gaining 5,943 points over the
beam-500 results. It processed 355,841 states and generated 1,364,907 states in
58.465 solver-seconds. Peak RSS was 121 MB. No score exceeded a verified best,
so no route promotion was required.

The beam-5,000 follow-up is now only Tocaccialli (-592), Gayawaku (-66), and
Sanctifan (-126). Gayawaku is already known to recover 50,105 at beam 5,000, so
the remaining diagnostic uncertainty is concentrated in Tocaccialli and
Sanctifan.

### Targeted beam-5,000 pass

The final three misses were rerun from open starts at beam 5,000 on 2026-09-04.
All other settings and safety limits remained unchanged. The raw result is
`benchmarks/results/deferred-dominance-targeted-b5000-20260904.jsonl`.

| ID | Town | Beam 2,000 | Beam 5,000 | Gain | Verified best | Gap | Seconds | Peak MB |
|---:|---|---:|---:|---:|---:|---:|---:|---:|
| 4 | Tocaccialli | 49,042 | 49,634 | +592 | 49,634 | 0 | 11.988 | 142 |
| 17 | Gayawaku | 50,039 | 50,105 | +66 | 50,105 | 0 | 20.670 | 189 |
| 19 | Sanctifan | 55,602 | 55,728 | +126 | 55,728 | 0 | 11.137 | 143 |

The pass gained the remaining 784 points, processed 215,497 states, and
generated 820,599 states in 43.795 solver-seconds. Peak RSS was 189 MB. No score
exceeded its verified best, so no route promotion was required.

The tiered rebaseline is now complete: beam 500 recovered 10 towns, beam 2,000
recovered another 9, and beam 5,000 recovered the final 3. Therefore the open
search independently reaches all 22 current replay-verified scores while
remaining far below the 1,024 MB hard cap.
