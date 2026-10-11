# Exact deferred symbolic dominance

## Result

Deferred Buy and sold-ledger states now participate in correctness-preserving
dominance before the beam. This is allocation-set containment, not a comparison
of representative scores.

The change recovers two existing replay-verified records at substantially
smaller widths:

- Normalia discovers 37,527 at beam 500; the same configuration previously
  stopped at 37,478.
- Gayawaku discovers 50,105 at beam 5,000; the same configuration previously
  stopped at 50,039.

`--audit-known-route` confirms that all 18 saved actions survive and finish in
both runs. The audit remains observational and does not reserve either route.

## Proof conditions

`DeferredSearchState` owns the small dominance interface. `FrontierReducer`
first groups states by ordinary `SolverContextKey` and an exact deferred
dominance partition, then asks whether one state contains another.

For an unresolved Buy, the states must have:

- the same pinned Buy prices and weights;
- the same existing trade inventory;
- the same accumulated-food timeline;
- the same ordered origin shape: original food and free capacity;
- no stricter minimum-food coverage in the stronger state; and
- for every paired origin, a stronger gold function over the weaker function's
  complete original-spend domain.

These conditions map every weaker food/cargo allocation to the identical
allocation in the stronger state. Food and cargo weights therefore match,
current food matches, and current gold is at least as large.

A sold ledger additionally requires identical accumulated base-food use,
pending-start status, and exact post-Sell action suffix. Sell and that common
suffix apply the same monotone mutations to the paired resources, so source
containment remains containment afterward.

Gold functions with identical operation suffixes use monotonicity of addition
and nonnegative Interest. Otherwise every integer spend accepted by the weaker
function is evaluated. No endpoint interpolation or floating approximation is
used; the comparison includes the game's rounding at every Interest operation.

Trace IDs and audit progress are not game semantics. The retained dominator
inherits the best route-audit witness, while its own concrete trace provenance
continues to materialize the winning allocation. Intermediate audit retention
therefore means the saved prefix still has a future-equivalent dominating
witness. Terminal `finished` counts rebuild and match the actual materialized
trace rather than trusting that inherited marker.

## Concrete dominance correction

The former component-wise rule treated additional food and trade cargo as
unconditionally stronger. That is false when inventory has weight: extra items
can prevent a later Buy and cannot be discarded freely.

Concrete dominance now requires identical trade cargo. Food must also be
identical when its weight is positive; additional food is monotone only in a
weightless-food world. Gold may be greater because it consumes no capacity and
all supported gold mutations are monotone.

## Verification

Focused tests cover:

- pointwise gold functions, including differing Interest/rounding suffixes;
- stronger and reversed weaker purchase envelopes;
- incomparable origin food/capacity shapes;
- multiple-origin containment;
- pinned-market rejection;
- preservation through start-of-day, Interest, Camel, food-only Buy, and Sell;
- sold-ledger containment and suffix rejection;
- route-audit witness transfer; and
- weighted versus weightless concrete food dominance.

The focused suites and the full native suite pass through the bounded test
wrapper.

## Bounded measurements

All runs were unseeded, single-pass, open-start searches with 5% continuation
lanes, a 1,024 MB external cap, and a 768 MB internal budget.

### Beam 500

| Town | Before score | After score | Before time | After time | Deferred prunes | After beam prunes | Reduction time |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Normalia | 37,478 | **37,527** | 1.26 s | 1.44 s | 4,261 | 17,682 | 27.8 ms |
| Mehaz | 48,738 | **48,738** | 1.79 s | 1.91 s | 3,279 | 22,010 | 31.9 ms |
| Gayawaku | 49,628 | **49,628** | 2.97 s | 2.96 s | 3,056 | 22,413 | 37.5 ms |

Normalia's former beam-prune count was 23,461, Mehaz's was 26,539, and
Gayawaku's was 26,151. The exact relation removes subsumed competition before
continuation-matrix scoring.

### Gayawaku beam 5,000

| Build | Score | Time | Peak RSS | Generated | Deferred prunes | Beam prunes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Exact equality only | 50,039 | 21.66 s | 192 MB | 291,989 | 0 | 215,672 |
| Deferred dominance | **50,105** | 20.86 s | 195 MB | 272,066 | 38,346 | 161,822 |

### Normalia production profile

This used a 50,000 broad beam, 5,000-state hybrid batches, a 50,000 suffix
beam, day-12 split, and structure-interleaved suffix ordering.

| Build | Score | Time | Peak RSS | Processed | Generated | Exact dominance | Beam prunes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Exact equality only | 37,527 | 225.4 s | 654 MB | 1,997,950 | 5,143,592 | 0 | 2,866,571 |
| Deferred dominance | **37,527** | **146.6 s** | **601 MB** | 1,244,668 | 2,605,161 | 639,506 | 629,683 |

The exact dominance pass consumed 9.56 seconds, including 850,945 directional
checks and 772.2 million integer-spend evaluations. Its pruning reduced total
runtime by 35.0% and peak RSS by 53 MB. The run remains well below the 1 GB hard
ceiling and the requested 10–15 minute town budget.

## Profiling fields

Benchmark JSON now includes:

- `deferred_dominance_checks`;
- `deferred_purchase_dominance_prunes`;
- `deferred_sold_dominance_prunes`;
- `deferred_gold_function_comparisons`;
- `deferred_exact_spend_evaluations`; and
- `frontier_reduction_ms`.

The production relation is deliberately conservative. Different origin shapes
are retained even when a more expensive matching proof might establish
containment. That is an optimization opportunity, not a correctness gap.
