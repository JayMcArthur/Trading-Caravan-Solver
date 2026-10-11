# Early-action skeleton search — 2026-09-01

All native runs used `scripts/run_safe.sh` with a 768 MB external hard cap.
These are replay-verified bounded searches, not optimality proofs.

## Model

`EarlyActionSkeleton` packs the first 1–8 player-controlled non-event actions.
It distinguishes Buy, Sell, Trader, Camel, Interest, and Wait, but deliberately
ignores Buy quantities. Merchant, Witch, and Disaster choices are added to the
beam-only family key without changing game-state equality, hashing, dominance,
or deferred-state semantic equality.

`--skeleton-share N` reserves part of each trimmed beam for the best state in
each family. `--skeleton-depth D` selects the prefix length. Hybrid suffixes can
use `--hybrid-skeleton-order
score|cohort|interleave|structure-interleave`:

- `score`: existing best-first suffix order;
- `cohort`: similar families share a batch;
- `interleave`: each batch receives strong representatives from many families.
- `structure-interleave`: use current crew, Interest, cargo phase, and encounter
  structure instead of the early-action key. This later follow-up is measured
  in `BajaarTargetAnalysis.md`.

Deferred Buy and Sell states extend the skeleton with their delayed trace
recipe, so a route has the same family before and after materialization.

## Bajaar results

| Broad beam | Hybrid split/batch | Depth | Share | Order | Best | Peak RSS |
| ---: | --- | ---: | ---: | --- | ---: | ---: |
| 5,000 | none | 8 | 0/10/25/50% | score | 45,835 | 77–87 MB |
| 5,000 | day 9 / 500 | 8 | 10% | score | **49,827** | 69 MB |
| 5,000 | day 9 / 500 | 8 | 25% | score | **49,827** | 70 MB |
| 5,000 | day 9 / 500 | 8 | 50/75% | score | 45,835 | 68–69 MB |
| 20,000 | day 8 / 500 | 8 | 10% | score | **49,827** | 101 MB |
| 20,000 | day 8 / 500 | 8 | 25% | score | **49,827** | 104 MB |
| 50,000 | day 8 / 500 | 8 | 10% | score | **49,827** | 193 MB |
| 50,000 | day 8 / 500 | 8 | 25% | score | **49,827** | 193 MB |
| 5,000 | day 9 / 500 | 8 | 0/10/25% | cohort | 45,835 | 70–90 MB |
| 5,000 | day 9 / 500 | 8 | 0/10/25% | interleave | **49,827** | 73–89 MB |
| 20,000 | day 8 / 500 | 8 | 10/25% | interleave | **49,827** | 120–145 MB |
| 5,000 | day 9 / 500 | 4 | 10/25% | interleave | 45,835 | 86–114 MB |
| 5,000 | day 9 / 500 | 6 | 10/25% | interleave | 45,835 | 92–111 MB |

The width-50,000 runs observed up to 8,370 action-only groups before the
event-aware refinement. A 25% reservation was large enough to retain a best
representative for every observed group and still did not improve 49,827.
Interleaving increased terminal coverage and preserved the record; cohorting
lost it. Depths 4 and 6 grouped routes more broadly but also lost the record.

## Conclusion

The missing 56,146 route is unlikely to be explained solely by ordinary beam
crowding among early action orders. The next investigation should decompose the
target score under the exact Bajaar scoring equation and determine whether it
requires a final resource mix or rule interaction absent from the current
model. `Best.txt` is unchanged because no new score was found.
