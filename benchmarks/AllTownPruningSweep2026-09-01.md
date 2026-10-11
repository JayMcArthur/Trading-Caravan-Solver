# All-town pruning sweep — 2026-09-01

## Goal

Run every normal-mode town under several bounded pruning policies without
assuming that Trader is the best first action. The 56,146 Bajaar score from
dr3xis is retained as an unverified reference, but it is not treated as a
verified target because its route and no-ad-continue provenance are unknown.

## Method

The reproducible runner is `scripts/run_all_town_pruning_experiments.sh`. It
runs towns sequentially, gives each solver process the full configured memory
allowance, uses the safe native wrapper, records every result immediately, and
resumes without duplicating recorded town/policy pairs.

The common configuration was:

- all 22 towns;
- beam 5,000;
- greedy purchase generation;
- open day one (no forced Trader);
- no seeded incumbent;
- 180-second per-process timeout;
- 640 MB internal memory budget under a 768 MB hard process cap.

The five policies changed pruning/ranking only:

| Policy | Configuration |
|---|---|
| Balanced | 25% optimistic / 75% cycle-aware |
| Exploit | 0% optimistic / 100% cycle-aware |
| Optimistic | 50% optimistic / 50% cycle-aware |
| Sell-aware | Balanced split plus experimental Sell-aware beam scoring |
| Diversity | Balanced split plus 25% structural-diversity reservation |

Gayawaku safely reached the 640 MB internal estimate before producing a
terminal state in every lane. It was rerun alone, still sequentially, with a
900 MB internal budget under the repository's maximum permitted 1,024 MB hard
cap. All five reruns completed. Actual peak RSS was 362 MB.

## Scores

The verified column excludes scores labeled probable-cheat and uses 49,827 for
Bajaar. No sweep result exceeded a verified score.

| Town | Verified | Balanced | Exploit | Optimistic | Sell-aware | Diversity | Sweep best | Delta |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Normalia | 35,114 | 35,114 | 35,114 | 35,114 | 35,114 | 35,114 | 35,114 | 0 |
| Fang-Mo | 35,163 | 35,163 | 35,163 | 35,163 | 34,918 | 35,163 | 35,163 | 0 |
| L'Exquise | 56,817 | 56,817 | 56,817 | 56,817 | 56,817 | 56,817 | 56,817 | 0 |
| Mahaji | 35,211 | 35,211 | 35,211 | 35,211 | 35,211 | 35,211 | 35,211 | 0 |
| Tocaccialli | 49,634 | 49,634 | 46,083 | 49,634 | 49,634 | 49,634 | 49,634 | 0 |
| Bajaar | 49,827 | 45,835 | 45,835 | 45,835 | 45,835 | 49,827 | 49,827 | 0 |
| Doba Lao | 44,631 | 44,631 | 44,631 | 44,631 | 44,631 | 44,631 | 44,631 | 0 |
| Huangoz | 35,114 | 35,114 | 35,114 | 35,114 | 35,114 | 35,114 | 35,114 | 0 |
| Port-Willam | 42,296 | 38,300 | 38,258 | 38,300 | 38,300 | 38,300 | 38,300 | -3,996 |
| Moonin | 37,527 | 37,527 | 37,527 | 37,527 | 37,527 | 37,527 | 37,527 | 0 |
| Cornecopea | 43,713 | 43,713 | 39,365 | 43,713 | 43,713 | 43,713 | 43,713 | 0 |
| Eduming | 80,043 | 80,043 | 80,043 | 80,043 | 80,043 | 80,043 | 80,043 | 0 |
| Octoyashi | 40,056 | 40,056 | 40,056 | 40,056 | 40,056 | 40,056 | 40,056 | 0 |
| Kifuai | 36,589 | 36,589 | 36,589 | 36,589 | 36,589 | 36,589 | 36,589 | 0 |
| Skjollird | 43,475 | 43,475 | 43,475 | 43,475 | 43,475 | 43,475 | 43,475 | 0 |
| Petrinov | 45,578 | 41,672 | 41,474 | 41,672 | 41,672 | 41,672 | 41,672 | -3,906 |
| Jilliqo | 37,759 | 37,759 | 37,759 | 37,759 | 37,759 | 37,759 | 37,759 | 0 |
| Gayawaku | 50,105 | 46,325 | 46,325 | 46,283 | 46,325 | 46,325 | 46,325 | -3,780 |
| Vilparino | 36,631 | 36,631 | 36,631 | 36,631 | 36,631 | 36,631 | 36,631 | 0 |
| Sanctifan | 51,130 | 51,067 | 47,979 | 51,067 | 51,067 | 51,067 | 51,067 | -63 |
| Mehaz | 48,738 | 46,468 | 46,468 | 46,468 | 46,468 | 48,738 | 48,738 | 0 |
| Ipartus | 36,295 | 36,295 | 36,295 | 36,295 | 36,295 | 36,295 | 36,295 | 0 |

The best of the five policies matched 18 of 22 verified records. The four
misses were Port-Willam, Petrinov, Gayawaku, and Sanctifan. The main matrix
processed 6,568,875 states and generated 23,135,506 in 582.785 solver-seconds.
The dedicated Gayawaku completions added 295,284 processed and 1,130,745
generated states in 395.815 solver-seconds.

## What the comparison says

- Structural diversity is the best general alternate lane. It was the only
  open-start beam-5,000 policy to retain the verified Bajaar 49,827 and Mehaz
  48,738 paths.
- Pure exploit pruning was faster, but lost Tocaccialli, Cornecopea,
  Sanctifan, and several smaller scores. It is too aggressive as a default.
- A 50% optimistic share produced no improvement over 25% and was slightly
  worse on Gayawaku.
- Sell-aware scoring did not improve any town and regressed Fang-Mo. It should
  remain optional.
- The four misses are not primarily a wider-beam problem. Three verified paths
  came from isolated late Merchant/Witch schedules; Sanctifan came from an
  exact suffix guided by a strong prefix. The generic beam prunes those timing
  structures before their payoff.

## Recommended next fork

Keep balanced and structural-diversity lanes, then deepen the diversity key so
it explicitly reserves encounter-timing structures: Merchant unused/used,
Witch unused/used, their selected day buckets, and days remaining until the
next event opportunity. That targets Port-Willam, Petrinov, and Gayawaku
without forcing one known schedule. Sanctifan should use a separate
prefix-to-exact-suffix lane because its remaining gap is only 63 points.

Raw results:

- `benchmarks/results/all-town-pruning-b5000-20260901.jsonl`
- `benchmarks/results/gayawaku-pruning-b5000-20260901.jsonl`

## Follow-up result

The recommendation above led to a correctness finding rather than another
pruning heuristic: unrestricted search was forcing Merchant and Witch on their
first eligible days. General encounter deferral plus an automatically learned
early-action refinement pass now matches all four misses without fixed days,
choices, or route prefixes. The implementation, measurements, and safe
reproduction commands are in
[`UnrestrictedMissRecovery2026-09-01.md`](UnrestrictedMissRecovery2026-09-01.md).
