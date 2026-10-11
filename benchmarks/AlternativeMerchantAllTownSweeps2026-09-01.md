# Sturdy Saddle and Blue Treasure all-town sweeps — 2026-09-01

These campaigns tested the two strongest alternatives to Magic Cleppsydra.
Sturdy Saddle ran across all 22 towns first; Blue Treasure then ran across all
22 towns. Runs were sequential. Only the Merchant option was pinned: Merchant
timing, Witch option and timing, buys, sales, crew, and Interest remained open
search decisions.

```bash
MAX_MB=512 INTERNAL_MB=384 ARRAY_LIMIT=2000 MAX_SECONDS=90 \
  ./scripts/run_safe.sh --town "<town>" \
  --merchant-option "<Sturdy Saddle|Blue Treasure>" \
  --hybrid-batch-size 250 --hybrid-suffix-limit 1000 \
  --hybrid-split-day 10 --no-incumbent --no-auto-refine \
  --benchmark-json
```

All 44 runs reported `search_complete: true`. Sturdy Saddle peaked at 131 MB
RSS and Blue Treasure peaked at 118 MB, both well below the 512 MB external
limit.

| Town | Saved best | Sturdy Saddle | Blue Treasure | Best pinned lane |
| --- | ---: | ---: | ---: | --- |
| Normalia | 37,527 | 34,892 | 33,421 | Magic Cleppsydra |
| Fang-Mo | 37,577 (superseded weightless model) | 34,518 | 33,592 | Magic Cleppsydra |
| L'Exquise | 56,817 | 48,110 | **56,817** | Blue Treasure |
| Mahaji | 37,589 | 34,889 | 33,848 | Magic Cleppsydra |
| Tocaccialli | 49,634 | **49,634** | 45,543 | Sturdy Saddle |
| Bajaar | 56,147 | 50,303 | **56,147** | Blue Treasure |
| Doba Lao | 45,543 | 40,214 | 42,087 | Magic Cleppsydra |
| Huangoz | 37,527 | 34,845 | 33,829 | Magic Cleppsydra |
| Port-Willam | 42,296 | 37,430 | 39,009 | Magic Cleppsydra |
| Moonin | 37,527 | 34,892 | 34,444 | Magic Cleppsydra |
| Cornecopea | 43,881 | 40,414 | 41,270 | Magic Cleppsydra |
| Eduming | 81,321 | 70,806 | 79,649 | Magic Cleppsydra |
| Octoyashi | 40,056 | 38,194 | 37,985 | Magic Cleppsydra |
| Kifuai | 37,547 | 35,046 | 34,329 | Magic Cleppsydra |
| Skjollird | 47,065 | 38,210 | 43,416 | Magic Cleppsydra |
| Petrinov | 45,578 | 34,469 | **45,578** | Blue Treasure + Oasis |
| Jilliqo | 37,819 | 36,150 | 34,692 | Magic Cleppsydra |
| Gayawaku | 50,105 | 40,950 | 49,344 | Magic Cleppsydra |
| Vilparino | 37,569 | 35,061 | 34,363 | Magic Cleppsydra |
| Sanctifan | 55,602 | 51,074 | 55,445 | Magic Cleppsydra; Blue is 157 behind |
| Mehaz | 48,738 | 40,894 | 41,657 | Route to Mahaji |
| Ipartus | 37,734 | 34,892 | 34,091 | Magic Cleppsydra |

## Conclusions

- Sturdy Saddle is a Tocaccialli specialist in this search: it exactly
  reproduced 49,634 and did not tie or improve another saved record.
- Blue Treasure exactly reproduced L'Exquise 56,817, Bajaar 56,147, and
  Petrinov 45,578. It found no new records.
- Sanctifan is the only close alternate-lane miss. Blue Treasure reached
  55,445, only 157 below the 55,602 Magic Cleppsydra record, so a deeper Blue
  Sanctifan run is justified if we want a focused follow-up.
- Magic Cleppsydra remains the strongest general Merchant lane. Mehaz remains
  the important exception outside these three options: its 48,738 record uses
  Route to Mahaji and Camelization.

No `Best.txt` entries were changed because every tied route was already stored
and replay verified. These bounded results compare equal-width searches; they
are not proofs that an option can never win with a larger search.

A later Route-to-Mahaji all-town sweep confirmed it as a Mehaz-only specialist
at this width. Blue Sanctifan widening and refinement experiments remained at
55,445. See `benchmarks/NextSearchExperiments2026-09-01.md`.
