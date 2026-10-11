# Classical buy-order experiments

## Question

Can the Classical solver reduce redundant purchase paths by allowing only one
cargo purchase before a Sell, represent an intentional stall separately, and
value the Sell action that held cargo still requires?

All runs in this report use normal game mode with disasters disabled. They were
run sequentially through the bounded wrappers with a 768 MB hard process limit
and a 512 MB internal solver budget.

## Conclusions

1. The production search now has a built-in "one cargo Buy until Sell" checker;
   it is run state, not a configuration boolean.
2. A non-food cargo Buy locks all later Buy actions until Sell. Food-only Buys
   are allowed before that cargo Buy, and Sell clears the lock.
3. `Wait (empty Buy)` remains an explicit replay action with the game's empty-Buy
   day semantics, but normal search no longer generates Wait successors.
4. The legacy beam overvalues held inventory because it prices cargo at its best
   future Sell value without charging the action needed to Sell it.
5. A sell-action-aware beam is a complementary lane, not a universal
   replacement. Best-of-lanes is safer than making either experiment the only
   search.
6. Moonin had an independent correctness bug: "call Merchant at any time" was
   forcing the Merchant immediately. Moonin may now defer the call while fixed
   Traveler encounter days remain mandatory.

## Why staged cargo can be correct

Buy is a full-day action. A route may need to Buy food before it can survive
another Trader or Camel day. The implemented rule therefore distinguishes food
from trade cargo: food-only Buys may happen before the single cargo Buy, but
putting trade cargo into a Buy commits that buy cycle until Sell.

An intentional no-purchase day can be represented as `Wait (empty Buy)` instead
of `Buy [0, 0, 0, 0, 0, 0]`. This preserves exact game-side replay behavior.
Normal search does not emit either empty action after the bounded screen found
no winning use for them.

Early cargo is also potentially useful before a real price boundary. In normal
mode, Virtue of Patience raises Jewelry's buy price, so Jewelry bought before the
Witch is not equivalent to Jewelry bought afterward.

## Cargo Buy/Sell adjacency audit — 2026-09-01

A stricter proposed normal form was tested: after ignoring automatic Merchant,
Witch, and disaster events, every cargo Buy would have to be followed by Sell.
That describes 44 of the 45 cargo Buys in the 22 replay-verified routes in
`Best.txt`, but it has one important exception. It also describes 210 of 211
cargo Buys across 110 modern result routes that report sold-ledger counters;
the same Mehaz route is the only exception.

Mehaz 48,738 contains:

```text
day 1 Trader
day 2 Camel
day 3 Buy [5 food, 8 spice]
day 4 Camel
day 5 Route to Mahaji, then Sell
```

The intervening Camel cannot simply be swapped ahead of the mixed Buy. Mehaz
starts with 5 food, its camels eat zero, and the day-1 Trader raises daily food
consumption to 2. Day 2 therefore starts with 3 food and day 3 with 1. If day 3
is changed from the mixed Buy to Camel, day 4 starts at -1 food and the caravan
dies before it can Buy. The mixed Buy supplies the food needed to use Camel as
a productive bridge to the day-5 price event.

This disproves cargo-Buy/Sell adjacency as an unconditional legality rule. It
does not prove that no entirely different normalized Mehaz route can tie the
record, but such an equivalence must be demonstrated before the verified route
can be pruned. The evidence supports a narrower experiment: require immediate
Sell unless holding cargo can reach a still-unapplied event that changes its
sale payout. Any intermediate player action must remain independently legal;
it cannot be assumed to commute across the Buy or Sell.

The original consolidation experiment was strongest when:

- the earlier Buy is not required for food;
- no buy price worsens before the later Buy;
- crew/events can be reordered before the purchase; and
- the route does not need the earlier Buy merely to occupy a required day.

The updated throwaway, no-build walkthrough at
`benchmarks/prototypes/cargo-buy-consolidation.html` makes these three cases
clickable and displays the cargo lock and explicit Wait after every action.

## Beam 1,000 screen

Before the rule became built in, the consolidation lane improved 5 of 16
under-record towns, tied 4, and lost 7.
It generated fewer states in every town. Notable score changes versus the
unrestricted legacy beam were Huangoz `28,756 -> 31,031`, Moonin (before its
timing fix) `26,168 -> 30,042`, Eduming `64,237 -> 64,563`, and Bajaar
`43,367 -> 43,379`. Port-Willam fell `29,909 -> 29,172`, demonstrating why the
restriction materially changes the explored search space.

