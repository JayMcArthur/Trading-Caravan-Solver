# Pruning variety and solved-suffix cache prototype

Date: 2026-09-04

## Question

Can repeated bounded runs deliberately retain different states near the lossy
beam cutoff, and can exact results from late-game states be reused so later
runs spend their work elsewhere while preserving a valid top-three proof?

The interactive, throwaway model is
[`pruning-variety-suffix-cache-prototype.html`](prototypes/pruning-variety-suffix-cache-prototype.html).
It keeps all state in memory and is not connected to the solver.

## Correctness boundary

A state hash can index two fundamentally different kinds of entries:

- **Partial/seen:** some descendants were searched or a beam retained one
  continuation. This may supply a discovered lower bound or an ordering hint.
  It cannot prune the state and cannot support a certificate.
- **Solved:** every legal child was generated without lossy pruning, and every
  child is terminal or itself solved. This entry may return its cached top
  continuations without expanding the state again.

Hash equality must still compare the complete semantic state; a 64-bit hash
match alone is not proof. The persistent key also needs a ruleset/version
fingerprint. The state portion must include all future-affecting values:
resources, crew, day and maximum day, markets, encounter/disaster state,
upgrades and flags, plus the complete deferred Buy or sold envelope. Trace
history is excluded.

For route output, full action lists do not need to be copied into every result.
The production graph stores parent-linked actions and terminal trace IDs, then
reconstructs a route backward only when it is reported.

## Real solver proxy sweep

The current solver has deterministic structure diversity but no run seed. Two
sequential, safely bounded proxy sweeps tested whether varying those existing
retention views changes results. Every process used beam 500, an open start, no
incumbent, no automatic refinement, a 768 MB internal budget, and the 1,024 MB
hard cap.

### Diversity-share sweep

| Town | 0% | 10% | 20% | 30% |
|---|---:|---:|---:|---:|
| Normalia | 37,527 | 37,527 | 37,527 | 37,527 |
| Tocaccialli | 49,042 | 49,042 | 49,042 | 49,042 |
| Gayawaku | 50,039 | 50,039 | 50,039 | 50,039 |
| Sanctifan | 55,602 | 55,602 | 55,602 | 55,602 |

Changing the share did not change any best score. It slightly reduced generated
states at higher shares because the same best member of each structural family
is still selected deterministically.

### Ranking-portfolio sweep

| Configuration | Tocaccialli | Gayawaku | Sanctifan |
|---|---:|---:|---:|
| optimistic 0%, lanes 5% | 46,523 | 50,039 | 55,445 |
| optimistic 50%, lanes 5% | 49,042 | 50,039 | 55,602 |
| optimistic 25%, lanes 15% | 49,042 | 50,039 | 55,602 |
| optimistic 50%, lanes 15%, diversity 10% | 49,042 | 50,039 | 55,602 |
| optimistic 25%, lanes 5%, skeleton 10% | 49,042 | 50,039 | 55,602 |

No configuration improved a score. Removing optimistic retention materially
hurt Tocaccialli and Sanctifan, so broad random replacement would be dangerous.
Peak RSS stayed between 88 and 107 MB.

## Prototype verdict

### Seeded pruning variety: superseded for the exact-map goal

Keep roughly 85-95% of each beam under the current deterministic continuation
matrix. Use the remaining 5-15% for a reproducible, stratified lottery:

1. Group unselected near-cutoff states by early-action skeleton and immediate
   continuation structure.
2. Give each state a deterministic lottery key derived from
   `semantic_state_hash + run_seed + active_day + lane`.
3. Retain at most one lottery winner per family before taking second members.
4. Never randomize exact duplicate removal or proven dominance.
5. Record the run seed and the sampled semantic hashes in benchmark JSON.
6. Always include the ordinary seed-zero run in a campaign, so campaign quality
   cannot fall below the normal deterministic result.

The present `--diversity-share` is not this algorithm: it changes the quota,
but repeatedly chooses the same best family representatives. The production
mapped search does not use diversity or fading: persistence itself moves work
to unexpanded states without discarding proof obligations.

### Solved suffix cache: the stronger idea

This directly benefits both varied beam campaigns and exact DFS:

1. Exactly solve reachable terminal-day semantic states and store their top
   three suffixes.
2. Maintain a reverse dependency count for a parent expanded without loss.
3. Mark that parent solved only when every legal child is solved.
4. Push newly solved parents onto a retrograde queue and continue toward
   earlier days.
5. On any later exact state match, splice the cached suffixes into the current
   trace and skip its descendants.
6. Treat cache eviction as a performance loss only; an evicted proof entry
   causes re-expansion, never an assumed result.

Beam-generated parents normally remain partial because their descendant search
contains lossy trims. They can still consume already-solved child entries, but
they cannot themselves become solved unless all legal children were generated
and closed exactly.

## Implemented production design

The follow-up implementation went directly to a transactional SQLite graph:

1. `open`, `partial`, and `solved` are stored per exact semantic state.
2. Expansions are atomic and never beam-trimmed.
3. Partial nodes retain lower bounds and reverse dependencies, but only open
   descendants are scheduled on later runs.
4. A compact hot lookup table is separated from snapshots, edges, and traces.
5. Hash hits are collision-checked against a full versioned key.
6. Solved status propagates backward only when every child is solved.
7. Paths are rebuilt from parent-linked actions instead of stored as repeated
   full lists.

Implementation and measurements are in
[`docs/persistent-exact-search.md`](../docs/persistent-exact-search.md).
