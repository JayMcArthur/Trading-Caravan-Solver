# Deferred Search Optimization Campaign — 2026-09-01

This campaign improves unrestricted normal-mode search without prescribing a
route, encounter day, encounter option, or concrete Buy allocation. Every
native measurement uses the bounded scripts documented in `AGENTS.md`.

## Invariants

- Normal mode keeps one symbolic state per player action branch.
- Allocation alternatives inside a symbolic state never become forced future
  actions.
- Continuation ranking may retain a state; normal expansion still generates
  every legal action from that state.
- The 1,024 MB external process cap remains authoritative.
- Exactness tests compare deferred results with exhaustive concrete oracles.
- A cargo Buy is not assumed adjacent to Sell; the verified Mehaz route is the
  regression counterexample.

## Baseline

Command shape:

```bash
MAX_MB=512 INTERNAL_MB=384 ARRAY_LIMIT=500 MAX_SECONDS=45 \
  ./scripts/run_safe.sh --town TOWN --no-incumbent --no-auto-refine \
  --benchmark-json
```

| Town | Score | Seconds | Peak RSS | Processed | Generated | Exact-Sell calls | Deferred rows |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Normalia | 37,478 | 0.831 | 33 MB | 7,481 | 29,564 | 384,612 | 129,080 |
| Mehaz | 45,931 | 0.977 | 22 MB | 7,535 | 33,522 | 526,838 | 134,374 |
| Gayawaku | 49,628 | 15.665 | 55 MB | 7,568 | 33,398 | 179,440 | 909,722 |

All three runs reported zero `purchase_food_range_total`,
`purchase_food_only_checks`, and legacy purchase queries. The remaining work is
therefore deferred allocation work, not `Helpers::run_buy_event`.

## Work order

1. Measure exact live bytes instead of multiplying by a historical maximum.
2. Prototype one-expansion grace, then absorb only the behavior not already
   provided more cheaply by continuation ranking views.
3. Add independent immediate-action ranking views owned by one stored symbolic
   state.
4. Replace materialize-then-compress food work with exact demand/capacity
   breakpoints or a symbolic range where every integer remains nondominated.
5. Share immutable allocation data while leaving action constraints and gold
   transformations branch-owned.
6. Add only proved Interest/Sell commutation dominance.
7. Turn the existing hybrid suffix mechanism into an automatic, actual-byte
   pressure schedule after Merchant/Witch windows.

Results and rejected alternatives will be appended as each stage is measured.

## Implemented results

### Continuation tickets and grace verdict

The grace prototype showed that expanding a new Buy before pruning protects a
hidden strong continuation, but temporarily creates one state per action. The
production implementation instead asks `DeferredSearchState` for legal
immediate-action views. Sell, Trader, Camel, Interest, and a currently legal
food-only Buy each compete as a ranking ticket. A winning ticket retains its
single symbolic owner; it does not select the owner's next action.

An interface regression asserts that creating views does not mutate the owner.
It also caught the consecutive-Buy rule: `FoodOnlyBuy` is absent immediately
after Buy and appears after an intervening action makes Buy legal again.
Tickets are suppressed when a mandatory Merchant or Witch encounter must run
before the next player action. This prevents an impossible direct-action score
from retaining an owner; event branches receive their own tickets after normal
same-day expansion.

At beam 500 this changed Mehaz from 45,931 to the verified 48,738 route,
including the non-adjacent `Buy → Camel → Merchant → Sell` cargo cycle. Normalia
remained 37,478.

The one-expansion grace runtime mode is therefore superseded: continuation
tickets provide its search coverage without storing five temporary states.
The throwaway HTML prototype was removed after this verdict was captured.

### Exact memory accounting and shared storage

Candidate memory no longer multiplies every state by the largest deferred
state ever observed. The estimator walks the live pending, same-day, next-day,
waiting, and current vectors; counts each outer deferred state once; and
deduplicates shared Buy markets, origin sets, trace nodes, and sold-ledger
representative caches by allocation identity. Trace arena, finished routes,
continuation-score cache, and purchase cache are included. JSON now reports
`peak_estimated_mb`; the independent process RSS and external cap remain.

Deferred branches now share:

- immutable pinned Buy-market snapshots;
- copy-on-write origin sets;
- persistent linked trace prefixes;
- existing immutable exact-sale cache rows; and
- sold-ledger representative caches.

Food constraints, mutable gold curves, current player mutations, and action
suffixes remain branch-owned.

### Food breakpoints and useful-demand ceiling

Origin compaction formerly tested every total-food integer against every seed.
It now treats each seed as a constant-value feasible interval and visits only
interval starts and end-plus-one breakpoints with a maximum heap. This selects
the identical winning origins without the quadratic integer scan.

The resolver also caps remaining food at a conservative maximum amount that
could still be consumed. The bound includes all remaining days, possible
Clepsydra and Time-is-Money extensions, Leader's Necklace, Animal Lover, the
largest crew hire every remaining day, and a deliberately over-approximated
Camelization/Animal-Lover combination. Food above this bound has no final
value and can only spend gold or occupy weight.

