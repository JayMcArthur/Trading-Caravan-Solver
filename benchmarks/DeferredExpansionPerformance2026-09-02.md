# Deferred Expansion Performance — 2026-09-02

This follow-up removes bookkeeping overhead from the production deferred Buy
and sold-ledger search. All native commands used `scripts/build_safe.sh`,
`scripts/run_tests_safe.sh`, or `scripts/run_safe.sh`; no run exceeded the
external 1,024 MB process cap.

## Accepted changes

- Sell candidates do not build action traces until they survive the local
  exact-food or top-N selection.
- `BestPurchaseHandler::ExactSaleMarketView` interns the exact Buy/Sell market
  signature once per deferred resolution. Queries use the same precomputed hash
  distribution as the old exact key, so cache identity remains exact while hot
  calls avoid hashing fifteen market integers repeatedly.
- Memory enforcement walks the complete live deferred object graph every 4,096
  checks. RSS, which measures the actual resident allocation, is sampled every
  64 checks below half of the internal working limit, every 32 checks above
  half, every 16 above 75%, and every action above 90%. The first RSS sample at
  or above the working limit forces an exact scan on that same action. A
  projected concrete Buy and each broad-layer scheduling decision also request
  exact accounting.
  The internal limiter retains 10% headroom and the independent OS limit
  remains 1,024 MB.
- Benchmark JSON exposes `memory_budget_checks`,
  `memory_budget_exact_scans`, `memory_budget_resident_samples`, and
  `memory_budget_scan_ms`.

These alter neither legal action generation nor beam ranking. Exact deferred
model and exhaustive oracle tests still pass.

## Normalia measurements

The controlled short command was:

```bash
ARRAY_LIMIT=500 MAX_SECONDS=30 ./scripts/run_safe.sh \
  --town Normalia --no-incumbent --no-auto-refine --benchmark-json
```

| Version | Score | Seconds | Processed | Peak RSS | Exact graph scans |
| --- | ---: | ---: | ---: | ---: | ---: |
| Before this work | 37,478 | 2.951 | 7,477 | 58 MB | per action |
| Accepted implementation | 37,478 | **1.249** | 7,477 | 68 MB | **8 / 7,509 checks** |

The same-width result, state counts, and route are unchanged. Wall time fell
57.7%.

For a time-limited wide search:

```bash
ARRAY_LIMIT=20000 MAX_SECONDS=22 ./scripts/run_safe.sh \
  --town Normalia --no-incumbent --no-auto-refine --benchmark-json
```

| Version | Processed in 20 s | States/s | Peak RSS | Scan time |
| --- | ---: | ---: | ---: | ---: |
| Per-action exact scan | 6,977 | 348.7 | 46 MB | included in expansion |
| Fixed scan every 64 checks | 65,204 | 3,219.6 | 183 MB | 14.842 s |
| Adaptive scan cadence | **159,830** | **7,841.4** | 306 MB | **2.936 s** |

The accepted version touches 22.9 times as many states in the same time window.
The higher RSS is intentional retained search breadth, not a raised limit.

A complete beam-20,000 run finished normally in 27.623 seconds, processed
223,864 states, peaked at 316 MB RSS, and independently rediscovered the
37,527 Normalia record. Its day-12 cargo Buy was `[2, 8, 9, 0, 1, 2]`.

## Recommended longer profiles

Extending a completed beam does not inspect more states. To spend a longer
budget productively, increase the broad width and explicitly split the suffix:

```bash
ARRAY_LIMIT=50000 MAX_SECONDS=602 ./scripts/run_safe.sh \
  --town Normalia --no-auto-refine \
  --hybrid-batch-size 5000 --hybrid-suffix-limit 50000 \
  --hybrid-split-day 12 --hybrid-skeleton-order structure-interleave \
  --benchmark-json
```

This unrestricted profile completed in 182.271 seconds, processed 1,792,528
states, generated 4,657,776 transitions, peaked at 624 MB RSS, and rediscovered
37,527. It completed all ten suffix batches. Exact memory walks consumed 23.848
seconds, down from 99.555 seconds at a 1,024-check cadence and 571.248 seconds
with the original near-limit per-action behavior.

The practical profiles are therefore:

| Purpose | Broad beam | Suffix beam | Normalia time | Processed | Peak RSS |
| --- | ---: | ---: | ---: | ---: | ---: |
| Fast complete check | 20,000 | none | 27.6 s | 223,864 | 316 MB |
| Deep complete search | 50,000 | 50,000 | 182.3 s | 1,792,528 | 624 MB |

All three independent continuation-retention mixes were then run at the deep
profile:

| Lane share | Score | Time | Processed | Generated | Peak RSS |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 0% | 37,527 | 185.443 s | 1,791,076 | 4,645,827 | 657 MB |
| 2% | 37,527 | 187.170 s | 1,792,285 | 4,652,994 | 628 MB |
| 5% | 37,527 | 182.271 s | 1,792,528 | 4,657,776 | 624 MB |

Together they used 554.884 seconds, processed 5,375,889 states, and generated
13,956,597 transitions. Every lane completed all ten suffix batches and found
the same replay-verified route recorded under Normalia in
[`Best.txt`](../Best.txt). Normalia has therefore reached a width and
continuation-retention plateau at this profile. Its next campaign should change
ranking or action structure rather than add timeout to these completed lanes.

## Memory-pressure check

A 10-second run used a 256 MB external cap and 128 MB internal budget. It
peaked at 90 MB RSS. RSS sampling tightened automatically as pressure rose.
The run ended on its time limit rather than an allocation failure or OS kill.

## Rejected prototypes

Two prototypes were removed completely after measurement:

- A persistent per-purchase Sell-resolution cache halved exact resolver calls
  but made Normalia slightly slower and added state-owned cache memory. Cheap
  exact-market cache hits were not the real bottleneck.
- Copy-on-write shared `DeferredGoldFunction` operation vectors improved the
  beam-500 run by about 3%, but reduced the wide 20-second result from 159,830
  to 147,267 processed states and increased memory-scan time from 2.936 to
  4.454 seconds. The contiguous branch-owned vectors remain production.

The next measured bottleneck is continuation-matrix scoring, not memory
enforcement or exact-market key construction.
