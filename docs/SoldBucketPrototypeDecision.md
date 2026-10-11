# Sold-bucket prototype decision

## Question

Find the cleanest exact representation for the state after Sell. The production
model currently resolves the old Buy into an eager `(remaining food, gold)` row
frontier. The replacement should keep one solver state per game-action path,
avoid work for states later removed by the beam, preserve whole-number and
rounded-Interest behavior, and compose exactly with a later Buy.

These are throwaway logic experiments, not production implementations:

- [`sold-bucket-options-prototype.html`](../benchmarks/prototypes/sold-bucket-options-prototype.html)
  compares eager, threshold, lazy, and hybrid Sell timing;
- [`sold-demand-ledger-prototype.html`](../benchmarks/prototypes/sold-demand-ledger-prototype.html)
  carries one route's food debt and gold operations without choosing food;
- [`sold-threshold-curve-prototype.html`](../benchmarks/prototypes/sold-threshold-curve-prototype.html)
  generates food levels that unlock distinct action continuations; and
- [`sold-next-buy-convolution-prototype.html`](../benchmarks/prototypes/sold-next-buy-convolution-prototype.html)
  composes old and new food purchases at the next Buy.

## Findings

### Demand ledger wins between Sell and the next Buy

For a fixed action history, the sold value is nonincreasing as historical food
increases: more food shrinks the feasible cargo set and cannot improve the
monotone deferred-gold operations. Therefore the route only needs its current
maximum-prefix food debt. Nights raise the debt; fixed costs, income, and
Interest append exact gold operations; no food allocation is selected.

A 1,000-case deterministic sweep covered free, weighted, and weightless food;
one to four origins; fixed affordability costs; and Interest above and below
`1.00`. Resolving only the route's minimum historical food matched the best
result from scanning every larger amount in 1,000/1,000 cases. The modeled
cargo-check reductions were:

| Workload | Eager checks | One ledger query | Reduction |
| --- | ---: | ---: | ---: |
| Weighted food | 5,217 | 111 | 97.9% |
| Weightless food | 22,644 | 111 | 99.5% |
| Four old origins | 80,364 | 444 | 99.4% |

This is stronger than merely delaying an eager frontier: most action paths need
one exact food query, not the complete curve.

### Continuation thresholds are useful hints, not a safe allocation domain

The threshold generator retained 10-53 rows instead of 256 across its modeled
continuations, a 79.3-96.1% reduction, while mapping every integer food amount
to an identical enabled-action set.

That does **not** make it safe to delete every non-threshold allocation. At a
later Buy, the solver chooses how much food was bought at the old versus new
price. Interest and cargo-envelope changes can make an intermediate old-food
amount economically optimal even when it enabled no unique action before the
new Buy. A focused counterexample had action thresholds `{0, 10}`: threshold
restriction selected `0`, while the exact winner was `5`.

Use continuation thresholds to request likely curve values, explain food
requirements, or schedule incremental work. Do not use them as the source of
truth for the old/new allocation join.

### Prefix convolution solves the next-Buy seam

Let `S(k)` be current gold if `k` units of surviving food came from the old Buy.
At a later Buy with food price `p`, a total-food requirement `T` buys `T-k` new
units. Before new fixed cargo spend and income, the value is:

```text
S(k) - p(T-k) = (S(k) + pk) - pT
```

For every `T`, the exact old-food witness is therefore the prefix maximum of
`S(k) + pk` for `k <= T`. Compile or extend that prefix once, retain its witness,
and answer each total-food query in constant time. Original old-Buy capacity is
already encoded in `S`; current capacity depends on total carried food and is
checked at the new Buy.

The prefix join matched explicit old/new enumeration in 1,000/1,000 randomized
cases, including new food prices from zero upward and rounded Interest above and
below `1.00` after the new Buy.

| Total food range | Explicit checks | Prefix checks | Reduction |
| --- | ---: | ---: | ---: |
| 0-46 | 1,128 | 94 | 91.7% |
| 0-180 | 16,471 | 362 | 97.8% |
| 0-255 | 32,896 | 512 | 98.4% |

