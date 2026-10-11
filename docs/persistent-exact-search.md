# Persistent exact search graph

The mapped search mode accumulates one exact state graph across bounded runs.
It is designed for the rule: **only a fully solved state may be skipped**.
An expanded-but-partial state is not expanded again, but every still-open
descendant remains eligible for later sessions.

## State lifecycle

Each semantic state has exactly one of three statuses:

- `open`: its legal action branches have not been generated yet.
- `partial`: it was expanded atomically, but at least one child is not solved.
- `solved`: it was expanded atomically and every child is solved. A leaf is
  solved immediately after its complete expansion.

An expansion is committed in one SQLite transaction. If the solver hits its
time or memory stop during generation, the transaction is not started and the
node stays `open`. Completing a child recomputes every reverse parent. Closure
therefore moves backward toward the root without regenerating prior nodes.

Partial nodes carry their best discovered lower bound. That score can be
reported immediately, but it is not a certificate. The root's score becomes
proven only when the root status becomes `solved`.

Exact frontier reduction remains enabled inside one expansion. Incumbent bound
pruning and transposition eviction are not used by mapped search. Depth order
does no scheduling trim. Mapped beam order selects a bounded day-layer view,
but every non-selected state remains open in the graph.

## Search orders

`--search-map-order depth` selects the deepest open state and then the best
beam score at that depth. It is useful for closing suffixes and moving proof
status backward.

`--search-map-order beam` traverses the already-known partial graph from its
root, processes same-day transitions together, and uses the normal
continuation-matrix/optimistic selector to advance `ARRAY_LIMIT` unresolved
states to the next day. The graph still stores every exact child. States that
do not fit are parked open, not deleted; another sweep starts from the root and
admits more work as selected descendants become solved. The two orders can be
alternated against the same scenario because they change scheduling, not game
semantics.

## Hot and cold storage

One SQLite file holds multiple independently versioned scenarios. It avoids
both a million tiny files and one monolithic in-memory table.

The hot lookup table, `node_index`, contains only:

- scenario and 64-bit lookup hash;
- `open`/`partial`/`solved` status;
- active day and queue priority;
- best known score; and
- cached total/solved child counts.

A hash match is only an index hit. The complete versioned semantic key in
`node_payload` is compared byte-for-byte before two states are merged, so a
64-bit collision cannot prove equality.

Cold tables hold:

- `node_payload`: compressed exact semantic keys plus compressed trace-only
  provenance deltas;
- `edges`: forward dependencies plus a reverse index for closure propagation;
- `traces`: parent-linked actions used to reconstruct routes backward; and
- `node_results`: at most three direct terminal trace links per expanded node.

Full action lists are not duplicated. Reporting a route starts at its terminal
trace ID and follows `traces.parent_id` back to zero. During expansion the
worker loads only the selected state's trace ancestors and deferred
provenances, not the complete trace table. New trace links are persisted only
when referenced by a retained successor or one of that node's top three direct
terminal outcomes.

Schema version 2 does not store the full state twice. Restart reconstructs a
node from its semantic key and its provenance delta. Compression is lossless;
the state hash is still calculated from the canonical uncompressed key, and a
hash hit is confirmed against the complete uncompressed bytes. `edges`,
`node_results`, and `traces` use SQLite `WITHOUT ROWID`, removing redundant
primary-key B-trees. Opening a version-1 map migrates it in one transaction and
then atomically vacuums the freed legacy pages.

The scenario identity contains the full starting semantic state, codec/rules
version, Merchant/Witch enablement and forced choices, and purchase profile.
Beam/ranking knobs are excluded because they do not participate in exact
mapped expansion.

## Running and resuming

For broad graph growth, use a persisted width-2,000 beam:

```bash
ARRAY_LIMIT=2000 MAX_SECONDS=600 ./scripts/run_safe.sh \
  --town Normalia \
  --no-incumbent \
  --no-auto-refine \
  --search-map search_maps/caravan-search.sqlite3 \
  --search-node-budget 100000 \
  --search-map-order beam \
  --benchmark-json
```