Weighted food cannot generally be reduced to a few sampled integers: after a
Sell, each remaining amount can be a real next-Buy carrying-capacity
breakpoint. Those meaningful alternatives remain exact. The optimization
removes provably useless quantities and generates origin-selection breakpoints
directly rather than calling them all "recommendations."

Post-change beam-500 measurements:

| Town | Score | Seconds | Peak RSS | Exact-Sell calls | Deferred rows | Max rows |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Normalia | 37,478 | 2.050 | 71 MB | 330,464 | 124,609 | 47 |
| Mehaz | **48,738** | 2.343 | 91 MB | 374,761 | 119,611 | 45 |
| Gayawaku | 49,628 | 3.583 | 92 MB | 83,117 | 415,547 | 150 |

Gayawaku's baseline was 15.665 seconds, 179,440 exact-Sell calls, 909,722
deferred rows, and a maximum of 592 rows. Continuation scoring costs more work
on small Normalia/Mehaz runs, but it recovered the Mehaz record and the food
changes removed Gayawaku's prior hot path.

### Interest/Sell order proof

For two adjacent terminal actions with unchanged markets, no encounter or
disaster between them, ordinary non-quick Interest, nonnegative sale proceeds
and income, and an Interest rate at least 1.00, Sell then Interest dominates
Interest then Sell: the sale proceeds and first day's income also receive the
nondecreasing Interest transformation.

This proof does **not** justify pruning an unresolved Interest action. That
branch may choose Trader, Camel, Buy, or an event instead of Sell, and under a
bounded beam the logically dominating Sell-first owner may already have been
discarded. Rates below 1.00, quick Interest, negative sale values, changing
markets, and food feasibility are direct counterconditions. Consequently no
new pre-action pruning rule was added. Concrete/materialized states continue
to use ordinary resource dominance, while continuation tickets favor an exact
immediate Sell without forcing it. This is the narrow correctness-preserving
outcome of the proposed rule.

### Automatic memory-pressure scheduling

The existing bounded hybrid drain is now automatic when no explicit hybrid
batch is configured. It remains broad through the final normal Merchant/Witch
day, then triggers only when exact estimated memory, projected candidate
storage, or RSS approaches the internal working budget. Automatic mode groups
the retained frontier by continuation structure, completes one structure at a
time in bounded local-beam batches, and keeps only bounded waiting states and
finished routes. Explicit interleave experiments retain their old behavior.

A stress run with a 512 MB hard cap, 128 MB internal budget, beam 1,000, and
Gayawaku triggered on day 12. It completed in 27.277 seconds at 100 MB peak RSS,
used 105 structure-local batches over 942 retained states and 105 structures,
processed 39,123 states, and discovered 49,753. The previous interleaved drain
took 10.984 seconds and nine batches; the structure-local schedule deliberately
trades that throughput for lower cross-structure competition and depth. The
external hard cap was unchanged.

## All-town validation

A sequential open-start sweep used the same general configuration for every
town: beam 500, no incumbent, no automatic refinement, 384 MB internal budget,
512 MB hard cap, and 30 seconds per town. Solver finalists are replayed through
`Game::auto_game_loop` before they are accepted. The sweep did not pin any
Merchant, Witch, Buy, Sell, or encounter timing.

| Town | Discovered | Prior verified bot | Result |
| --- | ---: | ---: | --- |
| Normalia | **37,478** | 35,114 | improved |
| Fang-Mo | **37,527** | 35,163 | improved |
| L'Exquise | 56,817 | 56,817 | matched |
| Mahaji | **37,533** | 35,211 | improved |
| Tocaccialli | 47,621 | 49,634 | retained fixture remains higher |
| Bajaar | **56,147** | 49,827 | improved; also exceeds unverified 56,146 claim |
| Doba Lao | **44,768** | 44,631 | improved |
| Huangoz | **37,478** | 35,114 | improved |
| Port-Willam | 42,087 | 42,296 | retained fixture remains higher |
| Moonin | 37,527 | 37,527 | matched |
| Cornecopea | 38,336 | 43,713 | retained fixture remains higher |
| Eduming | **81,321** | 80,043 | improved |
| Octoyashi | 40,007 | 40,056 | retained fixture remains higher |
| Kifuai | **37,500** | 36,589 | improved |
| Skjollird | 43,085 | 43,475 | retained fixture remains higher |
| Petrinov | 45,578 | 45,578 | matched |
| Jilliqo | 37,759 | 37,759 | matched |
| Gayawaku | 49,628 | 50,105 | retained fixture remains higher |
| Vilparino | **37,527** | 36,631 | improved |
| Sanctifan | 45,553 | 51,130 | retained fixture remains higher |
| Mehaz | 48,738 | 48,738 | matched |
| Ipartus | **37,704** | 36,295 | improved |

The ten improved routes are stored in the benchmark fixtures, asserted by the
safe test suite, and copied to `Best.txt`. This demonstrates better bounded
search coverage; it is not a proof that any score is globally optimal.

After the sweep, Jay manually improved the discovered Normalia structure by
swapping the day-7 Camel and day-8 Trader, then spending the extra income on one
additional pottery in the day-12 Buy. The exact route replays at **37,527** and
is now the seeded Normalia fixture; the general beam-500 discovery remains
37,478.
