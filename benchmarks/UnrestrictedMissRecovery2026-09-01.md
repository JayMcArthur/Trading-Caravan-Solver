# Unrestricted record recovery — 2026-09-01

## Goal

Recover the four records missed by the all-town pruning sweep without forcing
a Merchant day, Witch day, encounter choice, Trader start, or saved-route
prefix. The search must discover the action and encounter structure itself.

The four missed floors were Port-Willam 42,296, Petrinov 45,578, Gayawaku
50,105, and Sanctifan 51,130.

## Successor model

The deferred models already avoid expanding every historical food/cargo split
as a separate outer search state:

- Buy appends one deferred-purchase state;
- Sell appends one sold-ledger state;
- Trader, Camel, Interest, and crew actions append at most one ordinary state
  each; and
- the exact gold, food, and cargo alternatives remain rows inside the deferred
  model until a later action needs to resolve them.

A Sanctifan beam-5,000 run generated 222,486 outer states from 64,112 processed
states, about 3.47 legal action successors per input. It also evaluated 789,404
deferred rows and 1.41 million seed inputs. The remaining cost is therefore
mostly internal symbolic frontier work and beam ranking, not an eager branch
for every possible Buy or Sell quantity.

## Reachability bug

The old nominally unrestricted solver always took Merchant on the first
eligible day and Witch on the first eligible day. It returned immediately
after emitting those event choices, so ordinary actions could not carry an
otherwise valid state to a later eligible encounter day. The known late-event
Port-Willam, Petrinov, and Gayawaku routes were unreachable unless a CLI option
fixed their event days.

`EncounterTiming` now owns the encounter calendar for concrete, deferred-buy,
and sold-ledger states. On an early eligible day the solver emits all encounter
choices and also continues the ordinary player actions from the unchanged
state. On the last eligible day the encounter is mandatory. This does not add
a fake Wait action or a duplicate outer state: it keeps the legal action
successors that the early return previously discarded. Explicit fixed-day
experiments remain available and are never treated as deferrable.

The benchmark JSON exposes `generated_merchant_states`,
`generated_witch_states`, `merchant_deferral_continuations`, and
`witch_deferral_continuations` so this reachability behavior is measurable.

## Automatic refinement

Correct encounter reachability immediately recovered Port-Willam and Petrinov,
but the extra legal branches made beam competition harder for the two narrow
remaining routes. Raw widths and existing diversity/skeleton reservations
plateaued. In both cases, the best broad route differed from the verified route
by only one of its first eight player-controlled action types.

An unrestricted run now performs two general passes by default:

1. Run the ordinary broad search and take its best discovered route. A seeded
   fixture is never used as the refinement template.
2. Run the same unrestricted search again while reserving half of each trimmed
   frontier for distinct early-action skeletons within one mismatch of the
   discovered template.

Encounter actions and Buy quantities are excluded from the skeleton. The
reservation therefore describes a neighborhood learned by the first pass; it
does not prescribe an event day, event option, exact basket, or saved route.
States inside each skeleton are still ranked normally, and reserved slots are
distributed round-robin across the distinct skeleton groups. The better result
of the two passes is returned. `--no-auto-refine` retains the single broad pass
for controlled experiments.

When a wall-clock budget is present, both passes share it. The second pass gets
only the remaining internal time and is skipped if fewer than two seconds
remain. Memory remains sequential: the first and second solvers do not run at
the same time, and all native runs remain under `scripts/run_safe.sh`.

## Results

All measurements were unseeded (`--no-incumbent`), open-start, unrestricted
searches. No run used fixed encounter days/options or a route prefix.

| Town | Earlier sweep | Correct timing | Final method | Final score | Floor |
|---|---:|---:|---|---:|---:|
| Port-Willam | 38,300 | beam 5,000 | broad pass | **42,296** | 42,296 |
| Petrinov | 41,672 | beam 5,000 | broad pass | **45,578** | 45,578 |
| Gayawaku | 46,325 | 50,039 at beam 2,000 | automatic refinement, beam 2,000 | **50,105** | 50,105 |
| Sanctifan | 51,067 | 51,067 at beam 20,000 | automatic refinement, beam 20,000 | **51,130** | 51,130 |

The corrected timing pass also reached 49,628 for Gayawaku at beam 500 and
50,039 at beams 2,000 and 5,000. Increasing width alone did not cross that
plateau. Sanctifan likewise remained at 51,067 at beam 20,000 before automatic
refinement. No new score exceeded an existing verified record.

Representative completed-pass costs were approximately 48 seconds and 215 MB
for Gayawaku's beam-2,000 refinement, and 32 seconds and 321 MB for
Sanctifan's beam-20,000 refinement. The two-pass campaign wall time is the sum
of the broad and refinement passes; JSON reports both the selected-pass
`elapsed_seconds` and total `campaign_elapsed_seconds`.

## Reproduction

The floor checker runs the real unrestricted CLI through the bounded native
wrapper:

```bash
BEAM=2000 MAX_SECONDS=120 PROFILE=strategic \
  ./scripts/check_unrestricted_floor.sh Gayawaku 50105

BEAM=20000 MAX_SECONDS=120 PROFILE=greedy \
  ./scripts/check_unrestricted_floor.sh Sanctifan 51130
```

The replay-only regression is faster and checks every saved action and final
score independently:

```bash
./scripts/run_tests_safe.sh open-town-bot-floors
```

## Next useful optimization

The correctness gap is closed. The highest remaining cost is the number of
internal deferred rows evaluated while ranking each small set of outer action
successors. A future optimization should target exact dominance and caching
inside those symbolic frontiers, with replay equality as its correctness gate,
rather than forcing route structure or increasing raw beam width alone.