Run the same command again to continue. `--search-node-budget` counts atomic
node expansions in that session; omit it to use only the wrapper/time limit.
In beam order, `ARRAY_LIMIT` controls the retained scheduling layer. In depth
order it is ignored. Both orders obey the internal and external memory ceilings.

For suffix closure, change only the scheduling order and desired node budget:

```bash
MAX_SECONDS=600 ./scripts/run_safe.sh \
  --town Normalia \
  --no-incumbent \
  --no-auto-refine \
  --search-map search_maps/caravan-search.sqlite3 \
  --search-node-budget 5000 \
  --search-map-order depth \
  --benchmark-json
```

Inspect the map without launching native code:

```bash
scripts/search_graph.py search_maps/caravan-search.sqlite3 scenarios
scripts/search_graph.py search_maps/caravan-search.sqlite3 stats
scripts/search_graph.py search_maps/caravan-search.sqlite3 verify
scripts/search_graph.py search_maps/caravan-search.sqlite3 best --limit 3
scripts/search_graph.py search_maps/caravan-search.sqlite3 export \
  --output search-map-summary.json
```

`verify` checks cached child counts and the solved-status invariant against the
edge table. The Python inspection tool opens the SQLite file read-only and does
not mutate search data.

## Normalia map measurements

Four sequential 5,000-node sessions built the checked local map at
`search_maps/caravan-search.sqlite3`:

| Cumulative expansions | Unique nodes | Open | Partial | Solved | Reused child lookups | Best lower bound |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5,000 | 5,075 | 75 | 15 | 4,985 | 22 | 15,094 |
| 10,000 | 10,079 | 79 | 17 | 9,983 | 102 | 16,233 |
| 15,000 | 15,073 | 73 | 16 | 14,984 | 190 | 18,894 |
| 20,000 | 20,070 | 70 | 16 | 19,984 | 417 | 21,285 |

The fourth session peaked at 107 MB RSS. The root remains partial, so 21,285
is a discovered lower bound, not a Normalia record or proof. The stable open
count while solved nodes increase is expected from deepest-day-first ordering:
new suffixes are opened and closed while earlier action branches remain linked
for later work.

The first three sessions predated terminal-trace compaction and remain valid;
future writes use the smaller trace policy. At 20,000 nodes the integrity tool
reports no status/count inconsistencies.

A subsequent mapped beam session used width 500 and a 10,000-node budget. It
finished in 9.9 seconds at 106 MB peak RSS and independently rediscovered
37,478. After closing the database, inspection reported 52,518 nodes: 22,418
open, 7,105 partial, and 22,995 solved. The database was about 61 MB and
`verify` reported no errors. The root remains partial.

This reproduces the historical width-500 plateau, not the 37,527 record. The
retained route uses Camel before Trader and buys 8 pottery in its final cargo
purchase. The record reverses those two hires and buys 9 pottery. A transient
beam loses that close variant permanently; the persistent beam leaves it open
for later sweeps.

## Million-state campaign

The resumable all-town campaign uses width 2,000 and 100,000 atomic expansions
per bounded process:

```bash
scripts/start_million_state_campaign.sh
scripts/run_million_state_campaign.py --status
```

Width 500 is reserved for later diversity passes. On a copied Port-Willam map,
the version-2 migration reduced the closed database from 362 MB to 148 MB while
preserving graph counts, top-three routes, and a clean verification. Normalia
crossed the first target at 1,023,163 states and 659 MB; its last checkpoint
took 130 seconds and peaked at 145 MB RSS.

Trace IDs are allocated from the current committed maximum after acquiring
SQLite's write transaction. This is safe across multiple graph writers. The
campaign runner also holds an exclusive map-directory lock which is inherited
by solver children, preventing a replacement supervisor from overlapping an
orphaned child after interruption.
