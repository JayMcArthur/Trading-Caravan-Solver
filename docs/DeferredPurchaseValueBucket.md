# Deferred purchase value bucket

## Objective

Make `Buy` create one solver state. The Buy action records a marker instead of
immediately choosing food and cargo. Later actions constrain the marker. When
Sell is selected, the solver determines the best legal purchase that could have
been made at the marker and writes that concrete basket into the route.

The first production scope is classical mode without disasters.

## Prototype verdict

The throwaway
[`deferred-purchase-value-bucket-prototype.html`](../benchmarks/prototypes/deferred-purchase-value-bucket-prototype.html)
keeps an explicit-basket oracle beside one symbolic bucket. Its Interest,
crew-cost, and Sell walkthroughs produce identical physical-state frontiers.
An exhaustive short-sequence sweep found a counterexample to collapsing an
older food-only marker to its minimum food at a later Buy: Interest below
`1.00` can make earlier food spending preserve more value than holding gold.
After changing the boundary to retain that food/gold frontier, a bounded sweep
of 24,880 modeled state nodes across 16 small configurations matched explicit
enumeration. The prototype intentionally stops at the later-Buy boundary; the
production equivalence harness must also validate propagation through the new
marker.

The prototype established these representation rules:

- gold remains a whole number in every curve point;
- the Interest rate is exact hundredths, and the game rounding operation is
  applied to each point separately;
- later income changes current liquid gold but not original affordability;
- food debt is the maximum prefix of signed consumption, not only the current
  cumulative total;
- a later Buy proves that the old marker was food-only, but may leave several
  exact old food amounts; and
- Sell retains a `(remaining food, gold)` frontier after cargo is cleared.

It also showed that production does not need to store every displayed curve
point. In classical mode the curve can be a lazy monotone function: one
original-gold value, one maximum permitted original spend, and a short list of
additive or rounded-Interest operations.

## Core model

At Buy, snapshot facts that future events cannot change retroactively:

- original gold;
- original food and fixed inventory;
- original item buy prices and weights;
- original carrying capacity;
- day and trace marker; and
- the normal one-cargo-Buy-until-Sell rule.

Move the Buy-day gold into a deferred value bucket. The bucket is not one
`reserved_gold` integer. It is a compact curve:

```text
hypothetical exact Buy spend -> liquid gold remaining now, or impossible
```

This curve lets one state represent all choices of how much gold was kept for
food, crew, Interest, or cargo.

The conceptual point is:

```cpp
struct SpendPoint {
    int exact_original_spend;
    int current_liquid_gold;
};
```

`minimum_budget` is not a substitute for `exact_original_spend`. Production
code should calculate and return the basket's actual cost from the marker's
snapshotted Buy prices.

### Compact production representation

For an exact original spend `s`, start with:

```text
gold(s) = original gold - s + Buy-day income
```

Every supported classical transition preserves monotonicity:

- fixed cost or income adds a constant;
- nonnegative Interest applies the same monotone rounded function; and
- an affordability requirement restricts the domain to a smaller maximum
  spend.

Therefore store a lazy pipeline rather than an integer array:

```cpp
struct DeferredGoldFunction {
    int gold_after_zero_spend_buy;
    int maximum_original_spend;
    SmallVector<GoldOperation, 20> operations;

    int evaluate(int exact_original_spend) const;
    bool restrict_to_affordable(int cost);
};
```

`restrict_to_affordable` finds the largest permitted spend with monotone binary
search, then appends the cost operation. At Sell, evaluate only actual costs
from the purchase frontier. This keeps a Buy as one small state even when its
gold range is large. If a future disaster introduces a non-monotone gold
operation or negative multiplier, that case needs a materialized fallback or a
separate proof before enabling it.

## Action behavior

### Night / start of day

The game code consumes food at the next start of day. Increase the marker's
required-food debt. Do not choose a final food quantity yet. Use maximum-prefix
debt so later negative consumption cannot repair an earlier starvation.

Food eventually assigned to the marked Buy consumes its original gold and its
original capacity. Capacity gained later cannot enlarge the old Buy.

### Crew and Merchant costs

For every curve point, require enough liquid gold, subtract the exact cost, and
apply end-of-day income. Remove impossible points. A crew hire also raises later
food debt through the normal game transition.

This is the exact form of “crew reduces the bucket faster”: it reduces how much
the old Buy could have spent while still leaving enough gold for the hire.

