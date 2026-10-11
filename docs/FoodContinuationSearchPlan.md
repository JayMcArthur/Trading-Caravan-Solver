# Food-continuation search plan

> Historical design note. The exact deferred Buy/sold-ledger model,
> continuation ranking tickets, interval-origin breakpoints, and conservative
> useful-food ceiling are now implemented. See
> `../benchmarks/DeferredSearchOptimization2026-09-01.md`. Weighted food still
> requires every integer that represents a distinct future capacity point; the
> production solver does not use the prototype's sparse suggestions as a
> correctness-changing prune. The global top-three proof remains separate and
> unfinished.

## Objective

Use Normalia normal mode as the proving ground for two related goals:

1. replace the current integer food sweep with the smallest food quantities that
   actually enable distinct legal continuations; and
2. eventually certify the three highest-scoring distinct Normalia routes under
   the solver's declared action rules.

Normal mode here means disasters disabled, Merchant and Witch enabled, the
starting-Trader prefix enabled, the one-cargo-Buy-until-Sell rule active, and
Wait representable for replay but absent from search.

The interactive logic experiment is
[`benchmarks/prototypes/food-continuation-generator.html`](../benchmarks/prototypes/food-continuation-generator.html).
It is a throwaway prototype, not production pruning.

A complementary deferred-decision design groups multiple threshold quantities
into one food/gold flex state and resolves the old Buy at Sell, the next Buy, or
finish. See [`FoodGoldFlexDesign.md`](FoodGoldFlexDesign.md) and its
[`food-gold-flex-prototype.html`](../benchmarks/prototypes/food-gold-flex-prototype.html)
logic experiment.

## What the current measurements say

A beam-1,000 Normalia run after disabling Wait produced:

| Measurement | Value |
|---|---:|
| Score | 25,187 |
| Processed states | 6,560 |
| Generated states | 41,609 |
| Generated Buy states | 25,461 (61.2%) |
| Raw food-range positions | 15,122 |
| Maximum food range in one Buy state | 45 |
| Purchase queries | 25,770 |
| Unique Buy baskets | 25,461 |
| Duplicate Buy emissions | 13,326 |
| Beam-pruned states | 28,947 |
| States/second | 953 |
| Peak RSS | 68 MB |

The purchase handler reported 6,427 ms total, of which 6,423 ms was tree
loading/lookup preparation and only 1.4 ms was the compact tree queries. For a
small run, loading ten purchase worlds dominates elapsed time. In a long
Normalia proof search, food branching and frontier retention will dominate after
that fixed startup cost is amortized.

The Wait A/B screen also matters. Removing Wait reduced generated states by
5.2% across 22 towns, but increased purchase queries by 14.3% because different
states survived the beam. Total time rose 5.3%; three town scores improved and
one fell. This is evidence that beam composition can hide or exaggerate a local
speedup. Every food experiment therefore needs both work counters and score
comparisons.

## Food is a threshold resource

In normal mode, food has no sale value and contributes no final points. Buying
more food costs gold and carrying weight. Therefore, if two food quantities
enable exactly the same future actions and state transitions, the smaller
quantity dominates the larger one.

Useful food amounts are cumulative-consumption thresholds. Starting just after
a Buy, a continuation such as:

```text
Camel -> Sell -> Buy
```

requires food for the start of the Camel day at the old crew rate, the start of
the Sell day at the new crew rate, and the start of the next Buy day at that same
new rate. Hiring Trader or Camel changes every later threshold. Cornucopia,
Oasis, Camelization, Animal Lover, extra days, same-day events, and the cargo Buy
lock can also change it.

Any integer strictly between two adjacent complete thresholds unlocks no new
continuation. Those are the candidates the new generator can remove.

## Proposed deep module

Put the seam beside purchase generation rather than inside the beam scorer:

```cpp
struct FoodThreshold {
    uint16_t food_to_buy;
    std::vector<SolverAction> witness;
};

class FoodContinuationGenerator {
public:
    std::vector<FoodThreshold> generate(
        const SolverState& state_before_buy,
        bool prospective_buy_contains_cargo
    ) const;
};
```

The production interface should remain this small. Crew food cost, current
food, day, event state, cargo lock, and town modifiers already live in
`SolverState`; callers should not reassemble a second partial game state.
`witness` is diagnostic: it explains that a threshold exists because it enables
an action pattern such as `Camel -> Sell -> Buy`.

The implementation should use exact game transitions behind the module instead
of duplicating food formulas in the solver.

## Generation algorithm

Model food symbolically as cumulative debt:

1. Begin from the prospective post-Buy state, with either a food-only Buy or a
   cargo Buy cycle.
2. Explore only non-Buy transitions until another Buy becomes legal or the run
   finishes.
3. Each new action day adds the current signed `food_consumption` to cumulative
   path debt before applying that day's same-day Merchant/Witch effect and
   ordinary action. Required starting food is the maximum prefix debt, not the
   final sum. This preserves states where negative consumption adds food.
4. Same-day quick actions add no second food payment.
5. If the prospective Buy contains cargo, paths cannot terminate at another Buy
   until Sell clears the cargo lock.
6. Memoize by the compact continuation state and both cumulative debt and its
   maximum prefix. Reaching the same continuation state with an everywhere-worse
   debt profile is dominated; comparing only final debt is not sufficient when
   consumption can be signed.
7. Group terminal paths by minimum debt, subtract current food, clamp at zero,
   and return one threshold plus at least one witness per amount.
8. Intersect thresholds with the actual affordable and weight-feasible range.

