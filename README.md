# Trading Caravan Solver

This branch uses a bounded beam search so the solver can run under WSL without allowing the frontier or purchase-table cache to consume all available memory.

Current plans and unfinished work live in the
[Trading Caravan SilverBullet project](http://localhost:8313/yoss/Projects/TradingCaravan/Plan).
The repository keeps build/run contracts, code, and benchmark evidence.

## Build

Use the bounded build wrapper. It uses one build job, serializes compiler
processes, caps each compiler, and puts a timeout and memory ceiling around the
build driver:

```bash
./scripts/build_safe.sh
```

Release-mode tests keep assertions enabled. Always use the test wrapper:

```bash
./scripts/run_tests_safe.sh
```

The test executable also installs a 768 MB address-space limit, a 120-second
CPU limit, and disables core dumps inside the process. The solver executable
installs its own 1 GB ceiling. A lower wrapper limit wins. This makes an
accidental direct launch memory-bounded, while the wrappers additionally add a
wall-clock timeout. On non-Linux platforms, the executables refuse to start
because this implementation cannot verify an equivalent hard process limit.
The wrappers reject memory-limit overrides above 1 GB, and compiler processes
are capped at 768 MB and serialized even if a caller requests parallel jobs.

The wrappers require a running systemd user manager and cgroup v2. They cap
aggregate job memory (including charged filesystem cache), disable job swap,
limit CPU use to one core and allow at most 64 tasks. The existing per-process
address-space limits remain in force. Nested wrappers stay in the outer job,
whose memory ceiling is shared by all descendants. If containment cannot be
established, the command fails before starting the job.

Verify the actual kernel limits with `./scripts/run_tests_safe.sh safety-limits`.
CMake configuration and compilation both use the bounded wrapper.
The background campaign launcher also contains its Python supervisor, database
inspection and solver descendants in one job. Its total deadline defaults to
600 seconds (`CAMPAIGN_MAX_SECONDS`), with a 1,024 MB aggregate memory ceiling
(`CAMPAIGN_MAX_MB`). Use a finite batch such as `--max-sessions 1 --max-seconds
480` to leave time for reporting before that deadline. Do not launch a Python
campaign supervisor outside `run_limited.sh`.

## Safe solver run

Use the wrapper instead of invoking the executable directly:

```bash
./scripts/run_safe.sh
```

The wrapper applies two independent limits:

- `MAX_MB` caps total job memory and each process's virtual address space.
- `INTERNAL_MB` is the solver's earlier, graceful-stop budget and must be smaller than `MAX_MB`.
- `ARRAY_LIMIT` is the retained beam width. Candidate accumulators apply exact
  reduction on insertion and apply the lossy beam whenever they reach twice
  this width.
- `MAX_SECONDS` sets a positive wall-clock deadline and defaults to 180 seconds.

Examples:

```bash
# Fast exploratory run
MAX_MB=512 INTERNAL_MB=384 ARRAY_LIMIT=1000 MAX_SECONDS=120 ./scripts/run_safe.sh

# High-quality bounded run
MAX_MB=1024 INTERNAL_MB=768 ARRAY_LIMIT=100000 MAX_SECONDS=300 ./scripts/run_safe.sh --benchmark-json
```

Lazy purchase-tree loading is the default. Version-2 files on disk are still roughly 93–114 MB, but they are streamed into compact per-weight cost breakpoints that generally occupy 3–8 MB in memory. Up to 12 relevant worlds can therefore remain cached without the previous reload thrash. Newly written version-3 trees use the compact format on disk as well. `--warm-trees` remains unnecessary for normal runs.

Select any town by display name, punctuation-insensitive alias, or numeric ID:

```bash
MAX_SECONDS=10 ./scripts/run_safe.sh --list-towns
ARRAY_LIMIT=20000 ./scripts/run_safe.sh --town Ipartus --benchmark-json
```

Normal runs explore every legal day-one action. `--starting-trader` locks the
historical day-one Trader prefix for reproducible narrow experiments;
`--no-starting-trader` remains a compatibility alias for the open default.
`--no-merchants` and `--no-witches` isolate event-free variants.

For beam-free verification work, `--proof-search` uses exact best-ordered
depth-first expansion and a bounded transposition table instead of beam
trimming. Set its maximum retained table entries with
`--proof-transposition-limit N`. A time- or memory-stopped run explicitly
reports no certificate; `proof_top_three_certified` becomes true only when the
entire DFS stack is exhausted with three finished routes retained. The current
admissible score bound is deliberately conservative, so this mode is presently
most useful for small-horizon proofs and bounded campaign development.

For exact work that must accumulate across sessions, `--search-map FILE` uses
a disk-backed state graph. Expanded partial states are never regenerated;
later sessions expand only still-open descendants. A state is skipped as proven
only after every legal descendant is solved. The hot hash/status index is
separate from state snapshots and reverse-linked route traces, and hash hits are
confirmed against the full semantic key. `--search-map-order beam` applies the
normal continuation-aware beam selector as a persistent day-layer scheduler;
states outside `ARRAY_LIMIT` remain open instead of being deleted. Use
`--search-map-order depth` for deepest-first suffix closure. See
[`docs/persistent-exact-search.md`](docs/persistent-exact-search.md) for the
invariants, storage layout, commands, and initial Normalia measurements.

Merchant and Witch opportunities are unrestricted by default. On an early
eligible day, search explores every event choice and the ordinary action
continuations that can reach a later eligible day. The final eligible day is
mandatory. This is calendar behavior, not a generated Wait action.

Open-timing search runs an automatic second pass by default, including searches
with a pinned Merchant or Witch option. It learns the best early action-type
skeleton discovered by the broad pass, then reserves part of the same beam for
nearby distinct skeletons. It never learns from the seeded route and does not
fix encounter days, choices, Buy quantities, or an action prefix. Fixed-day and
saved-prefix experiments remain single-pass. Both passes share the configured
time budget and run sequentially under the same native memory guard. Use
`--no-auto-refine` for a single-pass experiment. `--refinement-share` controls
the reserved fraction of the second-pass beam, and `--refinement-mismatches`
controls how many of the first eight player-action types may differ from the
learned route.

The recovered Merchant, Witch, and all 12 Traveler disaster mutations—including
their exact `=`, `+=`, `-=`, and `*=` behavior—are documented in
[`docs/event-code-parity.md`](docs/event-code-parity.md). Fixed disaster
scenarios are represented on `Player` by `disaster_day` plus
`scheduled_disaster`; the solver applies the result automatically instead of
treating a random disaster as a player choice.

## Normalia controlled experiments

These historical experiment rows are unseeded discoveries using an explicitly
fixed Normalia initial-Trader prefix. Current normal runs use the open start by
default. A seeded score is only a retained floor; use `--no-incumbent` when
measuring search quality.

| Purchase profile | Beam 1,000 | Beam 20,000 | Beam 100,000 |
| --- | ---: | ---: | ---: |
| `greedy` | 25,423 | 34,806 | 35,054 |
| `scenario` | 26,528 | 34,806 | 35,037 |
| `strategic` | 26,528 | 34,806 | 35,037 |
| `wide` | **27,552** | 33,248 | 34,806 |

The wide store wins under a tight beam but loses as the beam grows because extra candidates compete for the same retained slots. A separate pruning sweep found the best Normalia result with the simpler `greedy` store and only 25% of slots chosen by the optimistic bound:

```bash
MAX_MB=1024 INTERNAL_MB=768 ARRAY_LIMIT=100000 MAX_SECONDS=420 \
  ./scripts/run_safe.sh --town Normalia --no-incumbent \
  --purchase-profile greedy --optimistic-share 25 --benchmark-json
```

That historical run independently discovered **35,114** at 170 MB peak RSS.
The continuation-aware deferred search discovers **37,478** with an open start
at beam 500. A manual refinement—Trader on day 7, Camel on day 8, and one extra
pottery in the day-12 Buy—replays at **37,527** and is now the seeded Normalia
fixture. The complete historical matrices and interpretation are in
`benchmarks/NormaliaExperiments.md`; the current redesign is measured in
[`benchmarks/DeferredSearchOptimization2026-09-01.md`](benchmarks/DeferredSearchOptimization2026-09-01.md).

The current 37,527-point route and the previous 34,848-point route are independently simulated as regression benchmarks:

```bash
./scripts/run_tests_safe.sh best-route
./scripts/run_tests_safe.sh known-route
```

The solver verifies and seeds the 37,527-point recorded incumbent at startup,
so bounded production runs retain it. JSON reports `incumbent_score`,
`discovered_score`, and `best_source` separately. `--optimistic-share` keeps the
score/bound split available as an explicit experiment; continuation tickets now
give unresolved Buy and sold-ledger states independent immediate-action ranking
views without forcing the next action. Those views are stored as a five-column
continuation matrix (Sell, Trader, Camel, Interest, and food-only Buy). By
default 5% of the beam protects the independent columns while every owner is
stored only once. `--continuation-lane-share N` changes that fraction.

## Scores above recorded baselines

The multi-town runner found and independently verified routes above the historical/local scores recorded in `Best.txt`. These are not claims of global optimality.

| Town | Previous recorded baseline | New verified score | Margin | Discovery beam |
| --- | ---: | ---: | ---: | ---: |
| Normalia | 35,114 | **37,527** | +2,413 | manual refinement of beam-500 route |
| Mahaji | 35,211 | **37,589** | +2,378 | Cleppsydra-only 2,000 hybrid |
| Fang-Mo | 35,163 | **37,569** | +2,406 | weighted-pottery 50,000 pinned/open sweep |
| Moonin | 36,732 | **37,584** | +852 | exact-replay polishing |
| Tocaccialli | 46,083 | **49,634** | +3,551 | 5,000 |
| Bajaar | 49,827 | **56,147** | +6,320 | 500 |
| Doba Lao | 44,631 | **45,543** | +912 | Cleppsydra-only 2,000 hybrid |
| Huangoz | 35,114 | **37,527** | +2,413 | Cleppsydra-only 2,000 hybrid |
| Port-Willam | 42,296 | **42,296** | tied | unrestricted 5,000 |
| Mehaz | 41,853 | **48,738** | +6,885 | 500 |
| Eduming | 80,043 | **81,321** | +1,278 | 500 |
| Kifuai | 36,589 | **37,547** | +958 | Cleppsydra-only 2,000 hybrid |
| Ipartus | 36,295 | **37,734** | +1,439 | Cleppsydra-only 2,000 hybrid |
| Cornecopea | 43,696 | **43,881** | +185 | Cleppsydra-only 2,000 hybrid |
| Skjollird | 43,446 | **47,065** | +3,619 | Cleppsydra-only 2,000 hybrid |
| Petrinov | 44,447 | **45,578** | +1,131 | unrestricted 5,000 |
| Jilliqo | 37,291 | **37,819** | +528 | Cleppsydra-only 2,000 hybrid |
| Vilparino | 36,631 | **37,569** | +938 | Cleppsydra-only 2,000 hybrid |
| Sanctifan | 51,130 | **55,728** | +4,598 | exact-replay route polish |

Fang-Mo's starting Pottery now uses ordinary weighted inventory. This corrected
the prior weightless 37,577 route to a replay-verified 37,569; diagnosis and
six deep confirmation lanes are in
[benchmarks/FangMoWeightedPotteryCorrection2026-09-02.md](benchmarks/FangMoWeightedPotteryCorrection2026-09-02.md).

Verify these routes without running a search:

```bash
./scripts/run_tests_safe.sh mahaji-record
./scripts/run_tests_safe.sh moonin-record
./scripts/run_tests_safe.sh mehaz-record
./scripts/run_tests_safe.sh ipartus-record
./scripts/run_tests_safe.sh doba-record
./scripts/run_tests_safe.sh cornecopea-record
./scripts/run_tests_safe.sh new-town-records
```

Sanctifan now has a replay-verified 55,728 bot route, 4,598 above its prior
51,130 human record. Vilparino and Kifuai also exceed their prior records. The
Cleppsydra-only all-town sweep and every forced-route result are recorded in
[`benchmarks/ClepsydraAllTownSweep2026-09-01.md`](benchmarks/ClepsydraAllTownSweep2026-09-01.md).
Matching pinned sweeps for Sturdy Saddle and Blue Treasure found no new records:
Sturdy tied Tocaccialli, while Blue tied L'Exquise, Bajaar, and Petrinov. The
complete 44-run matrix is in
[`benchmarks/AlternativeMerchantAllTownSweeps2026-09-01.md`](benchmarks/AlternativeMerchantAllTownSweeps2026-09-01.md).
The follow-up Blue Sanctifan search-policy matrix, pinned learned-refinement
support, and Route-to-Mahaji all-town sweep are documented in
[`benchmarks/NextSearchExperiments2026-09-01.md`](benchmarks/NextSearchExperiments2026-09-01.md).
The bounded exact-replay route polisher, its 126-point Sanctifan improvement,
and the 1.8-million-candidate Normalia plateau check are documented in
[`benchmarks/RoutePolishing2026-09-02.md`](benchmarks/RoutePolishing2026-09-02.md).
The original deferred-purchase sample
is in [`benchmarks/DeferredPurchaseResults.md`](benchmarks/DeferredPurchaseResults.md),
and the wider sold-ledger campaign is in
[`benchmarks/WideRecordSweep2026-08-31.md`](benchmarks/WideRecordSweep2026-08-31.md).

Exact Sell frontiers are cached by a 256-gold ceiling bucket and weight. Each
candidate is still checked against its exact symbolic gold budget. This cut the
same Skjollird 500-state search from 40.5 seconds to 5.6 seconds without changing
its generated states or 43,475 result. Final-day deferred Buy resolution also
uses only the minimum required food because food has no final point value. The
same rule now applies to a final-day Sell, and terminal resource frontiers send
only their best three scoring rows to replay. On the focused Gayawaku suffix,
those terminal changes reduced recorded finalists from 17,453 to 2,988 and
runtime from 2.59 to about 2.08 seconds. The remaining cost is primarily
iterating large cached cargo/food frontiers, not loading the exact-sale cache.

Gayawaku's route is now **50,105**, one point above the recorded 50,104.
Port-Willam ties **42,296**, Petrinov reaches **45,578**, and Sanctifan reaches
**55,728**. Their saved fixtures are reachable by bounded search without fixing
encounter days. The search chooses their encounter timing rather than receiving it as a
parameter. The separate dr3xis Bajaar 56,146 claim remains unverified because
its route and no-ad-continue provenance are unknown; the solver now has a
replay-verified **56,147** route without using an ad continuation.
Verify them with:

```bash
./scripts/run_tests_safe.sh petrinov-improved
./scripts/run_tests_safe.sh open-town-bot-floors
```

Historically, `--diversity-share` reserved an experimental part of the beam for distinct
crew, encounter, cargo-phase, and interest-rate structures. It defaults to zero
and remains an alternate search lane rather than the default. In the open-start
all-town beam-5,000 sweep, a 25% reservation was the only tested policy to
recover the then-current Bajaar 49,827 and Mehaz 48,738 routes. Across all five
pruning policies, the original sweep matched 18 of 22 verified records without
setting a new one. The later unrestricted-search correction matches all four
misses. The original matrix and follow-up are in
[`benchmarks/AllTownPruningSweep2026-09-01.md`](benchmarks/AllTownPruningSweep2026-09-01.md).
The diagnosis, automatic refinement algorithm, performance, and reproduction
commands are in
[`benchmarks/UnrestrictedMissRecovery2026-09-01.md`](benchmarks/UnrestrictedMissRecovery2026-09-01.md).

`--skeleton-share N` is a second experimental reservation based on the first
player-controlled action types; `--skeleton-depth 1-8` controls its horizon.
Hybrid suffixes can be ordered with `--hybrid-skeleton-order
score|cohort|interleave|structure-interleave`. Widths through 50,000, the first
three orders, and depths
4/6/8 did not improve the historical Bajaar 49,827 route. Keep these disabled unless running
a deliberate search experiment. Results are in
[`benchmarks/SkeletonSearchResults.md`](benchmarks/SkeletonSearchResults.md).

The exact Bajaar/Midas interaction, current route score decomposition, focused
prefix searches, and 1.69-million-state suffix campaigns are documented in
[`benchmarks/BajaarTargetAnalysis.md`](benchmarks/BajaarTargetAnalysis.md).
Recovered code confirms that Bajaar doubles the initial Trader rate before
Midas adds 150; the solver's current 190 final rate is exact. Generic and
continuation-structure schedules both plateaued at 49,827. Continuation-aware
ranking later found the replay-verified 56,147 route at beam 500, superseding
that search conclusion.

Skeleton-queue draining is automatic under actual memory pressure after the
normal Merchant/Witch decision window. It counts live shared deferred
allocations once, keeps the ordinary broad beam while memory is comfortable,
then groups the retained frontier by exact early-action/world skeleton. One
family at a time receives the original full beam and runs to completion; its
states never compete or deduplicate against a sister family. Waiting states
retain their full deferred Buy models and trace provenance in-process, while
completed families keep only their concrete replayable routes.

Family order is continuation-matrix-aware: the queue aggregates realistic,
optimistic, Sell, Trader, Camel, Interest, and food-only Buy tickets while
storing each family once. The reported optimistic family value is a scheduling
heuristic, not a proven ceiling for latent deferred allocations, so it is never
used to prune. JSON reports observed ceiling violations and explicitly records
`skeleton_queue_ceiling_pruning=false`.

`--skeleton-queue-day D` forces the queue at a particular day for experiments;
zero means the default automatic memory trigger. `--no-skeleton-queue` restores
the legacy continuation-structure pressure batches. Explicit legacy batches
remain available through `--hybrid-batch-size N`, `--hybrid-split-day D`, and
`--hybrid-suffix-limit M`.

This is a heuristic scheduling change: suffix states in different families no
longer compete or deduplicate globally. Raising the internal memory budget so
pressure is not reached preserves the historical broad-beam behavior. The
current explicit high-coverage Bajaar experiment is:

```bash
MAX_MB=768 INTERNAL_MB=640 ARRAY_LIMIT=20000 MAX_SECONDS=600 \
  ./scripts/run_safe.sh --town Bajaar --purchase-profile greedy \
  --hybrid-batch-size 500 --hybrid-suffix-limit 5000 \
  --hybrid-split-day 8 --no-incumbent --benchmark-json
```

That run processed 1.70 million states at 173 MB and reproduced 49,827. Full
measurements and tradeoffs are in
[`benchmarks/HybridFrontierResults.md`](benchmarks/HybridFrontierResults.md).
The continuation-matrix calibration, skeleton-queue measurements, and current
all-town screen are in
[`benchmarks/ContinuationMatrixSkeletonQueue2026-09-02.md`](benchmarks/ContinuationMatrixSkeletonQueue2026-09-02.md).

`--incumbent-prefix-actions N` replays the first `N` actions of that town's
verified fixture and searches every suffix from the resulting state. This is a
bounded local-improvement lane; JSON reports `prefix_actions=N`, and the saved
fixture remains the seeded floor unless `--no-incumbent` is also supplied.

`--merchant-option NAME` gives one Merchant choice the whole beam instead of
making it compete with all other Merchant branches. Benchmark JSON records the
constraint as `merchant_option`; omit it for the normal unconstrained search.
`--witch-option NAME` provides the same isolated lane for one Witch choice.
`--merchant-day 5|6|7` and `--witch-day 9|10|11` model a fixed realized
classical encounter schedule. Omit them for normal unrestricted encounter
timing. Fixed days cannot be combined with a saved-route prefix and disable the
automatic refinement pass because they are controlled experiments.
Run the bounded timing matrix for the four open record towns with
`BEAM=500 MAX_SECONDS=90 ./scripts/run_encounter_day_experiments.sh`.

The purchase generator now emits baskets optimized for both current prices and reachable future sell-price worlds. This captures buying spice before Route to Mahaji and avoids valuing held marble or jewelry as if Midas or Time is Money could not occur. A focused regression previously emitted at most 21 spice; the projected world correctly emits the 90-spice basket under the same gold and capacity.

Compact store rows also expose an exact three-choice portfolio: maximum profit, the cheapest basket retaining at least 90% of that profit, and the cheapest retaining at least 75%. Measured pruning enables these reserve-rich alternatives only when interest is persistent (`set_interest`). This raised Petrinov's 1,000-beam result from 32,238 to 41,474 while restoring ordinary trade towns to their original branch counts and scores. The primary-source design research is in `docs/purchase-candidate-research.md`.

`--benchmark-json` suppresses status output and emits one machine-readable result containing separate seeded/discovered scores, route source, completion status and stop reason, prefix length, purchase, beam, and hybrid-search profiles, elapsed time, peak RSS, state counts, beam prunes, purchase-tree misses, and phase timings for deferred expansion, compression, beam trimming, matrix scoring, and queue scheduling. `scripts/run_normalia_experiments.sh` runs profile/beam/start/share matrices sequentially under the same memory guard.

Exact state reduction now runs before lossy beam selection. Future-equivalent
states therefore cannot consume separate beam slots. Concrete dominance is
limited to truly monotone resources: cargo must match, and food must also match
unless it is weightless. Deferred Buy and sold-ledger states are partitioned by
allocation shape and use exact pointwise gold-envelope dominance. Benchmark
JSON separates `pre_beam_duplicate_prunes` and
`pre_beam_dominance_prunes` from `beam_prunes`, reports purchase/sold deferred
prunes and gold-envelope work separately, and reports the pass's cost as
`frontier_reduction_ms`. The proof, tests, and measured record recoveries are in
[benchmarks/DeferredDominance2026-09-03.md](benchmarks/DeferredDominance2026-09-03.md).

Use `--audit-known-route` with `--benchmark-json` to observe where the saved
route's action branch stops surviving:

```bash
ARRAY_LIMIT=500 MAX_SECONDS=120 ./scripts/run_safe.sh \
  --town Normalia --no-incumbent --no-auto-refine \
  --continuation-lane-share 5 --audit-known-route --benchmark-json
```

The audit is observational and never protects or forces the route. Its JSON
reports each matched action prefix before exact reduction, after exact
reduction, after dominance, after the beam, and at completion, plus the branch's
best realistic, optimistic, and five continuation-lane ranks. A deferred Buy
matches the Buy action without claiming that a particular quantity allocation
survived; `mode="symbolic-action-branch"` and
`buy_quantities_deferred=true` make that boundary explicit. Exact equality or
dominance may transfer the audit witness to a future-equivalent stronger trace;
terminal `finished` counts are rebuilt from the actual concrete trace. Design, test, and
performance results are in
[benchmarks/FrontierReductionRouteAudit2026-09-03.md](benchmarks/FrontierReductionRouteAudit2026-09-03.md).

The Classical solver has a built-in cargo-buy checker: after a Buy containing
trade cargo, another Buy cannot be generated until a Sell. Food-only Buys before
the cargo Buy remain legal. `Wait (empty Buy)` remains representable for exact
game replay, but normal search does not generate it: a 22-town screen explored
138,504 Wait branches without placing one in a winning route.
`--experimental-sell-aware-beam` remains available as a ranking experiment. See
[benchmarks/ClassicalBuyExperiments.md](benchmarks/ClassicalBuyExperiments.md)
for the cross-town results, new verified routes, and bounded depth-first
follow-on design.

Continuation-based food accounting is now the production path. One unresolved
Buy stores exact historical spend/gold curves and accumulated food use;
immediate Sell, Trader, Camel, Interest, and legal food-only Buy interpretations
rank that owner independently without becoming forced actions. Origin selection
is generated from interval breakpoints, and food above the conservative maximum
future demand is discarded. Weighted food can make each integer a real carrying
capacity breakpoint, so those values remain exact rather than being sampled.
The original logic prototype is
[benchmarks/prototypes/food-continuation-generator.html](benchmarks/prototypes/food-continuation-generator.html),
the remaining record/proof work is tracked in the
[current plan](http://localhost:8313/yoss/Projects/TradingCaravan/Plans/RecordAndProofWork20261015), and
the implemented measurements and decisions are in
[benchmarks/DeferredSearchOptimization2026-09-01.md](benchmarks/DeferredSearchOptimization2026-09-01.md).
The follow-up deferred-expansion pass made memory checks adaptive while keeping
frequent RSS sampling and the same external cap. A complete Normalia
beam-20,000 search now finishes in 27.6 seconds at 316 MB and rediscovers
37,527. The recommended deep profile uses a 50,000 broad beam with
structure-interleaved day-12 suffix batches; it completes in about three
minutes at 624 MB while processing 1.79 million states. Accepted changes,
commands, and rejected cache/storage prototypes are documented
in [benchmarks/DeferredExpansionPerformance2026-09-02.md](benchmarks/DeferredExpansionPerformance2026-09-02.md).
On Normalia, the 0%, 2%, and 5% continuation-retention variants form a roughly
nine-minute campaign; they collectively processed 5.38 million states under
657 MB, all tying the 37,527 record.
The subsequent all-town campaign completed three lanes for every town, processed
111.87 million states, and matched every saved bot score. Gayawaku requires a
20,000-state broad beam with the full 50,000-state suffix; all other towns used
50,000 for both. Full results and the phase-cost ranking are in
[benchmarks/AllTownDeepCampaign2026-09-02.md](benchmarks/AllTownDeepCampaign2026-09-02.md).
The related [deferred food/gold flex design](docs/FoodGoldFlexDesign.md) explores
holding several food thresholds inside one symbolic state and resolving the
original Buy only when the action path determines the minimum survivable food.
The full follow-on design is the
[deferred purchase value bucket](docs/DeferredPurchaseValueBucket.md): one
unresolved Buy marker, an exact-spend/current-gold curve for Interest and later
costs, and cargo resolution at Sell.

The interactive
[lazy sold-bucket prototype](benchmarks/prototypes/sold-bucket-options-prototype.html)
compares eager Sell resolution, continuation thresholds, a fully lazy Sell
marker, and a hybrid with one exact beam-ranking representative. It keeps the
Sell event immediate while allowing the historical food quantity to remain
symbolic.

Three follow-up logic experiments test the production shape more deeply:
[demand ledger](benchmarks/prototypes/sold-demand-ledger-prototype.html),
[sparse thresholds](benchmarks/prototypes/sold-threshold-curve-prototype.html),
and [next-Buy prefix convolution](benchmarks/prototypes/sold-next-buy-convolution-prototype.html).
Their combined verdict and the recommended clean module seam are in
[SoldBucketPrototypeDecision.md](docs/SoldBucketPrototypeDecision.md).
The demand ledger, terminal top-three query, and next-Buy feasible-range
envelope are now the production path. The eager post-Sell resource frontier was
deleted. Controlled A/B timings, compaction counters, and the complete
beam-500 town screen are in
[SoldDemandLedgerResults.md](benchmarks/SoldDemandLedgerResults.md).

## Why the old run crashed

The old memory estimate counted only logical solver-state sizes. It omitted vector capacities, compression hash tables, and all loaded purchase tables. It could report 11 MB while the process was already near 473 MB RSS. `array_limit` was also not applied by the layered solver, and startup eagerly warmed many ~100 MB tables.

The bounded version:

- measures real process RSS and keeps 10% headroom;
- compresses repeated purchase-table answers into cost breakpoints and includes their capacity in estimates;
- enforces `array_limit` as a deterministic beam;
- groups frontier states by world before expansion and caches the 12 relevant compact worlds;
- catches `std::bad_alloc` as a final diagnostic safety net; and
- stops on an internal deadline two seconds before the wrapper deadline so a
  bounded run can emit its incumbent and diagnostic JSON; and
- provides an external `prlimit` wrapper so internal accounting is not the last line of defense.