Later income updates the current-liquid-gold side of every surviving curve
point. It does not increase the old Buy's original spending limit: gold earned
after the marker cannot be spent retroactively at that marker.

### Interest

Interest is the main nonlinear case. For each possible amount kept after Buy,
apply the game's exact rounded Interest independently:

```text
curve[old spend] = rounded_interest(curve[old spend])
```

Do not apply Interest to a pretend all-gold balance and subtract food later.
Multiple Interest actions repeatedly transform the curve.

### Another Buy before Sell

If another Buy occurs before Sell, all older unresolved markers must have been
food-only. Any older cargo purchase would have activated the cargo lock and
made the new Buy illegal.

At that boundary:

1. remove every older allocation containing cargo;
2. materialize every still-feasible older food-only amount;
3. retain the highest-gold row for each exact current-food amount, with the
   older trace patch stored on each row; and
4. open a fresh marker for the new Buy using each retained origin row and the
   new day's prices and capacity.

Do not merge two Buy days into one market snapshot. At most one cargo-capable
marker stays open, but it may have several food/gold origin rows representing
older food-only markers. This keeps each Buy day's historical prices, capacity,
and affordability limit exact.

Do not generally collapse to the minimum older food. When Interest is below
`1.00`, spending on food before Interest can beat holding gold through Interest
and buying that food later. A conditional collapse is allowed only if the
recorded gold-operation pipeline proves that shifting every extra food unit to
the later Buy weakly improves gold. The initial implementation should retain
the small food/gold frontier instead.

### Origin rows for repeated Buys

The fresh marker after a later Buy still remains one solver state. It contains
an origin frontier rather than creating one search branch per older food
amount:

```cpp
struct BuyOriginRow {
    int gold_before_new_buy;
    int food_before_new_buy;
    int maximum_new_buy_spend;
    int maximum_new_buy_weight;
    TracePatches older_buy_patches;
};
```

All origin rows share the new Buy day's prices, day, capacity rules, and later
gold-operation pipeline. Starting gold, starting food, available purchase
weight, and earlier trace patches remain row-specific. Food debt after the new
marker is evaluated against each row's starting food.

Rows are deduplicated by food level under the same current world, keeping the
highest gold for each exact food amount. More food is not general physical
dominance because it also occupies carrying capacity and can block a later
cargo purchase. The surviving row carries the trace patches used if it
eventually wins. This allows any number of food-only Buys before Sell without
multiplying `GameSolver` branches by their food allocations.

### Sell

Sell resolves the open cargo decision. Query the purchase frontier using:

- prices and capacity from the original Buy marker;
- food required by the route;
- each still-feasible exact Buy spend; and
- sell prices and effects at the current Sell.

For a candidate basket:

```text
old spend = original food cost + actual cargo cost
sell gold = current_curve[old spend] + current cargo sale value
```

The purchase lookup must report the basket's actual cost. An “at most this much
gold” answer is insufficient because unspent gold must have remained liquid
through crew costs and Interest.

Sell should return one compact frontier keyed by leftover-food thresholds, not
one scalar winner and not one solver state per basket. The next action may be a
new Buy, Crew, or Interest; preserving the best gold result for each meaningful
leftover-food threshold keeps those continuations exact.

Build that result as a streaming table. A free-food Cornecopea search exposed
that materializing every cargo basket first could exceed one million temporary
rows before producing only a few dozen food levels. Retaining the best gold row
for each exact remaining-food amount during the join gives the same exact
frontier without the allocation spike.

Each surviving Sell row stores the concrete food/cargo basket used to patch the
marked Buy if that row eventually wins.

The purchase query must separate the two markets:

- cost and weight come from the marked Buy snapshot; and
- sale value comes from the current Sell state.

This is necessary when Route to Mahaji, Midas, or Time is Money happens between
Buy and Sell. Conversely, later Patience, Oasis, or Stonecutter changes must not
retroactively alter an old basket's cost or weight. A cache key based only on
the current `world` flags is therefore insufficient for this query; key the
frontier by the actual snapshotted Buy cost/weight vector plus the current sale
value vector.

The existing three-choice purchase portfolio is a search heuristic, not an
exact Sell join. The bucket needs every nondominated cargo breakpoint up to the
requested original cost and weight. A suitable seam is conceptually:

```cpp
struct ExactPurchaseChoice {
    buy_list basket;
    int actual_cost;
    int actual_weight;
    int sale_value;
};

PurchaseFrontier cargo_frontier(
    const BuyMarketSnapshot& buy_market,
    const SaleMarketSnapshot& sale_market,
    int maximum_cost,
    int maximum_weight);
```

