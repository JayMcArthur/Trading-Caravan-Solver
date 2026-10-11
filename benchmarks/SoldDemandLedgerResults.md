# Sold demand-ledger production results

## Production model

Normal-mode search now has two deferred allocation phases:

1. `DeferredPurchaseModel` keeps the Buy unresolved while days, crew costs,
   income, Interest, Merchant, and Witch effects accumulate.
2. Sell changes the state to `DeferredSoldModel`. It records a maximum-prefix
   historical-food requirement and the exact actions after Sell without
   expanding every `(food, gold)` result.

The complete sold curve is materialized only when a later Buy needs it. Opening
that Buy then removes incoming origins that cannot win for any feasible total
food amount. For food price `p`, origins are compared by `gold + p * old_food`
over the interval of total-food amounts each origin can reach. The winning
provenance is retained so route replay can patch both concrete Buys.

At game end, food has no point value. The solver therefore asks for only the
best three minimum-feasible-food results instead of materializing the complete
curve. It checks the first three feasible food quantities per origin, which is
enough for an exact global top three because additional food can only shrink
the cargo set and reduce gold through the monotone suffix operations.

The superseded `DeferredResourceFrontier` implementation has been deleted.
There is no runtime switch or fallback between eager and ledger behavior.

## Controlled eager-versus-ledger comparison

These are single, same-build, same-command A/B samples. The eager control was
temporarily restored only for measurement and was removed afterward.

| Case | Eager time | Ledger time | Time change | Eager deferred rows | Ledger deferred rows | Row change | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Normalia, open start, beam 50 | 0.350 s | 0.228 s | **-34.9%** | 17,355 | 9,499 | **-45.3%** | 33,617 |
| Gayawaku, open start, beam 50 | 2.096 s | 1.391 s | **-33.6%** | 205,307 | 80,804 | **-60.6%** | 45,675 |
| Gayawaku 50,105 suffix, beam 50 | 1.978 s | 0.940 s | **-52.5%** | 62,109 | 31,734 | **-48.9%** | 50,105 |

The score and winning route were identical between both implementations in all
three comparisons. Exact-sale calls on the Gayawaku suffix fell from 9,332 to
7,241. Normalia and the full Gayawaku run made more cache-hit calls, but still
finished faster because terminal branches no longer copied and transformed the
entire food curve.

## Next-Buy origin compaction

New JSON counters report the number of incoming sold-curve seeds and the number
of origins retained by the exact feasible-range envelope:

| Case | Incoming seeds | Origins retained | Reduction |
| --- | ---: | ---: | ---: |
| Normalia, beam 500 | 126,602 | 55,472 | **56.2%** |
| Gayawaku, beam 500 | 670,975 | 572,177 | **14.7%** |
| Gayawaku 50,105 suffix, beam 50 | 25,823 | 25,479 | **1.3%** |

This separates two effects. The ledger itself produces the large post-Sell row
reduction in Gayawaku, while next-Buy compaction contributes much more in
Normalia. The record suffix is already highly refined, so almost every incoming
origin wins at some total-food boundary.

## All-town beam-500 screen

Command shape: greedy purchase profile, 25% optimistic beam share, fixed
starting Trader, no incumbent, 500-state beam, 768 MB external cap, and 640 MB
internal budget. Every town completed; peak RSS was 29-59 MB. All except
Gayawaku finished in under one second. No score exceeded `Best.txt`, so the
record file was not changed.

| Town | Score | Seconds | Peak RSS MB |
| --- | ---: | ---: | ---: |
| Normalia | 34,848 | 0.661 | 40 |
| Fang-Mo | 34,918 | 0.675 | 50 |
| L'Exquise | 56,817 | 0.961 | 38 |
| Mahaji | 35,211 | 0.603 | 29 |
| Tocaccialli | 46,083 | 0.747 | 51 |
| Bajaar | 45,667 | 0.679 | 37 |
| Doba Lao | 44,631 | 0.626 | 44 |
| Huangoz | 34,848 | 0.632 | 47 |
| Port-Willam | 38,300 | 0.709 | 39 |
| Moonin | 34,806 | 0.858 | 41 |
| Cornecopea | 43,545 | 0.650 | 41 |
| Eduming | 80,043 | 0.540 | 39 |
| Octoyashi | 40,056 | 0.706 | 49 |
| Kifuai | 34,827 | 0.632 | 40 |
| Skjollird | 43,475 | 0.940 | 40 |
| Petrinov | 41,672 | 0.699 | 50 |
| Jilliqo | 37,291 | 0.793 | 40 |
| Gayawaku | 46,283 | 10.123 | 59 |
| Vilparino | 36,631 | 0.652 | 40 |
| Sanctifan | 36,654 | 0.746 | 40 |
| Mehaz | 46,432 | 0.859 | 41 |
| Ipartus | 36,295 | 0.731 | 53 |

The full safe regression suite also passes, including exhaustive deferred
purchase oracles, exact Sell frontier tests, recorded route replay, disaster
tests, and town record floors.

## Threshold memo and direct exact-Sell cache

A later Bajaar hybrid profile exposed 42.6 million exact-Sell queries in a
405,043-state run. `DeferredSoldModel` now shares the best post-Sell gold/food
representative for each requested food threshold across descendants of the
same Sell. Exact concrete routes still come from the symbolic source when a
state materializes, so trace provenance is not cached or approximated.

A 256-slot direct cache also fronts the bounded 4,096-entry exact-Sell map. It
served 41.7 million of 42.2 million cache hits in the controlled run. Together,
the final ownership-safe changes reduced the same Bajaar campaign from 60.5 to
57.0 seconds with identical processed/generated counts, score, and route at
177 MB peak RSS.
An 8,192-entry retained cache was tested and rejected because it increased both
runtime and memory despite reducing misses. Full measurements are in
`BajaarTargetAnalysis.md`.

The deep structural campaign exposed a cache-eviction lifetime bug: callers
could retain raw frontier pointers while a later query cleared the bounded map.
Frontiers are now shared immutable rows. The cache still retains at most 4,096
entries, while an active resolution safely owns any row it is currently using.
The formerly crashing 1.69-million-state workload completed at 180 MB RSS.
