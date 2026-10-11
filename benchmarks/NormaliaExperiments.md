# Normalia unseeded search experiments

These measurements use Normalia because the town itself has no special modifier. Every score is `discovered_score` from a run with `--no-incumbent`; no recorded route can mask a weaker search.

All runs were sequential under `scripts/run_safe.sh`, with a 1,024 MB address-space cap and a 768 MB internal budget unless noted. Peak RSS at beam 100,000 stayed between 166 and 171 MB.

## Purchase-store sweep

The beam ranking used its original 75% optimistic / 25% cycle-aware split.

| Profile | Candidate policy | Beam 1,000 | Beam 20,000 | Beam 100,000 |
| --- | --- | ---: | ---: | ---: |
| `greedy` | Current-price maxima at reserve milestones | 25,423 | 34,806 | **35,054** |
| `scenario` | Add maxima for reachable merchant/witch price worlds | 26,528 | 34,806 | 35,037 |
| `strategic` | Scenario plus near-max anchors for persistent interest | 26,528 | 34,806 | 35,037 |
| `wide` | Near-max reserve anchors in every state | **27,552** | 33,248 | 34,806 |

The wider store is useful when the beam is extremely tight, but its extra candidates compete away better long-run paths at larger widths. Higher candidate quality and higher end score are therefore separate questions.

## Beam-ranking sweep

These runs use `greedy`, a forced initial Trader, and vary the percentage retained by the optimistic final bound. Remaining slots use the cycle-aware score.

| Optimistic share | Beam 20,000 | Beam 100,000 |
| ---: | ---: | ---: |
| 0% | 33,656 | — |
| 25% | 34,806 | **35,114** |
| 50% | 34,018 | — |
| 75% | 34,806 | 35,054 |
| 100% | 34,806 | 34,848 |

The relationship is non-monotonic. Pure optimism exactly rediscovered the old 34,848 route at beam 100,000, while the more cycle-aware 25% mix preserved a different development path and improved the recorded Normalia score to 35,114.

## Starting-prefix control

At beam 20,000, an open start and the forced-Trader start both reached 34,806; the open solver chose Trader first itself. At beam 100,000 with `greedy` and 75% optimism, the forced prefix scored 35,054 and the open start scored 35,037. The prefix concentrates capacity but does not fabricate the route score.

## Reproduce the best result

```bash
MAX_MB=1024 INTERNAL_MB=768 ARRAY_LIMIT=100000 MAX_SECONDS=420 \
  ./scripts/run_safe.sh --town Normalia --no-incumbent \
  --purchase-profile greedy --optimistic-share 25 --benchmark-json
```

The independently simulated route is protected by `./scripts/run_tests_safe.sh best-route`.

## Cross-town check: Skjollird

The Normalia winner was not treated as a universal default. On Skjollird, 25% optimism improved the 20,000-state result from 39,830 to 40,017, but at beam 100,000 it fell to 40,077 versus the previously discovered 43,085. Strategic-store runs at 65% and 85% both returned 43,085, the same score found at 75%, and did not exceed the repository's recorded 43,446.

This is direct evidence that a configuration must be compared at the intended town and beam width. Small-beam gains are useful screening signals, not reliable predictions of the wide-beam endpoint.
