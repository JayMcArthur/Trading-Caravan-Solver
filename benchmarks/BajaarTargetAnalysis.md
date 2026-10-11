# Bajaar target analysis — 2026-09-01

All native runs used `scripts/run_safe.sh` with a 768 MB external hard cap.
Search results are replay-verified bounded discoveries, not optimality proofs.

## Exact faction and Witch interaction

The recovered game describes Bajaar as “Traders income is doubled.” The ARM64
implementation in `GlobalVariables.FactionImpact()` reads the `traderincome`
field at object offset `0x88`, doubles it with `UBFM W8, W8, 0x1F, 0x1E`, and
stores it back at `0x88`. This faction method runs during initial setup.

Midas is a later Witch event and executes `traderincome += 150`. The exact
normal-mode sequence is therefore:

```text
base 20 -> Bajaar doubles to 40 -> Midas adds 150 -> final rate 190
```

It is not `(20 + 150) * 2 = 340`. The solver's additive `+20` faction setup is
equivalent to the recovered doubling at the only point where the faction code
runs, so no rule correction is required.

## Current 49,827 route decomposition

The saved route ends with six Traders, three Camels, no cargo, and no statue.
Its exact final score is:

```text
49,587 gold
   150 Trader points  (6 * 25)
    90 Camel points   (3 * 30)
     0 cargo/statue
-------
49,827 total
```

The day-13 Sell leaves 22,310 gold. Two final Interest actions produce 34,605
and then 49,587 under the game's rounding. With the same final crew, a 56,146
score would require 55,906 final gold and exactly 26,622 gold before those two
Interest actions: a 4,312-gold pre-compounding improvement.

Focused searches fixing the known first four actions recovered 49,827 when the
suffix beam was allowed to regrow to 5,000. Fixing only the first two actions
also recovered 49,827 at suffix widths 500 and 5,000. Neither found an improved
continuation, so the missing margin is unlikely to be a small local edit after
the known opening.

## Deeper suffix coverage

| Order | Input batch | Suffix beam | Groups | Processed | Time | Peak RSS | Best |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| score | 500 | 5,000 | n/a | 405,043 | 57.0 s | 177 MB | **49,827** |
| structure-interleave | 100 | 5,000 | 434 | 1,689,482 | 325.4 s | 180 MB | **49,827** |

`structure-interleave` groups day-9 prefixes by crew, Interest rate,
cargo/deferred phase, and realized encounters before distributing them across
batches. Its 100-prefix campaign gives lower-ranked structures independent room
to regrow and visits more than four times as many states. It still found the
same route. This is stronger negative evidence against both generic suffix
breadth and coarse continuation-family crowding, but it is not an optimality
proof.

## Sell hot-path improvements

The 500/5,000 profile made 42.3 million exact-Sell requests for 405 thousand
expanded states. Two exact optimizations retain the same generated-state counts
and winning replay:

- `DeferredSoldModel` shares a compact per-family memo keyed by minimum
  remaining food. It stores only post-Sell gold/food plus one common template;
  concrete Buy/Sell actions and provenance are still resolved from the exact
  symbolic source when a state materializes.
- `BestPurchaseHandler` has a 256-slot direct hot cache in front of the bounded
  4,096-entry exact-Sell cache. In the controlled run, 41.7 million requests
  hit the direct cache.

The final ownership-safe comparable run fell from 60.5 seconds before these
changes to 57.0 seconds after them, with the same 405,043 processed states and
49,827 route. Doubling
the retained exact-Sell cache to 8,192 entries was rejected: it reduced misses
but increased runtime to 80.3 seconds and RSS to 192 MB.

The deep structural run also exposed a pre-existing eviction lifetime bug:
deferred resolution kept raw pointers to cache rows while later queries could
clear the bounded map. Exact-Sell queries now return shared immutable rows, so
active resolutions retain ownership across eviction. The original crashing
workload then completed all 1,689,482 states and 161.2 million Sell queries.

## Next search fork

Generic score order and continuation-structure order have both plateaued near
1.7 million processed states. The next useful fork should allocate suffix work
adaptively within a structure—for example, progressively deepen only families
whose terminal upper estimates remain competitive—rather than divide an equal
beam among every retained prefix. Any such bound must first be proved safe for
future hires and encounter mutations; the current optimistic beam score is a
ranking heuristic and must not be used as a correctness prune.
