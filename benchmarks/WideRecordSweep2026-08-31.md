# Wide normal-mode record sweep — 2026-08-31

All native runs used `scripts/run_safe.sh` (directly or through an experiment
script), a 768 MB external cap, a 640 MB internal solver budget, and a 180-second
per-process timeout. Results are bounded discoveries, not optimality proofs.

## Broad screens

| Screen | Towns | Outcome | Peak observed RSS |
| --- | --- | --- | ---: |
| Beam 5,000, greedy, 25% optimistic | all 22 | Five new records; Gayawaku stopped safely on its internal estimate | 198 MB actual for stopped Gayawaku; 111 MB for completed ordinary towns |
| Beam 20,000, greedy, 25% optimistic | all except Gayawaku | Repeated all floors and found Mehaz 48,738 | 244 MB |

The beam-20,000 screen processed roughly 178,000-212,000 states per ordinary
town and normally completed in 11-24 seconds. Increasing width did not improve
Bajaar or Petrinov's generic lanes, but it exposed Mehaz's distinct five-sale
route.

## Verified improvements

| Town | Previous bot | New bot | Gain | Search lane |
| --- | ---: | ---: | ---: | --- |
| Fang-Mo | 34,918 | 35,163 | +245 | unrestricted beam 5,000 |
| Tocaccialli | 46,083 | 49,634 | +3,551 | unrestricted beam 5,000 |
| Huangoz | 34,848 | 35,114 | +266 | unrestricted beam 5,000 |
| Port-Willam | 41,344 | 42,296 | +952 | Magic day 7, Midas day 9, beam 5,000 |
| Cornecopea | 43,696 | 43,713 | +17 | unrestricted beam 5,000 |
| Petrinov | 41,672 | 45,578 | +3,906 | Blue Treasure day 7, Oasis day 9, beam 5,000 |
| Jilliqo | 37,291 | 37,759 | +468 | unrestricted beam 5,000 |
| Mehaz | 46,432 | 48,738 | +2,306 | unrestricted beam 20,000 |

Every row above is stored in `benchmarks/RecordRoutes.cpp` and independently
replayed by the regression suite.

## Remaining gap

Bajaar is now the only town below its non-cheat historical target:
49,827 versus 56,146. The following failed to improve 49,827:

- generic beams 5,000 and 20,000;
- optimism shares 0%, 25%, 50%, 75%, and 100%;
- legacy and Sell-aware beam scoring;
- fixed Merchant days 5-7 and Witch days 9-11 for Saddle/Midas;
- every Merchant option at day 7 with Midas day 9;
- every Witch option at day 7/9 with Sturdy Saddle;
- every Witch option at the strongest day-5 Saddle lane.

A residual beam-50,000 run used a 704 MB internal estimate budget beneath the
768 MB external cap. It stopped safely before producing a finalist: 257 MB
actual peak RSS, 633 MB current estimate, and the next layer projected beyond
the internal budget. The completed beam-20,000 lane is therefore the widest
safe generic run with the present trace representation.

At beam 20,000, structural-diversity shares of 20% and 40% both reproduced
49,827. They completed in 30.3 and 36.8 seconds at 238 and 228 MB peak RSS,
respectively. Diversity increased terminal coverage but did not uncover a new
route skeleton.

The next useful search fork is a bounded bank of distinct early action
skeletons followed by prefix-guided suffix searches. A true branch-and-bound
DFS is not yet safe because the current final-value upper bound has not been
proven admissible and exact Buy coverage is not certified.

A subsequent hybrid-frontier implementation completed the previously stopped
width-50,000 search by retaining a compact waiting frontier and finishing
bounded suffix batches. The best safe high-coverage Bajaar run processed 1.70
million states in 260 seconds at 173 MB RSS and still reproduced 49,827. This
strengthens the early-skeleton conclusion: ordinary beam-ranked breadth is no
longer the main untested axis. See
[`HybridFrontierResults.md`](HybridFrontierResults.md) for the full matrix.

The implementation seam is `GameSolver::trim_to_beam_limit`: its current
experimental diversity key describes only the current crew, interest, event,
and cargo phase. It does not distinguish two different early action orders that
converge on those fields. A skeleton-bank prototype should add a compact rolling
signature for the first 6-8 non-event actions to `SolverState`, reserve a small
number of survivors per signature at a chosen cutoff layer, and continue those
states in-process. Keeping the deferred state in-process avoids prematurely
materializing symbolic Buy quantities. This must remain opt-in until an A/B run
proves that all existing record fixtures remain reachable.
