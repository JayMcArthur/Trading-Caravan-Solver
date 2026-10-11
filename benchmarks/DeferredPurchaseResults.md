# Deferred-purchase bounded results

These are unseeded classical-mode runs made after the deferred Buy cutover.
Every reported finalist was replayed automatically through `Game` before the
solver retained it. They are bounded-beam discoveries, not proofs of global
optimality.

Unless noted otherwise, runs used the `strategic` purchase profile, a
500-state beam, and the default 25% optimistic / 75% cycle-aware split.

| Town | Score | Seconds | Peak RSS | Result |
| --- | ---: | ---: | ---: | --- |
| Normalia | 35,114 | 47.8 | 108 MB | Existing bot record rediscovered at beam 1,000 |
| Mahaji | **35,211** | 25.5 | 91 MB | New record; fixture saved |
| Tocaccialli | 46,083 | 23.8 | 56 MB | Human record tied; fixture saved |
| Bajaar | 45,667 | 17.5 | 39 MB | Large bot-baseline improvement, below humans |
| Doba Lao | 44,631 | 10.1 | 124 MB | Existing bot record rediscovered |
| Huangoz | 34,848 | 10.6 | 86 MB | Human record tied in this run |
| Moonin | **37,527** | 13.6 | 46 MB | New record; fixture saved |
| Cornecopea | 37,475 | 15.5 | 49 MB | Below saved 43,696 record |
| L'Exquise | 55,615 | 13.6 | 58 MB | Below saved 56,817 record |
| Eduming | 80,043 | 3.5 | 40 MB | Existing bot record rediscovered |
| Octoyashi | 40,056 | 16.2 | 42 MB | Existing bot record rediscovered |
| Kifuai | 34,827 | 11.2 | 43 MB | Below human record |
| Jilliqo | 37,291 | 16.2 | 54 MB | Existing bot record rediscovered |
| Vilparino | 36,631 | 12.4 | 44 MB | Human record tied; fixture saved |
| Mehaz | **46,432** | 20.8 | 50 MB | New record; fixture saved |
| Sanctifan | 36,652 | 19.7 | 65 MB | Below human record |
| Ipartus | 36,295 | 28.0 | 104 MB | Existing bot record rediscovered |

Some elapsed values include one-time exact-sale tree construction. The
Cornecopea run was also the unminimized verification for the streaming Sell
fix: before the fix, one call materialized over one million rows and hit the
1 GB hard limit; afterward the same beam completed at 49 MB peak RSS.

Before the follow-up below, Gayawaku was the main residual performance case.
Its weightless food made one cargo-weight frontier reusable for every food
amount, but evaluating the nonlinear gold/cargo value envelope for up to 256
exact food quantities still exceeded 60 seconds at beam 100.

## Follow-up exact-frontier acceleration

The remaining cache and final-day hot paths were removed without approximate
pruning:

- exact Sell rows now reuse immutable 256-gold ceiling buckets and are filtered
  against the true symbolic spend;
- only the winning cargo choice for each exact remaining-food value constructs
  a player and trace; and
- final-day Buy resolution considers only minimum required food, because extra
  food has no final score and can only spend gold or weight.

The deterministic Skjollird beam-500 run kept the same 43,475 score and all
generated-state counts while dropping from 40.5 seconds to 5.6 seconds. A
Gayawaku beam-100 run that previously timed out completed in 8.1 seconds at
42 MB, and beam 500 reached 46,283 in 31.2 seconds at 52 MB. A later
Merchant-isolated open-start beam-1,000 run improved Gayawaku to 46,325, and
the fixed Merchant-day-7/Witch-day-9 beam-500 scenario reached 50,105.
The active
town-by-town campaign is tracked in `RecordCampaign.md`.

## Measured branch reduction

The clearest like-for-like comparison is the saved pre-cutover explicit
Normalia beam-5,000 sell-aware run versus the current deferred run with the
same fixed Trader prefix, beam, split, and scorer:

