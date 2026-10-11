# Continuation matrix and skeleton queue — 2026-09-02

## Result

Deferred Buy and sold-ledger owners now retain a complete continuation matrix
instead of collapsing all immediate actions into one score. The five columns
are immediate Sell, Trader, Camel, Interest, and food-only Buy. Each column has
a realistic score and optimistic bound. A winning ticket retains its symbolic
owner once; it does not prescribe the next action.

Five percent of the ordinary beam is reserved round-robin across matrix
columns. The remainder still competes globally by realistic and optimistic
score. `--continuation-lane-share 0-100` controls the reservation.

When the exact memory forecast becomes unsafe after the encounter window, the
solver now groups the retained frontier by exact early-action/world skeleton.
It completes one family at a time with the original full beam, retaining the
other families only as a compact waiting vector. Families never prune or
deduplicate one another. `--skeleton-queue-day D` can force an experimental
split; zero is the default automatic trigger, and `--no-skeleton-queue` selects
the legacy structure-batch fallback.

Queue ordering now consumes the same matrix rows as beam retention. It
aggregates every member's realistic, optimistic, and per-action scores into one
family owner. The queue head contains the strongest still-unique family for
each available ticket; the remaining families are ordered by the configured
realistic/optimistic/lane proportions. This changes ordering only, never legal
expansion.

## Lane-share calibration

All runs used beam 500, the strategic purchase profile, no incumbent, and no
automatic refinement.

| Town | Lane share | Score | Time | Peak RSS |
|---|---:|---:|---:|---:|
| Normalia | 0% | 37,478 | 3.075 s | 72 MB |
| Normalia | 5% | 37,478 | 3.014 s | 58 MB |
| Sanctifan | 0% | 55,445 | 3.517 s | 47 MB |
| Sanctifan | 1% | 55,445 | 3.644 s | 87 MB |
| Sanctifan | 2% | 55,445 | 3.567 s | 88 MB |
| Sanctifan | 5% | 55,445 | 3.572 s | 47 MB |
| Sanctifan | 10% | 45,553 | 3.514 s | 88 MB |
| Sanctifan | 20% | 45,553 | 3.548 s | 90 MB |
| Sanctifan | 30% | 45,553 | 3.543 s | 80 MB |

The cliff at 10% shows why continuation lanes must be protection rather than
the dominant ranking policy. Five percent is the default.

## Queue experiments

At a forced day-12 Normalia split, the queue formed 86 families from 494
states, completed all 86, processed 56,290 states in 16.527 seconds, peaked at
91 MB RSS, and reproduced 37,478. The ordinary beam processed 7,477 states in
3.021 seconds for the same score.

A deliberately early day-8 split formed 314 families. The original
representative-score order processed 71,001 states and completed 22 families
before the 30-second limit, at 110 MB peak RSS; its partial best was 31,827.
The matrix-aware order processed 68,869 states and completed 21 families at
111 MB while raising the partial best to 35,049. It put one unique family from
each of the seven available realistic, optimistic, and action tickets in the
queue head. This is an anytime-ordering improvement, but proactive early
splitting remains worse than allowing the ordinary beam to complete.

### The optimistic value is not a safe pruning bound

A small-horizon exhaustive oracle found a family completion above its reported
optimistic value. Deferred bucket representatives do not bound every latent
allocation. Production therefore reports checked families, violations, and
maximum overrun, but `skeleton_queue_ceiling_pruning` is always `false`. The
value may order work; it cannot remove work.

## Wider-beam stress screen

Normalia completed at beam 5,000 for lane shares 0%, 2%, and 5%. Every run found
the same replay-verified 37,527 route.