The returned rows are nondominated by original cost, original weight, and
current sale value. The deferred model, rather than `GameSolver`, joins those
rows to food thresholds and spend-curve points.

The implemented seam is `BestPurchaseHandler::getExactSaleFrontier`. It builds
a separately keyed compact tree whose item costs and weights come from the Buy
snapshot and whose values come from the Sell market. Unlike
`getBestBuyPortfolio`, it exposes every stored exact-cost improvement rather
than only the maximum, 90%, and 75% choices. One-unit cargo candidates are also
retained so a zero-value item can still enable a Hand of Midas Sell. Focused
brute-force comparison and the complete bounded test suite pass.

### Finish

Select the best surviving row, patch all marked Buy actions, and replay the
whole route through `Game`. Unsold final cargo is valued with the current Buy
prices used by `Game::check_points`, including a Patience change that happened
after the marked Buy; its historical acquisition cost still comes from the Buy
snapshot. Only a replayed score may update `Best.txt`.

## Solver branching rule

Allocation is table data, not a search branch. One bucket state branches only
by actual game actions:

- Sell;
- Trader;
- Camel;
- Interest;
- Merchant choice;
- Witch choice; or
- a later Buy boundary.

Each action copies and transforms one bucket. This is how the solver can
represent every legal allocation while touching far fewer solver states.

## Proposed deep interface

```cpp
class DeferredPurchaseModel {
public:
    OpenBucket mark_buy(const SolverState& before_buy) const;
    Bucket advance_night(const Bucket& bucket, const SolverState& route) const;
    Bucket hire(const Bucket& bucket, npcs selection) const;
    Bucket merchant(const Bucket& bucket, merch_options selection) const;
    Bucket witch(const Bucket& bucket, witch_options selection) const;
    Bucket interest(const Bucket& bucket) const;
    Bucket resolve_older_as_food_only(const Bucket& bucket) const;
    SoldBucket sell(const OpenBucket& bucket, const SolverState& route) const;
    ResolvedRoute finish(const Bucket& bucket) const;
};
```

`GameSolver` branches on actions. Original-day limits, food debt, exact Interest
curves, purchase queries, dominance, and trace patching stay inside this module.

## Implementation sequence

1. Freeze the explicit solver as the correctness oracle.
2. Implement a tested lazy spend-to-current-gold function.
3. Add additive income/cost transitions and affordability filtering.
4. Add exact rounded Interest transforms for every small gold value.
5. Add maximum-prefix food debt and original capacity limits.
6. Expose exact-cost purchase frontiers from `BestPurchaseHandler`.
7. Convert older markers to a food-only origin frontier at a later Buy boundary.
8. Implement Sell as a join of food thresholds, spend curve, weight, and cargo
   frontier; output one leftover-food/gold frontier.
9. Add unresolved trace markers and winning-row route patching.
10. Shadow-compare every symbolic transition with explicit enumeration.
11. Compare complete reachable state sets for small horizons.
12. Replay every stored record route and require identical scores.
13. Cut over classical mode only after zero state-set differences.
14. Run bounded all-town screens, targeted wider searches, and verified record
    replays before updating `Best.txt`.

## Required proof cases

- food-only, cargo-only, and mixed marked Buys;
- starvation at each possible night;
- Trader/Camel affordability and changed food rate;
- Cornucopia, Oasis, Camelization, and Animal Lover;
- original versus later capacity;
- all small-gold Interest rounding cases;
- multiple Interest actions before Sell;
- another Buy forcing every older marker to food-only while preserving
  nondominated quantities after sub-`1.00` Interest;
- cargo lock preventing another Buy;
- Sell valuation after Midas and Route to Mahaji;
- Sell frontier followed by Buy, Crew, and Interest;
- Cornecopea free food and Gayawaku weightless food; and
- every verified record route.

For small gold, capacity, and horizons, materializing the bucket must equal the
explicit reachable concrete-state set, not merely produce the same best score.

## Measurements

Track:

- Buy markers opened;
- curve points before and after each action;
- points removed by food, gold, and dominance;
- purchase-frontier rows examined at Sell;
- older markers resolved food-only;
- concrete allocations represented per solver state;
- generated/processed solver states;
- elapsed time and peak RSS; and
- discovered and replay-verified scores.

Success means one Buy creates one marked state, later actions transform one
table each, every old verified route remains reachable, and saved state memory
funds wider searches.
