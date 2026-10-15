# Trading Caravan Search

This context describes how the solver represents player decisions and delayed
resource allocations while exploring a game.

Current priorities and unfinished work live in the
[Trading Caravan plan](http://localhost:8313/yoss/Projects/TradingCaravan/Plan).
Operational examples below do not authorize a search or override its deferrals.

## Language

**Action branch**:
A distinct legal player action from the same game situation. Action branches
are evaluated and pruned independently.
_Avoid_: Child, sister, row

**Allocation alternative**:
One feasible interpretation of resources inside a symbolic state, such as a
possible food quantity or cargo basket for a delayed Buy. Allocation
alternatives do not compete independently for search width.
_Avoid_: Row, child state

**Symbolic state**:
One search state that carries multiple compatible allocation alternatives for
the same action history. Pruning a symbolic state removes all alternatives it
contains.
_Avoid_: Bucket when the distinction from an allocation alternative matters

**Buy market**:
The item purchase prices and weights in effect when a delayed Buy occurs.
_Avoid_: Buy world

**Sell market**:
The item sale prices in effect when the corresponding Sell occurs.
_Avoid_: Sale world

**Market pair**:
A Buy market and Sell market considered together when valuing a cargo basket.
Different event histories with identical markets form the same market pair.
_Avoid_: Town-Merchant-Witch combination

**Cargo cycle**:
The action interval from acquiring trade cargo in a Buy until disposing of it
in a Sell. Automatic events and player actions may occur inside the interval.
_Avoid_: Buy-Sell pair when adjacency is implied

**Continuation view**:
An exact, legal immediate-action interpretation used only to rank a symbolic
state. Sell, Trader, Camel, Interest, and a currently legal food-only Buy may
each provide a view. A view can retain its owner but never forces expansion to
take that action.
_Avoid_: Plan, prescribed continuation

**Ranking ticket**:
One continuation view's realistic score and optimistic bound competing for
beam retention. Multiple tickets share one symbolic-state owner and therefore
consume one retained slot when any ticket wins.
_Avoid_: Child state

**Continuation matrix**:
The five independent ranking-ticket columns for one unresolved symbolic owner:
immediate Sell, Trader, Camel, Interest, and food-only Buy. The matrix affects
retention only; normal expansion remains free to take every legal action.
_Avoid_: Best next action

**Skeleton family**:
States with the same quantity-free early player-action prefix and encounter
world. Under memory pressure, the work queue completes one family with the full
local beam before releasing the next. Families share immutable storage where
safe but never share beam pruning or deduplication. Family order consumes the
same realistic, optimistic, and action-lane matrix as beam retention.
_Avoid_: Child row, sister state

**Heuristic ceiling**:
The highest optimistic ranking value observed within one skeleton family. It
helps order queued work but is not a proven upper bound for every latent
deferred allocation. It is measured for violations and must never prune.
_Avoid_: Safe bound, branch-and-bound ceiling

**Exact frontier reduction**:
The correctness-preserving pass before beam selection. It merges
future-equivalent states, applies monotone-resource dominance to concrete
states, and applies allocation-set containment to compatible deferred states.
Weighted food and cargo are not assumed monotone because they consume future
Buy capacity.
_Avoid_: Beam pruning

**Deferred dominance partition**:
Deferred symbolic states that share every invariant required for an exact
containment comparison: mode, pinned Buy market, accumulated food timeline,
existing inventory, origin food/capacity shape, and—after Sell—the exact action
suffix. The partition excludes ordered constraints such as minimum food and
gold strength so a stronger state can dominate a weaker one.
_Avoid_: Heuristic cohort

**Gold-envelope dominance**:
A pointwise relation over every original-Buy spend admitted by the weaker
state. The stronger function must admit the whole spend domain and return at
least as much current gold at every spend. Identical operation suffixes use a
monotonicity proof; different suffixes are compared at every integer spend.
_Avoid_: Representative-gold comparison

**Route-survival audit**:
An observational witness following the action-prefix of a replay-verified route
through generation, exact reduction, dominance, beam retention, and completion.
It does not reserve a beam slot or force an action. A matched deferred Buy
means its action branch survived, not that the saved route's exact food/cargo
allocation survived. Exact reduction may carry the witness on a
future-equivalent or dominating trace; terminal matches are checked against the
materialized trace itself.
_Avoid_: Guided search, exact route proof

**Food breakpoint**:
A food quantity where feasibility, a demand threshold, an affordability
limit, carrying capacity, or the winning origin changes. Weighted food may
make every remaining integer a genuine next-Buy capacity breakpoint; those
values are not discarded merely to create a sparse recommendation list.

**Persistent search map**:
An exact state graph accumulated across bounded sessions. It shares normal
gameplay and successor-generation rules with transient search. Depth order
closes deep suffixes first; mapped beam order advances ranked day layers while
leaving every state outside the retained layer open on disk.
_Avoid_: Saved beam, route cache

**Mapped beam sweep**:
A lossy scheduling view over the lossless persistent graph. It uses the normal
continuation-matrix and optimistic ranking to advance at most the configured
beam width through each day layer. Non-selected states are parked, not deleted
or marked solved, and later sweeps admit them as earlier choices close.
_Avoid_: Proof beam, permanent beam prune

**Open mapped state**:
A semantic state whose complete set of legal action branches has not yet been
generated. Only open mapped states are selected for expansion.
_Avoid_: Unseen state when it has already been linked from a parent

**Partial mapped state**:
A state whose legal action branches were generated exactly once but which
still has at least one open or partial descendant. Its best score is a lower
bound, not a proof, and later sessions continue its descendants without
regenerating the state.
_Avoid_: Solved suffix, faded state

**Solved mapped state**:
An exactly expanded state for which every linked descendant is also solved.
Only this status permits future searches to skip the state as proven work.
_Avoid_: Seen state, cached state

**Mapped lower bound**:
The best terminal score actually reached by the persistent graph so far. A
seeded incumbent may be displayed by the solver without becoming part of this
bound or changing graph completeness.
_Avoid_: Proven best before the root is solved

## Agent operating note

For fast record discovery, build or resume the Normalia graph through the
bounded native wrapper with mapped beam order:

```bash
cd /home/jay/develop/personal/Trading-Caravan-Solver
ARRAY_LIMIT=2000 MAX_SECONDS=600 ./scripts/run_safe.sh \
  --town Normalia \
  --no-incumbent \
  --no-auto-refine \
  --search-map search_maps/caravan-search.sqlite3 \
  --search-node-budget 100000 \
  --search-map-order beam \
  --benchmark-json
```

Repeat the same command to continue. `ARRAY_LIMIT` is the mapped beam width;
`--search-node-budget` limits new atomic expansions during the session. Each
expanded node still records all exact children. The width only decides which
unresolved children advance during that sweep.

Use depth order when the immediate goal is closing suffixes rather than finding
a high score quickly:

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

Both orders safely use the same scenario and may be alternated. Keep town,
starting prefix, Merchant and Witch settings, forced options, and purchase
profile the same; changing a future-affecting setting creates a separate
scenario in the same SQLite file. Scheduling and ranking settings do not alter
the stored game graph.

Inspect the graph without running native code:

```bash
scripts/search_graph.py search_maps/caravan-search.sqlite3 scenarios
scripts/search_graph.py search_maps/caravan-search.sqlite3 stats
scripts/search_graph.py search_maps/caravan-search.sqlite3 verify
scripts/search_graph.py search_maps/caravan-search.sqlite3 best --limit 3
```

Use `--no-incumbent` when measuring what the map itself has discovered. Without
it, the solver may display the saved 37,527 Normalia route even though that
route has not been reached by the persistent graph. `root_status: partial`
means more exact work remains regardless of the displayed score. The complete
storage and correctness notes are in `docs/persistent-exact-search.md`.

In benchmark JSON, confirm `mapped_order`, `mapped_beam_width`,
`mapped_expanded_this_run`, `mapped_beam_sweeps`, and
`mapped_beam_parked_visits`. The first production width-500 sweep expanded
10,000 new nodes in 9.9 seconds at 106 MB peak RSS and raised the map's lower
bound from 21,285 to 37,478. The integrity inspector then reported 52,518
nodes, 22,418 open, 7,105 partial, 22,995 solved, and no errors.

The width-500 result is below the replayed 37,527 record for a concrete reason.
It retained the nearby Camel-before-Trader route with 8 pottery in the final
cargo Buy; the record uses Trader-before-Camel and 9 pottery. A transient
width-500 beam permanently drops that close combination when it falls outside
a retained day layer. A mapped beam parks it, so repeated sweeps can eventually
admit it, but 37,527 is not mapped until `best --limit 3` actually reports it.

The first non-Normalia mapped campaign is recorded in
`benchmarks/PersistentBeamPortWillam2026-09-04.md`. Ten width-2,000 runs followed
by ten width-500 runs added 200,000 exact expansions to
`search_maps/port-willam.sqlite3`. They tied Port-Willam at 42,296 and peaked at
144 MB RSS. The verified graph contains 279,382 states: 79,382 open, 26,230
partial, and 173,770 solved. Its top three remain 42,296, 42,291, and 42,200;
the root remains partial. Resume the same file for future Port-Willam ranking
or width experiments rather than starting over.

Persistent schema version 2 stores one compressed semantic key plus a compact
provenance delta instead of duplicating the full state. The verified
Port-Willam migration reduced 362 MB to 148 MB without changing its graph or
top-three routes. Trace IDs are allocated inside SQLite's serialized write
transaction, not from a process-local counter. The all-town runner also holds
an inherited map-directory lock, so interrupted supervisors cannot overlap an
orphaned solver child.

For the active million-state campaign, use
`scripts/start_million_state_campaign.sh` to resume and
`scripts/run_million_state_campaign.py --status` to inspect. Initial graph
growth uses width 2,000 and 100,000 expansions per bounded session; width 500
is reserved for later diversity passes.