| Metric | Explicit allocation branches | Deferred purchase | Change |
| --- | ---: | ---: | ---: |
| Generated Buy states | 337,392 | 33,414 | **-90.1%** |
| All generated states | 454,096 | 175,268 | **-61.4%** |
| Processed action states | 46,124 | 50,660 | +9.8% |
| Score | 35,049 | 35,114 | +65 |
| Elapsed | 10.46 s | 12.60 s | +20.5% |
| Peak RSS | 88 MB | 93 MB | +5 MB |

At beam 1,000, the older post-Wait-removal Normalia profile generated 25,461
Buy states and 41,609 total states. The current fixed-prefix run generated
7,784 Buy states (**-69.4%**) and 39,513 total states (**-5.0%**), while
processing 11,567 action states instead of 6,560 (**+76.3%**). This is the
intended effect: the bucket removes allocation branches, then the fixed-width
beam spends the recovered slots exploring more distinct actions. It is not yet
a universal wall-time reduction because exact Sell joins still evaluate the
allocations stored inside each symbolic state.

A focused Gayawaku suffix diagnosis separated cached lookup time from the rest
of Sell resolution. Before terminal pruning, 145 processed states produced
17,453 finalist checks and 75,784 cumulative deferred rows in 2.59 seconds;
the exact-sale cache calls themselves consumed only 176 ms. Restricting a
final-day Sell to minimum required food and retaining only the best three rows
from each terminal resource action reduced finalist checks to 2,988 and runtime
to roughly 2.08 seconds without changing the 50,105 result. The remaining hot
path is the exact iteration and transformation of large internal food/cargo
frontiers, especially for weightless-food Gayawaku.

## Lazy sold-bucket prototype

The throwaway
[`sold-bucket-options-prototype.html`](prototypes/sold-bucket-options-prototype.html)
tests whether Sell can freeze its sale market and clear the cargo lock without
immediately expanding every possible historical food quantity. The resulting
symbolic phase retains an exact `historical food -> current gold` relation plus
the winning cargo witness used to patch the old Buy.

The prototype compares four useful points:

- the current eager full food scan;
- only food thresholds justified by distinct continuations;
- a lazy bucket queried at four food requirements observed along one route; and
- a hybrid that also computes one exact minimum-food representative for beam
  ranking.

Candidate checks are deterministic and more useful than the prototype's
JavaScript wall time. One run produced:

| Modeled workload | Eager checks | Threshold checks | Lazy route checks | Hybrid checks |
| --- | ---: | ---: | ---: | ---: |
| Normal weighted food | 3,995 | 1,105 (-72.3%) | 340 (-91.5%) | 425 (-89.4%) |
| Gayawaku weightless food | 26,703 | 1,677 (-93.7%) | 516 (-98.1%) | 645 (-97.6%) |
| Four origin rows | 81,812 | 5,876 (-92.8%) | 1,808 (-97.8%) | 2,260 (-97.2%) |

A deterministic 120-case differential sweep covered weighted, weightless, and
free food; one to four origin rows; fixed costs; additive income; and Interest
rates above and below `1.00`. Materializing every lazy answer matched the eager
frontier in 120/120 cases. Every requested continuation-threshold answer also
matched its eager-oracle row in 120/120 cases. This validates the prototype's
local value-function transformations, not yet the complete production search.

Production opportunity counters support doing this work before a full C++
cutover. On the Gayawaku incumbent suffix at beam 50, the solver eagerly built
39 Sell results but expanded only 24 Sell-root states; 15 complete joins
(38.5%) were discarded by the beam before their first action. Beam 100 built 71
and expanded 44, again leaving 38.0% unused. A Normalia suffix with no beam
pruning built and expanded all 17 Sell roots, showing why threshold/lazy query
reduction is still required in addition to deferring work until after the beam.

The recommended production experiment is the hybrid: freeze the Sell snapshot,
compute one exact minimum-food representative for stable beam ranking, and keep
the rest as a lazy sold bucket. Nights, costs, income, and Interest transform
that bucket; the next Buy asks it for complete continuation thresholds. The
hard correctness seam is the next-Buy join, especially when sub-`1.00` Interest
makes earlier food spending better than buying the same food later.
