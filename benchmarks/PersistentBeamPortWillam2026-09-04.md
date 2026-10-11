# Port-Willam persistent-beam campaign — 2026-09-04

This campaign targeted Port-Willam because the saved bot score of 42,296 only
ties Matthew's trusted human record. It used ten sequential mapped-beam runs,
each with a 2,000-state day-layer width and a 10,000-node exact-expansion
budget. Every process ran through the bounded wrapper with a 1,024 MB hard cap
and 768 MB internal budget.

## Command

```bash
ARRAY_LIMIT=2000 MAX_SECONDS=600 ./scripts/run_safe.sh \
  --town Port-Willam \
  --no-incumbent \
  --no-auto-refine \
  --search-map search_maps/port-willam.sqlite3 \
  --search-node-budget 10000 \
  --search-map-order beam \
  --benchmark-json
```

The same command was invoked ten times. Since `--no-incumbent` was present,
42,296 appears only after the map itself reaches that route.

## Results

| Run | Time | Peak RSS | Mapped best | Unique nodes | Open | Partial | Solved |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 5.951 s | 62 MB | — | 39,798 | 29,798 | 9,950 | 50 |
| 2 | 9.573 s | 123 MB | — | 91,789 | 71,789 | 19,950 | 50 |
| 3 | 12.922 s | 131 MB | 42,296 | 119,311 | 89,311 | 27,046 | 2,954 |
| 4 | 19.815 s | 128 MB | 42,296 | 126,898 | 86,898 | 27,124 | 12,876 |
| 5 | 19.488 s | 126 MB | 42,296 | 134,222 | 84,222 | 26,669 | 23,331 |
| 6 | 17.203 s | 126 MB | 42,296 | 143,163 | 83,163 | 26,493 | 33,507 |
| 7 | 17.904 s | 126 MB | 42,296 | 152,586 | 82,586 | 26,605 | 43,395 |
| 8 | 18.254 s | 125 MB | 42,296 | 161,668 | 81,668 | 26,127 | 53,873 |
| 9 | 18.431 s | 126 MB | 42,296 | 172,069 | 82,069 | 26,730 | 63,270 |
| 10 | 18.905 s | 125 MB | 42,296 | 182,011 | 82,011 | 26,895 | 73,105 |

The campaign processed exactly 100,000 new expansions and generated 187,371
transitions in 158.446 seconds. Peak RSS was 131 MB. No process reached a time
or memory stop.

The final read-only inspection reported 160,580 terminal results, 987,505
trace links, and a 203,972,608-byte database. `verify` reported no cached-count
or solved-status errors. The root remains partial, so 42,296 is a lower bound,
not a proof that Port-Willam cannot score higher.

## Best mapped route

```text
Trader
Buy [15, 0, 0, 0, 0, 0]
Trader
Camel
Buy [31, 0, 0, 0, 0, 0]
Trader
Merchant - Magic Cleppsydra
Trader
Camel
Witch - Midas was a Trader
Camel
Buy [17, 8, 0, 0, 0, 1]
Sell
Buy [18, 7, 3, 0, 1, 4]
Sell
Buy [18, 0, 0, 0, 0, 8]
Sell
Interest
```

This exactly matches the existing replay-verified 42,296 route. The next two
mapped scores were 42,291 and 42,200. `Best.txt` was not changed because the
campaign tied rather than improved the record.

## Interpretation

Persistence behaved correctly: later runs did not restart the transient beam.
After the tie first appeared in run 3, the solved population increased from
2,954 to 73,105 by run 10 while the mapped best remained stable. A useful next
Port-Willam experiment is to resume the same graph under a different beam width
or ranking mix; scheduling changes do not require a new scenario and all
existing exact expansions remain reusable.

## Width-500 continuation

The same graph was immediately resumed for ten more runs with
`ARRAY_LIMIT=500`; every other option and the 10,000-node budget remained the
same.

| Run | Time | Peak RSS | Mapped best | Unique nodes | Open | Partial | Solved |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 20.625 s | 141 MB | 42,296 | 191,377 | 81,377 | 26,702 | 83,298 |
| 2 | 19.992 s | 141 MB | 42,296 | 200,817 | 80,817 | 26,446 | 93,554 |
| 3 | 20.690 s | 141 MB | 42,296 | 210,598 | 80,598 | 26,506 | 103,494 |
| 4 | 20.088 s | 142 MB | 42,296 | 220,234 | 80,234 | 26,433 | 113,567 |
| 5 | 19.538 s | 141 MB | 42,296 | 230,114 | 80,114 | 26,395 | 123,605 |
| 6 | 20.453 s | 144 MB | 42,296 | 239,910 | 79,910 | 26,359 | 133,641 |
| 7 | 19.938 s | 144 MB | 42,296 | 250,759 | 80,759 | 26,560 | 143,440 |
| 8 | 20.355 s | 142 MB | 42,296 | 259,585 | 79,585 | 26,242 | 153,758 |
| 9 | 19.517 s | 140 MB | 42,296 | 269,740 | 79,740 | 26,340 | 163,660 |
| 10 | 20.110 s | 141 MB | 42,296 | 279,382 | 79,382 | 26,230 | 173,770 |

These runs added another 100,000 exact expansions and 99,383 transitions in
201.306 seconds. The score did not improve, but the narrower schedule converted
100,665 additional states to solved while the open population fell from 82,011
to 79,382. Peak RSS was 144 MB.

Across both phases, the map has received 200,000 exact expansions and 286,754
transitions in 359.752 seconds. Final inspection reported 279,382 unique states,
372,183 terminal results, 2,098,061 trace links, and a 362,065,920-byte
database. `verify` still reported no errors. The top three scores remain
42,296, 42,291, and 42,200, and the root remains partial.