After adding the built-in checker and explicit Wait, a bounded 22-town,
beam-1,000 screen generated 138,504 Wait successors (5,426-11,010 per town).
None of the 22 winning routes used Wait. Peak RSS was 69 MB. The raw results are
in `benchmarks/results/classical-cargo-lock-wait-screen-b1000.jsonl`. Wait
generation was disabled after this result; the action remains available to route
replay and direct game-model tests.

Repeating the same 22-town screen with Wait disabled reduced generated states
from 1,052,818 to 997,559 (5.2%) and processed states from 172,914 to 162,750
(5.9%). It increased purchase queries from 495,444 to 566,470 (14.3%) and total
time from 115.8 to 121.9 seconds (5.3%), because removing Wait changed which
states survived the beam. Huangoz, Kifuai, and Jilliqo improved; Port-Willam
fell by 156. This is why future food work will compare scores and work counters,
not treat a smaller branch count as sufficient evidence of a speedup. The raw
no-Wait results are in
`benchmarks/results/classical-cargo-lock-no-wait-screen-b1000.jsonl`.

The sell-action-aware lane found a different set of winners at the same beam:

| Town | Legacy | Sell-aware | Change |
|---|---:|---:|---:|
| L'Exquise | 48,580 | 55,552 | +6,972 |
| Eduming | 64,237 | 71,283 | +7,046 |
| Huangoz | 28,756 | 31,031 | +2,275 |
| Port-Willam | 29,909 | 32,180 | +2,271 |
| Octoyashi | 33,664 | 34,508 | +844 |
| Jilliqo | 33,828 | 34,437 | +609 |
| Vilparino | 31,167 | 33,443 | +2,276 |
| Sanctifan | 30,766 | 32,726 | +1,960 |

It tied the corrected Moonin and Gayawaku results, but reduced Bajaar at beam
1,000. At beam 5,000 it recovered the same `45,978` Bajaar route as legacy.

## New verified scores from beam 5,000

Sell-aware ranking produced four routes above the scores previously listed in
`Best.txt`:

| Town | New score | Previous listed score | Peak RSS |
|---|---:|---:|---:|
| L'Exquise | 56,817 | 56,140 | 86 MB |
| Eduming | 80,043 | 77,398 | 85 MB |
| Octoyashi | 40,056 | 37,983 | 85 MB |
| Jilliqo | 37,291 | 37,050 | 85 MB |

These routes live in `benchmarks/RecordRoutes.cpp`, are checked by the bounded
test suite, and are seeded as incumbents by the command-line solver.

Other useful beam-5,000 results were Huangoz `32,832`, Port-Willam `37,535`
from the legacy lane, Moonin `33,225` after callable-Merchant deferral, Bajaar
`45,978`, and Gayawaku `38,724`.

## Memory and purchase-cache evidence

The wider targeted runs peaked around 80-86 MB RSS, well under both limits.
Purchase-tree cache misses stayed around 10-12 even when a run issued more than
200,000 purchase queries. The expensive growth is emitted successor and trace
state, not rebuilding purchase tables. `purchase_dominance_prunes` also remained
zero, so the existing component-wise buy-result dominance test is not reducing
the frontier.

## Reproduction

Build and test only through the bounded entry points:

```bash
./scripts/build_safe.sh
./scripts/run_tests_safe.sh
```

Run the two beam-ranking lanes sequentially for selected towns. The cargo-buy
checker is always active and has no command-line toggle:

```bash
TOWNS='2 11 12 16' \
BEAMS=5000 \
PROFILES=greedy \
SHARES=25 \
STARTS=trader \
BEAM_SCORERS='legacy sell-aware' \
MAX_MB=768 INTERNAL_MB=512 MAX_SECONDS=180 \
./scripts/run_classical_experiments.sh
```

## Bounded depth-first follow-on

A depth-first prototype should trade time for frontier memory without changing
legal actions. It needs these safeguards before it is useful:

1. an internal graceful time budget so the solver prints its best incumbent
   before the outer timeout;
2. branch-and-bound using the existing optimistic final bound;
3. branch ordering from both legacy and sell-aware scores;
4. trace rollback after each completed branch, rather than retaining every
   visited trace;
5. a bounded transposition structure or per-depth cache;
6. A/B validation against unrestricted beam results on every faction before any
   purchase candidate pruning is enabled.

The next purchase experiment should rank a buy by its legal one-action
continuations (Sell, Trader, Camel, Interest, and same-day event effects), but
keep the unpruned generator as the control lane until cross-town tests show no
lost winners.
