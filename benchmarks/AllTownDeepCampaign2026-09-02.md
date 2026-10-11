# All-Town Deep Campaign — 2026-09-02

This campaign tested continuation-retention shares 0%, 2%, and 5% for all 22
Classical towns. Each process ran sequentially through `scripts/run_safe.sh`
with a 1,024 MB external cap, 768 MB internal budget, 600-second solver budget,
and no automatic route refinement.

The standard profile used a 50,000-state broad beam, 5,000-state hybrid batch,
50,000-state suffix beam, day-12 split, and structure-interleaved suffix order.
Gayawaku's weightless-food models exhausted the internal working budget before
the split at broad width 50,000. Its completed recovery lanes reduced only the
broad beam to 20,000 and retained the full 50,000-state suffix.

Raw results:

- [`results/deep-all-town-20260902.jsonl`](results/deep-all-town-20260902.jsonl)
  contains the original 63 standard-profile completions and three Gayawaku
  pressure diagnostics. Its three Fang-Mo rows used the superseded weightless
  starting-pottery model and are excluded from every aggregate below.
- [`results/deep-gayawaku-recovery-20260902.jsonl`](results/deep-gayawaku-recovery-20260902.jsonl)
  contains Gayawaku's three completed recovery lanes.
- [`results/deep-fang-mo-weighted-20260902.jsonl`](results/deep-fang-mo-weighted-20260902.jsonl)
  contains Fang-Mo's three corrected weighted-pottery lanes.

## Results

| Town | Score | Three-lane time | Processed | Generated | Peak RSS | Best-route variants |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Normalia | 37,527 | 560.2 s | 5,375,889 | 13,956,597 | 628 MB | 1 |
| Fang-Mo | 37,569 | 581.7 s | 5,401,877 | 14,292,913 | 621 MB | 1 |
| L'Exquise | 56,817 | 612.3 s | 5,019,494 | 16,260,303 | 668 MB | 1 |
| Mahaji | 37,589 | 542.1 s | 5,316,185 | 13,850,002 | 630 MB | 1 |
| Tocaccialli | 49,634 | 1,037.1 s | 6,812,710 | 19,754,257 | 641 MB | 1 |
| Bajaar | 56,147 | 906.9 s | 5,201,135 | 15,406,221 | 644 MB | 1 |
| Doba Lao | 45,543 | 506.2 s | 5,477,882 | 14,106,692 | 630 MB | 1 |
| Huangoz | 37,527 | 562.2 s | 5,441,061 | 14,210,051 | 636 MB | 1 |
| Port-Willam | 42,296 | 648.3 s | 5,271,075 | 14,967,437 | 644 MB | 1 |
| Moonin | 37,527 | 551.8 s | 4,769,854 | 13,774,383 | 655 MB | 2 |
| Cornecopea | 43,881 | 392.4 s | 5,087,604 | 14,104,648 | 596 MB | 1 |
| Eduming | 81,321 | 425.0 s | 5,373,650 | 12,088,301 | 644 MB | 1 |
| Octoyashi | 40,056 | 581.8 s | 5,105,330 | 13,942,083 | 654 MB | 1 |
| Kifuai | 37,547 | 533.2 s | 5,042,848 | 13,668,491 | 624 MB | 1 |
| Skjollird | 47,065 | 753.6 s | 5,326,119 | 15,423,805 | 643 MB | 1 |
| Petrinov | 45,578 | 424.3 s | 4,918,647 | 12,856,788 | 621 MB | 1 |
| Jilliqo | 37,819 | 591.2 s | 4,990,635 | 14,146,554 | 649 MB | 1 |
| Gayawaku | 50,105 | 485.6 s | 1,337,304 | 4,822,655 | 550 MB | 1 |
| Vilparino | 37,569 | 546.5 s | 5,120,456 | 13,853,430 | 629 MB | 1 |
| Sanctifan | 55,728 | 704.9 s | 5,843,323 | 16,369,674 | 630 MB | 1 |
| Mehaz | 48,738 | 597.0 s | 4,362,427 | 14,377,535 | 641 MB | 1 |
| Ipartus | 37,734 | 571.8 s | 5,325,244 | 13,947,034 | 644 MB | 1 |

The 66 corrected completed lanes consumed 13,115.967 seconds (3 hours, 38
minutes, 35.967 seconds), processed 111,920,749 states, and generated 310,179,854
transitions. Peak RSS was 668 MB. Normal search generated zero Wait states.

Every lane independently rediscovered its reported score; none merely inherited
the seeded incumbent. Every campaign score exactly matched the saved computer
score in `Best.txt`, so no record or saved path required an update. The campaign
found equal-scoring
action-order alternatives for L'Exquise, Tocaccialli, Cornecopea, Jilliqo, and
Sanctifan. Moonin produced two equal-scoring campaign routes: the 0% lane called
Magic Cleppsydra between its two day-10 Camels, while the 2% and 5% lanes called
it before both. These are alternate optima, not score improvements; the
existing replay-verified paths remain the authoritative entries.

## Gayawaku memory result

The 50,000-state broad lanes stopped safely before hybrid scheduling:

| Lane share | Stop time | Peak RSS | Pending beam | Next-day candidates |
| ---: | ---: | ---: | ---: | ---: |
| 0% | 36.972 s | 691 MB | 50,000 | 88,598 |
| 2% | 38.477 s | 739 MB | 50,000 | 58,248 |
| 5% | 38.402 s | 745 MB | 50,000 | 59,342 |

At a 20,000-state broad beam and unchanged 50,000-state suffix, all three lanes
completed in 160.874–163.205 seconds, used 527–550 MB, and reproduced 50,105.
Future all-town campaigns should use this profile for Gayawaku unless its
weightless-food deferred storage is reduced.

## Measured next optimization

The following timers are inclusive and may overlap, so they must not be added
as percentages of wall time. Their ordering still identifies the hot paths:

| Phase | Aggregate | Median per lane | Maximum lane |
| --- | ---: | ---: | ---: |
| Deferred purchase expansion | 5,784.507 s | 83.913 s | 150.840 s |
| Beam trimming | 5,084.321 s | 73.891 s | 139.921 s |
| Continuation-matrix scoring | 4,337.312 s | 62.846 s | 122.567 s |
| Deferred sold expansion | 4,191.028 s | 56.370 s | 128.303 s |
| Layer compression | 2,971.952 s | 43.958 s | 73.629 s |
| Memory graph scans | 1,596.565 s | 24.455 s | 37.706 s |
| Exact Sell resolution | 1,502.861 s | 17.298 s | 83.109 s |
| Continuation-matrix selection | 644.422 s | 9.614 s | 15.189 s |

Beam trimming is the next isolated optimization target because it costs more
than matrix scoring and operates on 187,940,940 candidates that are ultimately
discarded. The correctness constraint is strict: a prototype may change data
layout, selection algorithm, or temporary allocation, but it must reproduce
the same retained-state ordering for identical inputs. Continuation-matrix
score reduction remains the next target after beam trimming; the corrected
campaign evaluated 954,810,817 continuation views.

The larger deferred-purchase timer is a broader expansion phase and is likely
to contain both candidate construction and downstream selection work. Profile
it again after an exact beam-trim improvement before treating it as a separate
root cause.