| Town/configuration | Score | Complete | Time | Peak RSS |
|---|---:|:---:|---:|---:|
| Normalia, 0% | 37,527 | yes | 216.474 s | 132 MB |
| Normalia, 2% | 37,527 | yes | 221.017 s | 135 MB |
| Normalia, 5% | 37,527 | yes | 224.264 s | 135 MB |
| Sanctifan, 5% | 55,602 | yes | 282.800 s | 141 MB |
| Mehaz, 5% | 48,738 | yes | 220.880 s | 139 MB |
| Gayawaku, 5% | 50,039 | yes | 241.227 s | 190 MB |

Sanctifan improved over its beam-500 result of 55,445 but remained below the
55,728 polished fixture. Gayawaku improved over 49,628 but remained below its
50,105 fixture. No saved record changed.

A representative Normalia beam-20,000 run used a 384 MB internal budget and a
15-minute deadline. It reached 46,820 processed states and 165,719 generated
states but timed out before the Witch window or any finished route. Peak RSS
was only 162 MB and the automatic queue never triggered.

A second 60-second beam-20,000 diagnostic isolated the broad-prefix cost:

| Timed region | Time | Share of wall time |
|---|---:|---:|
| Open deferred-Buy expansion | 37.723 s | 62.9% |
| Sold-ledger expansion | 21.882 s | 36.5% |
| Layer compression, including beam trim | 0.355 s | 0.6% |
| Beam trim | 0.265 s | 0.4% |
| Continuation-matrix row scoring | 0.223 s | 0.4% |
| Continuation-matrix selection | 0.031 s | 0.1% |

The nested figures overlap: beam trim is part of layer compression, and matrix
scoring/selection are part of beam trim. Open-purchase plus sold-ledger
expansion accounted for 59.605 seconds of the 60.012-second run. It processed
10,580 states, generated 44,987, and peaked at only 78 MB RSS. The next useful
optimization target is therefore the two deferred expansion paths, especially
open purchases—not matrix selection, queue scheduling, or retained-frontier
memory. Repeating beam 20,000 across all towns is not justified until those
paths are faster.

## Beam-500 all-town screen

Every town completed under the guarded native runner with the 5% matrix and
automatic queue. Automatic queueing did not trigger at this width, as intended.

| Town | Score | Town | Score |
|---|---:|---|---:|
| Normalia | 37,478 | Fang-Mo | 37,527 |
| L'Exquise | 56,817 | Mahaji | 37,533 |
| Tocaccialli | 47,621 | Bajaar | 56,147 |
| Doba Lao | 44,768 | Huangoz | 37,478 |
| Port-Willam | 42,200 | Moonin | 37,527 |
| Cornecopea | 38,336 | Eduming | 81,321 |
| Octoyashi | 40,007 | Kifuai | 37,500 |
| Skjollird | 43,085 | Petrinov | 45,578 |
| Jilliqo | 37,759 | Gayawaku | 49,628 |
| Vilparino | 37,527 | Sanctifan | 55,445 |
| Mehaz | 48,738 | Ipartus | 37,704 |

These are bounded search results, not global-optimum claims and not replacements
for higher replay-verified records.

## Reproduction

```bash
./scripts/run_tests_safe.sh skeleton-search

./scripts/run_safe.sh --town Normalia --no-incumbent --no-auto-refine \
  --array-limit 500 --memory-limit-mb 384 --time-limit-seconds 30 \
  --benchmark-json

./scripts/run_safe.sh --town Normalia --no-incumbent --no-auto-refine \
  --array-limit 500 --memory-limit-mb 384 --time-limit-seconds 30 \
  --skeleton-queue-day 12 --benchmark-json

ARRAY_LIMIT=5000 INTERNAL_MB=384 MAX_SECONDS=242 \
  ./scripts/run_safe.sh --town Normalia --no-incumbent --no-auto-refine \
  --continuation-lane-share 5 --benchmark-json

ARRAY_LIMIT=20000 INTERNAL_MB=384 MAX_SECONDS=62 \
  ./scripts/run_safe.sh --town Normalia --no-incumbent --no-auto-refine \
  --continuation-lane-share 5 --benchmark-json
```
