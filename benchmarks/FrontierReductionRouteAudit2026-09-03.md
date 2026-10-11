# Exact frontier reduction and saved-route survival audit

## Production change

`FrontierReducer` is now the single correctness-preserving reduction seam used
before every lossy beam trim and at layer compression. It first merges states
whose full solver semantics compare equal, retaining the lowest trace ID and
any route-audit witness. It then applies the existing component-wise dominance
rule to concrete states with the same `SolverContextKey`.

This initial cut deliberately excluded deferred states from component
dominance. The follow-up now implements their separate allocation-containment
proof; see
[DeferredDominance2026-09-03.md](DeferredDominance2026-09-03.md). Deferred
states never enter the concrete resource predicate.

The beam remains the only explicitly lossy reduction. There is no compatibility
path that restores beam-before-exact behavior.

## Route-survival audit

`--audit-known-route` follows the selected town's replay-verified route without
seeding, retaining, or otherwise guiding it. With `--benchmark-json`, the
`route_survival_audit` object records, for each matched prefix:

- generation count;
- counts before and after exact reduction;
- counts after concrete dominance and after the beam;
- terminal completions; and
- best realistic, optimistic, immediate-Sell, Trader, Camel, Interest, and
  food-only-Buy ranks at a lossy trim.

The mode is `symbolic-action-branch`. Buy quantities remain inside deferred
allocation alternatives, so matching a Buy observes only the action branch.
The JSON explicitly reports `buy_quantities_deferred=true`; this audit must not
be interpreted as proof that the saved quantity vector survived.

## What the audit found

All commands ran sequentially through `scripts/run_safe.sh` with its 1,024 MB
hard cap and 768 MB internal budget.

| Town / beam | Saved score | Highest generated prefix | Highest retained prefix | First useful diagnosis |
| --- | ---: | ---: | ---: | --- |
| Normalia / 500 | 37,527 | 8 / 18 | 7 / 18 | Prefix 8 was beam-pruned: realistic rank 501, optimistic 171; best Camel lane rank 76. |
| Normalia / 5,000 | 37,527 | 18 / 18 | 18 / 18 | The complete route survived and finished; the run discovered 37,527. |
| Mehaz / 500 | 48,738 | 17 / 17 | 17 / 17 | The complete route survived and finished at the small beam. |
| Gayawaku / 500 | 50,105 | 6 / 18 | 6 / 18 | The next action was not generated; at this width the action-only audit cannot distinguish internal allocation loss from later feasibility. |
| Gayawaku / 5,000 | 50,105 | 9 / 18 | 8 / 18 | Prefix 9 was beam-pruned before Witch: realistic rank 5,064, optimistic 5,735; best food-only-Buy lane rank 542. |

Before deferred dominance, this confirmed two real search-quality failures
instead of assuming that a
missing record merely needs a wider beam. Normalia loses a viable branch whose
Camel continuation ranks well, while Gayawaku's route still falls just outside
all protected columns at beam 5,000. Exact deferred dominance subsequently
removed enough subsumed competition for both complete routes to survive at
those same widths.

## Correctness and performance

Focused reducer/audit tests cover exact merging, trace choice, witness transfer,
concrete dominance, Buy wildcard matching, and a terminal integration route.
The full native suite also passes. Benchmark JSON emits the cumulative pass
time as `frontier_reduction_ms` for subsequent campaigns.

At beam 500, before/after runs used an open start, no incumbent, no automatic
refinement, and a 5% continuation-lane share:

| Town | Score | Processed before | Processed after | Change | After time | After RSS | Pre-beam exact prunes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Normalia | 37,478 | 7,477 | 7,662 | +2.5% | 1.26 s | 69 MB | 466 |
| Mehaz | 48,738 | 7,449 | 7,754 | +4.1% | 1.79 s | 91 MB | 1,033 |
| Gayawaku | 49,628 | 7,502 | 7,670 | +2.2% | 2.97 s | 94 MB | 440 |

No score regressed. More useful states reach expansion because exact duplicates
no longer occupy beam slots.

The production-width Normalia profile used a 50,000-state broad beam, 5,000
hybrid batch, 50,000 suffix beam, day-12 split, structure-interleaved ordering,
and 5% continuation lanes:

| Build | Score | Processed | Generated | Time | Peak RSS |
| --- | ---: | ---: | ---: | ---: | ---: |
| Previous beam-before-exact | 37,527 | 1,792,528 | 4,657,776 | 189.8 s | 628 MB |
| Exact-before-beam | 37,527 | 1,997,950 | 5,143,592 | 225.4 s | 654 MB |

The new reducer explores 11.5% more states for 18.8% more wall time. It remains
well below the 10–15 minute target and 1 GB hard ceiling. The first deep
prototype was much slower because deferred states entered a concrete dominance
bucket; excluding them removed that quadratic work before these final numbers.

## Follow-up

Exact deferred dominance is now implemented and measured in
[DeferredDominance2026-09-03.md](DeferredDominance2026-09-03.md). The remaining
audit extension is quantity-level survival inside a deferred Buy, so a
generation gap can be attributed to an allocation reduction rather than only
to its surrounding action branch.