In the bounded JavaScript logic experiment, 300 full-range joins at 255 food
took 1,279 ms explicitly and 17 ms with the prefix, about 73x faster. Absolute
JavaScript timing does not predict C++ timing; the operation-count reduction is
the transferable result.

## Production outcome

The production cutover now uses `DeferredPurchaseModel` for an open Buy and
`DeferredSoldModel` for the sold demand ledger, hidden behind
`DeferredSearchState`. The old `DeferredResourceFrontier` phase and its runtime
expansion path were deleted after controlled A/B comparison.

The later-Buy seam materializes the old curve only when the Buy is actually
expanded, then builds the exact feasible-range envelope of `gold + p * food`.
This is not the prototype's fully incremental prefix-expression
implementation; it is the simpler production form that preserved every route
and score while reducing both elapsed time and retained rows. Terminal actions
use a bounded exact top-three query and do not materialize the curve.

Measured results and all-town validation are recorded in
[`SoldDemandLedgerResults.md`](../benchmarks/SoldDemandLedgerResults.md).

## Longer-term module seam

The remaining architectural cleanup would replace the exposed
`DeferredPurchaseModel` / `DeferredSoldModel` phase split with one deep
`DeferredAllocation` module. `GameSolver` should know that
the state is deferred, but not whether its implementation currently contains
an open Buy, a sold demand ledger, a prefix composition, or a concrete frontier
fallback.

An illustrative external interface is:

```cpp
class DeferredAllocation {
public:
    static DeferredAllocation from_player(const Player&, uint32_t provenance);

    const Player& representative() const;
    std::optional<DeferredAllocation> start_day() const;
    std::optional<DeferredAllocation> apply(
        const SolverAction&,
        BestPurchaseHandler&
    ) const;
    std::vector<DeferredPurchaseResolution> finish(
        BestPurchaseHandler&
    ) const;
};
```

The implementation hides these internal phases:

```text
Concrete resources
    -- Buy --> Open purchase expression
    -- Sell -> Sold demand ledger
    -- Buy --> Composed open purchase expression
    -- finish -> concrete winning route(s)
```

The module owns:

- maximum-prefix food debt;
- old Buy prices, weight, capacity, and affordability;
- frozen Sell prices and one-time Sell bonuses/income;
- monotone whole-number gold operations and affordability guards;
- lazy exact food-value queries;
- incrementally extended `S(k) + pk` prefix maxima at later Buys;
- representative selection for beam ranking;
- winning food/cargo witnesses and trace patching; and
- an explicit rejection or specialized representation for any future
  non-monotone allocation operation.

The purchase handler remains an injected in-process dependency at an internal
seam. `GameSolver` continues to branch on game actions, while allocation work
and phase legality stay local to the deep module.

## Completed production sequence

1. Add a shadow `SoldDemandLedger` beside the current eager Sell frontier.
2. Compare its minimum-food representative with the eager frontier at every
   Sell; require identical gold, food, and replay witness validity.
3. Carry the shadow ledger through start-of-day, crew, Merchant, Witch, and
   Interest transitions; compare each action's feasibility and representative.
4. Add the lazy prefix composition at a later Buy and compare its requested
   values with explicit origin enumeration.
5. Extend the existing tiny exhaustive oracle through `Sell -> actions -> Buy ->
   actions -> Sell`, comparing complete physical state sets.
6. Measure Sell queries avoided before beam pruning, prefix extensions, cached
   values, memory per deferred state, elapsed time, score, and exact route replay.
7. Cut over classical mode only after zero differential mismatches; then delete
   the eager resource-frontier path rather than retaining two permanent models.

The intended normal-mode outcome of that sequence is complete: focused exact
oracles, controlled eager/ledger comparisons, full route replay, and the
all-town screen passed before the eager path was deleted. Disaster-mode search
remains outside this particular performance cutover and retains its existing
guarded behavior.

The first production cut should not implement the continuation-threshold
generator. The demand ledger and prefix convolution provide the exact core
without it. Thresholds can be reconsidered later as cached query scheduling if
profiling shows they add value.
