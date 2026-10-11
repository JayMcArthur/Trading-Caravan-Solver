# Fang-Mo Weighted Pottery Correction — 2026-09-02

Fang-Mo begins with one ordinary Pottery. The previous implementation stored it
in a separate `fang_mo_pot` field and omitted that field from cargo weight. It
still added the item to Sell and point values, making the starting Pottery
valuable but weightless.

The special representation has been removed from the C++ and Python models.
`Game::apply_town` now increments the ordinary Pottery inventory, so the same
inventory field controls capacity, Sell, points, state identity, dominance,
deferred resolution, and route replay.

## Regression signal

`./scripts/run_tests_safe.sh fang-mo-inventory` verifies all of the following:

- Fang-Mo starts with exactly one ordinary Pottery.
- Remaining capacity falls by the configured Pottery weight of five.
- The ordinary inventory check sees the item.
- Sell pays the configured Pottery value and clears the item.

The old record's day-9 `Buy [43, 0, 0, 0, 0, 0]` was illegal under the correct
model: 43 Food uses 86 weight and the starting Pottery uses five, totaling 91
against three Camels' 90 capacity.

## Corrected search

Merchant day 7 and Witch day 9 were first pinned to the old route's exact
encounter timing. Three independent continuation-retention lanes converged:

| Lane share | Score | Time | Peak RSS |
| ---: | ---: | ---: | ---: |
| 0% | 37,569 | 180.692 s | 552 MB |
| 2% | 37,569 | 187.179 s | 605 MB |
| 5% | 37,569 | 180.051 s | 604 MB |

An unrestricted, unseeded 5% run then searched all legal Merchant and Witch
days and independently found the same score, route, and day-7/day-9 timing in
186.675 seconds at 619 MB.

Finally, the resumable all-town runner repeated all three unrestricted lanes
against the corrected incumbent:

| Lane share | Score | Time | Peak RSS |
| ---: | ---: | ---: | ---: |
| 0% | 37,569 | 192.913 s | 621 MB |
| 2% | 37,569 | 194.537 s | 616 MB |
| 5% | 37,569 | 194.257 s | 619 MB |

All three reported `best_source=discovered` and produced the same route:

```text
1 Trader
2 Buy [10, 0, 0, 0, 0, 0]
3 Trader
4 Camel
5 Buy [27, 0, 0, 0, 0, 0]
6 Trader
Merchant: Magic Cleppsydra
7 Trader
8 Camel
Witch: Midas was a Trader
9 Buy [41, 0, 0, 0, 0, 0]
10 Camel
11 Camel
12 Buy [5, 7, 9, 0, 1, 2]
13 Sell
14 Buy [20, 0, 0, 0, 0, 9]
15 Sell
16 Interest
```

The independent validated replay scores 37,569. `Best.txt` and the compiled
record fixture now use this route. Historical Fang-Mo results produced before
this correction remain useful as search-performance measurements but are not
valid game scores.
