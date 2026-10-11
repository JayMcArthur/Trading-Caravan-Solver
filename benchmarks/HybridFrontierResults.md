# Hybrid frontier search — 2026-08-31

All native benchmarks used `scripts/run_safe.sh`. The main campaign used a
768 MB external hard cap and 640 MB internal budget; the two width-50,000
day-9 handoff runs used a 760 MB internal estimate threshold while retaining
the same 768 MB external hard cap. Results are replay-verified discoveries,
not optimality proofs.

## Search model

The ordinary solver keeps global beam competition through every day. Hybrid
mode leaves that behavior unchanged until a completed frontier reaches a
forced split day or predicts memory pressure. It then:

1. keeps the unprocessed frontier in-process with deferred Buy provenance;
2. moves a bounded prefix batch into the existing layered solver;
3. finishes that batch to terminal states;
4. retains concrete top routes and compacts away finished trace nodes; and
5. repeats until every retained prefix has been visited.

`--hybrid-batch-size` controls resident prefix inputs. The independent
`--hybrid-suffix-limit` controls how wide one batch may regrow. When omitted,
the suffix limit equals the batch size. Different batches intentionally do not
compete or deduplicate globally, so this remains opt-in.

## Bajaar measurements

| Broad beam | Split | Input batch | Suffix beam | Score | Processed | Time | Peak RSS |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5,000 | 7 | 1,000 | 1,000 | 45,667 | 60,587 | 6.6 s | 70 MB |
| 5,000 | 9 | 500 | 500 | **49,827** | 60,281 | 6.4 s | 68 MB |
| 5,000 | 9 | 1,000 | 1,000 | **49,827** | 60,230 | 6.0 s | 67 MB |
| 5,000 | 9 | 2,500 | 2,500 | **49,827** | 60,202 | 5.5 s | 74 MB |
| 5,000 | 11 | 1,000 | 1,000 | 45,835 | 60,034 | 5.6 s | 70 MB |
| 20,000 | 8 | 500 | 500 | **49,827** | 219,182 | 39.5 s | 102 MB |
| 20,000 | 9 | 500 | 500 | **49,827** | 217,643 | 37.0 s | 124 MB |
| 20,000 | 9 | 2,000 | 2,000 | **49,827** | 217,580 | 30.5 s | 124 MB |
| 20,000 | 9 | 5,000 | 5,000 | 46,358 | 217,259 | 27.3 s | 124 MB |
| 50,000 | automatic day 8 | 5,000 | 5,000 | 45,835 | 516,114 | 68.0 s | 190 MB |
| 50,000 | 9 | 500 | 500 | **49,827** | 510,218 | 138.7 s | 277 MB |
| 5,000 | 9 | 500 | 5,000 | **49,827** | 405,043 | 52.5 s | 164 MB |
| 20,000 | 8 | 500 | 5,000 | **49,827** | 1,702,746 | 260.1 s | 173 MB |

The prior width-50,000 breadth-only lane stopped safely before producing a
finalist. Hybrid mode completed all 49,426–49,408 retained prefixes. Small
suffix batches preserved the known record; larger batches and some later
cutoffs did not. More raw state coverage did not improve Bajaar beyond 49,827.

## Gayawaku provenance stress

The fixed Magic-Cleppsydra-day-7/Midas-day-9 route stresses weightless food and
large deferred models.

| Broad beam | Split | Input batch | Suffix beam | Score | Time | Peak RSS | Max deferred rows |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 500 | 9 | 50 | 50 | 50,039 | 12.9 s | 78 MB | 592 |
| 500 | 10 | 50 | 50 | 49,174 | 14.9 s | 44 MB | 576 |
| 500 | 9 | 50 | 500 | **50,105** | 134.2 s | 111 MB | 651 |

Letting each small batch regrow to the original suffix width recovered the
exact 50,105 record and its replay. This validates deferred provenance across
batch compaction, while showing the expected time/coverage tradeoff.

## Conclusions and next fork

- Hybrid draining solves the peak-memory scheduling problem without an unsafe
  score bound or guessed concrete Buy.
- It can visit far more states at bounded memory, but is not score-monotonic.
- Bajaar's plateau after 1.70 million processed states makes ordinary raw
  breadth a weak next investment.
- A follow-up reserved states by the first 4, 6, or 8 non-event actions and
  tested score, cohort, and interleaved suffix ordering through width 50,000.
  None improved 49,827; see `SkeletonSearchResults.md`. The next question is
  target feasibility under the exact scoring and rule model, not more generic
  action-family breadth.
- The next speed experiment should profile trace remapping and repeated
  cross-batch deferred Sell work. A bounded transposition cache is attractive,
  but it must retain replayable suffix recipes and exact deferred semantics.