The first production version should deliberately ignore future affordability
while building action skeletons. This produces a safe superset of food
thresholds. A later refinement may filter skeletons after a particular cargo
basket is known, but it must not make food and cargo generation recursively
depend on one another.

## Correctness gates before pruning

### 1. Shadow mode

Generate thresholds but continue emitting the current food range. Record:

- raw food counts considered;
- distinct thresholds generated;
- counts between thresholds;
- which threshold witness covered every emitted Buy;
- whether any retained or winning route used a non-threshold count.

No command-line policy toggle is needed for the final design. Shadow mode is
temporary instrumentation removed when validation is complete.

### 2. Tiny exhaustive equivalence

For days 1-6, crew counts 0-3, signed consumption states, both cargo-lock states,
and each food-changing Merchant/Witch effect, compare two searches:

- every feasible integer food amount; and
- generated threshold amounts only.

The reachable terminal solver-state sets must match exactly. Compare full state,
not just score.

### 3. Recorded-route coverage

Replay every verified route and require every recorded Buy's food amount to be
present in the generated threshold set at that exact state. In particular,
Normalia has three distinct verified lower bounds:

| Route | Score |
|---|---:|
| `record_normalia_route` | 37,527 |
| `improved_normalia_route` | 35,054 |
| `documented_normalia_route` | 34,848 |

These three routes should seed the future top-three proof search.

### 4. Bounded A/B matrices

Run Normalia at beams 1,000, 5,000, 20,000, and 100,000, then all 22 towns at
beam 1,000. Compare scores, top-three routes, generated states, purchase work,
states/second, and RSS. A lower state count is not a win if beam composition
loses a route.

## Normalia top-three definition

Two routes are distinct when their post-prefix action sequences differ in action
type, event choice, NPC choice, or any purchased quantity. The same action
sequence discovered through duplicate solver states counts once.

The current solver retains three finished entries, but it does not yet dedupe
identical routes or expose all three in benchmark JSON. Its beam-1,000 provisional
discoveries are 25,187, 24,248, and 24,164; these are not candidates for a proof
because verified routes already score 35,114, 35,054, and 34,848.

The top three are certified only when:

1. the purchase generator covers every legal Buy outcome relevant under the
   declared action rules;
2. no beam truncation occurs;
3. every dominance rule used by the proof search has a written monotonicity
   argument and exhaustive small-state tests;
4. every unexplored state has an admissible upper bound no greater than the
   current third-place score;
5. the search ends normally rather than at a time or memory limit; and
6. all three routes independently replay to their reported score.

The present purchase profiles choose immediate/future-profit portfolios; they
do not enumerate every legal basket. The present optimistic bound is a beam
ranking score, not yet a certified admissible proof bound. Existing inventory
dominance also needs review because extra food or cargo consumes capacity and
cannot automatically be assumed harmless. These prevent a “for sure” claim
today.

## Ordered implementation roadmap

### Phase A — Food thresholds without risk

1. Add phase timing around tree loading, food generation, Buy materialization,
   beam scoring, layer compression, dominance, and trace compaction.
2. Implement `FoodContinuationGenerator` using symbolic food debt and exact
   same-day event ordering.
3. Run shadow mode and tiny exhaustive reachable-state equivalence.
4. Add threshold witnesses to diagnostics so every retained food amount has an
   explanation.
5. Enable threshold-only generation only after all correctness gates pass.

### Phase B — More useful states per beam

1. Deduplicate `(world, gold budget, weight budget, food amount, reserve)` query
   keys before asking the purchase handler. Current Normalia generated 13,326
   duplicate Buy emissions.
2. Return compact `buy_list` candidates directly, eliminating the
   `deque<Player> -> vector<Player> -> SolverState` conversion chain.
3. Instrument and then reduce repeated trace compaction and per-layer hash-map
   allocation. `traces.shrink_to_fit()` on every layer is a specific candidate
   for removal after measurement.
4. Add a one-process multi-town benchmark runner with an injected shared
   purchase-tree module. This avoids paying compact-tree deserialization for
   every town process; it helps broad experiments more than one long Normalia
   search.
5. Keep `nth_element` beam retention; it is already linear-selection rather than
   a full frontier sort. Optimize its scoring only after phase timings show it
   matters.

### Phase C — Honest top-three reporting

1. Deduplicate finished routes by exact action sequence.
2. Seed all three verified Normalia routes, not only first place.
3. Emit all three routes and origins in benchmark JSON.
4. Add an explicit search status: `heuristic`, `exhaustive`, or `proven`, plus
   the reason a run is not proven.

### Phase D — Proof search

1. Replace heuristic Buy portfolios in proof mode with an exact reachable-basket
   generator or prove a smaller Pareto representation future-equivalent.
2. Build and test an admissible final-score bound covering crew purchases,
   interest compounding, Merchant/Witch choices, extra days, food, capacity, and
   required Sell actions.
3. Use memory-bounded depth-first branch-and-bound, ordered by the fast beam
   scores but pruned only by the admissible bound.
4. Use the third distinct incumbent score as the top-three pruning threshold.
5. Implement actual checkpoint/resume. `checkpoint_path` currently exists but
   has no save/resume implementation.
6. Produce a certificate summary: explored nodes, exact duplicates, proven
   dominance prunes, bound prunes, third-place threshold, and zero remaining
   open nodes.

## Expected priority

For short runs, compact purchase-tree loading is the wall-clock bottleneck. For
long searches, food-threshold generation is the best candidate for reducing the
61% Buy branch share without guessing which route is valuable. For a certified
Normalia top three, exact Buy coverage and an admissible bound are mandatory;
no beam-width increase alone can provide the guarantee.
