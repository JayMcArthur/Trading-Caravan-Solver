# Post-Merchant-sweep search experiments — 2026-09-01

This follow-up pursued the two strongest unresolved questions after the pinned
Magic Cleppsydra, Sturdy Saddle, and Blue Treasure campaigns:

1. Can a deeper or differently ranked Blue Treasure search beat Sanctifan's
   55,602 Magic Cleppsydra record?
2. Is Route to Mahaji useful outside its known Mehaz record?

All native work used `scripts/run_safe.sh`. No run exceeded its external memory
cap, and no new record was promoted to `Best.txt`.

## Blue Treasure / Sanctifan

The equal-width all-town sweep found 55,445, only 157 below the saved 55,602.
The following independent policies were tested without fixing encounter days,
action order, quantities, or a prefix.

| Policy | Result | Complete | Peak RSS | Interpretation |
| --- | ---: | --- | ---: | --- |
| Beam 2,000, 25% optimistic | 55,445 | yes | 82 MB | original near-record |
| Beam 10,000, split day 10 | no finalist | no | 112 MB | early frontier consumed 298 seconds |
| Beam 10,000, split day 8 | 51,039 | no | 101 MB | completed only 1 of 8,392 queued suffix inputs |
| Beam 5,000, structure-interleaved day-8 suffixes | 52,318 | no | 106 MB | 6 batches and 118,477 states; diversity hurt score quality |
| Beam 2,000, optimistic share 0% | 55,445 | yes | 79 MB | stable plateau |
| Beam 2,000, optimistic share 50% | 55,445 | yes | 85 MB | stable plateau |
| Beam 2,000, optimistic share 100% | 55,445 | yes | 86 MB | stable plateau |
| Beam 2,000, skeleton share 25% | 55,445 | yes | 82 MB | 22,918 reserved skeleton candidates |
| Beam 2,000, skeleton share 50% | 55,445 | yes | 82 MB | 36,182 reserved skeleton candidates |
| Beam 2,000, state diversity 25% | 55,445 | yes | 111 MB | stable plateau |
| Learned refinement, 50% share, radius 1 | 55,445 | yes | 81 MB | no local improvement |
| Learned refinement, 50% share, radius 2 | 55,445 | yes | 82 MB | no local improvement |
| Learned refinement, 75% share, radius 2 | 55,445 | yes | 82 MB | no local improvement |
| Learned refinement, 75% share, radius 3 | 55,445 | yes | 81 MB | no local improvement |

The repeated winning Blue route was:

```text
Trader
Buy [12, 0, 0, 0, 0, 0]
Trader
Trader
Merch - Blue Treasure
Buy [13, 0, 0, 0, 0, 0]
Camel
Buy [15, 0, 4, 0, 0, 0]
Sell
Witch - Midas was a Trader
Camel
Buy [14, 1, 0, 0, 0, 1]
Sell
Buy [21, 0, 0, 0, 0, 4]
Sell
Interest
Interest
Interest
```

The evidence does not prove 55,445 is Blue-optimal, but ordinary beam widening,
upper-bound balance, generic diversity, early skeleton diversity, and learned
one-to-three-action neighborhoods all reached the same plateau. More of those
same sweeps are now low value.

## Pinned refinement support

Automatic learned refinement previously ran only when no Merchant or Witch
option was pinned. Pinned option lanes still have open timing and actions, so
that restriction prevented the most relevant local follow-up.

The driver now permits automatic refinement for open-timing pinned-option
searches. Fixed-day, saved-prefix, and forced-start experiments remain
single-pass. Two new controls make the learned neighborhood explicit:

```text
--refinement-share <1-100>
--refinement-mismatches <0-8>
```

The full bounded native test suite passed after the change.

## Route to Mahaji all-town sweep

This used the same complete 2,000-state pinned configuration as the other
Merchant sweeps. All 22 runs completed; peak RSS was 113 MB.

| Town | Saved best | Route to Mahaji |
| --- | ---: | ---: |
| Normalia | 37,527 | 30,824 |
| Fang-Mo | 37,577 (superseded weightless model) | 30,509 |
| L'Exquise | 56,817 | 41,399 |
| Mahaji | 37,589 | 30,891 |
| Tocaccialli | 49,634 | 40,651 |
| Bajaar | 56,147 | 43,700 |
| Doba Lao | 45,543 | 37,202 |
| Huangoz | 37,527 | 30,824 |
| Port-Willam | 42,296 | 34,763 |
| Moonin | 37,527 | 31,079 |
| Cornecopea | 43,881 | 36,739 |
| Eduming | 81,321 | 57,967 |
| Octoyashi | 40,056 | 32,679 |
| Kifuai | 37,547 | 30,914 |
| Skjollird | 47,065 | 39,721 |
| Petrinov | 45,578 | 31,238 |
| Jilliqo | 37,819 | 33,055 |
| Gayawaku | 50,105 | 40,953 |
| Vilparino | 37,569 | 30,869 |
| Sanctifan | 55,602 | 43,015 |
| Mehaz | 48,738 | **48,738** |
| Ipartus | 37,734 | 30,824 |

Route to Mahaji is a Mehaz specialist at this width. The sweep independently
rediscovered the saved Route-to-Mahaji/Camelization/five-Sell path and did not
approach another town record.

## Recommended next fork

The next search work should target numeric neighborhoods rather than more
global beam policies. Build a replay-backed local optimizer that starts from a
discovered route's event timing and action types, then exactly re-optimizes:

- food quantities at each Buy;
- cargo portfolios at each Buy/Sell pair;
- adjacent Trader/Camel ordering where the encounter calendar remains legal;
- one inserted or removed Buy/Sell/Interest action at a time.

This directly attacks the remaining 157-point Blue Sanctifan gap while keeping
every candidate replayable. It also gives every saved record a cheap polishing
pass. A secondary systematic fork is to screen the meaningful Witch lanes:
Midas, Oasis, Camelization, and Vertue of Patience. The Merchant evidence now
supports scheduling only four independent lanes in serious record campaigns:
Magic Cleppsydra, Blue Treasure, Sturdy Saddle, and Route to Mahaji.
