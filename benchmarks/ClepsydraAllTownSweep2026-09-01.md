# Magic Cleppsydra all-town sweep — 2026-09-01

This experiment fixed only the Merchant choice to **Magic Cleppsydra**. Merchant
timing, Witch choice and timing, day actions, food, cargo, crew, and Interest
remained search decisions. Every town ran independently and sequentially through
the native safety wrapper.

```bash
MAX_MB=512 INTERNAL_MB=384 ARRAY_LIMIT=2000 MAX_SECONDS=90 \
  ./scripts/run_safe.sh --town "<town>" \
  --merchant-option "Magic Cleppsydra" \
  --hybrid-batch-size 250 --hybrid-suffix-limit 1000 \
  --hybrid-split-day 10 --no-incumbent --no-auto-refine \
  --benchmark-json
```

All 22 runs reported `search_complete: true`. Observed peak RSS stayed between
79 and 104 MB, well below the 512 MB external cap.

| Town | Previous saved best | Cleppsydra-only result | Change | Outcome |
| --- | ---: | ---: | ---: | --- |
| Normalia | 37,527 | 37,527 | 0 | tied |
| Fang-Mo | 37,527 | **37,577** | +50 | superseded: starting Pottery was weightless |
| L'Exquise | **56,817** | 56,141 | -676 | Blue Treasure remains better |
| Mahaji | 37,533 | **37,589** | +56 | new best |
| Tocaccialli | **49,634** | 47,621 | -2,013 | Sturdy Saddle remains better |
| Bajaar | **56,147** | 46,254 | -9,893 | Blue Treasure remains better |
| Doba Lao | 44,768 | **45,543** | +775 | new best |
| Huangoz | 37,478 | **37,527** | +49 | new best |
| Port-Willam | 42,296 | 42,296 | 0 | tied |
| Moonin | 37,527 | 37,527 | 0 | tied |
| Cornecopea | 43,713 | **43,881** | +168 | new best |
| Eduming | 81,321 | 81,321 | 0 | tied |
| Octoyashi | 40,056 | 40,056 | 0 | tied |
| Kifuai | 37,500 | **37,547** | +47 | new best |
| Skjollird | 43,475 | **47,065** | +3,590 | new best |
| Petrinov | **45,578** | 41,100 | -4,478 | Blue Treasure/Oasis remains better |
| Jilliqo | 37,759 | **37,819** | +60 | new best |
| Gayawaku | 50,105 | 50,105 | 0 | tied |
| Vilparino | 37,527 | **37,569** | +42 | new best |
| Sanctifan | 51,130 | **55,602** | +4,472 | new best |
| Mehaz | **48,738** | 46,468 | -2,270 | Route to Mahaji remains better |
| Ipartus | 37,704 | **37,734** | +30 | new best |

## Interpretation

Magic Cleppsydra is the strongest shared Merchant lane found so far: it tied or
improved 17 of 22 saved town records and produced 11 new records in this sweep.
It is not universally best. L'Exquise, Tocaccialli, Bajaar, Petrinov, and Mehaz
retain stronger recorded routes using another Merchant option.

The largest discoveries were not small cargo substitutions. Skjollird changed
to a three-Sell route and gained 3,590 points; Sanctifan gained 4,472 with a
different food/crew schedule. That supports keeping Cleppsydra as a dedicated
broad search lane while preserving unrestricted Merchant search for final
record attempts.

Every improved action list was copied into `benchmarks/RecordRoutes.cpp`, replayed
by the game simulator, and then promoted to `Best.txt`. These are verified local
best routes, not proofs of global optimality.
