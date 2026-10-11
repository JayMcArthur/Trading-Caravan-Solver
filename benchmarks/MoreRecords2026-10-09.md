# More record searches — 2026-10-09

Objective: beat additional saved records after Moonin's 37,584 discovery.
Search processes remain sequential and externally capped at 384 MB; use a
512 MB cap only if a measured beam experiment requires it. Builds use the
serialized bounded build wrapper.

## Plan

1. Completed: Huangoz, Normalia, Sanctifan, and improved Moonin all tied their
   saved records at 1,000,000 evaluations each, below 179 MiB RSS.
2. Completed: test a new neighborhood that adjusts food quantities together
   with changes in crew order, overcoming illegal intermediate routes.
   Focused one-round cargo/food recovery passed. Also investigate encounter
   option changes that alter the final day: append Interest for added days or
   remove player actions for shortened routes before strict replay.
3. Completed: run the new neighborhood across promising towns; retain every
   command, status, evaluated-state count, score, peak RSS, and action path.
   Food retiming tied all 22 towns (21,882,010 evaluations, 136.641 seconds,
   184.8 MiB peak). The duration/depth lane uses beam 128, 48 rounds and
   1,500,000 evaluations. L'Exquise stopped at the 384 MB address-space ceiling;
   preserve this diagnostic and retry after reducing candidate storage.
   Candidate ranking needs only its route, score and key; retain full replay
   details only for the final winner, preserving the same selection order.
   The structural lane also moves individual actions without exchanging the
   intervening actions, and exchanges food-only Buy with Trader/Camel/Interest.
   L'Exquise's recovery completed 1,500,000 evaluations at 231.6 MiB RSS under
   the unchanged 384 MB cap, tying 56,817.
   All 22 structural lanes completed, totaling 33,000,000 evaluations in
   154.213 seconds at 253.8 MiB peak; no additional record was found.
4. Completed: corrected prefix timing; the bounded CLI regression, safe build
   and full native suite passed. Previous bounded build,
   focused route-polisher tests and full native suite passed. Prefix searches
   inherited a hardcoded Merchant day 5 even when the fixture calls on day 7;
   keep prefix suffixes open-timed and derive realized fixture encounter days
   for fixed-scenario incumbent admission. Validate through the bounded CLI
   regression wrapper, safe build, and full native suite.
5. In progress: perform a fresh residual audit of improvements and unresolved
   search hypotheses, then continue any required work.

Raw artifacts: `benchmarks/results/more-records-20261009/`.

## Next search branch

After the polishing screens, try early suffix separation rather than adding
more evaluations around the same incumbent. Use explicit day-8 hybrid batches
of 100, broad/suffix beam 5,000, 20% continuation retention, and independent
0% / 100% optimistic ranking mixes. Begin with Sanctifan and Tocaccialli;
add Mehaz, Bajaar, Port-Willam and Skjollird if those lanes finish without
exceeding the working budget. Every lane gets at most 600 solver-seconds,
384 MB external memory and 288 MB internal memory. Gayawaku must use a smaller
broad beam if included because of its known weightless-food memory behavior.

The solver caps explicit suffix width at the broad width, so setting a suffix
larger than the broad beam adds no retention. Separating the prefix into
independent batches increases actual coverage while keeping each active
frontier bounded. Before launching this branch, finish the structural screen
and pass the bounded full test suite for the native polishing changes.

The prerequisite checks passed. Two early-suffix lanes finished at their time
limits; the remaining unlaunched lanes were replaced after those measurements.
Raw diagnostics and full routes are in
`benchmarks/results/more-records-20261009/suffix-summary.jsonl` and each lane's
`.log`. Session 95878 is terminal: only its supervisor was held to prevent new
launches, its current native child finished normally, and its complete result
was recovered from the log before ending the supervisor. Session 62308, the
Tocaccialli later-split recovery, also finished. Both early-split campaigns
are terminal; the subsequent prefix campaign is tracked separately below.

Completed polishing measurements:

| Lane | Attempts | Recorded evaluations | Seconds | Peak RSS MiB | Improvements |
| --- | ---: | ---: | ---: | ---: | ---: |
| Remaining baseline towns | 4 | 4,000,000 | 12.782 | 177.9 | 0 |
| Food retiming | 22 | 21,882,010 | 136.641 | 184.8 | 0 |
| Duration/depth | 22 | 30,989,212 | 168.443 | 263.9 | 0 |
| Structural/depth | 22 | 33,000,000 | 154.213 | 253.8 | 0 |

The duration/depth count excludes L'Exquise's failed diagnostic because the
old error path emitted no candidate count or final route. That diagnostic is
retained explicitly as failed, rather than counted as a completed search; the
structural/depth recovery subsequently finished at the same memory cap.
Every completed lane records its exact command and full replayable action log
in `summary.jsonl`. The totals above are heuristic route evaluations, not a
proof of optimality or additional score records.

## Fresh residual plan

1. Completed: measure both day-8 Sanctifan mixes and the wider day-11
   Tocaccialli experiment; all stopped cleanly at 600 seconds without records.
2. In progress: prefix timing validation passed; run wide-purchase searches
   from 12-action verified prefixes across all towns, then six-action prefixes
   for promising towns. Use beam 10,000, 20% continuation retention, 25%
   optimism and the existing 600-second, 384/288 MB bounds. Gayawaku uses
   beam 2,000 for its weightless-food model.
3. Pending: independently verify and promote any new record; the goal remains
   active until further records have actually been beaten.

## Early suffix checkpoint

Sanctifan / 0% optimistic ranking stopped cleanly at its 600-second internal
deadline. It independently discovered 55,445, below the 55,728 saved record;
the incumbent was disabled. It processed 592,757 states, generated 2,097,364,
recorded 298,454 finished candidates and peaked at 172 MB RSS. It began 14 of
50 possible 100-state batches from a 5,000-state day-8 frontier containing 969
continuation structures. The raw result includes its complete replayable route.

This establishes that day-8 separation adds exploration but leaves most batches
unvisited within ten minutes. Inclusive phase timers identify deferred Buy
expansion (433.775 s) and sold expansion (153.216 s) as the main work; the
reported timers overlap. The second ranking lane completed next, followed by
a later day-11 split with batches of 250 and a 10,000-state beam to measure
whether more independent suffix families could finish within the budget.

Sanctifan / 100% also found 55,445, processing 663,873 states and generating
2,397,815 at 173 MB peak RSS; it began 15 batches before the 600.014-second
deadline. Its native wrapper exited 0. The held supervisor was ended only after
the solver exited and the complete result/action path had been recovered.

Tocaccialli's day-11 split used broad/suffix beam 10,000, batches of 250,
20% continuation retention and 25% optimism under the same 384/288 MB caps.
It independently tied 49,634, processed 462,385 states and began 13 batches,
peaking at 180 MB before its 600-second deadline. The larger retained prefix
again exceeds the time budget; this does not support repeating that widening.

## Prefix timing branch

Code inspection found that early verified prefixes imposed Merchant day 5
and Witch day 9 even when their fixture chooses later encounters. A Normalia
prefix ending on day 6 therefore had no future Merchant opportunity, although
its valid record calls on day 7. Fixed day-5 experiments could also inherit
the day-7 fixture as an incompatible seeded score. Both affect search quality
and reported discovery provenance, not the legality of the saved open routes.

The correction keeps prefix suffixes open to legal encounter opportunities.
It derives the incumbent's realized encounter days by game replay and admits
it only when explicitly fixed days match. New CLI integration checks cover an
unseeded day-6 prefix rediscovery, incompatible day-5 incumbent exclusion, and
matching day-7 incumbent admission. The checks are registered with CTest and
run through `scripts/run_tests_safe.sh prefix-timing`.

The CLI regression reproduced Merchant day 5 before rebuilding and passed
after the correction. The safe build and full native suite passed. The wide
prefix campaign records exact commands, state counts, peak RSS, status and full
action paths in `prefix-summary.jsonl`. It stops on an improvement for replay
verification and promotion.

## Prefix checkpoint

All 22 wide-purchase searches from 12-action prefixes completed in 17.849
wrapper seconds, processing 7,641 states with no beam trims and a 31 MB peak
RSS. Every lane independently tied its saved record. Six-action prefix lanes
subsequently completed sequentially in tool session 73911. The next queue
is tool session 95814; `runtime/more-records-20261009-active.json` identifies
its active town and native wrapper PID.

Classical Buy expansion uses the exact deferred purchase representation,
regardless of the requested `wide` profile. The profile changes concrete
purchase generation for other scenarios, but it adds no Classical choices.
This campaign gains coverage by changing the starting prefix and retaining
a broader suffix, not by the profile setting. The 12-action searches had no
lossy beam trims; the earlier-prefix search still needs its final trim and
completion counters before choosing a follow-up.

The first three six-action lanes completed: Moonin tied 37,584 in
265.051 seconds (410,117 processed states, 49,980 beam prunes, 167 MB peak),
Sanctifan tied 55,728, and Tocaccialli tied 49,634. Their full command,
completion status, resource counters and action paths are in the JSONL.
Because the earlier suffixes lose states at beam 10,000, the next residual
item is a Moonin 20,000-state suffix under the unchanged memory caps, with
900 seconds allowed for the additional retained coverage. Do not extend
the time on an already-completed configuration. Run only after session
73911 is terminal, then cover the remaining fourteen six-action prefixes.

Add a focused open Blue Treasure / Sanctifan lane before the wider Moonin
suffix: beam 20,000, 50% optimistic ranking, 20% continuation retention,
600 seconds and the same 384/288 MB bounds. The historical Blue merchant
sweep used concrete purchase generation; pinning Blue in the current exact
deferred engine isolates a different early trading family without sharing
its beam with the Magic paths. The prepared follow-up runner begins only
after the current native queue is terminal.

The first prefix queue is terminal (tool 73911 exited 0): all 30 lanes
completed with independent discovered scores, no seeded incumbents and full
action paths. All tied existing records. The completed six-action lanes
covered the first eight towns listed by the runner, including Gayawaku at
beam 2,000. The focused Blue/wider-Moonin/remaining-prefix queue is next.

Tool 95814 is confirmed live: the focused Blue lane has an actual native
child under the safe wrapper. The first queue totaled 801,187 processed
states, 733.209 wrapper seconds and 167 MB peak RSS. These completed
heuristic lanes provide coverage evidence, not a global optimality proof.
No record promotion is justified yet; the active goal remains `beat more`.

## Fresh residual branch: sale changes

1. Completed implementation: add Buy/NPC/Interest-to-Sell and Sell-to-Interest/NPC/food-Buy
   mutations to `RoutePolisher.cpp`. Existing neighborhoods can move Sells
   but cannot change their count. Every candidate still requires strict
   replay. A weakened Normalia route has an empty final food Buy followed by
   Interest; one round must insert its sale and cargo to restore 37,527.
2. Completed: tool 95814 is terminal. The safe build, focused
   route-polisher checks and full native suite passed. Native source edits do not
   affect the currently running binary; do not overlap a build with a search.
3. In progress: run the expanded neighborhood across all 22 towns with recorded
   command, status, evaluations, peak RSS and complete action paths; verify
   and promote any strict record improvement.

The CLI polishing driver now searches each known seed independently and
shares the existing total evaluation budget among them. Previously Sanctifan
Magic descendants could evict its lower-scoring Blue seed after one round,
so adding more rounds did not deepen that family. Each independent run keeps
the same beam bound; total evaluations remain capped, and `rounds` records
the deepest seed run. This change is also pending the queued bounded checks
and the screen will verify the CLI budget and retained record scores.

The focused Blue open search completed normally: 55,445, 171,334 processed
states and 186 MB peak RSS. It remains below Sanctifan's 55,728 record.
The wider Moonin run is now live in session 95814. Session 24983 waits for
that complete queue before building, testing and running the sale screen.

The sale neighborhood now inserts an immediate Buy/Sell pair together and
reselects cargo at every Buy whose next player action is Sell. It uses the
existing exact sale frontier, current cash/weight and intervening Merchant
or Witch markets, reserving only event costs not covered by Buy-day income.
Strict replay remains the acceptance gate. The regression starts with an
empty final food Buy followed by Interest; one round must add the sale and
its complete cargo portfolio, recovering 37,527. The cargo-query cache is
limited to 256 entries. This implementation is pending the queued bounded
build and tests; no search currently uses it.

The queued sale screen uses beam 64, 48 rounds and 300,000 evaluations per
town initially, measuring the new cargo work before enlarging it. Its
waiting Python supervisor was replaced before it had any children; no native
search was interrupted. The current earlier-prefix queue remains untouched.

A second cargo regression uses a legal Moonin route with tiny portfolios on
both trades. One round must recover 37,584 with Merchant between Buy and
Sell. This checks that Buy-day income can fund the later Merchant, rather
than unnecessarily reserving its price out of the earlier purchase budget.
The replacement waiting screen supervisor is tool session 83400.

The wider Moonin lane stopped cleanly at its 900-second internal deadline,
tying 37,584 after 1,939,074 processed states and
3,300,357 generated transitions, with 212 MB peak RSS.
It added substantial coverage over the 410,117-state beam-10,000 run, but
remains incomplete and supplies no new score. The remaining prefix lanes
continue sequentially before the queued native checks and sale screen.

The prefix supervisor advanced to Petrinov before it was held. Eduming and
Petrinov both finished normally; only the Python supervisor was stopped.
Petrinov's complete JSON and route were recovered from its finished native
log after wrapper exit 0, before ending that supervisor. Eleven unstarted
prefix lanes are explicitly deferred in the checkpoint metadata. The waiting
sale-screen supervisor can now perform the bounded build and checks.

The serialized safe build finished successfully. The focused native suite
passed under 384 MB, including both new cargo regressions. The full native
suite is running under 512 MB; the screen is guarded on its successful exit.
Build/search ordering is verified: Petrinov's native wrapper was a zombie
with exit code zero and no children before the held supervisor ended and
the waiting build began. Validation and screen tool session: 83400.

All queued native checks passed. The new sale/cargo screen began in tool
83400, with 300,000 evaluations and a 384 MB hard cap per lane. Moonin
completed at 37,584 in 50.292 wrapper seconds, using 109.7 MiB peak RSS.
The remaining towns run sequentially and stop on a strict score improvement.

The measured 300,000-evaluation lanes leave room for deeper independent seed
runs: 14 towns have completed without improvements, at a maximum 125.5 MiB
RSS. Queue a 750,000-evaluation screen with the same beam 64, 48-round limit,
384 MB cap and 600-second native budget after the current screen is terminal.
It adds evaluated routes and seed depth; it does not merely extend timeout.
No build or test rerun is needed for this parameter-only experiment. It stops
if the first screen finds an improvement or fails, so those results must be
resolved before another native search starts.

## Sale/cargo screen checkpoint

All 22 first-stage lanes completed normally: 6,600,000 evaluations in
667.599 wrapper seconds, with a 131.8 MiB peak RSS. Every score tied its
existing record. All rows retain exact bounded commands, successful exits,
evaluation/round counts, measurements and complete replayable actions.
Tool 83400 exited 0. The deeper 750,000-evaluation screen is live in tool
74965: Moonin completed at 37,584 with 184.9 MiB peak RSS; Huangoz follows.
Polishing is bounded by the enforced 384 MB external ceiling; the internal
288 MB argument controls the GameSolver searches rather than this polisher.

## Next residual plan

1. In progress: finish and audit the deeper sale/cargo screen, checking that
   independent-seed CLI totals stay within 750,000 evaluations per town.
2. Pending if no improvement: re-examine fixed trade windows containing NPC
   hires (Mehaz's first Buy/Sell pair is one). Current cargo reselection
   handles immediate player-action Buy/Sell pairs and intervening encounters.
3. Pending: resume the eleven deferred six-action prefixes if they still add
   useful coverage, or replace them with a measured new branch. Keep exact
   results and commands; do not count ties as record improvements.
4. Pending: independently replay and promote any additional score record,
   then perform the completion audit. The goal remains active.

## Persistent continuation branch

Read-only inspection found all 22 existing million-node maps still partial,
with 50,845–96,203 open nodes per town. Their stored bests include 37,527 for
Moonin, 55,602 for Sanctifan and 47,621 for Tocaccialli; these are below the
current fixture scores and must not be mistaken for new records.

After the deeper polish screen is terminal, continue each existing map for
at most 100,000 atomic expansions or 600 solver seconds. Use mapped beam
order, width 2,000, 50% optimism and 20% continuation retention, with a
384 MB hard cap and 288 MB internal budget. This visits saved open work;
non-selected states remain stored. The runner inherits the established map
campaign lock into every native child and checks the 5 GiB free-space floor.
The measured free space is 664.6 GiB. Before/after node/solved counts, exact
command, elapsed time, RSS, completion status, score and action path are
recorded in `persistent-summary.jsonl`. Stop on an improvement for replay.

Waiting continuation tool: 40669. No persistent native process starts while
the deeper polishing supervisor is live. Defer the eleven unstarted prefix
lanes and the NPC-window refinement until these new global states are
measured; this changes the remaining plan because persistent work adds
coverage outside the current local route neighborhoods.

The deeper screen is terminal (tool 74965 exited 0): all 22 lanes completed,
using 16,500,000 evaluations in 1,720.847 wrapper seconds. Peak RSS was
208.4 MiB, all evaluation totals stayed at or below 750,000, and every row
has a complete action path and successful exit. No additional record was
found. The persistent continuation supervisor is live in tool 40669; its
first actual native child resumed Moonin's existing map at beam 2,000 under
the 384/288 MB limits. No native build or other solver overlapped its launch.

The remaining plan now prioritizes these global map continuations. NPC-window
cargo refinements and the eleven unstarted fixture-prefix lanes are deferred
until the stored open states have been measured. A record remains unproven;
all current polishing ties are logged as coverage evidence only.

Moonin's first persistent continuation completed its 100,000-expansion
budget normally, adding 131,012 unique nodes and marking 87,752 additional
nodes solved. Peak RSS was 132 MB. Its stored score remains 37,527, below
the current 37,584 fixture; this is state coverage, not a record. The full
JSON/action path and before/after graph counts are in the persistent JSONL.
Sanctifan is now the actual live native child in the same sequential queue.

## Cargo windows with intervening hires

The first three persistent continuations reached their 100,000-expansion
budgets normally, adding 372,645 unique nodes without a new record. The
supervisor is held while Gayawaku's native child finishes normally; do not
build until that child and its bounded wrapper have exited. The remaining
town continuations are deferred while testing the next search branch.

Current plan:
1. In progress: extend exact cargo reselection across intervening crew hires
   and reserve enough cash for each hire before its end-of-day income. Also
   allow new Buy/Sell pairs separated by hires; strict replay remains the gate.
2. Pending: validate the Mehaz regression, bounded build, focused polisher
   tests and full native suite, with no solver/build overlap.
3. Pending: screen all towns with the wider trade mutations under 384 MB,
   recording metrics, commands and complete routes.
4. Pending: replay and promote any improvement; otherwise resume the deferred
   map continuations or fixture prefixes with fresh coverage.

Gayawaku also completed 100,000 atomic expansions normally: score 49,753,
128 MB peak RSS, 280.914 native seconds. Its wrapper was verified exited
zero with no children before ending the held supervisor (tool 40669 exited
137); all four native results are complete, not interrupted. The recovered
Gayawaku row records native elapsed time, full JSON/route and before/after
counts without inventing wrapper elapsed time. Eighteen continuations remain
deferred. Build and tests for the hire-window branch can now run serially.

The hire-window build, focused route-polisher regressions, and full native
suite passed through the bounded wrappers (hire-build.log,
hire-focused-tests.log, hire-full-tests.log). The initial weakened test
fixture could not afford its unchanged later cargo; the corrected regression
uses legal small cargo in every trade and recovers Mehaz's record in one
round, requiring coordinated reselection across the intervening hire.
No rule or fixture score changed. The sequential 300,000-evaluation screen
is live in tool 50746, starting Mehaz then Jilliqo, with full metrics/routes
in hire-summary.jsonl. Mehaz tied 48,738; the remaining towns continue.

If the hire-window screen remains tied, test beam diversity in RoutePolisher:
retain half the beam by score, then reserve remaining slots for distinct
action sequences (including crew choices, encounter timing/options and
food-only versus cargo Buys). Current quantity variants can occupy every
slot with one sequence; this branch measures different strategies rather
than extending a completed lane's timeout. Keep the best legal route and
strict replay gate, validate through bounded build/focused/full checks, and
record the separate campaign's metrics and complete action paths.

Prepared the diversity change while the current campaign continues using
the already-built hire-window binary. RoutePolisher now retains half its
beam by score and reserves the rest for distinct full action sequences;
if insufficient distinct sequences exist, it fills remaining slots by
score. The best route is still selected by strict replay score.
A separate supervisor (tool 34528) waits for the current search supervisor
to exit successfully before bounded build, focused/full tests, and a
500,000-evaluation-per-town campaign. It refuses to proceed if the current
campaign reports an improvement or failure. No build/search overlap.

## Hire-window screen completion and diversity cutover

Tool 50746 exited zero. All 22 hire-window lanes completed normally with
6,600,000 evaluations in 843.418 wrapper seconds, 131.5 MiB peak RSS, and
complete replayable paths. Every score tied its existing record.
Tool 34528 observed the old supervisor's terminal exit before building.
The serialized safe build and focused regressions passed for the diversity
change. The bounded full suite is running; the separate 500,000-evaluation
screen remains gated on its successful exit. No compiler overlapped a solver.

Current plan:
1. In progress: validate the diversity cutover and complete its distinct
   action-sequence campaign, auditing evaluation limits and full action paths.
2. Pending: independently replay any improved score and promote its fixture,
   Best.txt and regression checks; complete a fresh residual audit.
3. Pending if tied: resume saved open graph states using a different ranking
   mixture and measured additional expansion budget, or complete deferred
   six-action prefixes. No additional record has yet been found.

The diversity safe build, focused tests, and full native suite all passed
(diversity-*.log). The live screen is in tool 34528. Moonin completed
500,000 evaluations / 11 rounds at 37,584, 104.609 wrapper seconds and
164.9 MiB peak RSS; Jilliqo follows. All current native changes are checked.

## Independent encounter-option branch

The first five diversity lanes all completed and tied their fixtures:
Moonin, Jilliqo, Mehaz, Sanctifan, and Tocaccialli. Hold only the supervisor
while its active Huangoz child finishes, then recover that complete native
result before ending the supervisor. Defer the remaining sixteen diversity
lanes; all native code checks have already passed.

New parameter-only experiment: independently force Blue Treasure and
Vertue of Patience for Tocaccialli, Sanctifan, Moonin, and Huangoz. This
forces a different savings strategy to receive its own beam, rather than
competing with the dominant Midas routes. Use width/suffix 8,000, day-10
structure-interleaved batches of 500, optimism 50%, continuation 20%,
600 solver seconds, 384 MB hard and 288 MB internal ceilings, no incumbent
and no auto refinement. Retain full JSON, processed counts, RSS, completion
status, command and route for every result. Stop on a record for replay.

Six diversity native lanes completed: 3,000,000 evaluations, all ties,
164.9 MiB peak RSS. Huangoz's complete result was recovered only after
verifying its wrapper exited zero and had no children; its timed elapsed
was 75.43 seconds. Only the held supervisor was ended (tool 34528 exited
137); no native lane was interrupted. Sixteen unstarted lanes remain
deferred. The independent Blue Treasure / Patience screen started
Tocaccialli in tool 99396 after the previous native wrapper and supervisor
were terminal. No code changes or additional build are required.

Tocaccialli and Sanctifan completed their independent Blue/Patience lanes
at 25,249 and 23,160 respectively, both below their fixtures (172,970 and
210,130 processed states, 91 MB peak RSS). These are discovered scores,
not inherited ties or new records. Extend the same bounded experiment
to L'Exquise, Eduming, Doba Lao, and Skjollird after the current four-town
supervisor exits normally. These cover reduced upkeep, higher interest and
crew bonuses. The waiting bonus-town runner refuses to launch if the current
result is an improvement or failure; no native runs overlap.

The first four independent Blue/Patience lanes completed normally (tool
99396 exited zero), with no record improvements. Scores: Tocaccialli
25,249; Sanctifan 23,160; Moonin 18,921; Huangoz 21,430. Full routes and
processed counts, elapsed/RSS and completion flags are recorded in
patience-summary.jsonl. The four bonus-town lanes are now active in tool
11464, following the same sequential, bounded configuration. No native
code changed since the successful diversity build and full tests.

## New persistent ranking mix

All eight Blue/Patience lanes completed normally without improving a fixture
(first supervisor 99396 exited zero; bonus supervisor 11464 exited zero).
The bonus-town scores were L'Exquise 48,641, Eduming 27,299, Doba Lao 31,093,
and Skjollird 26,854. Full JSON/routes and metrics remain in the two patience
JSONLs. This branch tested a different savings strategy independently and
found no further record; it is now closed as measured search coverage.

Resume all 22 existing exact-search maps with a new ranking mixture:
100% optimistic retention, 0% continuation slots, beam 6,000, and at most
300,000 atomic expansions or 600 solver seconds per town. Keep 384/288 MB
limits, inherited map lock and 5 GiB free-space floor. Compared with the
previous 100,000-expansion / beam-2,000 mix, this adds retained open states
and a different ordering. It does not merely raise a timeout. Start Jilliqo,
Mehaz, and Bajaar, then the other deferred towns; the previous four maps
follow last. Capture before/after graph counts, full JSON/actions and all
required metrics in persistent-optimistic-summary.jsonl. Stop for replay on
a score above the current saved fixture, not the map's older stored best.

Current plan:
1. In progress: execute and audit the new persistent ranking mix.
2. Pending: replay and promote any improved score, with bounded native checks.
3. Pending: review remaining map work and deferred prefix lanes after the
   campaign's measured coverage; do not treat partial graphs as a proof.
The record-search goal remains active. All native code checks are passed.

Read-only live inspection of Jilliqo's committed graph found 1,435,102
nodes and a best score of 37,819 (previously 37,789). This matches the
current fixture and is not an additional record. The graph remains partial;
completion metrics and the native action path must still come from the
finished lane. At that checkpoint native elapsed time was 339.4 seconds
and current RSS 169.6 MiB. The continuation is adding states and has already
rediscovered a score absent from the older million-node map.

The existing earlier six-action Moonin prefix runs already rediscovered
37,584, so there is no evidence that the replay-verified route is outside
the solver's generated search space. Any next survival audit should examine
lossy broad-beam ranking, not assume an exact-pruning correctness bug.
Pending parameter-only follow-up: isolate Magic Cleppsydra / Midas for a
full-opening beam and observe Moonin's known route without protecting it,
then extend to other towns if it measures new coverage. Do not overlap the
active persistent campaign.

Jilliqo's new persistent lane completed all 300,000 atomic expansions
normally in 502.725 native seconds: 131,456 processed deferred states,
452,921 additional unique graph nodes and 240,307 additional solved nodes,
170 MB peak RSS. Its best is now 37,819, matching the existing fixture.
The root remains partial. The exact bounded command, before/after counts,
full result and complete route are in persistent-optimistic-summary.jsonl.
Mehaz is the actual live child in the same sequential queue.

## Full-opening dominant-option experiment

After Jilliqo rediscovered its fixture, the active persistent lane is Mehaz.
Hold only the supervisor while this bounded native child finishes, then
recover its full result. Defer the remaining twenty map continuations while
measuring a more targeted full-opening experiment. No native lane is stopped.

Force Magic Cleppsydra / Midas independently for Moonin, Jilliqo, Normalia
and Huangoz, without a fixed opening prefix, incumbent, or auto refinement.
Use a 16,000-state broad/suffix beam, day-12 structure-interleaved batches
of 1,000, 50% optimism, 20% continuation and 30% first-eight-action skeleton
retention. Keep the same 384 MB hard / 288 MB internal caps and 600-second
solver budget. Enable the observational known-route audit to identify any
coverage loss during generation, exact/dominance reduction, or beam ranking.
The audit does not protect a reference route or change its score.

This isolates the strongest saved encounter combination and searches all
openings with added strategy retention. It follows the already-checked
native code; no build or tests rerun is needed for parameter-only searches.
Every actual lane retains full JSON/actions and required measurements.
Stop and replay if a score exceeds its fixture. Other map continuations,
unused diversity lanes and eleven prefix lanes remain explicitly deferred.

Mehaz's 300,000-expansion lane completed normally in 500.915 native
seconds: 129,506 processed states, 467,258 additional unique nodes and
239,178 additional solved nodes, 164 MB peak RSS, score 48,738 tied.
The full result was recovered after verifying its wrapper's zero exit
and lack of children. The initial graph counts/bytes were recovered from
the unchanged last Mehaz million-state manifest row, with provenance
recorded in the new row. Only the held supervisor was ended (tool 53685
exited 137); neither native continuation was interrupted. The two lanes
added 920,179 unique states and rediscovered current fixtures without a
new record. Twenty unstarted continuations remain deferred.

The prepared dominant-option full-opening screen can now start; its runner
requires a nonempty legal result route and stops on a failure or improvement.
The stored map campaigns already include separate Merchant choices but
leave Witch unrestricted; the new experiment fixes both choices and adds
full-opening skeleton retention and observational route coverage counts.

The dominant-option full-opening audit started Moonin in tool 62548 after
the persistent native wrappers and held supervisor were terminal. It uses
the checked native binary with the documented 384/288 MB limits.
A residual artifact audit compared 1,194 historical million-state campaign
rows with the current fixtures and found no unpromoted score improvements.
Thus no record is being overlooked in those saved mapped-search outputs.

The dominant-option full-opening supervisor (tool 62548) exited zero. All
four lanes completed and tied their fixtures: Moonin 37,584; Jilliqo 37,819;
Normalia and Huangoz 37,527. They processed 656,147
states at 107 MB peak RSS. Moonin's known route
was generated and retained through all 18 actions, so its search-space
reachability is directly observed without protecting or seeding it.
This is coverage under the forced encounter pair, not a global optimality
certificate. Full action paths and audit counters are preserved in
midas-opening-summary.jsonl. No additional record was found.

## Separate close encounter pairs

All four dominant-option full-opening lanes had zero beam truncations:
Moonin's largest candidate layer was 15,688; the other towns stayed below
10,964. Increasing their beam alone adds no coverage. Continue with close
alternatives from the recorded Merchant profiles instead.

Queue six independent full-opening pairs: Tocaccialli Magic/Midas,
Sanctifan Blue/Midas, Gayawaku Blue/Midas, Mehaz Route/Time, Mehaz
Route/Camelization, and Tocaccialli Saddle/Midas. Use broad/suffix width
32,000 (16,000 for Gayawaku, below its deep-campaign recovery ceiling),
day-12 batches of 1,000 and the same 50% optimistic / 20% continuation /
30% skeleton settings. Preserve 384/288 MB and 600-second bounds, no
incumbent or auto refinement. Each fixes both encounter choices; prior
persistent Merchant profiles left Witch unrestricted. Full routes and
metrics go to pair-alternatives-summary.jsonl, with a stop on improvement.
No new native code or native checks are needed for these configurations.

Six pair-alternative lanes are terminal (tool 54820 exited zero): five
completed without beam truncation, with no record improvements. Scores:
Tocaccialli Magic/Midas 47,621; Sanctifan Blue/Midas 55,445; Mehaz
Route/Time 37,542; Mehaz Route/Camelization 48,738; Tocaccialli Saddle/Midas
49,634. Gayawaku Blue/Midas retained a 49,344 path but stopped at its
internal working budget: peak RSS 259 MB, 81,178 processed states, 7,753
beam trims. Preserve this partial diagnostic, not an untrimmed completion.

Next parameter-only lanes: Bajaar Magic/Oasis, L'Exquise Magic/Midas,
Sanctifan Magic/Midas, then a measured Gayawaku Blue/Midas recovery.
The first three use the existing 32,000/day-12/384 MB profile. For Gayawaku
use the documented deep-campaign broad ceiling of 20,000, split at day 8,
suffix 20,000, and a 512 MB hard / 384 MB internal cap. The larger ceiling
is justified by its measured 259 MB memory stop; earlier separate suffix
batches and wider retention add coverage. Keep the 600-second native limit,
sequential execution, and full measurements/actions for every lane.
This is the only search lane raised above 384 MB, still below the enforced
1,024 MB native ceiling. No native code changed or builds/tests repeated.

The follow-up supervisor (tool 78011) exited zero after three untrimmed
completions and a Gayawaku budget diagnostic. Scores: Bajaar Magic/Oasis
46,254; L'Exquise Magic/Midas 56,141; Sanctifan Magic/Midas 55,728.
None improved a fixture. The Gayawaku recovery stopped normally at its
internal working budget, 245.116 seconds / 63,619 processed states /
354 MB peak RSS, with no finished route. This row is a diagnostic only;
its null score/empty path must not be counted as a completed score lane.

Next measured recovery keeps Gayawaku's 20,000 broad beam, day-8 split
and 512/384 MB caps, but reduces active suffix batches from 1,000 to 50
and suffix width from 20,000 to 5,000. Smaller independent batches separate
more prefix continuations while reducing the active trace graph. Maintain
600 solver seconds and retain full results/status/path or any diagnostic
in gay-small-batch-summary.jsonl. Do not raise timeout alone or treat a
working-budget stop as a global proof. No native source has changed.

The smaller Gayawaku suffix batches are live in tool 50646. A 302.9-second
checkpoint showed current RSS 118.5 MiB, comfortably below the previous
354 MB stop. Native completion/timeout status, actual peak RSS and a valid
action path still need to be recorded from the finished lane.

Queue the leading Blue/Oasis pair for Bajaar and Blue/Midas pair for
L'Exquise, followed by Moonin's early-call Cornucopia/Midas and
Leaders Necklace/Midas alternatives. These use full openings, broad/suffix
32,000, day-12 batches of 1,000, 384/288 MB caps and 600 seconds, with
unchanged ranking parameters. The waiting runner refuses to start if
Gayawaku finds an improvement or returns a failed/empty-route diagnostic.
The new rows go to pair-leading-summary.jsonl; no native runs overlap.

Gayawaku's smaller-batch recovery completed normally (tool 50646 exited
zero): score 49,344, 308,856 processed states, 328.038 seconds, 125 MB peak
RSS, all 14 batches complete, 170,990 beam trims. It is below the existing
50,105 record and remains a beam-limited search, not an optimality proof.
Full result/path is in gay-small-batch-summary.jsonl. The much smaller
active batches avoided the previous 354 MB working-budget stop.
The next waiting runner initially saw an already-exited supervisor during
its first /proc read; no native child launched. It now uses the recorded
supervisor start ticks and safely validates the terminal metadata even
when that PID has already disappeared. The prepared four pair lanes can
now start sequentially after the successful recovery.

## Current residual plan and added Gayawaku retention

The leading-pair supervisor (tool 36187) exited zero: Bajaar Blue/Oasis
56,147; L'Exquise Blue/Midas 56,817; Moonin Cornucopia/Midas 32,580;
Moonin Leaders Necklace/Midas 31,014. All four completed with zero beam
truncations; none improved its global fixture. Full paths and required
metrics are recorded in pair-leading-summary.jsonl.

The Gayawaku recovery's configured day-8 split occurred at day 8 with
50-prefix batches. The previous 1,000-prefix attempt actually waited until
day 9 because its day-8 frontier did not yet exceed one batch; the JSON
records both configured and realized split days. This accounts for the
different prefix sizes and is not evidence of a generation correctness bug.

1. In progress: double Gayawaku Blue/Midas suffix retention from 5,000 to
   10,000, preserving 50-prefix batches, broad 20,000, day-8 configuration,
   512/384 MB ceilings and the same ranking. The completed smaller suffix
   discarded 170,990 candidates, so wider retention adds actual state
   coverage. Allow 900 solver seconds (902 wrapper): the prior lane took
   328 seconds, and this experiment doubles retained work rather than
   extending a completed configuration's timeout alone. Record every metric
   and full path in gay-retention-summary.jsonl; stop on an improvement.
2. Pending: independently replay and promote any improved score, including
   fixture/Best.txt/expected-score updates and bounded native checks.
3. Pending if tied: review the remaining twenty map continuations, sixteen
   unused diversity lanes and eleven prefix lanes. Select a genuinely new
   coverage branch rather than repeating an untrimmed completed pair.
4. Pending: perform the completion residual audit after an additional record
   is found. The goal is active and no further record has yet been promoted.

No native code has changed since the successful diversity safe build,
focused regressions and full native suite. The additional native runs are
parameter-only experiments and remain sequential.

The next concrete coverage branch is prepared in
`results/more-records-20261009/run-prefix-remaining.py` (Python syntax checked).
Its supervisor is waiting in tool 9734 for the Gayawaku retention supervisor
to finish and will proceed only after terminal `completed` metadata. An
improvement or failed/empty-route diagnostic stops the queue for inspection.
The eleven unused six-action prefix lanes are Cornecopea, Normalia, Huangoz,
Fang-Mo, Jilliqo, Octoyashi, Kifuai, Ipartus, Doba Lao, Mahaji and Vilparino.
Each uses wide purchase coverage, broad 10,000, optimism 25, continuation 20,
384/288 MB ceilings and 600 solver seconds. Completed lanes are skipped;
full metrics and paths append to `prefix-summary.jsonl`. No native build,
test or solver overlaps the active Gayawaku process.

After those eleven lanes, tool 85103 is waiting on prefix supervisor
3193179/start ticks 86051080 to test Gayawaku's saved Magic Cleppsydra/Midas
pair with the small-batch schedule. This is a distinct pair from the active
Blue/Midas experiment: broad 20,000, suffix 10,000, configured split day 8,
50-prefix batches, optimism 50/continuation 20/skeleton 30, 512/384 MB ceilings,
and the default 600 solver seconds. Unlike the old unrestricted day-12 deep
campaign, it reserves continuation/action-structure coverage inside a fixed
pair and splits earlier. The script was Python syntax checked and will stop
without launching if its predecessor finds an improvement or fails.
Results/full route go to `gay-magic-small-batch-summary.jsonl`.

### Wider Blue/Midas result and current residual plan

Tool 60114 completed with exit zero. Gayawaku's 10,000-state suffix run
scored 49,344 (below 50,105), processed 452,140 states, generated 1,347,534
transitions, completed all fourteen batches in 702.133 native seconds /
702.191 wrapper seconds, and peaked at 217 MB RSS. Its 17-action route is
recorded in `gay-retention-summary.jsonl`. It still trimmed 174,653 candidates;
this is a completed bounded lane, not a global optimality certificate.
Against the 5,000-state suffix, it processed 143,284 additional states
(46.4%) without a score improvement. No further timeout-only retry is planned.

1. In progress: execute the eleven remaining six-action prefix lanes in
   tool 9734; Cornecopea started after the Gayawaku child and supervisor exited.
   Record complete metrics and paths, and stop for any improvement/diagnostic.
2. Pending: tool 85103's fixed Magic/Midas early-split Gayawaku run, after
   successful completion of the prefix supervisor.
3. Pending: independently replay and promote an improved global score, with
   the safe build, focused regressions and full suite.
4. Pending if tied: sixteen unused diverse-polisher lanes and twenty unused
   optimistic persistent-map continuations. Select the next lane from actual
   remaining coverage, with no overlapping native work.
5. Pending: perform the completion residual audit after an additional record.
   The goal remains active; none has been found in this campaign.

The first three resumed prefix lanes completed and tied their global scores:

| Town | Score | Processed | Native seconds | Peak RSS | Beam trims |
| --- | ---: | ---: | ---: | ---: | ---: |
| Cornecopea | 43,881 | 60,150 | 127.035 | 159 MB | 43,297 |
| Normalia | 37,527 | 66,126 | 86.504 | 156 MB | 53,602 |

Huangoz also completed at 37,527 / 66,385 processed / 148 MB; its remaining
metrics and suffix are present in `prefix-summary.jsonl`. Subsequent lanes
remain sequential under tool 9734.

An artifact audit found that native prefix-search JSON's `route` field is the
suffix only. To preserve complete reproducible paths without modifying native
code, `prefix-inputs.json` now freezes all 22 exact input fixtures as action
strings, records the defining functions, and hashes their source files. The
full route is `fixture.route[:result.prefix_actions] + result.route`.
`prefix-replay-paths.jsonl` reconstructs all 38 prefix results currently
recorded, including the three resumed lanes. New suffix rows already have
their immutable prefix in `prefix-inputs.json`; refresh the reconstructed
paths when tool 9734 finishes or finds an improvement. These reconstructed
paths have not been independently replayed; replay remains the promotion gate.

Fang-Mo's resumed prefix completed at 37,569 / 67,447 processed / 153 MB
peak RSS, tying its record. Jilliqo is the next live native lane. The Python
syntax-checked `reconstruct-prefix-paths.py` now refreshes complete paths from
the frozen inputs; it has reconstructed all 39 recorded prefix results.

The sixteen unused diversity-polisher lanes are prepared in
`run-diversity-remaining.py`, with a Python syntax check. Tool 95237 waits on
the Magic/Midas supervisor 3194870/start ticks 86080640 and launches only after
terminal successful completion. It skips the six previously completed towns,
uses beam 64 / 48 rounds / 500,000 evaluations, a 384 MB process cap and 600
seconds per lane, and records `/usr/bin/time` peak RSS, status, score,
evaluation count and full actions in `diversity-summary.jsonl`. An improvement
or failed lane stops the queue. No extra native build is scheduled: native
sources are unchanged since the passing diversity build/focused/full checks.

Current resumed-prefix checkpoint: eight completed lanes, 493,754 processed
states, all tied global records, with Doba Lao live and Mahaji/Vilparino still
pending. The six later completed results are:

| Town | Score | Processed | Native seconds | Peak RSS |
| --- | ---: | ---: | ---: | ---: |
| Huangoz | 37,527 | 66,385 | 87.191 | 148 MB |
| Fang-Mo | 37,569 | 67,447 | 90.416 | 153 MB |
| Jilliqo | 37,819 | 55,575 | 46.044 | 154 MB |
| Octoyashi | 40,056 | 45,217 | 26.209 | 145 MB |
| Kifuai | 37,547 | 66,156 | 83.111 | 153 MB |
| Ipartus | 37,734 | 66,698 | 76.302 | 158 MB |

All have complete bounded status and full metrics in `prefix-summary.jsonl`.
The reconstructed-path artifact now includes 43 prefix results, using the
frozen input snapshot. Its helper must be rerun after the remaining lanes.

### Prefix branch complete; fixed record-pair search active

Tool 9734 exited zero after all eleven remaining prefix lanes completed.
They processed 692,573 states in 874.832 aggregate native seconds, with
163 MB maximum RSS. All eleven tied their town's current global record;
none timed out or stopped on a memory diagnostic. Final rows are:

| Town | Score | Processed | Native seconds | Peak RSS | Beam trims |
| --- | ---: | ---: | ---: | ---: | ---: |
| Doba Lao | 45,543 | 66,343 | 80.740 | 163 MB | 55,321 |
| Mahaji | 37,589 | 66,234 | 85.452 | 141 MB | 53,872 |
| Vilparino | 37,569 | 66,242 | 85.828 | 157 MB | 54,518 |

All 22 towns now have six-action prefix coverage, in addition to the earlier
twelve-action prefix lanes. This branch remains heuristic; finishing it does
not establish global optimality. The reconstruction helper was rerun after
the supervisor exited, producing 46 complete paths in
`prefix-replay-paths.jsonl` from frozen `prefix-inputs.json`.

Current executable residual plan:

1. In progress: tool 85103's Gayawaku fixed Magic Cleppsydra/Midas run started
   only after the prefix supervisor exited. Broad 20,000 / suffix 10,000 /
   day-8 split / 50-prefix batches / optimism 50 / continuation 20 / skeleton
   30; 512 MB external, 384 MB working budget, 600 solver seconds. Collect
   full status, peak RSS, states, score and route from
   `gay-magic-small-batch-summary.jsonl`.
2. Pending: tool 95237's sixteen untested diversity-polisher lanes, already
   waiting on the fixed-pair supervisor; stop on improvement or failed lane.
3. Pending: independently replay and promote any improved global score,
   followed by the bounded build, focused regressions and full suite.
4. Pending if tied: twenty unused optimistic persistent-map continuations;
   choose subsequent runs from actual remaining coverage.
5. Pending: final completion residual audit after an additional verified
   record. The goal is active and no additional record has yet been found.

No native source changed during these parameter-only runs. The previous
safe build, focused polisher regressions and full native suite remain passing.

The remaining twenty optimistic persistent-map continuations are now a
concrete sequential queue in Python syntax-checked
`run-persistent-optimistic-remaining.py` (tool 48283). It waits on diversity
supervisor 3201339/start ticks 86139793, and requires terminal successful
metadata before acquiring the campaign file lock. It skips completed
Jilliqo/Mehaz, retains the 5 GiB disk floor and inherited lock descriptor,
and uses the already measured broad 6,000 / optimism 100 / continuation 0 /
300,000 atomic-expansion profile at 384/288 MB and 600 solver seconds per
town. Before/after map counts, peak RSS, processed states, status, score and
path append to `persistent-optimistic-summary.jsonl`. This closes the
previously unexecuted plan branch with scheduled work, not a claimed result;
the native Gayawaku fixed-pair process is still the only live solver.

### Fixed Magic/Midas partial result; diverse polisher resumed

Tool 85103 exited zero after the solver's internal 600-second deadline.
Gayawaku rediscovered its 50,105 global record with a complete 18-action
path, but the search itself remains incomplete: 183,985 processed states,
625,887 generated transitions, 104,973 beam trims, five suffix batches
started, 661 day-8 prefix states, 231 MB peak RSS, 600.009 native seconds /
600.067 wrapper seconds. The exact stop reason is
`internal time budget reached after 600 seconds`. The full path and all
metrics are in `gay-magic-small-batch-summary.jsonl`. This is a tied partial
lane, not a completed exhaustive search or additional record.

Tool 95237 then resumed the sixteen unused diversity-polisher lanes;
Normalia is the first live lane. The current plan is:

1. In progress: finish the sixteen unused diversity lanes sequentially, with
   the existing 500,000-evaluation / 384 MB / 600-second bounds, preserving
   full paths and measured RSS. Stop on improvement or failure.
2. Pending: tool 48283's twenty optimistic persistent-map continuations, after
   successful termination of the diversity supervisor.
3. Pending: independently replay/promote any improved global score and run
   the bounded build, focused regressions and full suite.
4. Open coverage branch if tied: Gayawaku Magic/Midas's unvisited suffix
   prefixes. Evaluate smaller per-batch retention or a different batch/ranking
   order against this measured partial result; do not call it exhausted.
5. Pending: completion residual audit after an additional verified record.
   No additional record has been found; the goal remains active.

Eight resumed diverse-polisher lanes have now completed: Normalia 37,527;
L'Exquise 56,817; Gayawaku 50,105; Eduming 81,321; Petrinov 45,578;
Bajaar 56,147; Port-Willam 42,296; Skjollird 47,065. Each evaluated 500,000
routes, tied its global record, and recorded its full best action path in
`diversity-summary.jsonl`. These eight used 285.061 aggregate wrapper seconds
and peaked at 168,436 KiB (164.5 MiB). The evaluation cap bounds a completed
polisher lane; it does not certify that the neighborhood is exhausted.
Cornecopea is live under tool 95237, with eight resumed diversity lanes still
unfinished. The twenty persistent continuations remain waiting in tool 48283.

### All-town diversity branch completed; persistent continuations live

Tool 95237 exited zero. All sixteen resumed diversity lanes completed,
evaluating 8,000,000 routes in 908.660 aggregate wrapper seconds, with a
170,952 KiB peak RSS. The final eight rows are:

| Town | Score | Wrapper seconds | Peak RSS (KiB) | Rounds |
| --- | ---: | ---: | ---: | ---: |
| Cornecopea | 43,881 | 75.141 | 161,364 | 11 |
| Fang-Mo | 37,569 | 79.105 | 168,376 | 10 |
| Octoyashi | 40,056 | 56.900 | 168,312 | 9 |
| Kifuai | 37,547 | 109.941 | 168,076 | 10 |
| Ipartus | 37,734 | 139.766 | 170,952 | 10 |
| Doba Lao | 45,543 | 11.177 | 143,244 | 11 |
| Mahaji | 37,589 | 58.204 | 156,948 | 10 |
| Vilparino | 37,569 | 93.365 | 168,200 | 10 |

Each evaluated 500,000 routes, tied its record, and recorded full actions.
The whole diversity branch now contains exactly one completed lane per town:
22 towns / 11,000,000 evaluated routes / no additional global score record.
Native sources remain unchanged since the passing bounded checks.

Fresh executable residual plan:

1. In progress: tool 48283 acquired the persistent campaign lock after the
   diversity supervisor exited and started Bajaar's continuation. Finish the
   twenty previously unstarted optimistic lanes sequentially at broad 6,000,
   optimism 100, continuation 0, 300,000 atomic expansions, 384/288 MB caps,
   and 600 solver seconds. Preserve before/after graph counts and all required
   run metrics/path in `persistent-optimistic-summary.jsonl`; stop on a higher
   global score or failed lane.
2. Pending: independent replay and promotion of any improved score, followed
   by the safe build, focused regressions and full native suite.
3. Open if tied: Magic/Midas Gayawaku suffix prefixes left unvisited by the
   measured deadline. Choose smaller retention or a different batch/ranking
   schedule for actual additional coverage.
4. Pending: completion residual audit after an additional verified record.
   The original objective remains active and unachieved.

Bajaar live-map checkpoints were read through read-only SQLite transactions
while its bounded continuation remained active:

| Observation (UTC) | Saved nodes | Open | Partial | Solved | Best score |
| --- | ---: | ---: | ---: | ---: | ---: |
| 2026-10-10 05:59:32 | 1,297,528 | 219,116 | 80,775 | 997,637 | 56,147 |
| 2026-10-10 06:01:58 | 1,358,117 | 216,834 | 80,903 | 1,060,380 | 56,147 |

The graph is visibly growing, but these are whole-map live snapshots rather
than final per-run state counts. Tool 48283 remains responsible for recording
the exact initial/final counts, native processed states, peak RSS,
completion/stop status and full path when the lane exits. No additional
score record is claimed from these checkpoints.

### Bajaar external deadline, route recovery and timed continuation

Tool 48283 stopped after Bajaar's external 602-second wrapper limit returned
124 (602.142 wrapper seconds). No benchmark JSON was emitted. Its exact
durable before/after map counts were recorded: 1,072,383 -> 1,516,521 unique
nodes (+444,138), 972,557 -> 1,210,233 solved nodes (+237,676), best score
56,147, root still partial. This failed diagnostic has no final native
processed-state count or terminal peak RSS; the last live high-water sample
was 169,000 KiB. Do not replace the missing final measurements with that
sample or count the lane as a completed positive result.

After checking the old native wrapper/supervisor had exited, the bounded
one-expansion recovery (tool 2387) completed successfully in 5.299 wrapper
seconds. It retrieved the full 56,147 route, exactly matching the frozen
Bajaar fixture, and measured 41,136 KiB peak RSS externally. The recovery
pruned one mapped node (zero new processed states), did not add unique nodes,
and does not supply the failed run's missing metrics. Its complete result is
in `persistent-recovery-summary.jsonl`.

Tool 7978 now runs Python syntax-checked `run-persistent-optimistic-timed.py`.
It retries Bajaar on the advanced durable frontier, then attempts the nineteen
remaining towns; successful Jilliqo/Mehaz rows are skipped. Each lane retains
broad 6,000 / optimism 100 / continuation 0 / 300,000 atomic-expansion cap /
384 MB external and 288 MB working caps. An explicit 540-second internal
deadline sits inside the 602-second native wrapper, leaving output/shutdown
margin; `/usr/bin/time` runs through a bounded 604-second outer wrapper and
records peak RSS even if the native deadline fires. Full commands, metrics,
before/after counts and paths append to `persistent-optimistic-summary.jsonl`
with `timed540` lane names. A failure or improved global score still stops
the queue. Native code and game rules are unchanged.

A separate Wait coverage check found `Game::event_wait` delegates to an empty
Buy, which is already tested directly against Wait in the full native suite.
The polisher's Interest/NPC replacements already consider an empty Buy before
food retiming, so adding a second label would not add that local behavior.
Normal search's historical Wait omission was based on a beam-1,000 screen,
not a global dominance proof; no claim of global optimality follows from it.

Current plan: finish the timed persistent continuations; independently replay
and promote any higher global score with bounded checks; if tied, revisit
Gayawaku's unvisited fixed-pair suffix prefixes. The goal remains active and
no additional score record has been found.

The timed Bajaar continuation returned exit zero with a full benchmark JSON
and 17-action path. It tied 56,147, processed 67,996 states, performed 246,794
atomic expansions (35,394 reused-child visits), and stopped on its session
time budget after 545.343 native seconds / 545.550 wrapper seconds. Peak RSS
was 175,444 KiB externally (171 MB native sample). Its root is still partial;
`search_complete` is false. The explicit internal deadline allowed final
metrics/path to be emitted before the external limit, unlike the prior attempt.

The map grew from 1,516,521 to 1,758,699 unique nodes (+242,178), and from
1,210,234 to 1,457,926 solved nodes (+247,692). Together the failed preserved
attempt and the timed continuation added 686,316 unique Bajaar states without
improving its score. These are durable coverage gains, not a record or proof.
The exact result and path are in `persistent-optimistic-summary.jsonl`.

All 22 improvement thresholds were checked against the current saved
Computer entries in `Best.txt`, accounting for its underscore spellings and
the separate entries marked probable cheats. They match the campaign's
`record_before` values, including Moonin 37,584.

Tool 7978 is now running Port-Willam with the timed profile; eighteen other
unstarted towns remain after it. Continue sequentially and stop on improvement
or failure. Native source is unchanged; bounded checks remain passing from
the previous build. The additional-record objective remains unachieved.

### Port-Willam timed continuation result

Port-Willam returned exit zero with a full 42,296 route, tying its record.
It processed 123,544 states and performed 295,315 atomic expansions, including
59,469 reused-child visits. The session stopped on its internal time budget:
543.480 native seconds / 543.720 wrapper seconds, 179,208 KiB externally
measured peak RSS (175 MB native sample). `search_complete` is false and the
root remains partial; this is a successful bounded partial continuation.

The map grew from 1,076,745 to 1,521,847 unique nodes (+445,102), and from
972,746 to 1,211,403 solved nodes (+238,657). Before/after counts, command,
environment, status, all native counters, measured RSS and the full 18-action
path are in `persistent-optimistic-summary.jsonl`.

The timed supervisor (tool 7978) has started Skjollird sequentially; seventeen
unstarted towns follow it. Continue that queue, stop and independently replay
any higher global score, and preserve the pending Gayawaku suffix coverage
branch if the persistent lanes tie. No additional record has been found, so
the goal remains active. No native source changed during this continuation.

### Skjollird atomic-budget result

Skjollird returned exit zero after completing its 300,000-atomic-expansion
session budget in 518.679 native seconds / 518.832 wrapper seconds. It
processed 120,434 states, reused 55,770 child visits, and tied 47,065.
Externally measured peak RSS was 183,352 KiB (179 MB native sample). The full
18-action path and all metrics are recorded in `persistent-optimistic-summary.jsonl`.
The root remains partial and `search_complete` is false; completing the
session budget does not complete the exact graph.

Unique nodes increased 1,073,716 -> 1,525,210 (+451,494); solved nodes
973,587 -> 1,217,340 (+243,753). The timed supervisor has now started
L'Exquise, with sixteen unstarted towns after it. All native work remains
sequential under the same 384/288 MB caps.

A comparison of completed persistent witnesses against frozen fixtures found
Mehaz, Bajaar and Port-Willam identical, and Jilliqo differing only by one
adjacent Trader/Merchant swap covered by the existing neighbor generator.
These rows do not establish a new seed-import requirement. Continue the
pending map coverage rather than introducing an unsupported native change.
No additional score record has yet been found; the goal remains active.

### L'Exquise timed continuation result

L'Exquise returned exit zero, tying 56,817 with its full 17-action path. It
processed 115,621 states, performed 272,548 atomic expansions, and reused
72,952 child visits before its session deadline. Native time was 543.807
seconds; wrapper time 543.991 seconds; externally measured peak RSS
157,140 KiB (154 MB native sample). The exact stop reason is
`persistent search session time budget reached`; the root remains partial
and `search_complete` is false.

The graph grew 1,001,552 -> 1,446,751 unique nodes (+445,199) and
889,357 -> 1,103,594 solved nodes (+214,237). Required measurements and
the full path are in `persistent-optimistic-summary.jsonl`. Tool 7978 has
started Eduming under the same bounds; fifteen unstarted towns follow it.

Continue the pending durable-map coverage, stop for independent verification
if any score exceeds the current saved Computer record, and revisit the
unvisited Gayawaku suffix branch if all lanes tie. The original additional-
record objective remains active and unachieved. No native source changed.

### Eduming atomic-budget result

Eduming returned exit zero after 300,000 atomic expansions, tying 81,321.
It processed 122,766 states and reused 45,567 child visits in 362.734 native
seconds / 362.881 wrapper seconds. Externally measured peak RSS was
157,216 KiB (154 MB native sample). Its full 18-action path and all counters
are in `persistent-optimistic-summary.jsonl`. The root remains partial and
`search_complete` is false; the session budget completed, not the graph.

Unique nodes increased 1,050,845 -> 1,463,871 (+413,026); solved nodes
974,863 -> 1,230,740 (+255,877). Tool 7978 has started Petrinov sequentially,
with fourteen unstarted towns after it. Continue the pending map runs, stop
and independently verify any higher global score, and keep the Gayawaku
unvisited-suffix branch open if all results tie. No additional record has
been found and no native source changed.

### Petrinov atomic-budget result

Petrinov returned exit zero after 300,000 atomic expansions, tying 45,578.
It processed 130,883 states and reused 87,065 child visits in 397.311 native
seconds / 397.533 wrapper seconds. Peak RSS measured externally was
170,936 KiB (167 MB native sample). Its full 17-action path and exact command,
environment and counters are in `persistent-optimistic-summary.jsonl`.
The root is still partial and `search_complete` is false; only the session
budget completed.

Unique nodes grew 1,067,897 -> 1,496,448 (+428,551), and solved nodes
972,691 -> 1,220,538 (+247,847). Tool 7978 has started Cornecopea sequentially;
thirteen unstarted towns follow it. Continue that queue under the same caps,
stop for independent replay of a higher global score, and retain the open
Gayawaku suffix branch if all lanes tie. The objective is still active and
unachieved. Native sources and the previously passing bounded checks are
unchanged.

### Cornecopea atomic-budget result

Cornecopea returned exit zero after 300,000 atomic expansions, tying 43,881.
It processed 127,893 states and reused 70,364 child visits in 321.009 native
seconds / 321.139 wrapper seconds. Externally measured peak RSS was
183,224 KiB (179 MB native sample). The full 18-action path, exact command,
environment and counters are in `persistent-optimistic-summary.jsonl`.
The root remains partial and `search_complete` is false.

Unique nodes increased 1,074,125 -> 1,519,519 (+445,394); solved nodes
972,834 -> 1,213,354 (+240,520). Tool 7978 has started Normalia sequentially,
with twelve unstarted towns after it. Continue the remaining map coverage,
stop for independent replay if a higher global score appears, and retain
the unvisited Gayawaku suffix branch if all lanes tie. No additional record
has been found; the goal remains active and no native source changed.

### Normalia atomic-budget result

Normalia returned exit zero after 300,000 atomic expansions, tying 37,527.
It processed 123,734 states and reused 49,573 child visits in 443.040 native
seconds / 443.162 wrapper seconds. Externally measured peak RSS was
169,320 KiB (165 MB native sample). The full 18-action path and every required
measurement are recorded in `persistent-optimistic-summary.jsonl`.
`search_complete` is false and the root remains partial; completing the
session budget is not an optimality certificate.

Unique nodes grew 1,034,316 -> 1,485,109 (+450,793), and solved nodes
939,580 -> 1,182,065 (+242,485). Tool 7978 has started Huangoz sequentially;
eleven unstarted towns follow it. Continue the remaining map coverage under
the same caps, stop for independent verification if a higher global score
appears, and retain the unvisited Gayawaku suffix branch if all lanes tie.
No additional record has been found and no native source changed.

### Huangoz atomic-budget result

Huangoz returned exit zero after 300,000 atomic expansions, tying 37,527.
It processed 123,765 states and reused 50,294 child visits in 452.062 native
seconds / 452.211 wrapper seconds. Externally measured peak RSS was
173,892 KiB (170 MB native sample). Its full 18-action path, exact command,
environment and counters are recorded in `persistent-optimistic-summary.jsonl`.
The root remains partial and `search_complete` is false.

Unique nodes grew 1,072,497 -> 1,519,547 (+447,050); solved nodes increased
974,359 -> 1,218,413 (+244,054). Tool 7978 has started Fang-Mo sequentially;
ten unstarted towns follow it. Continue that queue under the existing caps,
independently replay any higher global score, and keep the unvisited Gayawaku
suffix branch open if all lanes tie. No additional record has been found.

The final-day purchase hypothesis was checked against `Game::check_points`:
held cargo contributes its current purchase price, so buying cargo at that
price does not create the hypothesized sale-value gain. No rule or native
source change was justified by that hypothesis.

### Prepared Gayawaku suffix coverage follow-up

The current plan remains the sequential persistent queue (Fang-Mo in progress,
ten towns pending), followed by independent replay of any improvement. If that
queue finishes without an improvement, run
`results/more-records-20261009/run-gay-magic-coverage.py`. It requires the campaign
lock and a completed prior supervisor, so preparing it does not launch an
overlapping solver. Python syntax compilation passed; it has not run yet.

The prepared Magic Cleppsydra/Midas lane retains the day-8 split, broad width
20,000, 50-prefix batches, structure-interleaved order, and 50/20/30 ranking
shares. It reduces suffix retention from 10,000 to 2,500 to reach more prefix
batches: the earlier 600-second run started only five batches out of the
661-prefix frontier (fourteen batches total). This trades suffix depth for
previously unvisited prefix coverage; completion and improved coverage must
be measured, not assumed. The explicit internal time limit is 540 seconds,
external cap 512 MB, and internal memory budget 384 MB. The script records
external peak RSS, native counters/status, exact command/environment and the
full action path. Do not count a tie or batch coverage as a new record.

### Fang-Mo atomic-budget result

Fang-Mo returned exit zero after 300,000 atomic expansions, tying 37,569.
It processed 129,359 states and reused 58,061 child visits in 471.183 native
seconds / 471.365 wrapper seconds. Externally measured peak RSS was
169,900 KiB (166 MB native sample). Its full 18-action path and required
measurements are recorded in `persistent-optimistic-summary.jsonl`.
The root remains partial and `search_complete` is false.

Unique nodes increased 1,072,471 -> 1,525,489 (+453,018); solved nodes grew
974,520 -> 1,215,256 (+240,736). Tool 7978 started Octoyashi sequentially;
its native process was verified running. Nine unstarted towns follow it.
Continue this queue, independently verify any higher global score, then run
the prepared Gayawaku coverage lane if all results tie. No additional record
has been found; native sources and their previously passing checks are unchanged.

### Octoyashi atomic-budget result

Octoyashi returned exit zero after 300,000 atomic expansions, tying 40,056.
It processed 128,717 states and reused 59,435 child visits in 519.073 native
seconds / 519.228 wrapper seconds. Externally measured peak RSS was
174,544 KiB (171 MB native sample). Its full 18-action path, exact command,
environment and all required counters are in `persistent-optimistic-summary.jsonl`.
The root remains partial and `search_complete` is false; only the session's
atomic expansion budget completed.

Unique nodes grew 1,068,537 -> 1,524,624 (+456,087); solved nodes increased
973,987 -> 1,214,396 (+240,409). Tool 7978 started Kifuai sequentially;
eight unstarted towns follow it. Continue that queue within the current caps,
independently replay any higher global score, and run the prepared Gayawaku
suffix coverage lane if all results tie. No additional record has been found.
Native sources and their passing bounded checks are unchanged.

### Kifuai atomic-budget result

Kifuai returned exit zero after 300,000 atomic expansions, tying 37,547.
It processed 128,339 states and reused 62,890 child visits in 456.554 native
seconds / 456.794 wrapper seconds. Externally measured peak RSS was
177,436 KiB (174 MB native sample). Its full 18-action path and all required
measurements are recorded in `persistent-optimistic-summary.jsonl`.
The root remains partial and `search_complete` is false.

Unique nodes increased 1,069,350 -> 1,515,946 (+446,596); solved nodes grew
973,952 -> 1,216,392 (+242,440). Tool 7978 started Ipartus sequentially;
its native process was verified live. Seven unstarted towns follow it.
Continue the queue within the existing caps, independently replay any higher
global score, then run the prepared Gayawaku suffix coverage lane if all
results tie. No additional record has been found and no native source changed.

### Ipartus atomic-budget result

Ipartus returned exit zero after 300,000 atomic expansions, tying 37,734.
It processed 123,422 states and reused 49,510 child visits in 460.361 native
seconds / 460.568 wrapper seconds. Externally measured peak RSS was
175,480 KiB (171 MB native sample). Its full 18-action path, exact command,
environment and required counters are in `persistent-optimistic-summary.jsonl`.
The root remains partial and `search_complete` is false.

Unique nodes grew 1,069,627 -> 1,518,579 (+448,952); solved nodes increased
974,321 -> 1,217,832 (+243,511). Tool 7978 started Doba Lao sequentially;
its native process was verified running. Six unstarted towns follow it.
Continue the queue within its current caps, independently replay any higher
global score, then run the prepared Gayawaku suffix coverage lane if all
results tie. No additional record has been found and no native source changed.

### Doba Lao atomic-budget result

Doba Lao returned exit zero after 300,000 atomic expansions, tying 45,543.
It processed 120,082 states and reused 47,083 child visits in 381.837 native
seconds / 381.960 wrapper seconds. Externally measured peak RSS was
175,876 KiB (172 MB native sample). Its full 18-action path, exact command,
environment and required counters are in `persistent-optimistic-summary.jsonl`.
The root remains partial and `search_complete` is false.

Unique nodes increased 1,066,467 -> 1,495,846 (+429,379); solved nodes grew
972,197 -> 1,222,924 (+250,727). Tool 7978 started Mahaji sequentially;
its native process was verified running. Five unstarted towns follow it.
Continue that queue within the existing caps, independently replay any higher
global score, then run the prepared Gayawaku suffix coverage lane if all
results tie. No additional record has been found and no native source changed.

### Mahaji atomic-budget result

Mahaji returned exit zero after 300,000 atomic expansions, tying 37,589.
It processed 124,024 states and reused 53,360 child visits in 422.530 native
seconds / 422.692 wrapper seconds. Externally measured peak RSS was
165,592 KiB (161 MB native sample). Its full 18-action path, exact command,
environment and required counters are in `persistent-optimistic-summary.jsonl`.
The root remains partial and `search_complete` is false.

Unique nodes increased 1,072,228 -> 1,518,016 (+445,788); solved nodes grew
973,523 -> 1,218,693 (+245,170). Tool 7978 started Vilparino sequentially;
its native process was verified live. Four unstarted towns follow it.
Continue that queue within its current caps, independently replay any higher
global score, then run the prepared Gayawaku suffix coverage lane if all
results tie. No additional record has been found and no native source changed.

### Vilparino atomic-budget result

Vilparino returned exit zero after 300,000 atomic expansions, tying 37,569.
It processed 124,763 states and reused 56,083 child visits in 448.977 native
seconds / 449.105 wrapper seconds. Externally measured peak RSS was
163,240 KiB (159 MB native sample). Its full 18-action path, exact command,
environment and required counters are in `persistent-optimistic-summary.jsonl`.
The root remains partial and `search_complete` is false.

Unique nodes increased 1,068,071 -> 1,514,149 (+446,078); solved nodes grew
973,195 -> 1,217,146 (+243,951). Tool 7978 started Moonin sequentially;
its native process was verified live. Three unstarted towns follow it.
Moonin's improvement threshold remains 37,584, including the earlier verified
record. Continue the remaining queue under its current caps, independently
replay any higher global score, then run the prepared Gayawaku suffix coverage
lane if none improves. No additional record has been found and no native
source changed.

### Moonin session-deadline result

Moonin returned exit zero at its session deadline with 37,527, below the
current saved Computer record of 37,584. It processed 126,683 states, performed
288,542 atomic expansions and reused 66,599 child visits in 544.164 native
seconds / 544.380 wrapper seconds. Externally measured peak RSS was
183,908 KiB (180 MB native sample). Its full 18-action path, exact command,
environment and required counters are in `persistent-optimistic-summary.jsonl`.
The stop reason is `persistent search session time budget reached`;
the root remains partial and `search_complete` is false.

Unique nodes grew 1,227,215 -> 1,694,128 (+466,913); solved nodes increased
1,054,126 -> 1,280,714 (+226,588). Tool 7978 started Sanctifan sequentially;
its native process was verified running. Tocaccialli and Gayawaku are the two
remaining unstarted towns. Continue that queue within its current caps,
independently replay any higher global score, then run the prepared Gayawaku
suffix coverage lane if none improves. No additional record has been found
and no native source changed.

### Sanctifan atomic-budget result

Sanctifan returned exit zero after 300,000 atomic expansions, tying 55,728.
It processed 124,706 states and reused 47,317 child visits in 403.031 native
seconds / 403.182 wrapper seconds. Externally measured peak RSS was
176,892 KiB (173 MB native sample). Its full 19-action path, exact command,
environment and required counters are in `persistent-optimistic-summary.jsonl`.
The root remains partial and `search_complete` is false.

Unique nodes increased 1,209,670 -> 1,663,745 (+454,075); solved nodes grew
1,061,238 -> 1,305,259 (+244,021). The map's stored best rose 55,602 -> 55,728,
matching the current saved Computer record. This is a rediscovery, not an
additional global record. Tool 7978 started Tocaccialli sequentially; its
native process was verified live. Gayawaku is the last unstarted town.
Continue these two lanes under their current caps, independently replay any
higher global score, then run the prepared Gayawaku suffix coverage lane if
none improves. No native source changed.

### Tocaccialli atomic-budget result

Tocaccialli returned exit zero after 300,000 atomic expansions with 47,621,
below the current saved Computer record of 49,634. It processed 123,675 states
and reused 48,778 child visits in 425.386 native seconds / 425.584 wrapper
seconds. Externally measured peak RSS was 179,848 KiB (175 MB native sample).
Its full 19-action path, exact command, environment and required counters
are in `persistent-optimistic-summary.jsonl`. The root remains partial and
`search_complete` is false.

Unique nodes increased 1,190,060 -> 1,629,707 (+439,647); solved nodes grew
1,064,055 -> 1,313,215 (+249,160). Tool 7978 started Gayawaku, the last town
in this sequential persistent queue; its native process was verified running.
Finish this lane under its current caps, independently replay any higher
global score, then run `run-gay-magic-coverage.py` if the completed queue
contains no improvement. The goal remains active and unachieved. No native
source changed.

### Gayawaku persistent result and completed sweep

Gayawaku returned exit zero at its session deadline with 49,981, below the
saved Computer record of 50,105. It processed 108,670 states, performed
277,183 atomic expansions and reused 43,273 child visits in 545.916 native
seconds / 546.046 wrapper seconds. Externally measured peak RSS was
160,204 KiB (157 MB native sample). The full 18-action path and required
measurements are recorded in `persistent-optimistic-summary.jsonl`. Its
stop reason is `persistent search session time budget reached`; the root
remains partial and `search_complete` is false.

Unique nodes grew 1,195,508 -> 1,613,700 (+418,192); solved nodes increased
1,068,267 -> 1,298,938 (+230,671). The map best rose 49,753 -> 49,981.
The returned path differs from the frozen saved route in the placement of
one Camel and one Trader around the Witch, and in the first cargo portfolio.
It uses the same encounter pair and final action types. This comparison
does not establish a missing polishing feature or a new record.

Tool 7978 exited zero and the final native process ended. The latest
successful run for each of all 22 towns totals 2,680,038 processed states,
6,480,382 atomic expansions, 9,593,691 added unique nodes, 5,311,991 added
solved nodes, and 10,210.117 native seconds. None exceeds its saved record.
These totals exclude the initial failed Bajaar attempt and its separate
recovery. The twenty externally timed runs peaked at 183,908 KiB; the first
two successful lanes retain their native RSS samples in the raw rows.
Every root is still partial; this sweep supplies coverage, not a proof of
global optimality.

### Current plan: Gayawaku suffix coverage

1. In progress: tool 83427 runs `run-gay-magic-coverage.py` after the prior
   queue exited, with the campaign lock held and its native process verified
   live. Broad retention is 20,000, suffix retention 2,500, batch size 50,
   split day 8, and ranking shares 50/20/30. The explicit search limit is
   540 seconds, external memory 512 MB, internal memory 384 MB.
2. Pending: record peak RSS, processed states, completion/timeout status,
   score, full action path and started suffix batch count in
   `gay-magic-coverage-summary.jsonl`; compare against the earlier 5/14-batch
   timed run before claiming increased prefix coverage.
3. Pending: independently replay any score above 50,105, then promote it
   and run the repository's bounded native checks. If this lane ties, use its
   measured completion and retained-state coverage to choose the next ranking
   mix rather than repeat a completed profile with a longer timeout alone.

No additional record has been found. The goal remains active; no native
source changed in this sweep.

### Gayawaku 2,500-state suffix coverage result

Tool 83427 exited zero. Gayawaku tied 50,105 in 247.611 native seconds /
247.633 wrapper seconds, processing 233,281 states and generating 820,309.
Externally measured peak RSS was 87,052 KiB (84 MB native sample). All
fourteen batches of the 661-prefix day-8 frontier finished; `search_complete`
is true and the stop reason is null. There were 171,080 beam prunes, so this
completion describes the retained search, not a global optimality proof.
The full 18-action path equals the frozen saved route. Every required
measurement and the reproducible command are in `gay-magic-coverage-summary.jsonl`.

This resolves the unvisited-batch branch at suffix retention 2,500: the earlier
10,000-retention run started only five of these fourteen batches within
600 seconds. The new run visited 49,296 more processed states while trading
depth within each batch for coverage of all prefixes. It found no new record.

### Current plan: increase Gayawaku suffix retention

1. In progress: tool 67300 runs `run-gay-magic-coverage.py --suffix-limit 5000
   --seconds 840`, after the preceding process exited. Its native process
   was verified live. This doubles suffix retention with unchanged broad
   retention, batch size, split day, encounter pair and ranking shares.
   The extra time supports added retained-state coverage; it does not merely
   rerun the completed 2,500-retention configuration. Internal search time is
   840 seconds, both external time caps are 900 seconds, external memory
   512 MB and internal memory 384 MB.
2. Pending: record score, path, peak RSS, processed states, batch count and
   completion/timeout status in `gay-magic-coverage-summary.jsonl`. Compare
   coverage with the completed 2,500-retention run.
3. Pending: independently replay any score above 50,105, then promote it
   and run bounded native checks. If this lane ties, use a different ranking
   mix for another independent coverage lane.

The campaign script now accepts explicit suffix retention and search seconds,
preserving the original defaults and recording the chosen values in each
artifact name and exact command. Search seconds must be positive and at most
900; external time caps also never exceed 900. Python syntax compilation and
CLI help passed. No native source changed and no additional record has been
found; the goal remains active.

### Gayawaku 5,000-state suffix result

Tool 67300 exited zero. Gayawaku tied 50,105 after finishing all fourteen
batches of the 661-prefix day-8 frontier. Native time was 610.427 seconds;
wrapper time 610.450 seconds; external peak RSS 145,652 KiB. It processed
366,428 states and generated 1,267,089 transitions. `search_complete` is true
and the stop reason is null. There were 242,576 beam prunes; completion still
applies only to this retained search. Its full 18-action path equals the frozen
saved route. Required measurements and the exact command are recorded in
`gay-magic-coverage-summary.jsonl`.

Doubling suffix retention added 133,147 processed states compared with the
completed 2,500-retention run. The longer search budget therefore supported
measured additional coverage, but no additional record was found. This closes
the 50/20/30 ranking lane at suffix retention 5,000; do not repeat it with a
longer timeout alone.

### Current plan: independent Gayawaku ranking mix

1. In progress: tool 98052 runs `run-gay-magic-coverage.py --suffix-limit 5000
   --seconds 540 --optimistic-share 100 --continuation-lane-share 0
   --skeleton-share 0`. The preceding process ended and its native process
   was verified live. Broad retention 20,000, suffix retention 5,000,
   50-prefix batches, day-8 split, structure-interleaved batch order and
   Magic Cleppsydra/Midas encounters remain the same; within-batch ranking
   changes to the optimistic mix. Memory caps are 512 MB external / 384 MB
   internal, with a 540-second search budget and 602/604-second external caps.
2. Pending: record score, full route, peak RSS, processed states, batch count
   and completion/timeout status; compare with the completed 50/20/30 mix.
3. Pending: independently replay any score above 50,105, promote it and run
   bounded native checks. If this mix ties, choose another independent
   continuation-ranking mix from the remaining coverage evidence.

The campaign script accepts these three ranking shares explicitly, validates
their percentage range, and records each in the artifact name and exact
command. Its original defaults remain unchanged. Python syntax compilation
and CLI help passed. No native source changed and the goal remains active
and unachieved.

### Gayawaku optimistic mix result

Tool 98052 exited zero. Gayawaku tied 50,105, finishing all fourteen batches
of the 661-prefix day-8 frontier in 452.682 native seconds / 452.707 wrapper
seconds. Externally measured peak RSS was 152,164 KiB. It processed 366,428
states, generated 1,271,684 transitions, and recorded 227,686 beam prunes.
`search_complete` is true and the stop reason is null; completion covers the
retained search. Its full 18-action path equals the frozen saved route.
All required measurements and the exact command are recorded in
`gay-magic-coverage-summary.jsonl`.

The processed-state count matches the preceding 50/20/30 mix, but generated
transitions differ (1,271,684 vs 1,267,089), as do beam prunes (227,686 vs
242,576). Continuation lane owners changed 85,096 -> 0 and skeleton reserved
states 9,379 -> 0. Source inspection confirmed that suffix batches use the
layered frontier and its selection consumes the configured shares. Equal
processed counts do not establish equal retained states or ignored settings;
no native defect or change is justified by this observation.

### Current plan: Gayawaku continuation retention mix

1. In progress: run `run-gay-magic-coverage.py --suffix-limit 5000 --seconds
   540 --optimistic-share 0 --continuation-lane-share 80 --skeleton-share 20`.
   The preceding solver exited and the new native process was verified live.
   This keeps the same broad/suffix retention, split day, batch size and
   encounter pair while reserving more continuation choices. Caps remain
   512 MB external / 384 MB internal, with a 540-second search budget and
   602/604-second external time limits.
2. Pending: record score, full route, peak RSS, processed/generated states,
   batch count and completion/timeout status in the existing raw summary.
3. Pending: independently replay any score above 50,105, promote it and run
   bounded native checks. If the run ties, use its measured coverage to select
   the next search; do not treat a completed retained search as global proof.

No additional record has been found and no native source changed. The goal
remains active and unachieved.

### Gayawaku continuation mix deadline result

Tool 69285 exited zero after its internal deadline, tying 50,105. Native time
was 540.018 seconds; wrapper time 540.045 seconds; externally measured peak
RSS 147,504 KiB. It processed 220,920 states, generated 795,468 transitions,
and recorded 164,436 beam prunes. It started eight of fourteen batches from
the 661-prefix frontier. `search_complete` is false and the stop reason is
`internal time budget reached after 540 seconds`. The full 18-action path
equals the frozen saved route. Required measurements and the reproducible
command are in `gay-magic-coverage-summary.jsonl`.

This leaves later prefix batches unvisited under the 0/80/20 ranking mix at
5,000 suffix retention. The next run retains 2,500 per suffix with the same
ranking, split day and batch size, trading depth for coverage of those later
batches. It starts only after the preceding native process ended.

### Current plan: finish continuation mix prefix coverage

1. In progress: `run-gay-magic-coverage.py --suffix-limit 2500 --seconds 540
   --optimistic-share 0 --continuation-lane-share 80 --skeleton-share 20`;
   its native process was verified live. Caps remain 512 MB external /
   384 MB internal and 540 seconds internal / 602/604 seconds external.
2. Pending: record score, route, peak RSS, processed/generated states,
   started batch count and completion/timeout status. Compare with the
   preceding eight-batch deadline result before claiming wider prefix coverage.
3. Pending: independently replay any higher global score and promote it with
   bounded native checks. If this lane finishes and ties, choose a profile
   that changes retained-state coverage, such as smaller prefix batches;
   avoid merely extending a completed configuration's timeout.

No additional record has been found. The goal remains active and unachieved;
no native source changed.

### Gayawaku smaller-suffix continuation result

Tool 59801 exited zero. The 0/80/20 mix at suffix retention 2,500 finished
all fourteen batches of the 661-prefix frontier and tied 50,105. Native time
was 314.590 seconds; wrapper time 314.613 seconds; externally measured peak
RSS 93,636 KiB. It processed 233,281 states, generated 825,234 transitions,
and recorded 180,312 beam prunes. `search_complete` is true, the stop reason
is null, and the full 18-action path equals the frozen saved route. Required
measurements and the exact command are in `gay-magic-coverage-summary.jsonl`.

This reaches all prefixes under this ranking mix at the smaller retention,
compared with eight batches started by the 5,000-retention deadline run.
The deeper 5,000-retention lane remains partial; finishing the smaller search
does not establish complete deeper coverage or global optimality.

### Current plan: smaller prefix batches

1. In progress: tool 92995 runs `run-gay-magic-coverage.py --batch-size 25
   --suffix-limit 2500 --seconds 540 --optimistic-share 100
   --continuation-lane-share 0 --skeleton-share 0`, after the preceding native
   process ended. Its native process was verified live. Each suffix batch now
   combines fewer prefixes, changing how its fixed retention quota is shared.
   Broad retention remains 20,000, split day 8 and encounters Magic/Midas.
   Caps remain 512 MB external / 384 MB internal, search time 540 seconds,
   and external time caps 602/604 seconds.
2. Pending: record score, route, peak RSS, processed/generated states,
   batch count, frontier size and completion/timeout status. Compare coverage
   against the completed 50-prefix, 5,000-suffix optimistic run.
3. Pending: independently replay any score above 50,105, promote it and run
   bounded native checks. If it ties, choose another profile from the measured
   coverage; no retained-search completion certifies global optimality.

The campaign script accepts `--batch-size`, preserves its default of 50,
checks positivity, and records it in artifact names and exact solver commands.
Python syntax compilation and CLI help passed. No native source changed and
no additional record has been found; the goal remains active and unachieved.

### Gayawaku 25-prefix batch result

Tool 92995 exited zero. Gayawaku tied 50,105 and finished all twenty-seven
batches from the 661-prefix frontier. Native time was 318.852 seconds;
wrapper time 318.873 seconds; externally measured peak RSS 87,504 KiB.
It processed 392,476 states, generated 1,376,976 transitions and recorded
276,210 beam prunes. `search_complete` is true and the stop reason is null.
The full 18-action path equals the frozen saved route; all required
measurements and the exact command are in `gay-magic-coverage-summary.jsonl`.

This added 26,048 processed states compared with the completed 50-prefix,
5,000-suffix optimistic run (366,428), while using 2,500 suffix retention
per smaller batch. It provides measured coverage under a different quota
distribution; it found no new record and does not certify global optimality.

### Current plan: ten-prefix suffix batches

1. In progress: tool 40176 runs `run-gay-magic-coverage.py --batch-size 10
   --suffix-limit 2500 --seconds 840 --optimistic-share 100
   --continuation-lane-share 0 --skeleton-share 0`, after the preceding solver
   exited. Its native process was verified live. Smaller batches share the
   same 2,500-state suffix quota across fewer prefix families, allowing more
   retained states per prefix. Broad retention, split day, encounter pair,
   batch ordering and ranking are unchanged. The 840-second search budget
   supports this increased state retention; both external time caps are
   900 seconds. Memory remains 512 MB external / 384 MB internal.
2. Pending: record score, route, peak RSS, processed/generated states, batch
   count, frontier size and completion/timeout status. Compare against the
   completed 25-prefix run before claiming measured coverage growth.
3. Pending: independently replay any score above 50,105, promote it and run
   bounded native checks. If the run ties, select the next search from its
   retained-state and batch coverage; avoid timeout-only reruns of completed
   configurations.

No additional record has been found and no native source changed. The goal
remains active and unachieved.

### Gayawaku ten-prefix optimistic result

Tool 40176 exited zero. Gayawaku tied 50,105 after finishing all sixty-seven
batches of the 661-prefix day-8 frontier. Native time was 718.051 seconds;
wrapper time 718.072 seconds; externally measured peak RSS 96,280 KiB.
It processed 791,181 states, generated 2,696,377 transitions and recorded
574,669 beam prunes. `search_complete` is true and the stop reason is null.
The full 18-action path equals the frozen saved route; required measurements
and the exact command are in `gay-magic-coverage-summary.jsonl`.

Smaller prefix batches increased processed states from 392,476 at batch size
25 to 791,181 at batch size 10 (+398,705), with unchanged suffix retention
2,500 and optimistic ranking. The longer budget supported measured added
coverage. All batches completed, but suffix pruning remains and this is not
a global optimality certificate. No additional record was found.

### Current plan: balanced ranking with ten-prefix batches

1. In progress: tool 8265 runs `run-gay-magic-coverage.py --batch-size 10
   --suffix-limit 2500 --seconds 540 --optimistic-share 50
   --continuation-lane-share 20 --skeleton-share 30`, after the preceding
   solver exited. Its native process was verified live. This keeps the
   smaller batches and suffix quota while changing retained continuations
   through the balanced ranking mix. Broad retention, split day, encounter
   pair and batch ordering are unchanged. Caps remain 512 MB external /
   384 MB internal and 540 seconds search time / 602/604 seconds external.
2. Pending: record score, route, peak RSS, processed/generated states, batch
   count, frontier size and completion/timeout status. Compare with the
   completed optimistic run before choosing any follow-up.
3. Pending: independently replay any score above 50,105, promote it and run
   bounded native checks. If the run ties, choose further coverage from its
   measured batch and retention results; do not extend completed profiles
   with a longer timeout alone.

No additional record has been found and no native source changed. The goal
remains active and unachieved.

### Gayawaku ten-prefix balanced deadline result

Tool 8265 exited zero at its internal deadline, tying 50,105. Native time was
540.005 seconds; wrapper time 540.028 seconds; externally measured peak RSS
97,372 KiB. It processed 439,371 states, generated 1,484,921 transitions and
recorded 341,541 beam prunes. Thirty-seven of sixty-seven batches from the
661-prefix frontier were started. `search_complete` is false and the stop
reason is `internal time budget reached after 540 seconds`. Its full
18-action path equals the frozen saved route. Required measurements and the
exact command are in `gay-magic-coverage-summary.jsonl`.

The balanced mix leaves later batches unvisited at suffix retention 2,500.
The next run reduces retention to 1,500 under the same ranking and grouping
to reach more of those batches within the default search budget. Increased
batch coverage remains to be measured; no new record was found.

### Current plan: later balanced prefix batches

1. In progress: tool 43760 runs `run-gay-magic-coverage.py --batch-size 10
   --suffix-limit 1500 --seconds 540 --optimistic-share 50
   --continuation-lane-share 20 --skeleton-share 30`, after the preceding
   solver exited. Its native process was verified live. Broad retention,
   day-8 split, encounter pair and batch ordering are unchanged. Caps remain
   512 MB external / 384 MB internal and 540 seconds search time /
   602/604 seconds external.
2. Pending: record score, route, peak RSS, processed/generated states, batch
   count, frontier size and completion/timeout status. Compare with the
   thirty-seven-batch deadline result before claiming broader prefix coverage.
3. Pending: independently replay any score above 50,105, promote it and run
   bounded native checks. If it ties, select further coverage based on its
   retained-state and batch measurements.

No additional record has been found and no native source changed. The goal
remains active and unachieved.

### Gayawaku later balanced batches completed

Tool 43760 exited zero. Gayawaku tied 50,105 after completing all sixty-seven
batches of the 661-prefix day-8 frontier. Native time was 518.914 seconds;
wrapper time 518.934 seconds; externally measured peak RSS 74,268 KiB.
It processed 552,536 states, generated 1,903,591 transitions and recorded
469,167 beam prunes. `search_complete` is true and the stop reason is null.
Its full 18-action path equals the frozen saved route. Required measurements,
full route and exact command are in `gay-magic-coverage-summary.jsonl`.

Reducing suffix retention from 2,500 to 1,500 reached the thirty later batches
left unvisited by the preceding balanced deadline run, while processing
113,165 more states overall. This trades retention depth for prefix coverage;
it does not complete the deeper 2,500-state configuration or certify global
optimality. No additional record was found.

### Current plan: continuation-heavy ten-prefix batches

1. In progress: tool 67936 runs `run-gay-magic-coverage.py --batch-size 10
   --suffix-limit 1500 --seconds 840 --optimistic-share 0
   --continuation-lane-share 80 --skeleton-share 20`, after tool 43760 exited.
   Its native process (PID 3515004) was verified live. This changes the ranking
   mix and shares retention across ten prefixes
   instead of the fifty used by the completed continuation-heavy 2,500-state
   run. Broad retention, day-8 split, encounter pair and batch ordering remain
   unchanged. This increases the nominal suffix quota per prefix from fifty
   to 150 states. The longer budget supports this added retention and different
   retained continuations. Caps remain 512 MB external / 384 MB internal;
   search time is 840 seconds and both external caps are 900 seconds.
2. Pending: record score, route, peak RSS, processed/generated states, batch
   count, frontier size and completion/timeout status. Compare with the
   completed fourteen-batch continuation-heavy run before claiming measured
   state coverage growth.
3. Pending: independently replay any score above 50,105, promote it and run
   bounded native checks. If it ties, select further coverage from the
   measured retention and batch results.

No additional record has been found and no native source changed. The goal
remains active and unachieved.

### Prepared follow-up: persistent continuation-heavy ranking

The existing `run-persistent-optimistic-timed.py` supervisor now accepts town,
optimistic share, continuation-lane share, beam width and time arguments.
Default arguments preserve the historical optimistic lane and its artifacts.
Other profiles use `persistent-ranking-summary.jsonl`, include all changed
parameters in their artifact names, skip only matching completed town/lane
rows and refuse to overwrite an existing log. Python compilation and `--help`
passed. No native code changed and no second native process was launched.

1. Pending after tool 67936's result is inspected: run the existing mapped
   graphs sequentially for Eduming, Skjollird, Port-Willam and Cornecopea with
   `--optimistic-share 0 --continuation-lane-share 80 --beam-width 6000
   --seconds 540`. Their previous mapped runs used optimistic 100 / lanes 0.
   The mapped selector consumes both shares; full state keys and graph
   scenarios exclude ranking knobs. Existing open descendants remain eligible
   and solved states stay closed. This changes actual scheduled work rather
   than extending a completed transient configuration's timeout.
2. Pending: capture each town's score, full route, peak RSS, processed/atomic
   counts, unique-node and solved-node deltas, root status and stop reason.
   Every solver stays sequential at 384 MB external / 288 MB internal, with
   540-second search time and 602/604-second external caps. The existing lock
   and five-GiB free-space floor remain enforced.
3. Pending: stop the supervisor at any higher global score; independently
   replay and promote the route, then run bounded native checks. Map lower
   bound improvements alone do not satisfy the additional-record goal.

The Gayawaku lane remains the sole active native search. No additional record
has been found; the goal remains active and unachieved.

### Gayawaku continuation-heavy ten-prefix result

Tool 67936 exited zero. Gayawaku tied 50,105 after completing all sixty-seven
batches of the 661-prefix day-8 frontier. Native time was 664.840 seconds;
wrapper time 664.898 seconds; externally measured peak RSS 76,768 KiB.
It processed 552,536 states, generated 1,904,696 transitions and recorded
488,060 beam prunes. `search_complete` is true and the stop reason is null.
Its full 18-action path equals the frozen saved route. Required measurements,
full route and exact command are in `gay-magic-coverage-summary.jsonl`.

This processed 319,255 more states than the completed continuation-heavy
fifty-prefix / 2,500-state suffix run (233,281 states). Smaller batches shared
the suffix quota across fewer prefixes. Compared with the balanced ten-prefix
run, processed counts match but generated transitions, trims and lane-owner
counts differ; matching counts do not show that ranking settings were ignored.
All prefix batches completed, but suffix pruning remains and no global
optimality certificate or new score was obtained.

### Current plan: mapped continuation-heavy four-town follow-up

1. In progress: tool 99528 runs the prepared mapped supervisor for Eduming, Skjollird,
   Port-Willam and Cornecopea, in that order, after tool 67936 exited.
   Arguments are `--optimistic-share 0 --continuation-lane-share 80
   --beam-width 6000 --seconds 540`; the lane is
   `persistent-beam-o0-c80-n300k-timed540-b6000`. Eduming's native process
   (PID 3519452) was verified live. Each existing graph is resumed
   without resetting states. Current saved Computer scores were rechecked
   against frozen thresholds: 81,321; 47,065; 42,296; and 43,881 respectively.
2. Pending: record each lane's externally measured peak RSS, processed and
   atomic expansion counts, unique and solved deltas, completion/timeout
   status, score and full path in `persistent-ranking-summary.jsonl`.
   Solver processes remain sequential at 384 MB external / 288 MB internal,
   540-second internal budget and 602/604-second external caps.
3. Pending: independently replay any score above the saved global threshold,
   promote the route and run bounded native checks. If all four tie, choose
   further coverage based on their measured new-state and closure results.

No additional record has been found and no native source changed. The goal
remains active and unachieved.

### Eduming live mapped coverage checkpoint

Tool 99528 remains live; native PID 3519452 was rechecked at 3:13 elapsed,
with 151,264 KiB current RSS. A read-only SQLite transaction earlier in this
turn observed 1,629,320 unique nodes and 1,350,180 solved nodes: additions of
165,449 and 119,440 respectively relative to the completed optimistic lane.
The root remains partial and its best score remains 81,321. These are live
coverage measurements, not final lane metrics or a new global record.
Final RSS, state counts, status and route remain pending in the supervisor.

### Eduming mapped continuation-heavy result

Tool 99528 completed Eduming's lane with exit zero and tied 81,321. Native
time was 295.862 seconds; wrapper time 295.981 seconds; externally measured
peak RSS 151,248 KiB. It processed 107,948 states, completed 300,000 atomic
expansions and reused 25,796 child lookups. The session reached its node cap
without a time stop (`stop_reason: null`), but `search_complete` is false and
the root remains partial. The full 18-action route equals the frozen record.

Unique nodes increased from 1,463,871 to 1,801,608 (+337,737); solved nodes
from 1,230,740 to 1,521,136 (+290,396). The map's best lower bound remained
81,321, so graph growth does not count as an additional global record.
The exact command, environment, final metrics, map snapshots and full route
are in `persistent-ranking-summary.jsonl`.

The supervisor advanced sequentially to Skjollird; native PID 3520455 was
verified live. Port-Willam and Cornecopea remain queued. Their final metrics
and any higher score still require inspection and independent replay before
promotion. No native source changed; the goal remains active and unachieved.

### Skjollird mapped continuation-heavy result

Tool 99528 completed Skjollird's lane with exit zero and tied 47,065. Native
time was 297.354 seconds; wrapper time 297.492 seconds; externally measured
peak RSS 178,248 KiB. It processed 105,298 states, completed 300,000 atomic
expansions and reused 29,875 child lookups. The session reached its node cap
without a time stop (`stop_reason: null`), but `search_complete` is false and
the root remains partial. The full 18-action route equals the frozen record.

Unique nodes increased from 1,525,210 to 1,932,461 (+407,251); solved nodes
from 1,217,340 to 1,478,849 (+261,509). The best lower bound remained 47,065.
The exact command, environment, final metrics, snapshots and full route are
in `persistent-ranking-summary.jsonl`. Together the first two follow-up lanes
added 744,988 unique nodes and 551,905 solved nodes, with no new global score.

The supervisor advanced sequentially to Port-Willam; native PID 3521151 was
verified live at 0:58 elapsed and 164,544 KiB current RSS. Cornecopea remains
queued. Final measurements and independent verification of any higher score
remain pending. No native source changed; the goal remains active and
unachieved.

### Port-Willam mapped continuation-heavy result

Tool 99528 completed Port-Willam's lane with exit zero and tied 42,296.
Native time was 289.787 seconds; wrapper time 289.913 seconds; externally
measured peak RSS 174,200 KiB. It processed 104,953 states, completed 300,000
atomic expansions and reused 23,536 child lookups. The session reached its
node cap without a time stop (`stop_reason: null`), but `search_complete` is
false and the root remains partial. Its full 18-action route equals the
frozen record.

Unique nodes increased from 1,521,847 to 1,922,818 (+400,971); solved nodes
from 1,211,403 to 1,474,414 (+263,011). The best lower bound remained 42,296.
The exact command, environment, final metrics, snapshots and full route are
in `persistent-ranking-summary.jsonl`. The three finished follow-up lanes
have added 1,145,959 unique nodes and 814,916 solved nodes, with no new score.

The supervisor advanced sequentially to Cornecopea; native PID 3522045 was
verified live at 0:54 elapsed and 165,112 KiB current RSS. Its final result is
pending. No native source changed; the goal remains active and unachieved.

### Prepared follow-up: Midas alternatives to Oasis records

The existing `run-gay-magic-coverage.py` supervisor now accepts town, Merchant
(`Magic Cleppsydra` or `Blue Treasure`), broad retention and split day.
Its earlier defaults and Gayawaku artifact names are preserved. Other profiles
write `pair-coverage-summary.jsonl`; their town, Merchant, broad/suffix quotas,
split, batch size, ranking shares and time are encoded in artifact names.
The existing lock, no-overwrite check, exact-command capture, final route,
external RSS measurement and 512 MB external / 384 MB internal caps remain.
Python compilation and `--help` passed. No native source changed.

1. Pending after tool 99528's Cornecopea result is inspected: run Bajaar
   Magic/Midas, Bajaar Blue/Midas, Petrinov Magic/Midas and Petrinov Blue/Midas,
   sequentially. Their saved routes use Oasis, and the previous fixed-pair
   completions in this goal have not exhausted these four Midas pairs.
2. Pending: use `--broad-limit 32000 --split-day 12 --batch-size 1000
   --suffix-limit 32000 --seconds 540` and the existing balanced mix
   `--optimistic-share 50 --continuation-lane-share 20 --skeleton-share 30`.
   These profiles allocate retention separately to the alternative Witch
   family. Record score, full path, peak RSS, processed/generated states,
   trims and completion/timeout status for every lane. A completed retained
   search with trims does not certify global optimality.
3. Pending: independently replay any score above Bajaar 56,147 or Petrinov
   45,578, promote it and run bounded native checks. If all lanes tie or fall
   short, choose further coverage from their measured trims and completion.

Cornecopea remains the sole active native search. No additional record has
been found; the goal remains active and unachieved.

### Cornecopea result and four-town mapped follow-up completion

Tool 99528 exited zero after Cornecopea tied 43,881. Native time was
250.655 seconds; wrapper time 250.781 seconds; externally measured peak
RSS 177,292 KiB. It processed 108,201 states, completed 300,000 atomic
expansions and reused 25,977 child lookups. The session reached its node cap
without a time stop (`stop_reason: null`), but `search_complete` is false and
the root remains partial. Its full 18-action route differs from the frozen
fixture and is retained verbatim in `persistent-ranking-summary.jsonl`; the
tie does not warrant record promotion.

Unique nodes increased from 1,519,519 to 1,915,851 (+396,332); solved nodes
from 1,213,354 to 1,479,757 (+266,403). The best lower bound remained 43,881.
All four finished lanes have full commands, environments, external peaks,
final metrics, map snapshots and routes in the raw summary. Their totals are
426,400 processed states, 1,200,000 atomic expansions, 1,542,291 added unique
nodes, 1,081,319 added solved nodes and 1,133.658 native seconds. Peak RSS was
178,248 KiB. No saved global score improved and all four roots remain partial.

### Current plan: Midas pair coverage in Bajaar and Petrinov

1. In progress: tool 59566 runs the prepared runner for Bajaar Magic Cleppsydra/Midas,
   after tool 99528 exited. Use broad/suffix retention 32,000, split day 12,
   batch size 1,000, structure-interleaved order, balanced 50/20/30 ranking,
   540-second internal time and 602/604-second external caps. Memory remains
   512 MB external / 384 MB internal. The deep campaign report was reread
   before launch, including its Gayawaku profile. Saved Computer thresholds
   were rechecked: Bajaar 56,147 and Petrinov 45,578.
2. Pending: inspect score, full path, peak RSS, processed/generated counts,
   trims and completion/timeout status. Independently replay and promote any
   higher score, then run bounded native checks.
3. Pending if tied or below record: run Bajaar Blue/Midas, then Petrinov
   Magic/Midas and Blue/Midas sequentially. Use measured completion and trims
   to decide whether retention changes are necessary; do not extend a
   completed configuration's timeout alone.

No additional record has been found and no native source changed. The goal
remains active and unachieved.

### Bajaar Magic/Midas pair result

Tool 59566 exited zero. Bajaar reached 45,835, below its saved 56,147, after
115,645 processed states and 265,492 generated transitions. Native time was
25.682 seconds; wrapper time 25.702 seconds; externally measured peak RSS
89,960 KiB. All five batches of the 4,101-prefix frontier completed, with
zero beam trims, `search_complete: true` and `stop_reason: null`. Its full
18-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. The next planned lane is Bajaar Blue/Midas; Petrinov's two
Midas pairs remain pending. No new global record was found.

### Bajaar Blue/Midas pair result

Tool 59054 exited zero. Bajaar reached 46,189, below its saved 56,147, after
78,100 processed states and 177,113 generated transitions. Native time was
10.952 seconds; wrapper time 10.970 seconds; externally measured peak RSS
72,828 KiB. All five batches of the 4,258-prefix frontier completed, with
zero beam trims, `search_complete: true` and `stop_reason: null`. Its full
17-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. Petrinov Magic/Midas is next, followed by Blue/Midas.
No new global record was found; the goal remains active and unachieved.

### Petrinov Midas pair results and residual plan

Both Petrinov lanes exited zero and fell below the saved 45,578:

| Pair | Score | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / prefixes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Magic/Midas (tool 20112) | 41,100 | 17,719 | 43,056 | 1.895 / 1.913 | 52,828 | 2 / 1,407 |
| Blue/Midas (tool 77404) | 42,490 | 14,398 | 34,477 | 1.425 / 1.440 | 49,668 | 2 / 1,436 |

Both completed every batch with zero beam trims, `search_complete: true` and
`stop_reason: null`. Their full 18- and 17-action routes and exact commands
are in `pair-coverage-summary.jsonl`. No width or timeout-only extension is
justified for these completed untrimmed pairs. All four Bajaar/Petrinov Midas
lanes total 225,862 processed states, 520,138 generated transitions and
39.954 native seconds; externally measured peak RSS was 89,960 KiB.
None improved a saved global score.

The next residual branch is explicit Time-is-Money coverage: its extra day
changes route length and its Jewelry sale penalty changes cargo optimization.
The paired runner now accepts this Witch choice, preserving Midas defaults and
prior artifacts. Python compilation and `--help` passed; native code is
unchanged.

1. In progress: tool 11880 runs Bajaar Blue/Time after tool 77404 exited, using the
   unchanged broad/suffix 32,000, day-12 split, 1,000-prefix batches and balanced
   ranking. Caps remain 512 MB external / 384 MB internal and 540-second search
   time / 602/604-second external limits.
2. Pending: record score, full route, peak RSS, processed/generated counts,
   trims and completion/timeout status; independently replay any higher score
   before promotion and bounded native checks.
3. Pending if below record: run Petrinov Blue/Time, then assess Magic/Time
   coverage using measured pruning and completion. Do not increase time alone
   on any completed profile.

No additional record has been found; the goal remains active and unachieved.

### Bajaar Blue/Time pair result

Tool 11880 exited zero. Bajaar reached 41,070, below its saved 56,147, after
122,604 processed states and 280,516 generated transitions. Native time was
23.045 seconds; wrapper time 23.064 seconds; externally measured peak RSS
86,980 KiB. All five batches of the 4,289-prefix frontier completed, with
zero beam trims, `search_complete: true` and `stop_reason: null`. Its full
18-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. Petrinov Blue/Time is next under the unchanged caps and
retention settings. No new global record was found; the goal remains active.

### Remaining Time pair results and Oasis coverage plan

Petrinov Blue/Time and Bajaar Magic/Time exited zero and stayed below their
saved thresholds. Their metrics are:

| Pair | Score | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / prefixes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Petrinov Blue/Time | 44,165 | 19,587 | 47,214 | 1.011 / 1.027 | 45,044 | 2 / 1,364 |
| Bajaar Magic/Time (tool 27367) | 33,871 | 182,978 | 429,181 | 62.899 / 62.921 | 106,768 | 5 / 4,194 |
| Petrinov Magic/Time (tool 44747) | 21,674 | 24,465 | 59,397 | 1.315 / 1.331 | 48,124 | 2 / 1,348 |

All three completed all batches with zero beam trims, `search_complete: true`
and `stop_reason: null`. Full 18-, 19- and 19-action routes and exact commands
are in `pair-coverage-summary.jsonl`. No Time lane improved a global
record; widening these completed untrimmed profiles adds no coverage.

The runner now accepts Oasis of Sanctifan as a Witch choice; prior Midas and
Time defaults/artifacts are preserved. Python compilation and `--help` passed.
No native code changed.

1. In progress: tool 56048 runs Petrinov Blue/Oasis after tool 44747 exited. This is
   the saved record's encounter pair, but it has not yet received a complete
   fixed-pair lane in this goal. Use unchanged broad/suffix 32,000, day-12
   split, 1,000-prefix batches, balanced ranking and 512/384 MB caps, with
   540-second search time and 602/604-second external limits.
2. Pending: record score, full path, peak RSS, processed/generated states,
   trims and completion/timeout status; independently replay any score above
   45,578, then promote it and run bounded native checks.
3. Pending if tied: inspect any pruned or unfinished branch before choosing
   deeper retention; otherwise consider Petrinov Magic/Oasis as a separate
   encounter family. Do not extend completed configurations by timeout alone.

No additional record has been found; the goal remains active and unachieved.

### Petrinov Oasis pair completion and next retained-family plan

Both Petrinov Oasis lanes exited zero:

| Pair | Score | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / prefixes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Blue/Oasis (tool 56048) | 45,578 | 48,887 | 143,091 | 2.800 / 2.818 | 64,588 | 4 / 3,794 |
| Magic/Oasis (tool 9652) | 29,328 | 64,880 | 202,210 | 3.660 / 3.676 | 65,320 | 4 / 3,684 |

Both completed every batch with zero beam trims, `search_complete: true` and
`stop_reason: null`. Full 17- and 18-action routes, commands and environments
are in `pair-coverage-summary.jsonl`. Blue/Oasis tied the saved record;
Magic/Oasis fell short. These completed untrimmed profiles do not justify
width or timeout-only extensions. No new global record was found.

The next residual branch allocates the frontier separately to the record
families in towns whose mapped continuation-heavy runs remained partial.
Their default maps contain many encounter families; retaining the specified
pair separately can avoid competition before the suffix split.

1. In progress: tool 63344 runs Eduming Magic/Midas after tool 9652 exited, with the
   existing broad/suffix 32,000, day-12 split, 1,000-prefix batches, balanced
   ranking and 512 MB external / 384 MB internal caps. Search time remains
   540 seconds with 602/604-second external ceilings.
2. Pending: inspect score, full path, peak RSS, processed/generated counts,
   trims and completion/timeout status. Independently replay any score above
   Eduming 81,321 before promotion and bounded native checks.
3. Pending if tied: Eduming Blue/Midas, then the untested fixed Magic/Midas
   profiles for Skjollird, Port-Willam and Cornecopea. Select further retention
   only from actual trims or unfinished batches, and keep solver runs
   sequential.

Native source is unchanged. No additional record has been found; the goal
remains active and unachieved.

### Eduming Magic/Midas fixed-pair result

Tool 63344 exited zero. Eduming tied 81,321 after 89,952 processed states and
193,272 generated transitions. Native time was 14.454 seconds; wrapper time
14.474 seconds; externally measured peak RSS 79,800 KiB. All four batches of
the 3,254-prefix frontier completed, with zero beam trims,
`search_complete: true` and `stop_reason: null`. Its full 18-action route and
exact command are in `pair-coverage-summary.jsonl`. This completed untrimmed
pair does not justify a width or timeout-only extension.

Next is Eduming Blue/Midas under the same bounded settings, followed by the
untested fixed Magic/Midas profiles for Skjollird, Port-Willam and Cornecopea.
Any score above a saved global threshold still requires independent replay,
promotion and bounded native checks. No new global record was found; the goal
remains active and unachieved.

### Eduming Blue/Midas fixed-pair result

Tool 66558 exited zero. Eduming reached 79,649, below its saved 81,321, after
64,364 processed states and 136,069 generated transitions. Native time was
7.828 seconds; wrapper time 7.847 seconds; externally measured peak RSS
77,500 KiB. All four batches of the 3,295-prefix frontier completed, with
zero beam trims, `search_complete: true` and `stop_reason: null`. Its full
17-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. Tool 45059 runs Skjollird Magic/Midas under the unchanged bounded
settings; Port-Willam and Cornecopea follow. No new global record was found;
the goal remains active and unachieved.

### Skjollird Magic/Midas fixed-pair result

Tool 45059 exited zero. Skjollird tied 47,065 after 128,773 processed states
and 283,628 generated transitions. Native time was 30.569 seconds; wrapper
time 30.589 seconds; externally measured peak RSS 95,012 KiB. All five
batches of the 4,241-prefix frontier completed, with zero beam trims,
`search_complete: true` and `stop_reason: null`. Its full 18-action route and
exact command are in `pair-coverage-summary.jsonl`. No retained-width or
timeout-only extension is justified for this completed untrimmed pair.

Port-Willam Magic/Midas is next, followed by Cornecopea. No native code
changed and no new global record was found; the goal remains active and
unachieved.

### Port-Willam and Cornecopea fixed-pair completion

Both Magic/Midas lanes exited zero and tied their saved global scores:

| Town / tool | Score | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / prefixes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Port-Willam / 73462 | 42,296 | 133,732 | 295,526 | 28.988 / 29.007 | 91,824 | 5 / 4,485 |
| Cornecopea / 85341 | 43,881 | 171,429 | 381,381 | 31.892 / 31.911 | 97,996 | 6 / 5,855 |

Both completed every batch with zero beam trims, `search_complete: true` and
`stop_reason: null`. Their full 18-action routes and exact commands are in
`pair-coverage-summary.jsonl`. Neither warrants width or timeout-only
extensions. The five Eduming/Skjollird/Port-Willam/Cornecopea fixed-pair lanes
total 588,250 processed states, 1,289,876 generated transitions and 113.731
native seconds; externally measured peak RSS was 97,996 KiB. No new global
score was found.

### Current plan: Blue/Midas alternatives in three towns

1. In progress: tool 5470 runs Cornecopea Blue/Midas after tool 85341 exited, keeping
   broad/suffix 32,000, day-12 split, 1,000-prefix batches, balanced ranking,
   512 MB external / 384 MB internal caps and 540-second search time with
   602/604-second external ceilings. The historical Blue Merchant sweep
   remained below the record, but this goal has not completed this retained
   pair separately from its sister encounter families.
2. Pending: record score, full route, peak RSS, processed/generated counts,
   trims and completion/timeout status. Independently replay any score above
   43,881 before promotion and bounded native checks.
3. Pending if below record: run Port-Willam Blue/Midas, then Skjollird
   Blue/Midas under the same sequential bounded workflow. Choose further
   retention only from observed pruning or unfinished batches.

Native source is unchanged. No additional record has been found; the goal
remains active and unachieved.

### Cornecopea Blue/Midas fixed-pair result

Tool 5470 exited zero. Cornecopea reached 41,270, below its saved 43,881,
after 121,514 processed states and 266,849 generated transitions. Native time
was 17.196 seconds; wrapper time 17.216 seconds; externally measured peak
RSS 90,384 KiB. All six batches of the 5,967-prefix frontier completed, with
zero beam trims, `search_complete: true` and `stop_reason: null`. Its full
17-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. Port-Willam Blue/Midas is next, followed by Skjollird.
No new global record was found; the goal remains active and unachieved.

### Port-Willam Blue/Midas fixed-pair result

Tool 41656 exited zero. Port-Willam reached 39,009, below its saved 42,296,
after 92,175 processed states and 200,238 generated transitions. Native time
was 13.520 seconds; wrapper time 13.539 seconds; externally measured peak
RSS 82,084 KiB. All five batches of the 4,607-prefix frontier completed, with
zero beam trims, `search_complete: true` and `stop_reason: null`. Its full
17-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. Skjollird Blue/Midas is next under unchanged bounded settings.
No new global record was found; the goal remains active and unachieved.

### Skjollird Blue/Midas result and next encounter-family plan

Tool 36617 exited zero. Skjollird reached 43,416, below its saved 47,065,
after 87,412 processed states and 187,753 generated transitions. Native time
was 14.205 seconds; wrapper time 14.224 seconds; externally measured peak
RSS 81,088 KiB. All five batches of the 4,340-prefix frontier completed, with
zero beam trims, `search_complete: true` and `stop_reason: null`. Its full
17-action route and exact command are in `pair-coverage-summary.jsonl`.
The three-town Blue/Midas branch totaled 301,101 processed states, 654,840
generated transitions and 44.921 native seconds, with 90,384 KiB peak RSS.
Every lane fell below its saved global record and completed without trimming.
Width or timeout-only extensions do not add coverage to these profiles.

The next residual branch isolates Time is Money in L'Exquise. This goal's
previous fixed Magic/Midas and Blue/Midas lanes completed without trimming
at 56,141 and 56,817 respectively, but the extra-day Time family has not
received an equivalent fixed-pair run.

1. In progress: tool 37571 runs L'Exquise Blue/Time after tool 36617 exited, keeping
   broad/suffix 32,000, day-12 split, 1,000-prefix batches, balanced ranking,
   512 MB external / 384 MB internal caps and 540-second search time with
   602/604-second external ceilings.
2. Pending: record score, full route, peak RSS, processed/generated counts,
   trims and completion/timeout status. Independently replay any score above
   56,817 before promotion and bounded native checks.
3. Pending if below record: L'Exquise Magic/Time under the same sequential
   bounded workflow, then choose another unvisited encounter family from the
   measured completion and trimming results.

Native source is unchanged. No additional record has been found; the goal
remains active and unachieved.

### L'Exquise Blue/Time fixed-pair result

Tool 37571 exited zero. L'Exquise reached 38,096, below its saved 56,817,
after 324,756 processed states and 762,043 generated transitions. Native time
was 159.183 seconds; wrapper time 159.205 seconds; externally measured peak
RSS 125,968 KiB. All nine batches of the 8,364-prefix frontier completed,
with zero beam trims, `search_complete: true` and `stop_reason: null`. Its
full 18-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. L'Exquise Magic/Time is next under unchanged bounded settings.
No new global record was found; the goal remains active and unachieved.

### Prepared residual branch: Bajaar Blue/Patience

The earlier Patience screens covered Tocaccialli, Sanctifan, Moonin, Huangoz,
L'Exquise, Eduming, Doba Lao and Skjollird. Bajaar Blue/Patience remains
untested in this goal's fixed-pair coverage. Its stable Interest sequence is
a separate future context from Oasis and Time; the previous untrimmed pair
results do not exhaust it.

The paired runner now accepts the native spelling `Vertue of Patience`,
with a distinct `patience` artifact label. Existing choices, defaults,
artifact names, lock and bounded launch commands remain unchanged. Python
compilation and `--help` passed. No native source changed or second solver
was launched.

1. In progress: tool 31294 runs L'Exquise Magic/Time; inspect its final score,
   full route, peak RSS, state counts, trims and completion/timeout status.
2. Pending if below record: run Bajaar Blue/Patience with broad/suffix 32,000,
   day-12 split, 1,000-prefix batches, balanced ranking, 512/384 MB caps and
   540-second internal / 602/604-second external limits, after tool 31294
   exits. Record every required lane metric and full route.
3. Pending: independently replay any score above the saved threshold,
   promote it and run bounded native checks. If it falls short, choose further
   retained-family coverage from measured pruning and completion.

No additional record has been found; the goal remains active and unachieved.

### L'Exquise Magic/Time result and Bajaar Patience launch

Tool 31294 exited zero. L'Exquise reached 29,514, below its saved 56,817,
after 470,655 processed states and 1,145,238 generated transitions. Native
time was 350.279 seconds; wrapper time 350.302 seconds; externally measured
peak RSS 151,428 KiB. All eight batches of the 7,992-prefix frontier completed,
with zero beam trims, `search_complete: true` and `stop_reason: null`. Its
full 19-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. Neither L'Exquise Time lane improved a saved global score.

1. In progress: launch the prepared Bajaar Blue/Patience pair after tool 31294
   exited. Use broad/suffix 32,000, day-12 split, 1,000-prefix batches,
   structure-interleaved ordering and balanced ranking, with 512 MB external /
   384 MB internal memory caps and 540-second search time / 602/604-second
   external ceilings. The deep campaign report, including its Gayawaku profile,
   was reread before launch.
2. Pending: record score, full route, peak RSS, processed/generated counts,
   trims and completion/timeout status. Independently replay any score above
   Bajaar 56,147 before promotion and bounded native checks.
3. Pending if below record: inspect any pruned or unfinished branch before
   choosing deeper retention, or select another unvisited retained family
   if the configuration finishes untrimmed.

No native source changed and no additional record has been found. The goal
remains active and unachieved.

### Bajaar Blue/Patience result and free-food follow-up

Tool 90781 exited zero. Bajaar reached 49,653, below its saved 56,147, after
120,935 processed states and 257,910 generated transitions. Native time was
14.224 seconds; wrapper time 14.244 seconds; externally measured peak RSS
84,736 KiB. All seven batches of the 6,765-prefix frontier completed, with
zero beam trims, `search_complete: true` and `stop_reason: null`. Its full
17-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. No new global record was found.

The next residual branch is Cornecopea Blue/Patience. It was not included in
the earlier eight-town Patience screen. Its free-food market changes the
cash needed to hold food through a stable Interest sequence; the completed
Bajaar pair does not exhaust this separate scenario.

1. In progress: launch Cornecopea Blue/Patience after tool 90781 exited,
   keeping broad/suffix 32,000, day-12 split, 1,000-prefix batches, balanced
   ranking, 512 MB external / 384 MB internal caps and 540-second search time
   with 602/604-second external ceilings.
2. Pending: record score, full path, peak RSS, processed/generated counts,
   trims and completion/timeout status. Independently replay any score above
   Cornecopea 43,881 before promotion and bounded native checks.
3. Pending if below record: inspect any pruned or unfinished branch before
   increasing retention, or choose another unvisited retained family if this
   configuration finishes untrimmed.

Native source is unchanged. No additional record has been found; the goal
remains active and unachieved.

### Cornecopea Blue/Patience result and retained-state follow-up

Tool 86720 exited zero. Cornecopea reached 25,787, below its saved 43,881,
after 198,241 processed states and 406,774 generated transitions. Native time
was 34.655 seconds; wrapper time 34.675 seconds; externally measured peak
RSS 87,812 KiB. All ten batches of the 9,555-prefix frontier completed, with
zero beam trims, `search_complete: true` and `stop_reason: null`. Its full
17-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. No new global record was found.

The next residual branch is wider L'Exquise Blue/Patience coverage. Its earlier
8,000-state screen reached 48,641 but recorded 110,805 beam prunes; unlike the
completed untrimmed Bajaar and Cornecopea pairs, retained width can add work
here. Its saved global threshold remains 56,817.

1. In progress: tool 13760 runs L'Exquise Blue/Patience after tool 86720 exited, using
   broad/suffix 32,000, day-12 split, 1,000-prefix batches, balanced ranking,
   512 MB external / 384 MB internal caps and 540-second search time with
   602/604-second external ceilings. This increases actual retention from the
   earlier 8,000-state configuration rather than merely extending its timeout.
2. Pending: record score, full path, peak RSS, processed/generated counts,
   trims and completion/timeout status. Independently replay any score above
   56,817 before promotion and bounded native checks.
3. Pending if below record: use the resulting trims and unfinished batches
   to decide whether another grouping/retention profile adds coverage, or
   move to a different retained family if this run finishes untrimmed.

Native source is unchanged. No additional record has been found; the goal
remains active and unachieved.

### L'Exquise wider Blue/Patience result and weightless-food follow-up

Tool 13760 exited zero. L'Exquise reached 48,641, below its saved 56,817,
after 302,559 processed states and 649,683 generated transitions. Native time
was 97.491 seconds; wrapper time 97.512 seconds; externally measured peak RSS
98,736 KiB. All thirteen batches of the 12,712-prefix frontier completed,
with zero beam trims, `search_complete: true` and `stop_reason: null`. Its
full 17-action route and exact command are in `pair-coverage-summary.jsonl`.
This wider configuration completed without the earlier screen's 110,805
beam prunes, but its best score remained unchanged. Different split/grouping
can change duplicate work, so lower processed counts alone do not measure
unique-state coverage. Width or timeout-only extensions of this completed
untrimmed pair are not justified. No new global record was found.

The next residual branch is Gayawaku Blue/Patience, absent from the earlier
eight-town Patience screen. Its weightless-food market is distinct from
L'Exquise's food-free Traders and Cornecopea's free food. The deep campaign
report and Gayawaku-specific memory profile were reread before tuning.

1. In progress: tool 61296 runs Gayawaku Blue/Patience after tool 13760 exited, using
   broad 20,000, day-8 split, fifty-prefix batches, suffix retention 5,000,
   structure-interleaved ordering and balanced ranking. Caps remain 512 MB
   external / 384 MB internal and 540-second search time with 602/604-second
   external ceilings. Smaller batches avoid the previously observed pressure
   from thousands of weightless-food prefixes in one suffix batch.
2. Pending: record score, full route, peak RSS, processed/generated counts,
   trims, batch coverage and completion/timeout status. Independently replay
   any score above 50,105 before promotion and bounded native checks.
3. Pending if below record: choose another grouping or retention setting
   only from measured trimming/unfinished batches, or move to a different
   unvisited family if this run completes untrimmed.

Native source is unchanged. No additional record has been found; the goal
remains active and unachieved.

### Gayawaku Blue/Patience result and closer-lane prioritization

Tool 61296 exited zero. Gayawaku reached 30,397, below its saved 50,105,
after 333,275 processed states and 1,141,266 generated transitions. Native
time was 180.064 seconds; wrapper time 180.089 seconds; externally measured
peak RSS 164,716 KiB. All fourteen batches of the 686-prefix frontier
completed, with `search_complete: true` and `stop_reason: null`, but 204,777
beam prunes remain. Its full 17-action route and exact command are in
`pair-coverage-summary.jsonl`. This is completed retained coverage, not an
untrimmed family or an optimality certificate. Deeper Patience retention
remains open and is deferred behind a closer measured candidate family.

A read-only audit of the nine historical `million-state-merchant-*.jsonl`
files ranked each town/Merchant maximum against the frozen current global
threshold. Superseded Fang-Mo rows were excluded. The closest already-tested
families are Moonin Magic/Midas, Sanctifan Magic/Midas and Blue/Midas,
L'Exquise Magic/Midas, and Gayawaku Blue/Midas. The next closest untested
fixed pair in this goal is Doba Lao Blue/Midas: its mapped lower bound reached
44,451, just 1,092 below the saved 45,543. That historical map improvement is
not a new global record.

1. In progress: tool 23604 runs Doba Lao Blue/Midas after tool 61296 exited, keeping
   broad/suffix 32,000, day-12 split, 1,000-prefix batches, balanced ranking,
   512 MB external / 384 MB internal caps and 540-second search time with
   602/604-second external ceilings. This allocates retention to the close
   encounter family independently of sister branches.
2. Pending: record score, full route, peak RSS, processed/generated counts,
   trims and completion/timeout status. Independently replay any score above
   45,543 before promotion and bounded native checks.
3. Pending if below record: select further coverage from its trims/completion
   and the next untested close historical lanes. Gayawaku Blue/Patience's
   pruned suffix coverage remains explicitly deferred, not declared exhausted.

Native source is unchanged. No additional record has been found; the goal
remains active and unachieved.

### Doba Lao Blue/Midas result and Saddle follow-up

Tool 23604 exited zero. Doba Lao reached 44,451, below its saved 45,543,
after 65,723 processed states and 140,274 generated transitions. Native time
was 8.196 seconds; wrapper time 8.215 seconds; externally measured peak RSS
79,024 KiB. All four batches of the 3,383-prefix frontier completed, with
zero beam trims, `search_complete: true` and `stop_reason: null`. Its full
17-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. The historical lower bound was rediscovered, not improved
past the global record.

The next close untested retained pair is Jilliqo Sturdy Saddle/Midas. Its
historical maximum was 36,150, 1,669 below the saved 37,819. The paired runner
now accepts Sturdy Saddle with a distinct `saddle` artifact label; all prior
choices/defaults/artifacts and bounded commands remain unchanged. Python
compilation and `--help` passed. No native source changed.

1. In progress: tool 1310 runs Jilliqo Saddle/Midas after tool 23604 exited, keeping
   broad/suffix 32,000, day-12 split, 1,000-prefix batches, balanced ranking,
   512 MB external / 384 MB internal caps and 540-second search time with
   602/604-second external ceilings.
2. Pending: record score, full route, peak RSS, processed/generated counts,
   trims and completion/timeout status. Independently replay any score above
   37,819 before promotion and bounded native checks.
3. Pending if below record: select further coverage from measured pruning
   and completion; the next close untested historical pair is Octoyashi
   Saddle/Midas (38,194 versus its saved 40,056).

No additional record has been found; the goal remains active and unachieved.

### Jilliqo Saddle/Midas result and Octoyashi follow-up

Tool 1310 exited zero. Jilliqo reached 36,150, below its saved 37,819,
after 112,544 processed states and 252,318 generated transitions. Native
time was 20.293 seconds; wrapper time 20.312 seconds; externally measured
peak RSS 91,596 KiB. All six batches of the 5,358-prefix frontier completed,
with zero beam trims, `search_complete: true` and `stop_reason: null`. Its
full 17-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. The historical lower bound was rediscovered, not improved
past the global record.

1. In progress: tool 27046 runs Octoyashi Saddle/Midas after tool 1310 exited, keeping
   broad/suffix 32,000, day-12 split, 1,000-prefix batches, balanced ranking,
   512 MB external / 384 MB internal caps and 540-second search time with
   602/604-second external ceilings. Its historical maximum 38,194 is 1,862
   below the saved 40,056 and this fixed pair remains untested in this goal.
2. Pending: record score, full route, peak RSS, processed/generated counts,
   trims and completion/timeout status. Independently replay any score above
   40,056 before promotion and bounded native checks.
3. Pending if below record: use measured pruning and completion to select
   further coverage; Octoyashi Blue/Midas and Mehaz Magic/Midas are the next
   close untested historical families.

Native source is unchanged. No additional record has been found; the goal
remains active and unachieved.

### Octoyashi Saddle/Midas result and Blue follow-up

Tool 27046 exited zero. Octoyashi reached 38,194, below its saved 40,056,
after 114,433 processed states and 247,362 generated transitions. Native
time was 22.331 seconds; wrapper time 22.354 seconds; externally measured
peak RSS 90,660 KiB. All six batches of the 5,310-prefix frontier completed,
with zero beam trims, `search_complete: true` and `stop_reason: null`. Its
full 17-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. The historical lower bound was rediscovered, not improved
past the global record.

1. In progress: tool 51098 runs Octoyashi Blue/Midas after tool 27046 exited, keeping
   broad/suffix 32,000, day-12 split, 1,000-prefix batches, balanced ranking,
   512 MB external / 384 MB internal caps and 540-second search time with
   602/604-second external ceilings. Its historical maximum 37,985 is 2,071
   below the saved 40,056 and this fixed pair remains untested in this goal.
2. Pending: record score, full route, peak RSS, processed/generated counts,
   trims and completion/timeout status. Independently replay any score above
   40,056 before promotion and bounded native checks.
3. Pending if below record: use measured pruning and completion to choose
   further coverage; Mehaz Magic/Midas is the next close untested historical
   pair (46,468 versus its saved 48,738).

Native source is unchanged. No additional record has been found; the goal
remains active and unachieved.

### Octoyashi Blue/Midas result and Mehaz follow-up

Tool 51098 exited zero. Octoyashi reached 37,985, below its saved 40,056,
after 72,929 processed states and 154,880 generated transitions. Native
time was 9.422 seconds; wrapper time 9.441 seconds; externally measured
peak RSS 78,668 KiB. All four batches of the 3,688-prefix frontier completed,
with zero beam trims, `search_complete: true` and `stop_reason: null`. Its
full 17-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. The historical lower bound was rediscovered, not improved
past the global record.

1. In progress: tool 96934 runs Mehaz Magic/Midas after tool 51098 exited, keeping
   broad/suffix 32,000, day-12 split, 1,000-prefix batches, balanced ranking,
   512 MB external / 384 MB internal caps and 540-second search time with
   602/604-second external ceilings. Its historical maximum 46,468 is 2,270
   below the saved 48,738 and this fixed pair remains untested in this goal.
2. Pending: record score, full route, peak RSS, processed/generated counts,
   trims and completion/timeout status. Independently replay any score above
   48,738 before promotion and bounded native checks.
3. Pending if below record: choose another untested close retained pair from
   the historical audit, or increase retention only where observed pruning
   or unfinished batches support added coverage.

Native source is unchanged. No additional record has been found; the goal
remains active and unachieved.

### Mehaz Magic/Midas result and Statuette follow-up

Tool 96934 exited zero. Mehaz reached 46,468, below its saved 48,738,
after 267,546 processed states and 630,875 generated transitions. Native
time was 98.427 seconds; wrapper time 98.449 seconds; externally measured
peak RSS 114,376 KiB. All eight batches of the 7,685-prefix frontier completed,
with zero beam trims, `search_complete: true` and `stop_reason: null`. Its
full 18-action route and exact command are in `pair-coverage-summary.jsonl`.
No retained-width or timeout-only extension is justified for this completed
untrimmed pair. The historical lower bound was rediscovered, not improved
past the global record.

The next close untested retained pair is Mahaji Wooden Statuette/Midas:
35,211 historically, 2,378 below the saved 37,589. The paired runner now
accepts Wooden Statuette with a distinct `statuette` artifact label; all prior
choices/defaults/artifacts and bounded commands remain unchanged. Python
compilation and `--help` passed. No native source changed.

1. In progress: tool 84904 runs Mahaji Statuette/Midas after tool 96934 exited, keeping
   broad/suffix 32,000, day-12 split, 1,000-prefix batches, balanced ranking,
   512 MB external / 384 MB internal caps and 540-second search time with
   602/604-second external ceilings.
2. Pending: record score, full route, peak RSS, processed/generated counts,
   trims and completion/timeout status. Independently replay any score above
   37,589 before promotion and bounded native checks.
3. Pending if below record: select further coverage from measured pruning
   and completion; Kifuai Statuette/Midas (35,144 versus 37,547) is the next
   close untested historical pair.

No additional record has been found; the goal remains active and unachieved.

### Mahaji Statuette/Midas result and Kifuai follow-up

Tool 84904 finished with exit zero; its session has closed. Mahaji reached
35,211, below its saved 37,589, after 53,914 processed states and 122,175
generated transitions. Native time was 5.125 seconds, wrapper time 5.161
seconds, and externally measured peak RSS 60,228 KiB. All four batches of
its 3,203-prefix frontier completed with zero beam trims,
`search_complete: true` and `stop_reason: null`. The exact command,
environment and full 17-action route are recorded in
`results/more-records-20261009/pair-coverage-summary.jsonl` under both town
and lane. Observation truncation did not require a rerun.

1. In progress: run Kifuai Wooden Statuette/Midas with broad/suffix 32,000,
   day-12 split, 1,000-prefix batches, balanced ranking, 512 MB external /
   384 MB internal ceilings and 540-second search time. Its historical
   score 35,144 is below the saved 37,547.
2. Pending: record complete metrics and full route; independently replay
   any score above the current global record before promotion.
3. Pending: check Jilliqo, Normalia, Huangoz, Vilparino and Moonin Statuette/
   Midas sequentially if Kifuai finishes below its record. Choose subsequent
   coverage from observed pruning rather than extending completed lanes.

No additional global record has been found. Native code is unchanged;
the goal remains active.

### Kifuai result and remaining close Statuette families

Tool 22555 exited zero. Kifuai reached 35,144 versus saved 37,547, processing
56,640 states and generating 128,285 transitions in 4.761 native seconds /
4.779 wrapper seconds. External peak RSS was 63,604 KiB. All four batches
of the 3,347-prefix frontier finished with zero beam trims,
`search_complete: true`, and no stop reason. Its full 17-action route,
command and environment are preserved in the pair summary.

1. In progress: sequentially check Jilliqo, Normalia, Huangoz, Vilparino and
   Moonin Wooden Statuette/Midas using the same 32,000-state broad/suffix,
   day-12 split and 1,000-prefix batch profile. Each retains the 512/384 MB
   external/internal limits and 540-second search budget. Stop the sequence
   immediately if a candidate beats its saved global record.
2. Pending: record each town's metrics and full path in the pair summary
   and reconcile results here. Replay any winner before promotion.
3. Pending: select the next untested family from historical score gaps,
   or expand a pruned Gayawaku lane where retention adds coverage.

No additional global record has been found. Native code remains unchanged.

### Five-town Statuette results and Gayawaku Blue coverage

Tool 69577 completed all five towns sequentially with exit zero. Every lane
finished all batches, with zero beam trims, `search_complete: true` and
`stop_reason: null`. All full 17-action routes, commands and environments
are in `pair-coverage-summary.jsonl`; no score exceeds its saved record.

| Town | Score / saved | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Jilliqo | 35,410 / 37,819 | 64,512 | 148,715 | 5.931 / 5.948 | 67,568 | 4 / 3,828 |
| Normalia | 35,114 / 37,527 | 51,651 | 116,677 | 4.083 / 4.101 | 62,440 | 4 / 3,110 |
| Huangoz | 35,114 / 37,527 | 51,590 | 116,652 | 4.131 / 4.148 | 61,800 | 4 / 3,120 |
| Vilparino | 35,129 / 37,569 | 52,356 | 118,183 | 4.222 / 4.240 | 62,820 | 4 / 3,137 |
| Moonin | 35,133 / 37,584 | 271,548 | 582,980 | 74.506 / 74.529 | 92,708 | 10 / 9,592 |

1. In progress: Gayawaku Blue Treasure/Midas with broad 20,000, split day 8,
   batches of 10, suffix 2,500 and optimistic ranking 100%, continuation
   and skeleton shares zero. This differs from prior completed 50-prefix
   balanced lanes; narrower groups and independent ranking change retained
   coverage. Budget 840 seconds, caps 512 MB external / 384 MB internal.
   The 14-minute allowance buys changed coverage, not a timeout-only rerun.
2. Pending: record metrics, completion/pruning and full route; strictly
   replay and promote any score above saved 50,105 with bounded checks.
3. Pending: use measured completion to choose balanced or continuation-heavy
   independent ranking; untrimmed Statuette lanes require no width increase.

No additional global record has been found. Native code remains unchanged.

### Fixed-pair coverage audit during Gayawaku Blue search

`remaining-fixed-pair-audit.json` derives the actual Witch choice from each
historical full action path (historical rows often label the configurable
Witch field `any`). It compares town/Merchant/Witch against successful,
completed, zero-beam-trim rows across this campaign's summary artifacts.
Fang-Mo's superseded historical model is excluded. This is an artifact
coverage audit, not a global optimality certificate.

1. In progress: tool 71313, Gayawaku Blue/Midas, 10-prefix batches and pure
   optimistic ranking, suffix 2,500, 840-second budget. A live `/proc`
   observation showed 93,020 KiB VmHWM; this is a lower bound on final peak,
   not a substitute for the final external `/usr/bin/time` measurement.
2. Pending after exit: reconcile final score, count, full route and status.
3. Pending: test leading Magic/Midas families lacking completed untrimmed
   fixed-pair artifacts: Mahaji, Doba Lao, Octoyashi, Kifuai, Vilparino and
   Ipartus, sequentially with the 32,000/day-12/1,000 profile. Their historic
   leading scores tie the current records; beating those scores is still
   required for a new record.
4. Pending: Gayawaku Magic/Midas retains substantial pruning despite prior
   ranking checks; choose further retention only from measured results.
5. Pending: Kifuai/Vilparino Saddle and Ipartus Statuette are the next close
   lower-score historical families after leading-pair coverage.

No new global record has been found; the goal remains active.

### Gayawaku Blue optimistic completion and leading-pair follow-up

Tool 71313 exited zero. Gayawaku Blue/Midas reached 49,344,
below the saved 50,105, after 664,445 processed states and
2,105,177 generated transitions. Native / wrapper time was
410.218 / 410.240 seconds, external
peak RSS 93,020 KiB. All 69 batches of the
686-prefix frontier completed with
`search_complete: true`, `stop_reason: null`, and 414,205 beam trims.
Its full 17-action path and exact bounded command/environment
are in `pair-coverage-summary.jsonl`.

This processed 212,305 more states than the previous
completed 50-prefix/suffix-10,000 balanced lane. Different batch grouping
and ranking prevent interpreting this difference as a unique-state count.
The score is still 49,344, not an improvement beyond the global record.

1. In progress: sequential Magic/Midas checks for Mahaji, Doba Lao, Octoyashi,
   Kifuai, Vilparino and Ipartus, broad/suffix 32,000, split day 12, batch
   1,000, balanced ranking, 512/384 MB limits and 540-second time budgets.
   Stop immediately on a candidate above its saved global record.
2. Pending: record each result and full route; independently replay any
   winner before promotion and bounded native verification.
3. Pending: measured Gayawaku pruning remains open; another ranking mix
   can change coverage. Completed untrimmed fixed pairs need no extension.

Native code is unchanged. No additional global record has been found.

### Six leading Magic/Midas completions and next coverage plan

Tool 61804 completed all six towns with exit zero. Each independently
rediscovered its saved score, finished all batches, and had zero beam trims,
`search_complete: true` and `stop_reason: null`. Exact full paths and bounded
commands/environments remain in `pair-coverage-summary.jsonl`.

| Town | Score | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Mahaji | 37,589 | 94,524 | 204,424 | 14.481 / 14.501 | 76,268 | 4 / 3,435 | 18 |
| Doba Lao | 45,543 | 93,359 | 203,225 | 15.962 / 15.982 | 81,008 | 4 / 3,333 | 18 |
| Octoyashi | 40,056 | 102,484 | 221,232 | 18.697 / 18.716 | 81,728 | 4 / 3,619 | 18 |
| Kifuai | 37,547 | 92,626 | 201,401 | 14.444 / 14.465 | 79,216 | 4 / 3,377 | 18 |
| Vilparino | 37,569 | 77,485 | 170,651 | 12.483 / 12.504 | 80,764 | 3 / 2,930 | 18 |
| Ipartus | 37,734 | 89,774 | 192,463 | 14.344 / 14.363 | 82,208 | 4 / 3,238 | 18 |

The legacy purchase-profile avenue was checked in source:
`GameSolver.cpp` uses `DeferredPurchaseModel::open` for ordinary Classical
purchases; `generate_buy_successors` consumes the legacy profile in the
Traveler/disaster branch. Profile-only reruns are therefore not supported
as an added-coverage experiment for these ordinary Classical lanes.
No native code changed or build/test process was launched.

1. Pending next execution: Kifuai and Vilparino Saddle/Midas, then Ipartus
   Statuette/Midas, sequentially using the completed 32,000/day-12/1,000
   profile. Their historical gaps are 2,501, 2,508 and 2,620 respectively.
2. Pending: Normalia, Huangoz, Moonin, Mahaji and Ipartus Saddle/Midas,
   ordered by historical gap. Stop on any score above the current record.
3. Pending: Gayawaku Blue/Midas remains pruned. A balanced 10-prefix mix or
   continuation-heavy mix changes coverage relative to the new optimistic
   result. Select retention/budget using measured completion, within caps.
4. Pending on any actual winner: strict route replay plus direct Game replay,
   consistent record promotion, bounded safe build and focused/full tests.

At this phase boundary the metadata is terminal/completed for Ipartus and
no search remains running. No additional global record has been found;
the goal is active and unachieved.

### Saddle and Ipartus Statuette execution

The previous goal turn made progress: fourteen bounded searches completed,
new coverage and full metrics were recorded, and the fixed-pair audit narrowed
next actions. Current metadata is terminal/completed for Ipartus Magic/Midas.
The deep campaign guidance was reread before launching this phase.

1. In progress: Kifuai Saddle, Vilparino Saddle, Ipartus Statuette, then
   Normalia, Huangoz, Moonin, Mahaji and Ipartus Saddle, all with Midas,
   sequentially. Broad/suffix 32,000, day-12 split, batches 1,000,
   balanced ranking, 512/384 MB limits, 540-second native budget and
   bounded outer wrappers. Stop on a failed run or a global-score candidate.
2. Pending: record complete metrics, full routes and completion/pruning.
3. Pending: Gayawaku Blue/Midas balanced small-group ranking, chosen from
   the completed optimistic run's remaining 414,205 beam trims.
4. Pending on a winner: strict/direct Game replay, promotion and safe checks.

No additional global record yet; the goal remains active.

### Eight Saddle/Statuette completions and balanced Blue coverage

Tool 31977 completed all eight cases with exit zero. Every batch finished,
with zero beam trims, `search_complete: true` and `stop_reason: null`.
Every score is below its saved global record. Full paths, commands and
environments remain in `pair-coverage-summary.jsonl`.

| Town / Merchant | Score / global | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Kifuai / Sturdy Saddle | 35,046 / 37,547 | 100,940 | 221,466 | 17.181 / 17.199 | 83,068 | 5 / 4,824 | 17 |
| Vilparino / Sturdy Saddle | 35,061 / 37,569 | 90,136 | 199,005 | 14.005 / 14.023 | 83,936 | 5 / 4,379 | 17 |
| Ipartus / Wooden Statuette | 35,114 / 37,734 | 51,457 | 116,346 | 4.384 / 4.401 | 65,276 | 4 / 3,103 | 17 |
| Normalia / Sturdy Saddle | 34,892 / 37,527 | 96,619 | 209,673 | 15.554 / 15.573 | 86,588 | 5 / 4,548 | 17 |
| Huangoz / Sturdy Saddle | 34,892 / 37,527 | 97,512 | 212,215 | 17.477 / 17.498 | 84,316 | 5 / 4,555 | 17 |
| Moonin / Sturdy Saddle | 34,892 / 37,584 | 356,772 | 750,637 | 128.953 / 128.973 | 105,172 | 13 / 12,017 | 17 |
| Mahaji / Sturdy Saddle | 34,923 / 37,589 | 101,184 | 221,951 | 16.809 / 16.829 | 81,660 | 5 / 4,897 | 17 |
| Ipartus / Sturdy Saddle | 34,892 / 37,734 | 96,593 | 209,731 | 17.014 / 17.031 | 85,868 | 5 / 4,536 | 17 |

Huangoz Saddle improved its historical family score from 34,845 to 34,892
(+47), and Mahaji Saddle improved from 34,889 to 34,923 (+34). Neither
beats the corresponding town global record; neither is promoted or counted
as goal success. These completed untrimmed pairs need no wider rerun.

1. In progress: Gayawaku Blue/Midas, broad 20,000, split day 8, batches 10,
   suffix 2,500, balanced shares o50/c20/k30. This changes ranking relative
   to the completed pure-optimistic run and grouping relative to the old
   balanced 50-prefix runs. The 840-second budget is for this new retained
   coverage; limits remain 512 MB external / 384 MB internal.
2. Pending: score, full route, final RSS, processed/generated counts,
   completion/timeout status and beam trims; replay any global candidate.
3. Pending: choose continuation-heavy Blue coverage or the next untested
   family from the updated audit after observing this lane.

No native code changed and no additional global record has been found.

### Balanced Gayawaku Blue completion and next fixed pairs

Tool 24465 exited zero. The balanced ten-prefix Blue/Midas lane reached
49,344, below 50,105, after 664,445 processed
states and 2,100,489 generated transitions. Native / wrapper
time was 484.250 / 484.268 seconds;
external peak RSS 88,572 KiB. All 69 batches of
its 686-prefix frontier finished with
`search_complete: true`, `stop_reason: null`, and 423,191 beam
trims. Its full 17-action path, exact command and environment
are in the pair summary. The equal processed count across ranking mixes
is not evidence that ranking was ignored: generated transitions and beam
trims differ. Neither mix beats the saved global record.

All 22 current Computer maxima in `Best.txt` were independently compared
with frozen campaign thresholds during this run and matched exactly.

1. In progress: Jilliqo, Moonin, Vilparino and Kifuai Blue/Midas, followed
   by Cornecopea Saddle/Midas, sequential 32,000/day-12/1,000/balanced lanes
   with 512/384 MB ceilings and 540-second search budgets. Stop on any
   failure or score above the saved global record.
2. Pending: record metrics and full routes, update the coverage audit.
3. Pending: choose Gayawaku continuation-heavy retention from remaining
   beam trims, or the next historical untested pair by gap.
4. Pending on a winner: strict/direct replay, promotion and bounded checks.

No additional global record has been found. Native code remains unchanged.

### Five Blue/Saddle completions and residual coverage plan

Tool 1045 completed all five cases with exit zero. Every batch finished,
with zero beam trims, `search_complete: true` and `stop_reason: null`.
Each score is below its saved global record. Exact full action paths,
commands and environments are preserved in `pair-coverage-summary.jsonl`.

| Town / Merchant | Score / global | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Jilliqo / Blue Treasure | 34,692 / 37,819 | 68,955 | 151,490 | 7.994 / 8.012 | 77,304 | 4 / 3,768 | 17 |
| Moonin / Blue Treasure | 34,444 / 37,584 | 295,564 | 624,307 | 79.047 / 79.066 | 94,476 | 11 / 10,352 | 17 |
| Vilparino / Blue Treasure | 34,363 / 37,569 | 54,036 | 117,216 | 5.269 / 5.286 | 71,192 | 4 / 3,004 | 17 |
| Kifuai / Blue Treasure | 34,329 / 37,547 | 65,752 | 140,276 | 7.491 / 7.510 | 75,732 | 4 / 3,447 | 17 |
| Cornecopea / Sturdy Saddle | 40,414 / 43,881 | 189,698 | 422,081 | 46.765 / 46.786 | 101,512 | 9 / 8,206 | 17 |

The residual artifact sweep removed these completed untrimmed cases from
`remaining-fixed-pair-audit.json`. The new next-coverage plan is:

1. Pending next execution: Ipartus Blue/Midas, then Huangoz and Mahaji
   Blue/Midas, using sequential 32,000/day-12/1,000/balanced bounded runs.
2. Pending: Tocaccialli Blue/Midas, Normalia Blue/Midas and Octoyashi
   Statuette/Midas, ordered by historical gap after the three above.
3. Pending: Gayawaku Blue/Midas continuation-heavy ranking with small
   groups remains untested; both optimistic and balanced completed lanes
   still trim states. Select its retention from the observed results.
4. Pending on any candidate above the current saved Computer score: strict
   replay and direct Game replay, consistent promotion, safe build and
   focused/full tests. No winner exists to exercise this gate yet.

This goal turn completed fourteen additional bounded searches. Thirteen
fixed-pair runs finished without beam trims; Gayawaku balanced Blue/Midas
finished with trims and a score below its global record. No native source
changed. The terminal metadata now belongs to Cornecopea and no solver
remains running. No additional global record has been found, so the goal
remains active and unachieved.

### Next six fixed-pair executions

The previous goal turn made progress by completing fourteen bounded searches,
recording full paths and metrics, and removing completed untrimmed cases from
the remaining-coverage audit. Current metadata is terminal/completed for
Cornecopea Saddle/Midas. The deep campaign guidance was reread.

1. In progress: Ipartus, Huangoz, Mahaji, Tocaccialli and Normalia Blue/Midas,
   then Octoyashi Statuette/Midas, sequentially. Broad/suffix 32,000, split
   day 12, batches 1,000, balanced ranking; 512/384 MB external/internal caps
   and 540-second search budgets. Stop on failure or a global candidate.
2. Pending: record final RSS, processed/generated counts, score, full path,
   completion/timeout status and pruning, then refresh the audit.
3. Pending: Gayawaku Blue/Midas continuation-heavy small-group coverage.
4. Pending on a winner: strict/direct replay, promotion and bounded checks.

No additional global record has been found. Native code is unchanged.

### Six Blue/Statuette completions and continuation-heavy Gayawaku

Tool 90575 completed all six cases with exit zero. Each finished every
batch, with zero beam trims, `search_complete: true` and
`stop_reason: null`. Scores remain below current global records. Full paths,
exact commands and environments are in `pair-coverage-summary.jsonl`.

| Town / Merchant | Score / global | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Ipartus / Blue Treasure | 34,091 / 37,734 | 64,126 | 135,363 | 7.591 / 7.609 | 79,308 | 4 / 3,281 | 17 |
| Huangoz / Blue Treasure | 33,829 / 37,527 | 64,724 | 137,229 | 7.611 / 7.629 | 75,984 | 4 / 3,310 | 17 |
| Mahaji / Blue Treasure | 33,848 / 37,589 | 66,680 | 141,769 | 7.631 / 7.649 | 73,184 | 4 / 3,492 | 17 |
| Tocaccialli / Blue Treasure | 45,543 / 49,634 | 107,923 | 233,078 | 31.596 / 31.615 | 96,732 | 4 / 3,274 | 18 |
| Normalia / Blue Treasure | 33,421 / 37,527 | 64,264 | 135,558 | 7.393 / 7.412 | 75,752 | 4 / 3,292 | 17 |
| Octoyashi / Wooden Statuette | 35,857 / 40,056 | 60,534 | 137,142 | 5.327 / 5.345 | 65,684 | 4 / 3,525 | 17 |

1. In progress: Gayawaku Blue/Midas, broad 20,000, split day 8, batches
   10, suffix 2,500, continuation share 80%, skeleton share 20%, optimistic
   share zero. This independently ranks retained candidates relative to
   both completed ten-prefix mixes; 840-second budget buys new coverage,
   not a timeout-only repeat. Limits remain 512 MB external / 384 MB internal.
2. Pending: record score, processed/generated counts, full route, external
   RSS, trimming and completion/timeout status; replay any score above 50,105.
3. Pending: choose another historical untested pair from the refreshed
   audit or alter retention where observed pruning justifies it.
4. Pending on a global winner: strict/direct replay, promotion, safe checks.

No native code changed. No additional global record has been found.

The paired Python runner now accepts Route to Mahaji (`route`), Canvas Bag
(`bag`) and Leaders Necklace (`necklace`) with distinct artifact labels.
Exact native names were checked against `consts/enums.h`. Python compilation
and `--help` passed; native code, wrapper limits, prior names and defaults
were not changed. These choices enable the next audited families:
Sanctifan Saddle/Midas; Jilliqo Route/Oasis; Port-Willam Saddle/Midas;
Octoyashi Bag/Midas and Necklace/Midas; Doba Lao Saddle/Midas.
They remain pending until the current Gayawaku process exits.

### Continuation-heavy Gayawaku Blue result and next six pairs

Tool 77910 exited zero. Gayawaku Blue/Midas reached 49,344,
below its saved 50,105, after 664,445 processed states and
2,101,977 generated transitions. Native / wrapper time was
548.030 / 548.051 seconds; external
peak RSS 94,552 KiB. All 69 batches of its
686-prefix frontier finished with
`search_complete: true`, `stop_reason: null` and 432,506 beam trims.
Continuation lane ownership was 528,512;
skeleton reservations 9,216. Its full
17-action route and exact command/environment are in the pair
summary. All three ten-prefix ranking mixes completed and returned 49,344;
they do not certify the global optimum because pruning remains.

1. In progress: Sanctifan Saddle/Midas, Jilliqo Route/Oasis, Port-Willam
   Saddle/Midas, Octoyashi Bag/Midas and Necklace/Midas, Doba Lao Saddle/Midas,
   sequentially. Broad/suffix 32,000, split day 12, batches 1,000, balanced
   ranking, 512/384 MB caps, 540-second search budgets. Stop on a failure or
   a score above the current global record.
2. Pending: record metrics and full paths, update the remaining-pair audit.
3. Pending: select added Gayawaku retention using measured complete runs,
   or the next untested historical family by gap.
4. Pending on a winner: strict/direct replay, promotion and safe checks.

No native code changed. No additional global record has been found.

### Six additional Merchant-family completions and residual plan

Tool 53328 completed all six cases with exit zero. Each finished every
batch with zero beam trims, `search_complete: true` and
`stop_reason: null`. Full action paths, commands and environments are
preserved in `pair-coverage-summary.jsonl`. None exceeds its saved global
record.

| Town / Merchant / Witch | Score / global | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Sanctifan / Sturdy Saddle / Midas was a Trader | 51,708 / 55,728 | 163,031 | 352,258 | 45.623 / 45.643 | 100,824 | 7 / 6,795 | 18 |
| Jilliqo / Route to Mahaji / Oasis of Sanctifan | 33,055 / 37,819 | 198,335 | 549,355 | 27.730 / 27.749 | 86,468 | 12 / 11,814 | 17 |
| Port-Willam / Sturdy Saddle / Midas was a Trader | 37,430 / 42,296 | 125,131 | 276,813 | 26.736 / 26.756 | 98,140 | 6 / 5,824 | 17 |
| Octoyashi / Canvas Bag / Midas was a Trader | 34,925 / 40,056 | 95,019 | 216,677 | 11.360 / 11.381 | 82,732 | 6 / 5,104 | 17 |
| Octoyashi / Leaders Necklace / Midas was a Trader | 34,872 / 40,056 | 50,001 | 109,118 | 5.355 / 5.373 | 68,108 | 3 / 2,888 | 17 |
| Doba Lao / Sturdy Saddle / Midas was a Trader | 40,214 / 45,543 | 101,139 | 221,850 | 19.399 / 19.418 | 90,808 | 5 / 4,760 | 17 |

Sanctifan Saddle improved its historical family score from 51,074 to
51,708 (+634). Octoyashi Bag improved from 34,895 to 34,925 (+30). These
are family improvements only, below town globals 55,728 and 40,056; no
record promotion or goal success is claimed. The three added Python
Merchant choices were exercised successfully through the bounded solver.
Python compilation and help checks passed; no native source was changed.

The residual sweep refreshed the remaining-pair audit. Next actions:

1. Pending next execution: Jilliqo Necklace/Midas and Bag/Midas, sequential
   32,000/day-12/1,000/balanced runs within the existing memory/time limits.
2. Pending: Normalia Cornucopia/Midas (add its existing native choice to
   the Python runner), Mahaji Bag/Midas, Bajaar Saddle/Oasis, then Sanctifan
   Statuette/Midas; verify the Witch choices against historical full paths
   before launching each case.
3. Pending: Gayawaku has completed all three ten-prefix Blue ranking mixes
   but remains pruned. Wider suffix or smaller independent prefix groups
   must add actual coverage within the per-lane budget.
4. Pending on any global candidate: strict and direct Game replay, record
   promotion, safe build and bounded focused/full tests. No global winner
   is available for this gate.

This turn completed thirteen additional bounded searches. Native metadata
is terminal/completed for Doba Lao Saddle/Midas, and no solver remains
running. No additional global record has been found; the goal remains
active and unachieved.

### Next six audited Merchant families

The previous turn made progress: thirteen bounded searches completed, full
metrics and paths were recorded, and the remaining-pair audit was refreshed.
Current metadata is terminal/completed for Doba Lao. Deep campaign guidance
was reread. The Python runner now accepts the existing native Cornucopia
choice as `corn`; its name was checked against `consts/enums.h`. Compilation
and help checks passed. Native code and wrapper ceilings are unchanged.

1. In progress: Jilliqo Necklace/Midas and Bag/Midas, Normalia Corn/Midas,
   Mahaji Bag/Midas, Bajaar Saddle/Oasis, Sanctifan Statuette/Midas,
   sequentially with broad/suffix 32,000, split day 12, batches 1,000,
   balanced ranking, 512/384 MB caps and 540-second search budgets. These
   Witch choices match the refreshed audit of historical full paths.
2. Pending: record score, full path, RSS, processed/generated counts,
   completion/timeout status and beam trimming; refresh the audit.
3. Pending: change Gayawaku retained coverage after all three ten-prefix
   Blue ranking mixes completed below the global record with beam trims.
4. Pending on a global candidate: strict/direct replay, promotion, safe checks.

No additional global record has been found; the goal remains active.

### Six audited completions and smaller-group Magic retention

Tool 2773 completed all six cases with exit zero. Each finished every
batch, with zero beam trims, `search_complete: true` and
`stop_reason: null`. None exceeds its current global record. Exact full
action paths, commands and environments are in `pair-coverage-summary.jsonl`.

| Town / Merchant / Witch | Score / global | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Jilliqo / Leaders Necklace / Midas was a Trader | 34,190 / 37,819 | 52,808 | 118,137 | 5.725 / 5.742 | 66,500 | 4 / 3,092 | 17 |
| Jilliqo / Canvas Bag / Midas was a Trader | 32,358 / 37,819 | 101,962 | 237,920 | 11.792 / 11.811 | 78,256 | 6 / 5,543 | 17 |
| Normalia / Cornucopia / Midas was a Trader | 31,764 / 37,527 | 100,709 | 248,666 | 10.779 / 10.798 | 76,508 | 6 / 5,373 | 17 |
| Mahaji / Canvas Bag / Midas was a Trader | 31,749 / 37,589 | 84,774 | 194,010 | 9.074 / 9.093 | 72,256 | 5 / 4,681 | 17 |
| Bajaar / Sturdy Saddle / Oasis of Sanctifan | 50,303 / 56,147 | 325,592 | 893,114 | 53.274 / 53.297 | 107,268 | 19 / 18,024 | 17 |
| Sanctifan / Wooden Statuette / Midas was a Trader | 49,874 / 55,728 | 83,597 | 189,047 | 8.786 / 8.805 | 70,668 | 5 / 4,337 | 18 |

Jilliqo Necklace improves its historical family maximum from 32,411 to
34,190 (+1,779), and Bag improves 32,351 to 32,358 (+7). Neither reaches
its saved global 37,819; no promotion or goal success is claimed. The added
Cornucopia choice was exercised through the bounded solver successfully.

1. In progress: Gayawaku Magic/Midas, broad 20,000, split day 8, batches
   five, suffix 1,500, optimistic share 100%, other ranking shares zero,
   840-second budget and 512/384 MB caps. This changes retained coverage
   relative to the completed ten-prefix/suffix-2,500 lane, whose 574,669
   beam trims leave the Magic family unresolved. Smaller groups increase
   retention per independent prefix. This is not a timeout-only rerun.
2. Pending: final external RSS, processed/generated counts, full route,
   score, trimming and completion/timeout status. Replay any score above
   50,105 before promotion and bounded native checks.
3. Pending: Ipartus Necklace/Midas and Bag/Midas, Tocaccialli Bag/Midas,
   then the next historical untested families by gap.

All three ten-prefix Blue ranking mixes completed at 49,344, below the
global score. Magic is now prioritized because it already reaches 50,105
and remains pruned; its retained coverage may still contain a better route.
No native source changed and no additional global record has been found.

### Five-prefix Magic completion and next three families

Tool 35161 exited zero. Gayawaku Magic/Midas reached 50,105,
tying its saved global record, after 942,005 processed
states and 3,207,416 generated transitions. Native / wrapper
time was 746.887 / 746.941 seconds;
external peak RSS 81,384 KiB. All 133 batches of
the 661-prefix frontier finished with
`search_complete: true`, `stop_reason: null` and 754,496 beam
trims. The full 18-action route, command and environment are in
`gay-magic-coverage-summary.jsonl`. It processed 150,824
more states than the completed ten-prefix/suffix-2,500 optimistic lane;
this difference is not a unique-state count because grouping changes
duplicate work. Pruning remains, so no global optimality claim follows.

1. In progress: Ipartus Necklace/Midas and Bag/Midas, then Tocaccialli
   Bag/Midas, sequential 32,000/day-12/1,000/balanced lanes with 512/384 MB
   limits and 540-second budgets. Stop on a failed run or global candidate.
2. Pending: record complete metrics and full paths, refresh remaining audit.
3. Pending: balanced five-prefix Magic coverage with suffix chosen to
   complete the frontier within the budget, or the next untested family.
4. Pending on a global winner: strict/direct replay, promotion, safe checks.

No native code changed. No additional global record has been found.

### Three Bag/Necklace completions and next coverage plan

Tool 49318 completed all three cases with exit zero. Every batch finished
with zero beam trims, `search_complete: true` and `stop_reason: null`.
Each score remains below its saved global record. Full action paths,
commands and environments remain in `pair-coverage-summary.jsonl`.

| Town / Merchant | Score / global | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Ipartus / Leaders Necklace | 31,802 / 37,734 | 42,732 | 92,311 | 4.399 / 4.422 | 66,536 | 3 / 2,565 | 17 |
| Ipartus / Canvas Bag | 31,773 / 37,734 | 80,518 | 183,375 | 8.600 / 8.618 | 77,820 | 5 / 4,435 | 17 |
| Tocaccialli / Canvas Bag | 43,612 / 49,634 | 127,692 | 294,678 | 27.207 / 27.226 | 94,700 | 5 / 4,349 | 18 |

The residual sweep refreshed the remaining-pair audit. The next plan is:

1. Pending next execution: Gayawaku Magic/Midas, five-prefix groups, suffix
   1,000, balanced ranking, broad 20,000 and day-8 split, 840-second budget
   within 512/384 MB caps. This adds an independent ranking mix relative
   to the completed optimistic five-prefix/suffix-1,500 lane, while smaller
   suffix retention aims to finish every group within the budget.
2. Pending: Port-Willam Statuette/Midas, Tocaccialli Necklace/Midas, Doba Lao
   Statuette/Midas, then subsequent audited families by gap. Completed
   untrimmed pairs require no width-only or timeout-only repeat.
3. Pending: assess Gayawaku pruning and completion before further retention
   changes. Neither a tie nor more processed states achieves the goal.
4. Pending on a global-score candidate: strict and direct Game replay,
   consistent promotion, safe build and bounded focused/full tests.

This turn completed ten additional bounded searches. Nine fixed-pair runs
finished untrimmed; the changed Gayawaku Magic lane finished every batch
with beam trims and a global tie. Native metadata is terminal/completed
for Tocaccialli; no solver remains running. Python compilation/help checks
and the new Cornucopia lane passed. No native source changed. No additional
global record has been found; the goal remains active and unachieved.

### Balanced five-prefix Magic execution

The previous goal turn made progress: ten bounded searches completed and
full paths/metrics were recorded. Gayawaku's optimistic five-prefix lane
completed 133 batches but still trimmed 754,496 states and tied 50,105.
Current metadata is terminal/completed for Tocaccialli. Deep campaign
instructions were reread before launching this new ranking mix.

1. In progress: Gayawaku Magic/Midas, broad 20,000, split day 8, batches
   five, suffix 1,000, balanced o50/c20/k30, 840-second native budget,
   512/384 MB external/internal caps. This changes ranking and retained
   coverage; it does not extend the timeout of a completed configuration.
2. Pending: final external RSS, processed/generated counts, full route,
   score, pruning, completion/timeout status and all-batch coverage.
3. Pending: Port-Willam Statuette/Midas, Tocaccialli Necklace/Midas,
   Doba Lao Statuette/Midas, then further historical untested pairs by gap.
4. Pending on any score above its current global record: strict/direct
   replay, consistent promotion, safe build and bounded tests.

No native source changed. No additional global record has been found.

### Current-model Fang-Mo coverage gap

A read-only audit of every campaign summary found no forced-Merchant
Fang-Mo row. Its frozen current global record is 37,569. The historical
merchant audit intentionally excluded Fang-Mo because those older rows used
superseded starting weights; that exclusion must not omit new runs using
the current corrected model. The corrected three-lane deep campaign is
available but is not an untrimmed fixed-pair completion in this goal.

The next execution plan is refined while the Gayawaku process remains live:

1. In progress: tool 22344, balanced five-prefix Gayawaku Magic/Midas.
2. Pending immediately after exit: current-model Fang-Mo Magic/Midas with
   broad/suffix 32,000, split day 12, batches 1,000, balanced ranking and
   the existing 512/384 MB ceilings, 540-second native budget.
3. Pending: Fang-Mo Blue/Midas, Saddle/Midas and Statuette/Midas if the
   leading pair finishes below or tied with its current 37,569 global.
4. Pending: the previously planned Port-Willam Statuette, Tocaccialli
   Necklace and Doba Lao Statuette cases follow the current-model gap.
5. Pending on an actual global candidate: strict/direct replay, promotion
   and bounded native checks. Family-only improvements still do not count.

No native code or game rules changed. The goal remains active.

### Balanced five-prefix Magic result and corrected Fang-Mo checks

Tool 22344 exited zero. Gayawaku Magic/Midas reached 50,105,
a global tie, after 704,692 processed states and
2,412,163 generated transitions. Native / wrapper time was
619.540 / 619.557 seconds; external
peak RSS 67,524 KiB. All 133 batches of the
661-prefix frontier finished with
`search_complete: true`, `stop_reason: null`, and 645,569 beam
trims. The full 18-action path and command/environment are in
`gay-magic-coverage-summary.jsonl`. This independently ranked smaller-group
coverage still does not beat 50,105 or certify optimality.

All 22 current Computer maxima were rechecked against campaign thresholds
and matched, including corrected-model Fang-Mo at 37,569.

1. In progress: Fang-Mo Magic, Blue, Saddle and Statuette with Midas,
   sequential 32,000/day-12/1,000/balanced lanes with 512/384 MB limits
   and 540-second budgets. Stop on any failure or global-score candidate.
2. Pending: final metrics, full paths and completion/pruning; do not mix
   these corrected-model results with superseded historical Fang-Mo rows.
3. Pending: Port-Willam Statuette, Tocaccialli Necklace and Doba Lao
   Statuette, followed by further uncovered families.
4. Pending: existing beam-free `--proof-search` is available for unresolved
   pruned coverage; inspect its documented incumbent/map semantics before
   choosing a bounded targeted proof lane. No proof trial was launched here.
5. Pending on a winner: strict/direct replay, promotion and safe checks.

No native source changed. No additional global record has been found.

### Four corrected-model Fang-Mo completions

Tool 51565 completed all four cases with exit zero. Each finished every
batch with zero beam trims, `search_complete: true` and
`stop_reason: null`. Magic/Midas tied the saved 37,569; other scores are
lower. These current-model results are distinct from the superseded
weightless-starting-pottery historical merchant rows. Exact full action
paths, bounded commands and environments are in `pair-coverage-summary.jsonl`.

| Merchant | Score | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Magic Cleppsydra | 37,569 | 136,220 | 283,516 | 29.357 / 29.377 | 98,164 | 5 / 4,416 | 18 |
| Blue Treasure | 33,529 | 93,286 | 189,655 | 15.784 / 15.802 | 79,980 | 5 / 4,478 | 17 |
| Sturdy Saddle | 34,934 | 143,157 | 298,823 | 33.964 / 33.982 | 96,676 | 7 / 6,249 | 17 |
| Wooden Statuette | 35,099 | 74,122 | 161,634 | 8.023 / 8.041 | 65,908 | 5 / 4,165 | 17 |

1. In progress: Port-Willam Statuette/Midas, Tocaccialli Necklace/Midas
   and Doba Lao Statuette/Midas, sequential 32,000/day-12/1,000/balanced
   runs with 512/384 MB caps and 540-second budgets. Stop on any failure or
   global-score candidate.
2. Pending: complete metrics, full paths, audit refresh.
3. Pending: existing proof documentation confirms conservative bounds,
   bounded transposition storage, and no certificate on partial runs.
   Inspect a targeted continuation-heavy run on Gayawaku's existing
   persistent graph to retain new coverage across sessions rather than
   treating another transient tie as proof. No proof trial was launched.
4. Pending on a winner: strict/direct replay, promotion and safe checks.

No native source changed. No additional global record has been found.

### Three fixed-pair completions and persistent-coverage next plan

Tool 88812 completed all three cases with exit zero. Every batch finished
with zero beam trims, `search_complete: true` and `stop_reason: null`.
All scores are below current globals. Full paths, commands and environments
are recorded in `pair-coverage-summary.jsonl`.

| Town / Merchant | Score / global | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Port-Willam / Wooden Statuette | 36,195 / 42,296 | 78,537 | 181,248 | 8.022 / 8.040 | 72,060 | 5 / 4,406 | 17 |
| Tocaccialli / Leaders Necklace | 43,442 / 49,634 | 66,170 | 143,835 | 12.022 / 12.041 | 82,444 | 3 / 2,552 | 18 |
| Doba Lao / Wooden Statuette | 39,050 / 45,543 | 54,465 | 123,165 | 4.606 / 4.624 | 64,132 | 4 / 3,236 | 17 |

The residual sweep checked the current mapped runner and existing
nondefault mapped summaries: Gayawaku has no nondefault mapped-ranking
lane recorded. Its graph can retain still-open descendants across runs,
so this is a useful next independent-ranking experiment after transient
Magic and Blue ties. The next plan is:

1. Pending next execution: existing `run-persistent-optimistic-timed.py`
   with `--town Gayawaku --optimistic-share 0 --continuation-lane-share 80
   --beam-width 6000 --seconds 540`. It reuses the existing graph, caps
   atomic expansion at 300,000 and keeps 384/288 MB external/internal
   ceilings. Verify the graph remains partial before launch; do not reset it.
2. Pending: record full path, score, RSS, counts, completion/timeout,
   graph before/after statistics, unique/solved increments and root status.
   A partial graph provides no optimality certificate.
3. Pending: remaining historical pairs, beginning Moonin Route/Midas and
   Normalia Necklace/Midas, plus current-model Fang-Mo's remaining Merchant
   choices. Historical Fang-Mo data stays excluded from the audit; new
   current-model trials must not be omitted for that reason.
4. Pending on any global candidate: strict/direct replay, promotion, safe
   build and bounded tests. A tie does not satisfy the objective.

This turn completed eight additional bounded searches: one Gayawaku
balanced retention lane, four corrected-model Fang-Mo pairs, and three
other fixed pairs. Seven fixed pairs finished untrimmed; Gayawaku finished
all batches but remains pruned. No score exceeded a current global record.
No native source changed. Metadata is terminal/completed for Doba Lao and
no solver remains running. The goal is active and unachieved.

### Gayawaku persistent continuation-heavy execution

The previous turn made progress: eight bounded searches completed and a
current-model Fang-Mo coverage gap was checked. None exceeded its town
record. Current metadata is terminal/completed for Doba Lao. The deep
campaign instructions were reread before launching this mapped lane.

Before launch, `gayawaku.sqlite3` has 1,613,700 nodes: 236,517 open,
78,245 partial and 1,298,938 solved. The root is partial, stored best score
49,981, database bytes 1,766,318,080. This stored lower bound is below the
saved global 50,105; improving only the graph lower bound does not count
as beating a town record. The existing graph is preserved.

1. In progress: Gayawaku mapped beam, optimistic zero / continuation 80%,
   beam 6,000, atomic expansion budget 300,000, 540-second native time,
   384/288 MB external/internal caps, through the existing bounded runner.
2. Pending: record final RSS, processed/generated counts, score, full route,
   completion/timeout, graph before/after, unique/solved increments and root
   status. Independently replay only a candidate above current global 50,105.
3. Pending: Moonin Route/Midas and Normalia Necklace/Midas, then the next
   uncovered current-model Fang-Mo choices and historical families by gap.
4. Pending on a global winner: strict/direct replay, promotion, safe checks.

No native source changed. The goal remains active and unachieved.

### Persistent Gayawaku result and remaining Fang-Mo families

Tool 46824 exited zero. Gayawaku reached 50,105, tying its town global and
improving the graph's previous lower bound by 124. It processed 79,814
states and generated 372,584 transitions; atomic expansion count 248,554,
reused children 17,873. Native / wrapper time was 547.537 / 547.658 seconds;
external peak RSS 165,900 KiB. It stopped on `persistent search session time
budget reached`, with `search_complete: false` and zero beam pruning.
The atomic cap was not reached. The exact command, environment and full
18-action path are in `persistent-ranking-summary.jsonl`.

The graph grew by 354,711 unique nodes and 217,582 solved nodes. Afterward:
1,968,411 total; 342,674 open; 109,217 partial; 1,516,520 solved; root partial;
2,136,944,640 database bytes. Graph growth is retained coverage, not a new
global record or an optimality certificate.

The Python paired runner now accepts Hand of Midas as `hand`, checked against
`consts/enums.h`. Compilation and help checks passed. It now supports all
nine existing native Merchant choices. Native code and ceilings are unchanged.

1. In progress: Moonin Route/Midas, Normalia Necklace/Midas, then corrected-
   model Fang-Mo Route, Cornucopia, Bag, Necklace and Hand with Midas,
   sequential 32,000/day-12/1,000/balanced lanes, 512/384 MB limits and
   540-second budgets. Stop immediately on failure or a global candidate.
2. Pending: record all full paths, counts, RSS, score, completion and pruning;
   refresh the audit without importing superseded Fang-Mo historical scores.
3. Pending: further durable Gayawaku coverage using changed ranking or a
   new bounded continuation session on the still-partial graph.
4. Pending on a winner: strict/direct replay, promotion and safe checks.

No native source changed. No additional global record has been found.

### Seven fixed-pair completions and next durable coverage plan

Tool 50902 completed all seven cases with exit zero. Every batch finished
with zero beam trims, `search_complete: true` and `stop_reason: null`.
All scores are below the respective global records. Full paths, bounded
commands and environments remain in `pair-coverage-summary.jsonl`.

| Town / Merchant | Score / global | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Moonin / Route to Mahaji | 31,079 / 37,584 | 289,289 | 619,762 | 69.885 / 69.904 | 99,644 | 10 / 9,843 | 17 |
| Normalia / Leaders Necklace | 30,989 / 37,527 | 42,755 | 92,320 | 4.239 / 4.257 | 63,632 | 3 / 2,572 | 17 |
| Fang-Mo / Route to Mahaji | 30,869 / 37,569 | 73,971 | 162,521 | 8.049 / 8.068 | 66,628 | 5 / 4,161 | 17 |
| Fang-Mo / Cornucopia | 31,806 / 37,569 | 152,491 | 358,704 | 27.149 / 27.167 | 81,724 | 8 / 7,445 | 17 |
| Fang-Mo / Canvas Bag | 31,641 / 37,569 | 120,140 | 264,184 | 19.622 / 19.642 | 79,900 | 7 / 6,124 | 17 |
| Fang-Mo / Leaders Necklace | 31,019 / 37,569 | 61,460 | 128,225 | 7.916 / 7.935 | 70,628 | 4 / 3,400 | 17 |
| Fang-Mo / Hand of Midas | 30,824 / 37,569 | 75,478 | 162,323 | 9.403 / 9.424 | 71,680 | 5 / 4,116 | 17 |

Together with the previous four leading pairs, current-model Fang-Mo
has now completed all nine Merchant choices with Midas, without beam
trimming. Magic ties 37,569; all other Merchant/Midas scores are lower.
This does not certify all Witch choices or a global optimum. The added
Hand choice was successfully exercised through the bounded solver.

The residual sweep refreshed the historical audit while retaining the
corrected-model Fang-Mo results separately. The next plan is:

1. Pending next execution: Gayawaku existing mapped graph with a balanced
   o50/c20 mix, beam 6,000, 300,000-atomic cap and 540-second budget,
   384/288 MB limits. Check the graph remains partial and the new artifact
   lane is unrecorded before launch; preserve all existing graph data.
2. Pending: score, full route, final RSS, processed/generated counts,
   completion/timeout and before/after unique/solved counts. Only a score
   above saved 50,105 is a global candidate.
3. Pending: historical untested pairs by gap, next Huangoz, Kifuai, Mahaji
   and Vilparino Necklace/Midas; corrected-model Fang-Mo Witch alternatives remain
   open after the nine Midas completions.
4. Pending on a global winner: strict/direct replay, promotion, safe build
   and bounded tests. Graph lower-bound recovery and ties do not count.

This turn completed eight bounded searches: one time-stopped persistent
Gayawaku run and seven untrimmed fixed pairs. Gayawaku retained 354,711 new
unique nodes and 217,582 newly solved nodes, but only tied the global record.
No native source changed. Python compilation/help and the Hand lane passed.
Metadata is terminal/completed for Fang-Mo Hand/Midas; no solver remains
running. No additional global record has been found; the goal stays active.

### Balanced mapped Gayawaku execution

The previous turn made progress: a persistent lane saved 354,711 new unique
states and seven fixed-pair searches completed. No global record improved.
Current metadata is terminal/completed for Fang-Mo Hand/Midas. The deep
campaign guidance was reread. Gayawaku's graph is still partial and the new
balanced mapped lane has no prior artifact.

Before this run: 1,968,411 nodes, 342,674 open, 109,217 partial, 1,516,520
solved, best 50,105, root partial, 2,136,944,640 database bytes.

1. In progress: Gayawaku mapped o50/c20, beam 6,000, 300,000-atomic cap,
   540-second budget, 384/288 MB limits, using the existing graph unchanged.
2. Pending: full path, score, final RSS, counts, completion/timeout and
   unique/solved increments. A global candidate must exceed 50,105.
3. Pending: Huangoz, Kifuai, Mahaji and Vilparino Necklace/Midas, sequential
   bounded fixed-pair checks after this mapped process exits.
4. Pending on a global winner: strict/direct replay, promotion, safe checks.

No native source changed. The goal remains active and unachieved.

The next fixed-pair plan is refined: current-model Fang-Mo Blue/Oasis and
Blue/Time precede the four distant Necklace/Midas families. The nine current
Fang-Mo completions covered Midas only, so other Witch choices remain a
real coverage gap. Completed Petrinov Blue runs demonstrate that changing
Witch choices can change a Merchant-family maximum substantially; they do
not predict a Fang-Mo score. No concurrent native process is launched.

### Balanced mapped Gayawaku result and Witch alternatives

Tool 70543 exited zero. Gayawaku reached 50,105, a town-global
tie. It processed 56,510 states, generated
240,484 transitions, expanded 213,103
atomic states and reused 20,010 children.
Native / wrapper time was 547.778 /
547.949 seconds; external peak RSS
164,916 KiB. Search is incomplete, stopping on
`persistent search session time budget reached`. The root remains partial. Full
18-action path, command, environment and graph snapshots are
in `persistent-ranking-summary.jsonl`.

The graph gained 220,474 unique nodes and
209,543 solved nodes. Afterward it has
2,188,885 total, 350,045 open,
112,777 partial, 1,726,063 solved, and
2,360,963,072 database bytes. This is durable new coverage,
not an additional global record or optimality certificate.

1. In progress: corrected-model Fang-Mo Blue/Oasis and Blue/Time, then
   Huangoz, Kifuai, Mahaji and Vilparino Necklace/Midas, sequentially with
   broad/suffix 32,000, split day 12, batches 1,000, balanced ranking,
   512/384 MB caps and 540-second budgets. Stop on failure or global candidate.
2. Pending: full metrics, paths, completion/pruning and audit refresh.
3. Pending: continue still-partial persistent coverage with an untried
   ranking mix or a clearly labeled bounded continuation session.
4. Pending on a global winner: strict/direct replay, promotion, safe checks.

No native source changed. No additional global record has been found.

### Six fixed-pair completions and wider mapped frontier plan

Tool 82218 completed all six cases with exit zero. Every batch finished
with zero beam trims, `search_complete: true` and `stop_reason: null`.
All scores are below town globals. Full paths, commands and environments
remain in `pair-coverage-summary.jsonl`.

| Town / Merchant / Witch | Score / global | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Fang-Mo / Blue Treasure / Oasis of Sanctifan | 22,628 / 37,569 | 322,818 | 833,937 | 79.841 / 79.862 | 97,080 | 17 / 16,009 | 17 |
| Fang-Mo / Blue Treasure / Time is Money | 19,900 / 37,569 | 152,145 | 319,858 | 37.265 / 37.285 | 96,316 | 5 / 4,426 | 18 |
| Huangoz / Leaders Necklace / Midas was a Trader | 30,989 / 37,527 | 42,631 | 92,115 | 4.416 / 4.435 | 62,580 | 3 / 2,578 | 17 |
| Kifuai / Leaders Necklace / Midas was a Trader | 31,004 / 37,547 | 46,900 | 102,182 | 4.929 / 4.949 | 65,860 | 3 / 2,784 | 17 |
| Mahaji / Leaders Necklace / Midas was a Trader | 31,041 / 37,589 | 44,384 | 96,510 | 4.272 / 4.289 | 60,968 | 3 / 2,656 | 17 |
| Vilparino / Leaders Necklace / Midas was a Trader | 31,004 / 37,569 | 42,891 | 93,369 | 4.204 / 4.223 | 63,076 | 3 / 2,598 | 17 |

The corrected-model Fang-Mo Witch alternatives were measured directly:
Blue/Oasis 22,628 and Blue/Time 19,900 are both below Blue/Midas 33,529
and the global 37,569. Evidence from another town did not predict these
scores; completed untrimmed pairs require no width-only extension.

The residual sweep refreshed the historical audit. The next plan is:

1. Pending next execution: Gayawaku existing persistent graph, optimistic
   100%, continuation zero, wider mapped beam 8,000, atomic cap 300,000,
   540-second time, existing 384/288 MB limits. Verify partial root and
   unrecorded lane before launch; preserve the graph. This changes frontier
   scheduling and resumes still-open descendants rather than a timeout-only
   repetition of an exhausted search.
2. Pending: full route, score, final RSS, counts, completion/timeout, graph
   before/after and unique/solved increments. Only a score above 50,105
   is a global candidate.
3. Pending: next audited historical families by gap; additional Fang-Mo
   Witch choices remain open, although all nine Midas families and the
   two Blue alternatives now have completed untrimmed current-model runs.
4. Pending on a winner: strict/direct replay, promotion, safe build and
   bounded tests. No global candidate exists for this gate yet.

This turn completed seven bounded searches: a time-stopped persistent
Gayawaku lane and six untrimmed fixed pairs. Gayawaku retained 220,474 new
unique nodes and 209,543 newly solved nodes, while tying 50,105. No native
source changed. Metadata is terminal/completed for Vilparino Necklace/Midas
and no solver remains running. No additional global record has been found;
the goal remains active and unachieved.

### Wider optimistic mapped Gayawaku execution

The previous turn made progress: one persistent lane added 220,474 unique
nodes, and six fixed pairs finished untrimmed. No global score improved.
Current metadata is terminal/completed for Vilparino. Deep campaign guidance
was reread. Gayawaku's graph remains partial, and this 8,000-frontier lane
has no prior artifact.

Before launch: 2,188,885 total nodes; 350,045 open; 112,777 partial;
1,726,063 solved; root partial; best 50,105; 2,360,963,072 database bytes.

1. In progress: existing mapped runner, Gayawaku, o100/c0, beam 8,000,
   300,000-atomic cap, 540-second budget, 384/288 MB limits. It preserves
   the graph and changes scheduling through a wider frontier and ranking.
2. Pending: final score, full path, external RSS, counts, stop/completion,
   graph snapshots and unique/solved increments; candidate threshold 50,105.
3. Pending: next historical untested pairs by gap, beginning Huangoz Bag,
   Kifuai Route and Kifuai Hand, all with Midas, sequential bounded runs.
4. Pending on a global winner: strict/direct replay, promotion, safe checks.

No native source changed. The goal remains active and unachieved.

### Wider optimistic mapped result and three fixed pairs

Tool 22567 exited zero. Gayawaku reached 50,105, a global
tie, with source `discovered`. It processed 83,884
states, generated 350,741 transitions, expanded
258,587 atomic states and reused
36,856 children. Native / wrapper time
was 545.058 / 545.211 seconds;
external peak RSS 176,296 KiB. Search is incomplete, stop
`persistent search session time budget reached`, root partial. Full 18-action path,
command, environment and graph snapshots are in the persistent ranking
summary. No new town record is claimed.

The graph gained 313,885 unique nodes and
241,750 solved nodes. Afterward:
2,502,770 total, 405,343 open,
129,614 partial, 1,967,813 solved,
2,652,762,112 database bytes.

1. In progress: Huangoz Bag/Midas, Kifuai Route/Midas and Hand/Midas,
   sequential 32,000/day-12/1,000/balanced runs with 512/384 MB caps and
   540-second budgets. Stop on failure or score above the current global.
2. Pending: full paths, RSS, counts, score, completion/pruning and audit.
3. Pending: choose further still-open mapped coverage from observed
   frontier changes, or the next historical untested pair by gap.
4. Pending on a global winner: strict/direct replay, promotion, safe checks.

No native source changed. The goal remains active and unachieved.

### Three fixed-pair completions and next mapped mix

Tool 90798 completed all three cases with exit zero. Every batch finished
with zero beam trims, `search_complete: true` and `stop_reason: null`.
All scores are below town globals. Full paths, commands and environments
remain in `pair-coverage-summary.jsonl`.

| Town / Merchant | Score / global | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Huangoz / Canvas Bag | 30,933 / 37,527 | 80,853 | 184,221 | 8.765 / 8.783 | 75,736 | 5 / 4,486 | 17 |
| Kifuai / Route to Mahaji | 31,094 / 37,547 | 57,378 | 131,179 | 5.634 / 5.658 | 63,332 | 4 / 3,373 | 17 |
| Kifuai / Hand of Midas | 30,869 / 37,547 | 58,940 | 131,528 | 5.397 / 5.416 | 69,168 | 4 / 3,334 | 17 |

Kifuai Route improves its historical family score from 30,914 to 31,094
(+180), but remains below the global 37,547. No promotion is made.

The residual sweep refreshed the remaining-pair audit. Next actions:

1. Pending next execution: Gayawaku persistent graph, wider 8,000 frontier
   with o0/c80, 300,000-atomic cap, 540-second budget, 384/288 MB limits.
   Verify partial root and an unrecorded lane, and preserve all graph data.
   This independently ranks still-open descendants relative to the new
   wider optimistic completion.
2. Pending: full route, score, RSS, counts, completion/timeout and graph
   unique/solved increments. The candidate threshold remains 50,105.
3. Pending: next historical untested pairs by gap, beginning Normalia,
   Huangoz and Moonin Hand, then Mahaji Route and Vilparino Hand/Route with
   Midas; verify audit ordering
   before launch. Completed untrimmed pairs need no width-only repeat.
4. Pending on a global winner: strict/direct replay, promotion, safe build
   and bounded tests. No global candidate exists for this gate yet.

This turn completed four bounded searches: one time-stopped persistent
Gayawaku lane and three untrimmed fixed pairs. Gayawaku retained 313,885
new unique states and 241,750 newly solved states, but tied 50,105. No native
source changed. Metadata is terminal/completed for Kifuai Hand/Midas and
no solver remains running. No additional global record has been found;
the goal remains active and unachieved.

### Wider continuation-heavy mapped execution

The previous turn made progress: 313,885 new unique graph states were saved,
and three fixed-pair runs completed. No global record improved. Current
metadata is terminal/completed for Kifuai Hand/Midas. Deep campaign guidance
was reread. Gayawaku remains partial and this ranking/frontier lane is new.

Before launch: 2,502,770 total; 405,343 open; 129,614 partial; 1,967,813
solved; best 50,105; root partial; 2,652,762,112 database bytes.

1. In progress: Gayawaku persistent o0/c80, beam 8,000, 300,000-atomic
   expansion cap, 540-second budget, 384/288 MB limits. Preserve the graph.
2. Pending: full path, score, RSS, counts, stop/completion and unique/solved
   increments. Global candidate threshold remains greater than 50,105.
3. Pending: Normalia, Huangoz and Moonin Hand/Midas, Mahaji Route/Midas,
   Vilparino Hand/Midas and Route/Midas, sequential bounded fixed pairs.
4. Pending on a global candidate: strict/direct replay, promotion, safe checks.

No native source changed. The goal remains active and unachieved.

### Wider continuation-heavy mapped result and six fixed pairs

Tool 32897 exited zero. Gayawaku reached 50,105, a global tie.
It processed 74,338 states, generated
327,973 transitions, expanded
272,740 atomic states and reused
18,144 children. Native / wrapper time was
545.553 / 545.717 seconds; external
peak RSS 184,056 KiB. Search is incomplete, stop
`persistent search session time budget reached`, root partial. The full 18-action path,
command, environment and graph snapshots are in the persistent ranking summary.

The graph gained 309,829 unique nodes and
261,578 solved nodes. Afterward:
2,812,599 total, 442,432 open,
140,776 partial, 2,229,391 solved,
3,009,900,544 database bytes. No new global record or
optimality certificate follows from this retained coverage.

1. In progress: Normalia, Huangoz and Moonin Hand/Midas, then Mahaji
   Route/Midas and Vilparino Hand/Midas and Route/Midas, sequential
   32,000/day-12/1,000/balanced runs, 512/384 MB limits, 540-second budgets.
   Stop on failure or a score above the current global record.
2. Pending: full paths, score, RSS, counts, completion/pruning and audit.
3. Pending: choose further still-open mapped coverage and remaining
   historical families from the updated evidence.
4. Pending on a global winner: strict/direct replay, promotion, safe checks.

No native source changed. The goal remains active and unachieved.

### Six Hand/Route completions and wider balanced next plan

Tool 80899 completed all six cases with exit zero. Every batch finished
with zero beam trims, `search_complete: true` and `stop_reason: null`.
All scores remain below current town globals. Full paths, commands and
environments are in `pair-coverage-summary.jsonl`.

| Town / Merchant | Score / global | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Normalia / Hand of Midas | 30,839 / 37,527 | 53,176 | 119,002 | 4.686 / 4.705 | 66,064 | 4 / 3,114 | 17 |
| Huangoz / Hand of Midas | 30,839 / 37,527 | 53,693 | 119,628 | 4.819 / 4.837 | 65,896 | 4 / 3,140 | 17 |
| Moonin / Hand of Midas | 30,896 / 37,584 | 284,270 | 608,797 | 82.857 / 82.877 | 94,752 | 10 / 9,667 | 17 |
| Mahaji / Route to Mahaji | 31,101 / 37,589 | 53,744 | 122,736 | 4.832 / 4.850 | 61,644 | 4 / 3,175 | 17 |
| Vilparino / Hand of Midas | 30,869 / 37,569 | 54,061 | 121,401 | 4.966 / 4.984 | 65,912 | 4 / 3,165 | 17 |
| Vilparino / Route to Mahaji | 31,049 / 37,569 | 52,525 | 120,337 | 4.533 / 4.551 | 62,184 | 4 / 3,113 | 17 |

Mahaji Route improves its historical family score from 30,891 to 31,101
(+210), and Vilparino Route improves 30,869 to 31,049 (+180). Both remain
below their town globals, so no record promotion or goal success is claimed.

The residual sweep refreshed the audit. Next actions:

1. Pending next execution: Gayawaku persistent graph, balanced o50/c20,
   wider beam 8,000, 300,000-atomic cap, 540-second budget, 384/288 MB
   limits. Verify partial root and unrecorded lane; preserve all graph data.
2. Pending: full route, score, final RSS, counts, completion/timeout and
   graph unique/solved increments. The global candidate threshold is 50,105.
3. Pending: next historical untested pairs by gap, ordered from the updated
   audit, and additional current-model Witch alternatives where relevant.
4. Pending on a global winner: strict/direct replay, promotion, safe build
   and bounded tests. No global candidate exists for this gate yet.

This turn completed seven bounded searches: a time-stopped persistent
Gayawaku lane and six untrimmed fixed pairs. Gayawaku retained 309,829 new
unique nodes and 261,578 newly solved nodes, but tied 50,105. No native
source changed. Metadata is terminal/completed for Vilparino Route/Midas
and no solver remains running. No additional global record has been found;
the goal remains active and unachieved.

### Wider balanced mapped execution

The previous turn made progress: 309,829 unique graph states were saved and
six fixed-pair runs completed untrimmed. No global record improved. Current
metadata is terminal/completed for Vilparino Route/Midas. Deep campaign
instructions were reread. Gayawaku's root remains partial and this wider
balanced ranking lane has no prior artifact.

Before launch: 2,812,599 total; 442,432 open; 140,776 partial; 2,229,391
solved; best 50,105; root partial; 3,009,900,544 database bytes.

1. In progress: Gayawaku persistent o50/c20, frontier 8,000, 300,000-atomic
   cap, 540-second budget and 384/288 MB limits. Preserve the existing graph.
2. Pending: full route, score, RSS, counts, stop/completion, graph snapshots
   and unique/solved increments. Global candidate threshold is above 50,105.
3. Pending: next historical pairs by gap from the refreshed audit, sequential
   bounded fixed-pair runs after this process exits.
4. Pending on a global winner: strict/direct replay, promotion, safe checks.

No native source changed. The goal remains active and unachieved.

### Wider balanced mapped result and next eight fixed pairs

Tool 1290 exited zero. Gayawaku reached 50,105, a global tie.
It processed 61,578 states, generated
256,834 transitions, expanded
228,476 atomic states and reused
25,173 children. Native / wrapper time was
553.541 / 553.803 seconds; external
peak RSS 174,860 KiB. Search is incomplete, stop
`persistent search session time budget reached`, root partial. Full 18-action path,
command, environment and graph snapshots remain in the persistent ranking
summary. No additional global record is claimed.

The graph gained 231,661 unique nodes and
226,546 solved nodes. Afterward:
3,044,260 total, 445,617 open,
142,706 partial, 2,455,937 solved,
3,248,439,296 database bytes.

1. In progress: Normalia Route, Huangoz Route, Mahaji Hand, Kifuai Bag,
   Normalia Bag, Vilparino Bag, Moonin Bag, Ipartus Hand, all with Midas,
   sequential 32,000/day-12/1,000/balanced lanes, 512/384 MB caps and
   540-second budgets. Stop on failure or global-score candidate.
2. Pending: full paths, score, RSS, counts, completion/pruning and audit.
3. Pending: further still-open persistent coverage or an untested family
   from the refreshed audit.
4. Pending on a global winner: strict/direct replay, promotion, safe checks.

No native source changed. The goal remains active and unachieved.

### Eight fixed-pair completions and next frontier plan

Tool 51914 completed all eight cases with exit zero. Every batch finished
with zero beam trims, `search_complete: true` and `stop_reason: null`.
All scores are below the respective globals. Full paths, commands and
environments remain in `pair-coverage-summary.jsonl`.

| Town / Merchant | Score / global | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Normalia / Route to Mahaji | 31,004 / 37,527 | 51,420 | 117,771 | 4.508 / 4.530 | 61,852 | 4 / 3,084 | 17 |
| Huangoz / Route to Mahaji | 31,004 / 37,527 | 51,658 | 118,077 | 4.803 / 4.820 | 61,524 | 4 / 3,097 | 17 |
| Mahaji / Hand of Midas | 30,899 / 37,589 | 54,470 | 121,867 | 5.050 / 5.069 | 63,808 | 4 / 3,157 | 17 |
| Kifuai / Canvas Bag | 30,762 / 37,547 | 87,121 | 200,137 | 9.738 / 9.761 | 78,188 | 5 / 4,787 | 17 |
| Normalia / Canvas Bag | 30,732 / 37,527 | 80,682 | 183,620 | 8.696 / 8.714 | 74,944 | 5 / 4,445 | 17 |
| Vilparino / Canvas Bag | 30,762 / 37,569 | 81,681 | 187,438 | 8.526 / 8.545 | 74,404 | 5 / 4,528 | 17 |
| Moonin / Canvas Bag | 30,732 / 37,584 | 375,027 | 791,944 | 131.969 / 131.993 | 101,724 | 13 / 12,951 | 17 |
| Ipartus / Hand of Midas | 30,839 / 37,734 | 53,574 | 119,555 | 5.256 / 5.276 | 68,596 | 4 / 3,128 | 17 |

Normalia and Huangoz Route improve their historical family scores by
180 each (30,824 to 31,004); Mahaji Hand improves by 90 (30,809 to 30,899).
None reaches its town global, so no promotion or goal success is claimed.

The residual sweep refreshed the audit. Next actions:

1. Pending next execution: Gayawaku persistent graph, optimistic o100/c0,
   wider frontier 10,000, 300,000-atomic cap, 540-second budget, existing
   384/288 MB limits. Verify partial root and unrecorded lane; preserve the
   graph. This schedules a wider set of still-open descendants.
2. Pending: full route, score, final RSS, counts, stop/completion and graph
   unique/solved increments. The global candidate threshold remains 50,105.
3. Pending: further historical untested pairs by gap from the refreshed
   audit, and additional current-model Witch alternatives where relevant.
4. Pending on a global winner: strict/direct replay, promotion, safe build
   and bounded tests. No global candidate exists for this gate yet.

This turn completed nine bounded searches: one time-stopped persistent
Gayawaku lane and eight untrimmed fixed pairs. Gayawaku retained 231,661
new unique states and 226,546 newly solved states, but tied 50,105. No native
source changed. Metadata is terminal/completed for Ipartus Hand/Midas and
no solver remains running. No additional global record has been found;
the goal remains active and unachieved.

### 10,000-frontier optimistic mapped execution

The previous turn made progress: 231,661 unique graph states were saved and
eight fixed-pair runs finished untrimmed. No global record improved. Current
metadata is terminal/completed for Ipartus Hand/Midas. Deep campaign guidance
was reread. Gayawaku remains partial and this wider lane has no prior artifact.

Before launch: 3,044,260 total; 445,617 open; 142,706 partial; 2,455,937
solved; best 50,105; root partial; 3,248,439,296 database bytes.

1. In progress: Gayawaku persistent o100/c0, frontier 10,000, 300,000-atomic
   cap, 540-second budget, 384/288 MB limits. Preserve the existing graph.
2. Pending: full path, score, final RSS, counts, stop/completion and graph
   unique/solved increments. Global candidate threshold remains above 50,105.
3. Pending: Cornecopea Statuette, Ipartus Route, Tocaccialli Statuette,
   Cornecopea Necklace, Mehaz Blue, Cornecopea Route, all with Midas,
   sequential bounded fixed-pair runs after the mapped process exits.
4. Pending on a global winner: strict/direct replay, promotion, safe checks.

No native source changed. The goal remains active and unachieved.

### 10,000-frontier optimistic result and six fixed pairs

Tool 47831 exited zero. Gayawaku reached 50,105, a global tie.
It processed 78,351 states, generated
332,320 transitions, expanded
251,992 atomic states and reused
34,765 children. Native / wrapper time was
556.075 / 556.362 seconds; external
peak RSS 192,780 KiB. Search is incomplete, stop
`persistent search session time budget reached`, root partial. Full 18-action path,
command, environment and graph snapshots are in the persistent ranking
summary. No additional global record is claimed.

The graph gained 297,555 unique nodes and
233,430 solved nodes. Afterward:
3,341,815 total, 491,180 open,
161,268 partial, 2,689,367 solved,
3,514,859,520 database bytes.

1. In progress: Cornecopea Statuette/Midas, Ipartus Route/Midas, Tocaccialli
   Statuette/Midas, Cornecopea Necklace/Midas, Mehaz Blue/Midas, Cornecopea
   Route/Midas, sequential 32,000/day-12/1,000/balanced lanes, 512/384 MB
   limits and 540-second budgets. Stop on failure or global-score candidate.
2. Pending: full paths, score, RSS, counts, completion/pruning and audit.
3. Pending: further still-open mapped coverage or next uncovered families.
4. Pending on a global winner: strict/direct replay, promotion, safe checks.

No native source changed. The goal remains active and unachieved.

### Six fixed-pair completions and next 10,000-frontier mix

Tool 91771 completed all six cases with exit zero. Every batch finished
with zero beam trims, `search_complete: true` and `stop_reason: null`.
All scores remain below their town globals. Full paths, commands and
environments remain in `pair-coverage-summary.jsonl`.

| Town / Merchant | Score / global | Processed | Generated | Native / wrapper seconds | Peak RSS KiB | Batches / frontier | Actions |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Cornecopea / Wooden Statuette | 36,983 / 43,881 | 87,484 | 204,280 | 8.684 / 8.710 | 74,560 | 5 / 4,936 | 17 |
| Ipartus / Route to Mahaji | 31,004 / 37,734 | 51,714 | 118,166 | 4.540 / 4.558 | 66,140 | 4 / 3,095 | 17 |
| Tocaccialli / Wooden Statuette | 42,692 / 49,634 | 78,062 | 178,624 | 11.838 / 11.856 | 77,608 | 4 / 3,060 | 18 |
| Cornecopea / Leaders Necklace | 36,885 / 43,881 | 92,796 | 206,739 | 11.364 / 11.383 | 85,240 | 5 / 4,954 | 17 |
| Mehaz / Blue Treasure | 41,657 / 48,738 | 184,823 | 418,433 | 45.089 / 45.110 | 92,628 | 9 / 8,082 | 17 |
| Cornecopea / Route to Mahaji | 36,739 / 43,881 | 85,539 | 200,485 | 8.188 / 8.206 | 69,696 | 5 / 4,827 | 17 |

Ipartus Route improves its historical family score from 30,824 to 31,004
(+180), but remains below the global 37,734. No promotion is made.

The residual sweep refreshed the audit. Next actions:

1. Pending next execution: Gayawaku persistent graph, continuation-heavy
   o0/c80, frontier 10,000, 300,000-atomic cap, 540-second budget, existing
   384/288 MB limits. Verify partial root and unrecorded lane; preserve
   the graph. This independently ranks still-open descendants.
2. Pending: full path, score, RSS, counts, stop/completion and graph
   unique/solved increments. The global threshold remains above 50,105.
3. Pending: next historical untested pairs by gap, plus current-model
   Witch alternatives where relevant. Completed untrimmed pairs need no
   width-only or timeout-only repeat.
4. Pending on a global winner: strict/direct replay, promotion, safe build
   and bounded tests. No global candidate exists for this gate yet.

This turn completed seven bounded searches: one time-stopped persistent
Gayawaku lane and six untrimmed fixed pairs. Gayawaku retained 297,555 new
unique nodes and 233,430 newly solved nodes, but tied 50,105. No native
source changed. Metadata is terminal/completed for Cornecopea Route/Midas
and no solver remains running. No additional global record has been found;
the goal remains active and unachieved.

## Continuation-heavy 10,000-state Gayawaku frontier

Execution preflight confirms terminal prior metadata, partial Gayawaku root,
3,341,815 stored nodes and best score 50,105. The o0/c80, 10,000-state,
540-second mapped lane has no existing result. Resume this graph under the
384/288 MB limits; record full metrics and route before moving sequentially
to the next untested fixed Merchant/Witch pairs. No new global candidate
is available yet.

The next sequential fixed-pair queue is Doba Lao Necklace/Midas, Doba Lao
Bag/Midas, Octoyashi Route/Midas, and Port-Willam Route/Midas. Each uses
broad/suffix width 32,000, split day 12, batch 1,000, k30/c20/o50,
540-second solver budget and 512/384 MB external/internal limits. None
has a matching existing lane. Stop the queue on failure or a global
improvement; inspect full completion/pruning metrics after each run.

Gayawaku o0/c80 at frontier 10,000 exited 0: score 50,105 (tie),
76,180 processed, 340,516 generated, 273,965 atomic expansions, 22,690
reused children. Native/wrapper elapsed 555.684/555.941 seconds; external
peak RSS 197,888 KiB. Stop: persistent search session time budget reached;
search_complete=false, root remains partial. Graph grew by 317,826 unique
nodes and 259,407 solved nodes to 3,659,641/2,948,774; 535,041 open and
175,826 partial nodes remain. Full route and command are in
`results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

## Four completed fixed pairs after Gayawaku continuation mix

All four returned code 0, search_complete=true, null stop reason and zero
beam prunes. They used the previously specified bounded profile.

| Town / Merchant (Midas) | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| Doba Lao / Leaders Necklace | 38,335 / 45,543 | 45,765 / 99,213 | 5.146 / 5.166 | 69,748 |
| Doba Lao / Canvas Bag | 38,305 / 45,543 | 85,757 / 195,989 | 9.931 / 9.950 | 78,924 |
| Octoyashi / Route to Mahaji | 32,679 / 40,056 | 62,544 / 142,574 | 5.975 / 5.993 | 64,668 |
| Port-Willam / Route to Mahaji | 34,763 / 42,296 | 78,502 / 182,300 | 8.468 / 8.487 | 69,644 |

Full native commands, limits, and action paths are stored in
`results/more-records-20261009/pair-coverage-summary.jsonl`.

Doba Lao / Leaders Necklace: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Leaders Necklace → Buy [11, 0, 0, 0, 0, 0] → Camel → Buy [29, 0, 0, 0, 0, 0] → Camel → Witch - Midas was a Trader → Camel → Buy [12, 9, 7, 0, 0, 1] → Sell → Buy [27, 0, 2, 0, 1, 4] → Sell → Interest → Interest

Doba Lao / Canvas Bag: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Canvas Bag → Buy [8, 0, 0, 0, 0, 0] → Trader → Buy [25, 0, 0, 0, 0, 0] → Camel → Witch - Midas was a Trader → Camel → Buy [12, 6, 8, 0, 0, 1] → Sell → Buy [24, 0, 1, 0, 1, 4] → Sell → Interest → Interest

Octoyashi / Route to Mahaji: Trader → Trader → Buy [14, 0, 0, 0, 0, 0] → Trader → Camel → Buy [30, 0, 0, 0, 0, 0] → Merch - Route to Mahaji → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [18, 18, 0, 0, 0, 1] → Sell → Buy [27, 1, 0, 0, 0, 5] → Sell → Interest → Interest

Port-Willam / Route to Mahaji: Trader → Buy [15, 0, 0, 0, 0, 0] → Camel → Camel → Buy [5, 9, 0, 0, 0, 0] → Merch - Route to Mahaji → Sell → Buy [10, 11, 0, 2, 0, 0] → Sell → Witch - Midas was a Trader → Buy [10, 18, 0, 0, 0, 1] → Sell → Buy [10, 9, 0, 0, 0, 4] → Sell → Buy [10, 0, 0, 0, 0, 7] → Sell → Interest

Residual sweep: the four completed untrimmed families were removed from
`remaining-fixed-pair-audit.json`; this audit is a historical-family queue,
not a certificate for every Witch choice. Current saved thresholds match
the frozen comparison values for all towns searched in this phase.
Metadata is terminal/completed for Port-Willam Route/Midas, and no native
solver remains running. No native source changed.

Next plan:

1. Pending: Gayawaku persistent balanced o50/c20, frontier 10,000,
   300,000 atomic cap, 540 seconds, 384/288 MB limits. Lane is unrecorded;
   preserve the partial graph and verify terminal metadata before launch.
2. Pending: record full score/path, RSS, processed/generated counts,
   stop/completion and unique/solved graph increments.
3. Pending: next non-Gayawaku untested families include Mehaz Saddle/Midas,
   Jilliqo Hand/Midas, Port-Willam Bag/Midas and Skjollird Route/Oasis;
   run sequentially under the existing fixed-pair bounded profile. Gayawaku
   Necklace/Bag/Statuette remain distinct pending memory-sensitive families.
4. Pending on a score above its current town record: stop campaigns,
   strict and direct replay, promote consistently, safe build and bounded
   focused/full tests, followed by a fresh residual sweep.

This phase completed five bounded searches. Gayawaku retained 317,826 new
unique nodes and 259,407 newly solved nodes but tied 50,105; all four
fixed pairs finished below their town records. No additional global
record has been found. The goal remains active and unachieved.

## Balanced 10,000-state Gayawaku continuation

Previous goal turn made progress: five bounded searches yielded retained
graph states and four completed untrimmed families. Current preflight
confirms terminal Port-Willam metadata, no native solver running, and
Gayawaku partial graph with 3,659,641 nodes / 2,948,774 solved. Next execute
unrecorded o50/c20 frontier 10,000, 300,000 atomic cap, 540 seconds,
384/288 MB limits. Preserve existing graph. After metrics are recorded,
run Mehaz Saddle/Midas, Jilliqo Hand/Midas, Port-Willam Bag/Midas, and
Skjollird Route/Oasis sequentially under the 32,000-width bounded profile.
Stop on failure or a score above the current saved town record.

Coverage inventory during this run: the fixed-pair result files contain
122 distinct completed, untrimmed, routed town/Merchant/Witch families:
107 Midas, seven Time, five Oasis, and three Patience. This excludes
partial/pruned searches and is not an all-choice optimality certificate.
The queued Route/Oasis run addresses a distinct non-Midas family.

Gayawaku balanced o50/c20 at frontier 10,000 returned 0, score 50,105 (tie),
65,237 processed / 273,980 generated,
244,967 atomic expansions / 28,268 reused children.
Native/wrapper seconds 550.922/551.151;
external peak RSS 191,108 KiB. Stop: persistent search session time budget reached;
search_complete=false. Graph added 245,712 unique
nodes and 245,470 solved nodes; root remains partial. Full command, limits,
route and before/after counts are in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

## Four completed fixed pairs after balanced Gayawaku mix

All four returned 0, search_complete=true, null stop reason, zero beam
prunes and a full action path. No score exceeded its saved town record.

| Town / Merchant / Witch | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| Mehaz / Sturdy Saddle / Midas was a Trader | 40,895 / 48,738 | 224,691 / 506,750 | 73.223 / 73.246 | 107,808 |
| Jilliqo / Hand of Midas / Midas was a Trader | 31,156 / 37,819 | 65,078 / 149,461 | 6.454 / 6.472 | 68,644 |
| Port-Willam / Canvas Bag / Midas was a Trader | 34,184 / 42,296 | 111,197 / 258,199 | 15.146 / 15.166 | 82,224 |
| Skjollird / Route to Mahaji / Oasis of Sanctifan | 38,825 / 47,065 | 196,091 / 544,596 | 26.779 / 26.798 | 83,912 |

Mehaz Saddle improved its historical pair score by 1, Jilliqo Hand by
1,399, and Port-Willam Bag by 64. These remain below their town-global
records; no promotion is made. Full commands and action paths are in
`results/more-records-20261009/pair-coverage-summary.jsonl`.

Mehaz / Sturdy Saddle / Midas was a Trader: Trader → Trader → Buy [15, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Merch - Sturdy Saddle → Trader → Trader → Witch - Midas was a Trader → Trader → Camel → Buy [13, 12, 8, 0, 1, 2] → Sell → Buy [14, 0, 1, 0, 1, 9] → Sell → Interest

Jilliqo / Hand of Midas / Midas was a Trader: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Trader → Buy [13, 0, 0, 0, 0, 0] → Merch - Hand of Midas → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [18, 8, 5, 0, 0, 2] → Sell → Buy [18, 0, 0, 0, 0, 7] → Sell → Interest

Port-Willam / Canvas Bag / Midas was a Trader: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [34, 0, 0, 0, 0, 0] → Trader → Merch - Canvas Bag → Trader → Trader → Witch - Midas was a Trader → Camel → Buy [16, 1, 14, 0, 0, 1] → Sell → Buy [27, 0, 2, 0, 0, 5] → Sell → Interest → Interest

Skjollird / Route to Mahaji / Oasis of Sanctifan: Trader → Buy [13, 0, 0, 0, 0, 0] → Camel → Buy [0, 10, 0, 0, 0, 0] → Merch - Route to Mahaji → Sell → Buy [8, 0, 0, 3, 0, 0] → Sell → Buy [4, 14, 0, 0, 0, 1] → Witch - Oasis of Sanctifan → Sell → Buy [0, 1, 0, 2, 0, 3] → Sell → Buy [0, 0, 0, 0, 1, 6] → Sell → Interest → Interest

Additional pending queue after the refreshed audit: Doba Lao Route/Midas,
Skjollird Statuette/Midas, Mehaz Statuette/Midas and Cornecopea Bag/Midas.
Matching lanes are absent. Use the same 32,000 broad/suffix, split12,
batch1,000, k30/c20/o50, 540 seconds and 512/384 MB bounded profile,
sequentially; stop on failure or a global candidate.

## Additional four completed fixed pairs

All four returned 0, search_complete=true, null stop reason, zero beam
prunes and full action paths. No town record was exceeded.

| Town / Merchant (Midas) | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| Doba Lao / Route to Mahaji | 37,202 / 45,543 | 54,610 / 125,412 | 5.094 / 5.112 | 61,404 |
| Skjollird / Wooden Statuette | 38,693 / 47,065 | 75,686 / 173,999 | 8.617 / 8.635 | 74,980 |
| Mehaz / Wooden Statuette | 40,079 / 48,738 | 134,189 / 324,451 | 22.591 / 22.612 | 90,388 |
| Cornecopea / Canvas Bag | 35,888 / 43,881 | 156,198 / 358,108 | 26.854 / 26.875 | 94,640 |

Cornecopea Bag improved its historical family score by 690 but remains
below its 43,881 global record; no promotion. Full commands and paths are
in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Doba Lao / Route to Mahaji: Trader → Buy [13, 0, 0, 0, 0, 0] → Camel → Camel → Buy [7, 9, 0, 0, 0, 0] → Merch - Route to Mahaji → Sell → Buy [10, 7, 0, 2, 0, 0] → Sell → Witch - Midas was a Trader → Buy [10, 0, 0, 0, 0, 1] → Sell → Buy [20, 3, 0, 0, 0, 3] → Sell → Interest → Interest → Interest

Skjollird / Wooden Statuette: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [35, 0, 0, 0, 0, 0] → Merch - Wooden Statuette → Trader → Trader → Trader → Witch - Midas was a Trader → Camel → Buy [15, 0, 14, 0, 0, 1] → Sell → Buy [27, 0, 1, 0, 0, 5] → Sell → Interest → Interest

Mehaz / Wooden Statuette: Trader → Trader → Buy [15, 0, 0, 0, 0, 0] → Camel → Merch - Wooden Statuette → Trader → Trader → Buy [30, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [5, 0, 14, 0, 0, 2] → Sell → Buy [12, 0, 0, 0, 0, 8] → Sell → Interest

Cornecopea / Canvas Bag: Camel → Buy [23, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Canvas Bag → Trader → Trader → Buy [34, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [20, 3, 0, 0, 1, 2] → Sell → Buy [20, 1, 0, 0, 0, 8] → Sell → Interest

Residual sweep and next plan:

1. Pending: widen the Gayawaku persistent frontier to 12,000 for pure
   optimistic o100/c0, 300,000 atomic cap and 540 seconds under the existing
   384/288 MB limits. This lane is unrecorded. Revalidate partial graph,
   terminal metadata and disk floor before launch; preserve graph data.
2. Pending: record RSS, counts, completion/stop, score, full action path and
   unique/solved increments. All three 10,000-state mixes are now recorded,
   and the graph remains partial with 3,905,353 nodes, 3,194,244 solved,
   535,786 open and 175,323 partial. No certificate of optimality exists.
3. Pending: sequential fixed pairs from the refreshed historical audit:
   L'Exquise Saddle/Midas, Port-Willam Necklace/Midas, Skjollird Saddle/Midas
   and Tocaccialli Route/Oasis, using the established bounded profile.
   Gayawaku Necklace/Bag/Statuette remain distinct memory-sensitive work.
4. Pending if a current town record is exceeded: strict/direct replay,
   promotion, safe build, bounded focused/full tests and a fresh final audit.

This phase completed nine bounded searches: one time-stopped mapped lane
and eight untrimmed fixed pairs. Gayawaku added 245,712 unique nodes and
245,470 newly solved nodes while tying 50,105; all fixed pairs remain below
their global records. Full metrics and routes are saved. Current saved
thresholds were verified for every searched town. No native source changed.
Metadata is terminal/completed for Cornecopea Bag/Midas and no native
solver remains running. No additional global record has been found;
the goal remains active and unachieved.

## Optimistic 12,000-state Gayawaku frontier

Previous goal turn made progress: nine completed bounded runs, new retained
graph nodes and eight untrimmed fixed families. Current preflight verifies
terminal metadata, no native solver, partial Gayawaku root, 3,905,353 nodes
and 3,194,244 solved nodes. Pure optimistic o100/c0 at frontier 12,000 is
unrecorded. Launch for 540 seconds / 300,000 atomic cap under 384/288 MB
limits, preserve graph, record all metrics and full route. Then test
L'Exquise Saddle/Midas, Port-Willam Necklace/Midas, Skjollird Saddle/Midas
and Tocaccialli Route/Oasis sequentially with the established fixed profile.
Stop on failure or a global candidate.

Gayawaku pure optimistic o100/c0, frontier 12,000 returned 0; score
50,105 (tie), 82,404 processed / 339,895
generated, 261,320 atomic expansions and
34,867 reused children. Native/wrapper seconds
554.276/554.572; external peak RSS
206,948 KiB. Stop: persistent search session time budget reached;
search_complete=false, root remains partial.
Graph added 305,028 unique nodes and 242,544 solved nodes. Full command,
limits and before/after counts are in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

## Four completed fixed pairs after 12,000-state optimistic run

All four returned 0, search_complete=true, null stop reason, zero beam
prunes and full action paths. No town record was exceeded.

| Town / Merchant / Witch | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| L'Exquise / Sturdy Saddle / Midas was a Trader | 49,262 / 56,817 | 252,836 / 577,420 | 95.607 / 95.632 | 115,216 |
| Port-Willam / Leaders Necklace / Midas was a Trader | 33,537 / 42,296 | 67,303 / 151,632 | 8.154 / 8.173 | 74,044 |
| Skjollird / Sturdy Saddle / Midas was a Trader | 38,238 / 47,065 | 116,751 / 255,361 | 24.645 / 24.665 | 97,232 |
| Tocaccialli / Route to Mahaji / Oasis of Sanctifan | 40,651 / 49,634 | 254,587 / 770,615 | 57.698 / 57.721 | 97,888 |

L'Exquise Saddle improved its historical pair score by 1,152 and Skjollird
Saddle by 28, but both remain below town-global records; no promotion.
Full commands and paths are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

L'Exquise / Sturdy Saddle / Midas was a Trader: Trader → Trader → Trader → Trader → Merch - Sturdy Saddle → Trader → Buy [9, 0, 0, 0, 0, 0] → Trader → Trader → Witch - Midas was a Trader → Camel → Buy [0, 15, 0, 0, 0, 2] → Sell → Buy [6, 1, 0, 0, 0, 7] → Sell → Interest → Interest

Port-Willam / Leaders Necklace / Midas was a Trader: Trader → Buy [15, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Leaders Necklace → Buy [17, 0, 0, 0, 0, 0] → Trader → Camel → Buy [36, 0, 0, 0, 0, 0] → Witch - Midas was a Trader → Camel → Camel → Buy [10, 0, 14, 0, 0, 2] → Sell → Buy [20, 0, 0, 0, 1, 7] → Sell → Interest

Skjollird / Sturdy Saddle / Midas was a Trader: Trader → Buy [15, 0, 0, 0, 0, 0] → Trader → Camel → Buy [30, 0, 0, 0, 0, 0] → Trader → Merch - Sturdy Saddle → Trader → Trader → Witch - Midas was a Trader → Camel → Buy [18, 28, 4, 0, 0, 1] → Sell → Buy [18, 1, 16, 0, 0, 5] → Sell → Buy [9, 0, 1, 0, 0, 13] → Sell

Tocaccialli / Route to Mahaji / Oasis of Sanctifan: Trader → Buy [13, 0, 0, 0, 0, 0] → Camel → Camel → Buy [7, 9, 0, 0, 0, 0] → Merch - Route to Mahaji → Sell → Buy [10, 7, 0, 2, 0, 0] → Sell → Witch - Oasis of Sanctifan → Buy [0, 6, 0, 0, 0, 1] → Sell → Buy [0, 12, 0, 0, 0, 3] → Sell → Buy [0, 1, 0, 0, 0, 7] → Sell → Interest → Interest

Residual sweep and next plan:

1. Pending: Gayawaku persistent continuation-heavy o0/c80, frontier 12,000,
   300,000 atomic cap, 540 seconds, existing 384/288 MB limits. Matching
   lane is absent. Revalidate partial graph and terminal metadata, preserve
   graph, then launch sequentially.
2. Pending: save full path, score, RSS, processed/generated counts, stop
   and completion, and retained unique/solved graph increments. Current
   graph has 4,210,381 nodes, 3,436,788 solved, 579,494 open and 194,099
   partial nodes; root remains partial and best 50,105.
3. Pending: next untested fixed pairs from the refreshed historical queue:
   Octoyashi Hand/Midas, Skjollird Necklace/Midas, Sanctifan Necklace/Midas
   and L'Exquise Statuette/Midas. Use the same bounded fixed-pair profile.
   Gayawaku Necklace/Bag/Statuette remain separate memory-sensitive families.
4. Pending on a score above the current saved town record: strict/direct
   replay, consistent promotion, safe build and bounded focused/full tests,
   then a fresh residual audit. No candidate exists for this gate yet.

This phase completed five bounded searches: Gayawaku time-stopped with
305,028 new unique nodes and 242,544 newly solved nodes but tied 50,105;
four fixed pairs completed untrimmed below their town records. Every run
has its full route and required metrics recorded. No native source
changed. Metadata is terminal/completed for Tocaccialli Route/Oasis and
no native solver remains running. No additional global record has been
found; the goal remains active and unachieved.

## Continuation-heavy 12,000-state Gayawaku frontier

Previous goal turn made progress: five bounded runs, retained graph nodes
and four completed untrimmed fixed pairs. Current preflight confirms
terminal Tocaccialli metadata, no native solver, Gayawaku partial graph
with 4,210,381 nodes and 3,436,788 solved. The o0/c80 frontier 12,000
lane is unrecorded. Launch under 384/288 MB limits, 300,000 atomic cap
and 540-second budget; preserve graph and record full required metrics.
Then run Octoyashi Hand/Midas, Skjollird Necklace/Midas, Sanctifan
Necklace/Midas and L'Exquise Statuette/Midas sequentially using the
established fixed-pair profile. Stop on failure or a global candidate.

Gayawaku o0/c80 frontier 12,000 returned 0, score 50,105 (tie),
77,096 processed / 342,622 generated,
276,980 atomic expansions / 25,492
reused children. Native/wrapper seconds 562.667/563.047;
external peak RSS 213,208 KiB. Stop: persistent search session time budget reached;
search_complete=false, root remains partial.
Graph added 317,130 unique nodes and 261,293 solved nodes. Full command,
limits and before/after counts are in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

## Four completed fixed pairs after 12,000-state continuation-heavy run

All four returned 0, search_complete=true, null stop reason, zero beam
prunes and full action paths. No town record was exceeded.

| Town / Merchant (Midas) | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| Octoyashi / Hand of Midas | 31,119 / 40,056 | 63,831 / 142,427 | 6.837 / 6.861 | 71,020 |
| Skjollird / Leaders Necklace | 37,467 / 47,065 | 68,565 / 153,027 | 10.875 / 10.895 | 74,504 |
| Sanctifan / Leaders Necklace | 45,839 / 55,728 | 71,719 / 157,285 | 9.601 / 9.620 | 72,600 |
| L'Exquise / Wooden Statuette | 46,849 / 56,817 | 156,444 / 376,107 | 31.059 / 31.080 | 88,744 |

Octoyashi Hand improved its historical family score by 79 but remains
below its 40,056 global record; no promotion. Full commands and routes
are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Octoyashi / Hand of Midas: Trader → Trader → Buy [14, 0, 0, 0, 0, 0] → Camel → Camel → Buy [45, 0, 0, 0, 0, 0] → Merch - Hand of Midas → Trader → Trader → Witch - Midas was a Trader → Trader → Camel → Buy [15, 0, 11, 0, 0, 2] → Sell → Buy [20, 0, 0, 0, 1, 6] → Sell → Interest

Skjollird / Leaders Necklace: Trader → Buy [15, 0, 0, 0, 0, 0] → Trader → Camel → Buy [33, 0, 0, 0, 0, 0] → Trader → Merch - Leaders Necklace → Camel → Camel → Witch - Midas was a Trader → Camel → Buy [20, 10, 0, 0, 0, 1] → Sell → Buy [20, 22, 0, 0, 1, 4] → Sell → Buy [10, 0, 2, 0, 0, 14] → Sell

Sanctifan / Leaders Necklace: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Merch - Leaders Necklace → Buy [20, 0, 0, 0, 0, 0] → Trader → Trader → Buy [28, 0, 0, 0, 0, 0] → Witch - Midas was a Trader → Camel → Camel → Buy [17, 0, 11, 0, 0, 2] → Sell → Buy [20, 0, 0, 0, 1, 6] → Sell → Interest → Interest

L'Exquise / Wooden Statuette: Trader → Trader → Trader → Trader → Trader → Buy [13, 0, 0, 0, 0, 0] → Merch - Wooden Statuette → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [0, 0, 11, 0, 1, 1] → Sell → Buy [9, 0, 0, 0, 0, 6] → Sell → Interest → Interest

Additional pending queue: Sanctifan Bag/Midas, Bajaar Hand/Oasis,
Port-Willam Hand/Midas and Doba Lao Hand/Midas. Matching lanes are absent;
use established 32,000 broad/suffix, split12, batch1,000, k30/c20/o50,
540 seconds and 512/384 MB bounded profile, sequentially. Stop on
failure or a global candidate.

## Additional four completed fixed pairs

All four returned 0, search_complete=true, null stop reason, zero beam
prunes and full action paths; scores match historical family results
and remain below town-global records. No promotion.

| Town / Merchant / Witch | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| Sanctifan / Canvas Bag / Midas was a Trader | 45,551 / 55,728 | 128,072 / 292,470 | 20.008 / 20.028 | 80,408 |
| Bajaar / Hand of Midas / Oasis of Sanctifan | 45,880 / 56,147 | 219,924 / 608,529 | 35.082 / 35.101 | 90,852 |
| Port-Willam / Hand of Midas / Midas was a Trader | 31,940 / 42,296 | 82,756 / 187,763 | 10.533 / 10.553 | 78,272 |
| Doba Lao / Hand of Midas / Midas was a Trader | 35,088 / 45,543 | 57,506 / 128,041 | 6.005 / 6.023 | 70,108 |

Full commands and routes are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Sanctifan / Canvas Bag / Midas was a Trader: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Canvas Bag → Buy [16, 0, 0, 0, 0, 0] → Trader → Camel → Buy [39, 0, 0, 0, 0, 0] → Witch - Midas was a Trader → Camel → Camel → Buy [2, 16, 0, 0, 0, 2] → Sell → Buy [18, 0, 4, 0, 0, 7] → Sell → Interest → Interest

Bajaar / Hand of Midas / Oasis of Sanctifan: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [7, 0, 0, 2, 0, 0] → Merch - Hand of Midas → Sell → Buy [10, 6, 0, 0, 0, 1] → Sell → Witch - Oasis of Sanctifan → Buy [0, 1, 0, 1, 0, 3] → Sell → Buy [0, 0, 0, 0, 0, 5] → Sell → Interest → Interest → Interest

Port-Willam / Hand of Midas / Midas was a Trader: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [34, 0, 0, 0, 0, 0] → Trader → Merch - Hand of Midas → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [25, 12, 0, 0, 0, 1] → Sell → Interest → Buy [18, 0, 0, 0, 0, 8] → Sell → Interest

Doba Lao / Hand of Midas / Midas was a Trader: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Trader → Buy [13, 0, 0, 0, 0, 0] → Merch - Hand of Midas → Camel → Buy [15, 0, 0, 1, 0, 0] → Sell → Witch - Midas was a Trader → Camel → Buy [14, 0, 10, 0, 0, 1] → Sell → Buy [21, 0, 0, 0, 0, 4] → Sell → Interest → Interest

Residual sweep and next plan:

1. Pending: Gayawaku persistent balanced o50/c20, frontier 12,000,
   300,000 atomic cap, 540 seconds under 384/288 MB limits. Matching lane
   is absent; revalidate partial graph and terminal metadata, preserve
   graph data and run sequentially.
2. Pending: record full route, score, RSS, counts, stop/completion and
   unique/solved graph increments. Current graph has 4,527,511 nodes,
   3,698,081 solved, 619,644 open and 209,786 partial; root partial,
   best 50,105. These results do not certify global optimality.
3. Pending: next untested fixed pairs from the refreshed historical audit:
   Petrinov Statuette/Midas, Eduming Saddle/Midas, L'Exquise Necklace/Midas
   and Petrinov Saddle/Midas, using the established bounded fixed profile.
   Gayawaku Necklace/Bag/Statuette remain distinct memory-sensitive work.
4. Pending on a higher current town record: strict/direct replay,
   consistent promotion, safe build and bounded focused/full tests,
   followed by a fresh residual audit. No global candidate exists yet.

This phase completed nine bounded searches: one time-stopped mapped run
and eight untrimmed fixed pairs. Gayawaku retained 317,130 new unique
nodes and 261,293 newly solved nodes but tied 50,105; all fixed pairs
finished below town records. All required metrics and full action paths
are saved. Current saved thresholds were verified for every searched town.
No native source changed. Metadata is terminal/completed for Doba Lao
Hand/Midas and no native solver remains running. No additional global
record has been found; the goal remains active and unachieved.

## Balanced 12,000-state Gayawaku frontier

Previous goal turn made progress: nine bounded searches, retained graph
nodes and eight untrimmed fixed families. Current preflight confirms
terminal Doba Lao metadata, no native solver, Gayawaku partial graph with
4,527,511 nodes and 3,698,081 solved. The o50/c20 frontier 12,000 lane
is unrecorded. Launch under 384/288 MB limits, 300,000 atomic cap and
540-second budget; preserve graph and record full metrics and path.
Then run Petrinov Statuette/Midas, Eduming Saddle/Midas, L'Exquise
Necklace/Midas and Petrinov Saddle/Midas sequentially under the established
fixed-pair profile. Stop on failure or a global candidate.

Gayawaku balanced o50/c20 frontier 12,000 returned 0, score 50,105
(tie), 68,993 processed / 287,329 generated,
262,591 atomic expansions / 28,526
reused children. Native/wrapper seconds 562.986/563.336;
external peak RSS 209,476 KiB. Stop: persistent search session time budget reached;
search_complete=false, root remains partial.
Graph added 258,803 unique nodes and 263,308 solved nodes. Full command,
limits and before/after counts are in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

## Four completed fixed pairs after balanced 12,000-state run

All four returned 0, search_complete=true, null stop reason, zero beam
prunes and full action paths. No town record was exceeded.

| Town / Merchant (Midas) | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| Petrinov / Wooden Statuette | 35,114 / 45,578 | 11,914 / 30,552 | 1.113 / 1.133 | 42,424 |
| Eduming / Sturdy Saddle | 70,806 / 81,321 | 97,340 / 211,921 | 19.140 / 19.160 | 86,876 |
| L'Exquise / Leaders Necklace | 46,180 / 56,817 | 194,994 / 470,473 | 45.611 / 45.631 | 98,656 |
| Petrinov / Sturdy Saddle | 37,361 / 45,578 | 20,301 / 51,714 | 2.426 / 2.445 | 57,740 |

Petrinov Saddle improved its historical pair score by 2,892 to 37,361,
but remains below its 45,578 global record; no promotion. Full commands
and action paths are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Petrinov / Wooden Statuette: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Trader → Buy [13, 0, 0, 0, 0, 0] → Merch - Wooden Statuette → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [18, 13, 0, 0, 0, 2] → Sell → Buy [18, 0, 0, 0, 0, 7] → Sell → Interest

Eduming / Sturdy Saddle: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Sturdy Saddle → Buy [13, 0, 0, 0, 0, 0] → Camel → Buy [43, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Trader → Trader → Buy [19, 1, 0, 0, 0, 2] → Sell → Interest → Interest → Interest

L'Exquise / Leaders Necklace: Trader → Trader → Trader → Trader → Merch - Leaders Necklace → Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [0, 2, 8, 0, 0, 2] → Sell → Buy [9, 0, 0, 0, 0, 6] → Sell → Interest → Interest

Petrinov / Sturdy Saddle: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Sturdy Saddle → Buy [13, 0, 0, 0, 0, 0] → Camel → Buy [25, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Trader → Interest → Buy [32, 0, 0, 0, 0, 3] → Sell → Interest → Interest → Interest

Residual sweep and next plan:

1. Pending: widen Gayawaku persistent frontier to 14,000, pure optimistic
   o100/c0, 300,000 atomic cap and 540 seconds under existing 384/288 MB
   limits. Matching lane is absent. Revalidate partial graph and terminal
   metadata; preserve graph and run sequentially. All three 12,000 mixes
   are now recorded, and still-open descendants remain.
2. Pending: save full route, score, RSS, counts, stop/completion and
   unique/solved graph increments. Current graph has 4,786,314 nodes,
   3,961,389 solved, 615,856 open and 209,069 partial; root partial,
   best 50,105. No global optimality certificate exists.
3. Pending: next fixed-pair queue from the refreshed historical audit:
   Skjollird Bag/Midas, Cornecopea Hand/Midas, Petrinov Bag/Midas and
   L'Exquise Bag/Midas, using the established bounded profile. Gayawaku
   Necklace/Bag/Statuette remain distinct memory-sensitive families.
4. Pending if a current town record is exceeded: strict/direct replay,
   consistent promotion, safe build and bounded focused/full tests,
   followed by a fresh residual audit. No candidate exists yet.

User status query answered: one new town-global record overall, Moonin
37,584 versus 37,527 (+57), obtained before this follow-on goal. No
additional town-global record has been found for the active beat-more
goal. Historical fixed-pair improvements and graph coverage are not new
town records.

This phase completed five bounded searches: one time-stopped mapped run
with 258,803 new unique nodes and 263,308 newly solved nodes, tying
50,105, plus four untrimmed fixed pairs below town records. All required
metrics and full paths are saved. No native source changed. Metadata is
terminal/completed for Petrinov Saddle/Midas and no native solver remains
running. The goal remains active and unachieved.

## Optimistic 14,000-state Gayawaku frontier

Previous goal turn made progress: five bounded searches, retained graph
nodes and four completed untrimmed fixed pairs. Current preflight confirms
terminal Petrinov metadata, no native solver, Gayawaku partial graph with
4,786,314 nodes and 3,961,389 solved. The o100/c0 frontier 14,000 lane
is unrecorded. Launch with 384/288 MB limits, 300,000 atomic cap and
540-second budget; preserve graph, record full metrics and path. Then
run Skjollird Bag/Midas, Cornecopea Hand/Midas, Petrinov Bag/Midas and
L'Exquise Bag/Midas sequentially under the established fixed-pair
profile. Stop on failure or a global candidate.

Gayawaku o100/c0 frontier 14,000 returned 0, score 50,105 (tie),
75,175 processed / 315,709 generated,
248,301 atomic expansions / 30,970
reused children. Native/wrapper seconds 569.175/569.565;
external peak RSS 220,016 KiB. Stop: persistent search session time budget reached;
search_complete=false, root remains partial.
Graph added 284,739 unique nodes and 236,146 solved nodes. Full command,
limits and before/after counts are in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

## Four completed fixed pairs after 14,000-state optimistic run

All four returned 0, search_complete=true, null stop reason, zero beam
prunes and full action paths. No town record was exceeded.

| Town / Merchant (Midas) | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| Skjollird / Canvas Bag | 35,905 / 47,065 | 108,152 / 247,680 | 17.213 / 17.242 | 84,572 |
| Cornecopea / Hand of Midas | 32,828 / 43,881 | 106,146 / 237,461 | 13.706 / 13.728 | 87,656 |
| Petrinov / Canvas Bag | 34,320 / 45,578 | 17,662 / 46,194 | 1.553 / 1.570 | 48,468 |
| L'Exquise / Canvas Bag | 45,288 / 56,817 | 236,154 / 558,335 | 61.091 / 61.117 | 111,864 |

Cornecopea Hand improved its historical pair score by 182 to 32,828,
but remains below its 43,881 global record; no promotion. Full commands
and routes are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Skjollird / Canvas Bag: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [35, 0, 0, 0, 0, 0] → Trader → Merch - Canvas Bag → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [15, 15, 0, 0, 0, 1] → Sell → Buy [18, 17, 4, 0, 1, 4] → Sell → Buy [9, 0, 1, 0, 0, 13] → Sell

Cornecopea / Hand of Midas: Camel → Buy [23, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Hand of Midas → Trader → Trader → Buy [24, 0, 0, 0, 0, 0] → Camel → Witch - Midas was a Trader → Camel → Buy [18, 3, 12, 0, 0, 1] → Sell → Buy [27, 0, 1, 0, 0, 5] → Sell → Interest → Interest

Petrinov / Canvas Bag: Trader → Buy [11, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Canvas Bag → Buy [24, 0, 0, 0, 0, 0] → Trader → Camel → Buy [24, 0, 0, 0, 0, 0] → Witch - Midas was a Trader → Camel → Interest → Interest → Buy [24, 0, 0, 0, 0, 5] → Sell → Interest → Interest

L'Exquise / Canvas Bag: Trader → Trader → Trader → Trader → Merch - Canvas Bag → Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [0, 2, 14, 0, 1, 1] → Sell → Buy [9, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Residual sweep and next plan:

1. Pending: Gayawaku persistent continuation-heavy o0/c80, frontier 14,000,
   300,000 atomic cap and 540 seconds under 384/288 MB limits. Matching
   lane is absent. Revalidate partial graph and terminal metadata, preserve
   graph and run sequentially.
2. Pending: full route, score, RSS, counts, stop/completion and unique/solved
   graph increments. Current graph has 5,071,053 nodes, 4,197,535 solved,
   652,294 open and 221,224 partial; root partial, best 50,105. No global
   optimality certificate exists.
3. Pending: next untested fixed pairs from the refreshed historical audit:
   Tocaccialli Hand/Midas, Bajaar Bag/Oasis, Eduming Bag/Midas and Mehaz
   Hand/Midas, under the established bounded fixed-pair profile. Gayawaku
   Necklace/Bag/Statuette remain distinct memory-sensitive families.
4. Pending if a current town record is exceeded: strict/direct replay,
   consistent promotion, safe build and bounded focused/full tests,
   followed by a fresh residual audit. No candidate exists for this gate.

This phase completed five bounded searches: one time-stopped mapped run
with 284,739 new unique nodes and 236,146 newly solved nodes, tying
50,105, plus four untrimmed fixed pairs below their town records. All
required metrics and full action paths are saved. No native source
changed. Metadata is terminal/completed for L'Exquise Bag/Midas and
no native solver remains running. No additional global record has been
found; the goal remains active and unachieved.

## Continuation-heavy 14,000-state Gayawaku frontier

Previous goal turn made progress: five bounded searches, retained graph
nodes and four completed untrimmed fixed families. Current preflight
confirms terminal L'Exquise metadata, no native solver, Gayawaku partial
graph with 5,071,053 nodes and 4,197,535 solved. The o0/c80 frontier
14,000 lane is unrecorded. Launch under 384/288 MB limits, 300,000 atomic
cap and 540 seconds; preserve graph and record full metrics and path.
Then run Tocaccialli Hand/Midas, Bajaar Bag/Oasis, Eduming Bag/Midas and
Mehaz Hand/Midas sequentially with the established fixed-pair profile.
Stop on failure or a global candidate.

Gayawaku o0/c80 frontier 14,000 returned 0, score 50,105 (tie),
73,472 processed / 329,024 generated,
261,083 atomic expansions / 29,829
reused children. Native/wrapper seconds 574.820/575.316;
external peak RSS 227,120 KiB. Stop: persistent search session time budget reached;
search_complete=false, root remains partial.
Graph added 299,195 unique nodes and 244,052 solved nodes. Full command,
limits and before/after counts are in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

## Four completed fixed pairs after 14,000-state continuation-heavy run

All four returned 0, search_complete=true, null stop reason, zero beam
prunes and full action paths. All scores match historical family results
and remain below town records; no promotion.

| Town / Merchant / Witch | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| Tocaccialli / Hand of Midas / Midas was a Trader | 37,935 / 49,634 | 82,866 / 188,047 | 16.225 / 16.253 | 83,508 |
| Bajaar / Canvas Bag / Oasis of Sanctifan | 44,417 / 56,147 | 338,907 / 935,796 | 68.367 / 68.389 | 102,828 |
| Eduming / Canvas Bag / Midas was a Trader | 69,588 / 81,321 | 81,009 / 184,524 | 10.255 / 10.275 | 76,588 |
| Mehaz / Hand of Midas / Midas was a Trader | 36,790 / 48,738 | 147,374 / 346,349 | 29.558 / 29.579 | 89,704 |

Full commands and routes are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Tocaccialli / Hand of Midas / Midas was a Trader: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Hand of Midas → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 2, 13, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Bajaar / Canvas Bag / Oasis of Sanctifan: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Merch - Canvas Bag → Buy [13, 0, 0, 2, 0, 0] → Sell → Camel → Buy [6, 2, 0, 0, 0, 1] → Witch - Oasis of Sanctifan → Sell → Buy [0, 12, 0, 0, 0, 3] → Sell → Buy [0, 1, 2, 0, 0, 8] → Sell → Interest → Interest

Eduming / Canvas Bag / Midas was a Trader: Trader → Buy [11, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Canvas Bag → Buy [24, 0, 0, 0, 0, 0] → Trader → Trader → Buy [18, 0, 0, 0, 0, 0] → Witch - Midas was a Trader → Trader → Camel → Buy [35, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Mehaz / Hand of Midas / Midas was a Trader: Trader → Trader → Buy [15, 0, 0, 0, 0, 0] → Camel → Camel → Buy [0, 9, 0, 1, 0, 0] → Merch - Hand of Midas → Sell → Buy [6, 2, 0, 0, 1, 0] → Witch - Midas was a Trader → Sell → Buy [6, 4, 0, 0, 0, 2] → Sell → Buy [9, 0, 0, 0, 0, 6] → Sell → Interest → Interest

Residual sweep and next plan:

1. Pending: Gayawaku persistent balanced o50/c20, frontier 14,000,
   300,000 atomic cap and 540 seconds under 384/288 MB limits. Matching
   lane is absent. Revalidate partial graph and terminal metadata;
   preserve graph and run sequentially.
2. Pending: save full route, score, RSS, counts, stop/completion and
   unique/solved graph increments. Current graph has 5,370,248 nodes,
   4,441,587 solved, 690,406 open and 238,255 partial; root partial,
   best 50,105. No global optimality certificate exists.
3. Pending: next fixed-pair queue from the refreshed historical audit:
   Bajaar Route/Oasis, Mehaz Necklace/Midas, Skjollird Hand/Midas and
   Petrinov Necklace/Midas, using the established bounded profile.
   Gayawaku Necklace/Bag/Statuette remain distinct memory-sensitive work.
4. Pending if a current town record is exceeded: strict/direct replay,
   consistent promotion, safe build and bounded focused/full tests,
   followed by a fresh residual audit. No candidate exists yet.

This phase completed five bounded searches: one time-stopped mapped run
with 299,195 new unique nodes and 244,052 newly solved nodes, tying
50,105, plus four untrimmed fixed pairs below their town records. All
required metrics and full action paths are saved. No native source
changed. Metadata is terminal/completed for Mehaz Hand/Midas and
no native solver remains running. No additional global record has been
found; the goal remains active and unachieved.

## Balanced 14,000-state Gayawaku frontier

Previous goal turn made progress: five bounded searches, retained graph
nodes and four completed untrimmed fixed families. Current preflight
confirms terminal Mehaz metadata, no native solver, Gayawaku partial graph
with 5,370,248 nodes and 4,441,587 solved. The o50/c20 frontier 14,000
lane is unrecorded. Launch under 384/288 MB limits, 300,000 atomic cap
and 540 seconds; preserve graph and record full metrics and path. Then
run Bajaar Route/Oasis, Mehaz Necklace/Midas, Skjollird Hand/Midas and
Petrinov Necklace/Midas sequentially with the established fixed-pair
profile. Stop on failure or a global candidate.

Gayawaku o50/c20 frontier 14,000 returned 0, score 50,105 (tie),
68,528 processed / 287,571 generated,
255,378 atomic expansions / 30,601
reused children. Native/wrapper seconds 568.338/568.761;
external peak RSS 222,364 KiB. Stop: persistent search session time budget reached;
search_complete=false, root remains partial.
Graph added 256,970 unique nodes and 255,155 solved nodes. Full command,
limits and before/after counts are in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

## Four completed fixed pairs after balanced 14,000-state run

All four returned 0, search_complete=true, null stop reason, zero beam
prunes and full action paths. No town record was exceeded.

| Town / Merchant / Witch | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| Bajaar / Route to Mahaji / Oasis of Sanctifan | 43,700 / 56,147 | 221,579 / 612,540 | 36.987 / 37.013 | 93,800 |
| Mehaz / Leaders Necklace / Midas was a Trader | 36,249 / 48,738 | 140,897 / 331,409 | 29.254 / 29.276 | 87,744 |
| Skjollird / Hand of Midas / Midas was a Trader | 34,543 / 47,065 | 83,203 / 186,885 | 11.597 / 11.617 | 75,012 |
| Petrinov / Leaders Necklace / Midas was a Trader | 32,822 / 45,578 | 10,817 / 26,048 | 1.088 / 1.106 | 45,188 |

Skjollird Hand improved its historical pair score by 47 to 34,543,
but remains below its 47,065 global record; no promotion. Full commands
and routes are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Bajaar / Route to Mahaji / Oasis of Sanctifan: Trader → Buy [13, 0, 0, 0, 0, 0] → Camel → Camel → Buy [7, 12, 0, 1, 0, 0] → Merch - Route to Mahaji → Sell → Buy [10, 5, 0, 2, 1, 0] → Sell → Witch - Oasis of Sanctifan → Buy [0, 6, 0, 2, 0, 2] → Sell → Buy [0, 4, 0, 0, 0, 6] → Sell → Interest → Interest → Interest

Mehaz / Leaders Necklace / Midas was a Trader: Trader → Trader → Buy [15, 0, 0, 0, 0, 0] → Trader → Merch - Leaders Necklace → Camel → Buy [27, 0, 0, 0, 0, 0] → Trader → Camel → Witch - Midas was a Trader → Camel → Camel → Buy [11, 0, 20, 0, 0, 2] → Sell → Buy [12, 3, 2, 0, 1, 8] → Sell → Interest

Skjollird / Hand of Midas / Midas was a Trader: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [35, 0, 0, 0, 0, 0] → Trader → Merch - Hand of Midas → Trader → Trader → Witch - Midas was a Trader → Camel → Buy [15, 1, 13, 0, 0, 1] → Sell → Buy [27, 0, 1, 0, 0, 5] → Sell → Interest → Interest

Petrinov / Leaders Necklace / Midas was a Trader: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [27, 0, 0, 0, 0, 0] → Merch - Leaders Necklace → Trader → Trader → Camel → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Camel → Interest → Buy [30, 1, 0, 0, 1, 4] → Sell → Interest → Interest

Residual sweep and next plan:

1. Pending: widen Gayawaku persistent frontier to 16,000, pure optimistic
   o100/c0, 300,000 atomic cap and 540 seconds under existing 384/288 MB
   limits. Matching lane is absent; all three 14,000 mixes are recorded.
   Revalidate partial graph and terminal metadata, preserve graph and run
   sequentially.
2. Pending: full route, score, RSS, counts, stop/completion and unique/solved
   graph increments. Current graph has 5,627,218 nodes, 4,696,742 solved,
   691,998 open and 238,478 partial; root partial, best 50,105. No global
   optimality certificate exists.
3. Pending: next fixed pairs from refreshed historical audit: Mehaz
   Bag/Midas, Petrinov Route/Midas, Sanctifan Route/Midas and L'Exquise
   Hand/Midas, using the established bounded profile. Gayawaku
   Necklace/Bag/Statuette remain distinct memory-sensitive work.
4. Pending if a current town record is exceeded: strict/direct replay,
   consistent promotion, safe build and bounded focused/full tests,
   followed by a fresh residual audit. No candidate exists yet.

This phase completed five bounded searches: one time-stopped mapped run
with 256,970 new unique nodes and 255,155 newly solved nodes, tying
50,105, plus four untrimmed fixed pairs below their town records. All
required metrics and full action paths are saved. No native source
changed. Metadata is terminal/completed for Petrinov Necklace/Midas and
no native solver remains running. No additional global record has been
found; the goal remains active and unachieved.

## Optimistic 16,000-state Gayawaku frontier

Previous goal turn made progress: five bounded searches, retained graph
nodes and four completed untrimmed fixed families. Current preflight
confirms terminal Petrinov metadata, no native solver, Gayawaku partial
graph with 5,627,218 nodes and 4,696,742 solved. The o100/c0 frontier
16,000 lane is unrecorded. Launch under 384/288 MB limits, 300,000 atomic
cap and 540 seconds; preserve graph and record full metrics and path.
Then run Mehaz Bag/Midas, Petrinov Route/Midas, Sanctifan Route/Midas and
L'Exquise Hand/Midas sequentially with the established fixed-pair profile.
Stop on failure or a global candidate.

Gayawaku o100/c0 frontier 16,000 returned 0, score 50,105 (tie),
84,137 processed / 353,609 generated,
268,807 atomic expansions / 43,155
reused children. Native/wrapper seconds 571.272/572.265;
external peak RSS 276,252 KiB. Stop: persistent search session time budget reached;
search_complete=false, root remains partial.
Graph added 310,454 unique nodes and 254,788 solved nodes. Full command,
limits and before/after counts are in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

## Four completed fixed pairs after 16,000-state optimistic run

All four returned 0, search_complete=true, null stop reason, zero beam
prunes and full action paths. No town record was exceeded.

| Town / Merchant (Midas) | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| Mehaz / Canvas Bag | 35,868 / 48,738 | 202,382 / 475,798 | 50.791 / 50.821 | 100,212 |
| Petrinov / Route to Mahaji | 31,238 / 45,578 | 12,046 / 30,821 | 0.915 / 0.931 | 42,132 |
| Sanctifan / Route to Mahaji | 45,959 / 55,728 | 85,838 / 195,659 | 9.312 / 9.329 | 65,656 |
| L'Exquise / Hand of Midas | 42,218 / 56,817 | 158,814 / 376,946 | 36.090 / 36.114 | 97,568 |

Historical family improvements: Mehaz Bag +71, Sanctifan Route +4,754,
L'Exquise Hand +97. All remain below town-global records; no promotion.
Full commands and paths are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Mehaz / Canvas Bag: Trader → Trader → Buy [15, 0, 0, 0, 0, 0] → Trader → Merch - Canvas Bag → Trader → Buy [22, 0, 0, 0, 0, 0] → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [10, 1, 14, 0, 0, 1] → Sell → Buy [18, 0, 1, 0, 1, 5] → Sell → Interest → Interest

Petrinov / Route to Mahaji: Trader → Buy [13, 0, 0, 0, 0, 0] → Camel → Camel → Buy [7, 9, 0, 0, 0, 0] → Merch - Route to Mahaji → Sell → Buy [15, 5, 0, 2, 0, 0] → Sell → Witch - Midas was a Trader → Interest → Buy [10, 12, 0, 0, 1, 1] → Sell → Buy [15, 0, 0, 0, 0, 5] → Sell → Interest → Interest

Sanctifan / Route to Mahaji: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Trader → Buy [13, 0, 0, 0, 0, 0] → Merch - Route to Mahaji → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [18, 13, 0, 0, 0, 2] → Sell → Buy [18, 0, 0, 0, 0, 7] → Sell → Interest → Interest

L'Exquise / Hand of Midas: Trader → Trader → Trader → Trader → Trader → Buy [13, 0, 0, 0, 0, 0] → Merch - Hand of Midas → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [0, 0, 11, 0, 1, 1] → Sell → Buy [9, 0, 0, 0, 0, 6] → Sell → Interest → Interest

Additional pending queue: Eduming Necklace/Midas, Bajaar Statuette/Midas,
L'Exquise Route/Midas and Sanctifan Hand/Midas. Matching lanes are absent.
Use established 32,000 broad/suffix, split12, batch1,000, k30/c20/o50,
540 seconds and 512/384 MB limits, sequentially. Stop on failure or a
global candidate. Historical remaining-family audit has 17 entries; it
is not a complete inventory of every possible Witch family.

## Additional four completed fixed pairs

All four returned 0, search_complete=true, null stop reason, zero beam
prunes and full action paths. No town record was exceeded.

| Town / Merchant (Midas) | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| Eduming / Leaders Necklace | 66,442 / 81,321 | 43,045 / 93,051 | 4.590 / 4.607 | 64,488 |
| Bajaar / Wooden Statuette | 41,240 / 56,147 | 76,028 / 175,649 | 9.433 / 9.452 | 77,604 |
| L'Exquise / Route to Mahaji | 42,285 / 56,817 | 150,199 / 364,003 | 28.091 / 28.111 | 85,972 |
| Sanctifan / Hand of Midas | 45,711 / 55,728 | 88,132 / 197,593 | 10.156 / 10.174 | 72,024 |

L'Exquise Route improved its historical pair score by 886 and Sanctifan
Hand by 5,522. Both remain below town-global records; no promotion.
Full commands and paths are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Eduming / Leaders Necklace: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [27, 0, 0, 0, 0, 0] → Merch - Leaders Necklace → Trader → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Interest → Buy [30, 0, 0, 0, 0, 0] → Interest → Interest → Interest

Bajaar / Wooden Statuette: Trader → Buy [13, 0, 0, 0, 0, 0] → Camel → Camel → Buy [42, 0, 0, 0, 0, 0] → Merch - Wooden Statuette → Trader → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [20, 2, 0, 0, 1, 2] → Sell → Buy [20, 0, 1, 0, 1, 8] → Sell → Interest

L'Exquise / Route to Mahaji: Trader → Trader → Trader → Trader → Trader → Buy [13, 0, 0, 0, 0, 0] → Merch - Route to Mahaji → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [0, 14, 0, 0, 1, 1] → Sell → Buy [9, 0, 0, 0, 0, 6] → Sell → Interest → Interest

Sanctifan / Hand of Midas: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Trader → Buy [13, 0, 0, 0, 0, 0] → Merch - Hand of Midas → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [18, 8, 0, 0, 0, 2] → Sell → Buy [18, 0, 0, 0, 0, 7] → Sell → Interest → Interest

Residual sweep and next plan:

1. Pending: Gayawaku persistent continuation-heavy o0/c80, frontier 16,000,
   300,000 atomic cap and 540 seconds under existing 384/288 MB limits.
   Matching lane is absent. Revalidate partial graph and terminal metadata;
   preserve graph, run sequentially and record full required metrics.
2. Pending: full route, score, RSS, counts, stop/completion and unique/solved
   graph increments. Current graph has 5,937,672 nodes, 4,951,530 solved,
   733,645 open and 252,497 partial; root partial, best 50,105. No global
   optimality certificate exists.
3. Pending: next fixed-pair queue: Petrinov Hand/Midas, Bajaar Necklace/Midas,
   Eduming Statuette/Midas and Eduming Route/Midas, using the established
   bounded profile. The refreshed historical audit has 13 remaining entries;
   it does not enumerate every possible Witch choice. Gayawaku
   Necklace/Bag/Statuette remain distinct memory-sensitive work.
4. Pending if a current town record is exceeded: strict/direct replay,
   consistent promotion, safe build and bounded focused/full tests,
   followed by a fresh residual audit. No candidate exists yet.

This phase completed nine bounded searches: one time-stopped mapped run
with 310,454 new unique nodes and 254,788 newly solved nodes, tying
50,105, plus eight untrimmed fixed pairs below their town records. All
required metrics and full paths are saved. Current saved thresholds were
verified for every searched town. No native source changed. Metadata is
terminal/completed for Sanctifan Hand/Midas and no native solver remains
running. No additional global record has been found; the goal remains
active and unachieved.

## Continuation-heavy 16,000-state Gayawaku frontier

Previous goal turn made progress: nine bounded searches, retained graph
nodes and eight completed untrimmed fixed families. Current preflight
confirms terminal Sanctifan metadata, no native solver, Gayawaku partial
graph with 5,937,672 nodes and 4,951,530 solved. The o0/c80 frontier
16,000 lane is unrecorded. Launch under 384/288 MB limits, 300,000 atomic
cap and 540 seconds; preserve graph and record full metrics and path.
Then run Petrinov Hand/Midas, Bajaar Necklace/Midas, Eduming
Statuette/Midas and Eduming Route/Midas sequentially with the established
fixed-pair profile. Stop on failure or a global candidate.

Gayawaku o0/c80 frontier 16,000 returned 0, score 50,105 (tie),
72,448 processed / 324,746 generated,
248,045 atomic expansions / 26,687
reused children. Native/wrapper seconds 563.404/563.842;
external peak RSS 241,300 KiB. Stop: internal time budget reached after 540 seconds;
search_complete=false, root remains partial.
Graph added 298,055 unique nodes and 228,822 solved nodes. Full command,
limits and before/after counts are in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

## Four completed fixed pairs after 16,000-state continuation-heavy run

All four returned 0, search_complete=true, null stop reason, zero beam
prunes and full action paths. No town record was exceeded.

| Town / Merchant (Midas) | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| Petrinov / Hand of Midas | 30,839 / 45,578 | 12,265 / 30,543 | 1.051 / 1.072 | 45,136 |
| Bajaar / Leaders Necklace | 39,939 / 56,147 | 64,714 / 148,168 | 9.281 / 9.299 | 75,748 |
| Eduming / Wooden Statuette | 62,467 / 81,321 | 51,810 / 117,028 | 4.501 / 4.519 | 64,180 |
| Eduming / Route to Mahaji | 57,967 / 81,321 | 51,947 / 118,901 | 4.654 / 4.671 | 61,420 |

Petrinov Hand improved its historical pair score by 1,062 to 30,839,
but remains below its 45,578 global record; no promotion. Full commands
and routes are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Petrinov / Hand of Midas: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Trader → Buy [13, 0, 0, 0, 0, 0] → Merch - Hand of Midas → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [18, 8, 0, 0, 0, 2] → Sell → Buy [18, 0, 0, 0, 0, 7] → Sell → Interest

Bajaar / Leaders Necklace: Trader → Buy [10, 0, 0, 0, 0, 0] → Trader → Camel → Merch - Leaders Necklace → Buy [30, 0, 0, 0, 0, 0] → Camel → Camel → Buy [7, 9, 0, 0, 1, 0] → Witch - Midas was a Trader → Sell → Buy [16, 1, 12, 0, 0, 2] → Sell → Buy [24, 0, 0, 0, 0, 6] → Sell → Interest → Interest

Eduming / Wooden Statuette: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [23, 0, 0, 0, 0, 0] → Merch - Wooden Statuette → Trader → Trader → Trader → Witch - Midas was a Trader → Buy [26, 0, 0, 0, 0, 0] → Trader → Interest → Buy [27, 0, 0, 0, 0, 0] → Interest → Interest → Interest

Eduming / Route to Mahaji: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [23, 0, 0, 0, 0, 0] → Merch - Route to Mahaji → Trader → Trader → Trader → Witch - Midas was a Trader → Buy [26, 0, 0, 0, 0, 0] → Trader → Interest → Buy [27, 0, 0, 0, 0, 0] → Interest → Interest → Interest

Additional sequential queue after residual audit: Eduming Hand/Midas
under the established bounded fixed-pair profile, then Gayawaku
Necklace/Midas, broad20,000 / split8 / batch10 / suffix1,500, pure
optimistic k0/c0/o100, 540 seconds and existing 512/384 MB limits.
Matching lanes are absent. The Gayawaku choice uses a profile already
exercised on its Magic family and explores a previously untested forced
Merchant/Witch family; preserve the independent persistent graph. Stop
on failure or a global candidate.

Eduming Hand/Midas completed with score 57,610 below global 81,321.
The next Python invocation rejected the misspelled `--continuation-share`
flag before any Gayawaku solver started. Supervisor exited 2, metadata
remains terminal Eduming; this is an invocation diagnostic, not a native
search result. Correct to `--continuation-lane-share 0` and launch only
the still-unexecuted Gayawaku Necklace lane, preserving Eduming result.

## Eduming Hand and Gayawaku Necklace results

Both returned 0 with full routes and search_complete=true, null stop reason.

| Town / Merchant (Midas) | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB | Beam prunes |
| --- | ---: | ---: | ---: | ---: | ---: |
| Eduming / Hand of Midas | 57,610 / 81,321 | 53,751 / 120,051 | 5.180 / 5.199 | 65,248 | 0 |
| Gayawaku / Leaders Necklace | 42,306 / 50,105 | 774,425 / 2,597,348 | 533.337 / 533.357 | 76,332 | 605,565 |

Gayawaku Necklace processed all 109 batches across
1,086 frontier states; its 605,565 beam prunes mean
this is completion of retained work, not exhaustive family coverage.
The score 42,306 remains below global 50,105. No promotion is made.
All non-Gayawaku families in the historical audit now have completed
untrimmed results; this does not certify every Witch choice or a global
optimum. Remaining eight historical entries are Gayawaku families.
Full commands, limits and paths are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Eduming / Hand of Midas: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [23, 0, 0, 0, 0, 0] → Trader → Merch - Hand of Midas → Trader → Trader → Witch - Midas was a Trader → Buy [26, 0, 0, 0, 0, 0] → Trader → Interest → Buy [27, 0, 0, 0, 0, 0] → Interest → Interest → Interest

Gayawaku / Leaders Necklace: Trader → Buy [11, 0, 0, 0, 0, 0] → Trader → Trader → Buy [47, 0, 0, 0, 0, 0] → Merch - Leaders Necklace → Trader → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [11, 9, 0, 0, 1, 1] → Sell → Buy [30, 0, 2, 0, 1, 6] → Sell → Interest → Interest

Residual sweep and next plan:

1. Pending: Gayawaku persistent balanced o50/c20, frontier 16,000,
   300,000 atomic cap and 540 seconds under 384/288 MB limits. Matching
   lane is absent. Revalidate partial graph and terminal metadata; preserve
   graph, run sequentially and record all required metrics.
2. Pending: full route, score, RSS, counts, stop/completion and unique/solved
   graph increments. Current graph has 6,235,727 nodes, 5,180,352 solved,
   783,655 open and 271,720 partial; root partial, best 50,105. No global
   optimality certificate exists.
3. Pending: Gayawaku Bag/Midas and Statuette/Midas, each using the newly
   exercised Necklace profile: broad20,000 / split8 / batch10 / suffix1,500,
   k0/c0/o100, 540 seconds, 512/384 MB limits. These are independent
   untested families, not timeout-only repeats. The remaining historical
   audit contains eight Gayawaku families; broader Witch coverage remains
   outside that historical inventory.
4. Pending if a current town record is exceeded: strict/direct replay,
   consistent promotion, safe build and bounded focused/full tests,
   followed by a fresh residual audit. No candidate exists yet.

This phase completed seven bounded native searches: one time-stopped
mapped run with 298,055 new unique nodes and 228,822 newly solved nodes,
tying 50,105; five untrimmed fixed pairs below town records; and Gayawaku
Necklace/Midas, completed with 605,565 beam prunes and score 42,306.
The misspelled Python option diagnostic launched no Gayawaku solver and
is not counted as a native search. All required metrics and full paths
are saved. No native source changed. Metadata is terminal/completed for
Gayawaku Necklace/Midas and no native solver remains running. No
additional global record has been found; the goal remains active and
unachieved.

## Balanced 16,000-state Gayawaku frontier

Previous goal turn made progress: seven bounded native searches, retained
graph nodes, five untrimmed fixed families and a completed pruned
Necklace family. Current preflight confirms terminal Gayawaku metadata,
no native solver, partial graph with 6,235,727 nodes and 5,180,352 solved.
The o50/c20 frontier 16,000 lane is unrecorded. Launch under 384/288 MB
limits, 300,000 atomic cap and 540 seconds; preserve graph and record
full metrics and path. Then run Gayawaku Bag/Midas and Statuette/Midas
sequentially under the Necklace profile: broad20,000 / split8 / batch10 /
suffix1,500, k0/c0/o100, 540 seconds, 512/384 MB limits. Stop on failure
or a global candidate.

Gayawaku o50/c20 frontier 16,000 returned 0, score 50,105 (tie),
73,785 processed / 308,369 generated,
283,667 atomic expansions / 36,549
reused children. Native/wrapper seconds 569.468/569.914;
external peak RSS 242,512 KiB. Stop: persistent search session time budget reached;
search_complete=false, root remains partial.
Graph added 271,820 unique nodes and 293,720 solved nodes. Full command,
limits and before/after counts are in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Runner coverage gap found while Bag/Midas runs: native `consts/enums.h`
contains eight Witch choices, but the fixed-pair Python runner exposed
only four. Added exact native names Camelization, Present's Gift, Animal
Lover and The Stonecutter to the existing label map, with distinct lane
slugs. Defaults, caps and native game rules are unchanged. Python
compilation and `--help` pass and list all eight choices. No C++ file
changed. New choices will be exercised by sequential bounded searches
after the active Bag/Statuette queue completes; no overlapping solver.

Gayawaku Bag/Midas returned 0, score 42,172 vs global 50,105;
639,706 processed / 2,147,449 generated.
Native/wrapper seconds 386.957/386.983;
external peak RSS 87,712 KiB. search_complete=true,
stop reason None; 89 batches /
889 frontier states, 483,536 beam prunes.
This completes retained work, not exhaustive family coverage. Full command
and route are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Reproducible route: Trader → Buy [10, 0, 0, 0, 0, 0] → Trader → Trader → Buy [48, 0, 0, 0, 0, 0] → Merch - Canvas Bag → Trader → Trader → Trader → Witch - Midas was a Trader → Camel → Buy [5, 6, 0, 0, 1, 1] → Sell → Buy [27, 0, 0, 0, 1, 6] → Sell → Interest → Interest

Gayawaku Statuette/Midas returned 0, score 42,105 vs global
50,105; 513,081 processed / 1,723,424 generated.
Native/wrapper seconds 277.713/277.731;
external peak RSS 71,628 KiB. search_complete=true,
stop reason None; 72 batches /
714 frontier states, 383,777 beam prunes.
This completes retained work, not exhaustive family coverage. Full command
and route are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Reproducible route: Trader → Buy [10, 0, 0, 0, 0, 0] → Trader → Trader → Buy [48, 0, 0, 0, 0, 0] → Merch - Wooden Statuette → Trader → Trader → Trader → Witch - Midas was a Trader → Camel → Buy [5, 10, 0, 0, 1, 1] → Sell → Buy [27, 0, 0, 0, 0, 5] → Sell → Interest → Interest

Pending sequential validation/search queue for newly exposed Witch choices:
current-model Fang-Mo Magic/Camelization, Magic/Present's Gift,
Magic/Animal Lover and Magic/The Stonecutter, using broad/suffix32,000,
split12, batch1,000, k30/c20/o50, 540 seconds and 512/384 MB limits.
Matching lanes are absent. Stop on failure or a global candidate.

## Four newly exposed Witch choices exercised

All four current-model Fang-Mo Magic runs returned 0, reported the
requested Witch, search_complete=true, null stop reason, zero beam
prunes and full routes. Each is below the saved town record of 37,569.
This also verifies the new Python choices reach the native solver.

| Witch | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| Camelization | 21,916 / 37,569 | 155,155 / 290,102 | 21.763 / 21.783 | 70,904 |
| Present's Gift | 21,465 / 37,569 | 165,543 / 347,449 | 41.333 / 41.351 | 90,864 |
| Animal Lover | 20,564 / 37,569 | 113,039 / 219,684 | 8.726 / 8.743 | 60,308 |
| The Stonecutter | 19,873 / 37,569 | 182,946 / 377,071 | 42.426 / 42.445 | 94,392 |

No promotion. Full native commands, caps and routes are in
`results/more-records-20261009/pair-coverage-summary.jsonl`.

Fang-Mo Magic / Camelization: Trader → Buy [8, 0, 1, 0, 0, 0] → Sell → Camel → Buy [13, 1, 0, 1, 0, 0] → Sell → Merch - Magic Cleppsydra → Camel → Buy [15, 3, 1, 2, 0, 0] → Sell → Interest → Witch - Camelization → Buy [16, 11, 2, 1, 0, 1] → Sell → Buy [16, 0, 2, 2, 0, 3] → Sell → Buy [8, 0, 0, 0, 1, 8] → Sell

Fang-Mo Magic / Present's Gift: Trader → Buy [8, 0, 1, 0, 0, 0] → Sell → Camel → Buy [13, 1, 0, 1, 0, 0] → Sell → Merch - Magic Cleppsydra → Camel → Buy [15, 3, 1, 2, 0, 0] → Sell → Interest → Witch - Present's Gift → Buy [10, 1, 0, 2, 0, 1] → Sell → Buy [10, 1, 2, 2, 0, 3] → Sell → Buy [5, 1, 0, 0, 0, 8] → Sell

Fang-Mo Magic / Animal Lover: Sell → Camel → Buy [6, 11, 0, 0, 0, 0] → Sell → Buy [6, 4, 0, 1, 0, 0] → Sell → Merch - Magic Cleppsydra → Buy [9, 0, 0, 2, 0, 0] → Sell → Interest → Witch - Animal Lover → Buy [10, 20, 0, 0, 0, 1] → Sell → Buy [10, 2, 0, 0, 0, 3] → Sell → Buy [5, 1, 2, 0, 0, 8] → Sell

Fang-Mo Magic / The Stonecutter: Sell → Camel → Buy [6, 11, 0, 0, 0, 0] → Sell → Buy [6, 4, 0, 1, 0, 0] → Sell → Merch - Magic Cleppsydra → Buy [9, 0, 0, 2, 0, 0] → Sell → Interest → Buy [6, 1, 0, 1, 0, 1] → Witch - The Stonecutter → Sell → Buy [6, 0, 2, 0, 0, 3] → Sell → Buy [6, 0, 0, 0, 0, 4] → Sell → Interest

Residual sweep and next plan:

1. Pending: widen Gayawaku persistent frontier to 18,000, pure optimistic
   o100/c0, 300,000 atomic cap and 540 seconds under existing 384/288 MB
   limits. Matching lane is absent; all three 16,000 mixes are recorded.
   Revalidate partial graph and terminal metadata, preserve graph and
   run sequentially; save all required metrics.
2. Pending: full path, score, RSS, counts, stop/completion and unique/solved
   increments. Current graph has 6,507,547 nodes, 5,474,072 solved,
   771,808 open and 261,667 partial; root partial, best 50,105. No global
   optimality certificate exists.
3. Pending: current-model Fang-Mo Magic/Time, Magic/Oasis and Magic/Patience
   under the established 32,000-width bounded profile. These are the three
   untested Witch choices for its Magic family. Then Gayawaku Saddle/Midas
   and Route/Camelization under the exercised broad20,000 / split8 /
   batch10 / suffix1,500 / k0/c0/o100, 540-second, 512/384 MB profile.
   Each is an independent previously untested family, not a timeout-only
   repeat. The historical audit remains eight Gayawaku entries because
   completed pruned runs are not untrimmed exhaustive family coverage.
4. Pending if a current town record is exceeded: strict/direct replay,
   consistent promotion, safe build and bounded focused/full tests,
   followed by a fresh residual audit. No candidate exists yet.

This phase completed seven bounded searches: one time-stopped mapped run
with 271,820 new unique nodes and 293,720 newly solved nodes, tying
50,105; two Gayawaku families completed their pruned retained work below
the global record; four current-model Fang-Mo Witch alternatives completed
untrimmed below the global record. All metrics and full paths are saved.
Current thresholds were verified for both searched towns. The Python
runner now exposes all eight native Witch choices; compilation/help and
four actual bounded native runs validate the additions. No C++ source
changed. Metadata is terminal/completed for Fang-Mo Magic/Stonecutter and
no native solver remains running. No additional global record has been
found; the goal remains active and unachieved.

## Optimistic 18,000-state Gayawaku frontier

Previous goal turn made progress: seven bounded searches, retained graph
nodes, two completed pruned Gayawaku families and four untrimmed Fang-Mo
Witch alternatives; the Python runner now exposes all native Witch choices.
Current preflight confirms terminal metadata, no native solver and partial
Gayawaku graph with 6,507,547 nodes / 5,474,072 solved. The o100/c0
frontier 18,000 lane is unrecorded. Launch under 384/288 MB limits,
300,000 atomic cap and 540 seconds; preserve graph and record all metrics
and path. Then run the three remaining Fang-Mo Magic Witch choices
(Time/Oasis/Patience) and the pending Gayawaku families sequentially.
Stop on failure or a global candidate.

Gayawaku o100/c0 frontier 18,000 returned 0, score 50,105 (tie),
87,142 processed / 366,977 generated,
276,980 atomic expansions / 46,165
reused children. Native/wrapper seconds 579.542/579.948;
external peak RSS 256,552 KiB. Stop: persistent search session time budget reached;
search_complete=false, root remains partial.
Graph added 320,812 unique nodes and 259,680 solved nodes. Full command,
limits and before/after counts are in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Sequential pending fixed queue: Fang-Mo Magic/Time, Magic/Oasis,
Magic/Patience with broad/suffix32,000, split12, batch1,000, k30/c20/o50;
then Gayawaku Saddle/Midas and Route/Camelization with broad20,000,
split8, batch10, suffix1,500, k0/c0/o100. Each gets 540 seconds and
512/384 MB limits. Matching lanes were checked absent. Stop on failure
or a global candidate; record full paths and all required metrics.

## Fang-Mo Magic covers all eight native Witch choices

The remaining Time/Oasis/Patience runs returned 0, search_complete=true,
null stop reason, zero beam prunes and full paths. No town record was
exceeded. Together with prior current-model results, Magic now has a
completed untrimmed run for each of the eight Witch choices. This does
not certify all Merchant/Witch families or global optimality.

| Witch | Score / global | Processed / generated | Native / wrapper seconds | Peak RSS KiB |
| --- | ---: | ---: | ---: | ---: |
| Time is Money | 18,372 / 37,569 | 217,359 / 473,117 | 88.071 / 88.094 | 113,432 |
| Oasis of Sanctifan | 27,972 / 37,569 | 458,344 / 1,293,881 | 186.604 / 186.627 | 107,112 |
| Vertue of Patience | 21,086 / 37,569 | 226,381 / 451,773 | 53.876 / 53.900 | 96,644 |

Full commands, limits and paths are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Fang-Mo Magic / Time is Money: Sell → Camel → Buy [6, 11, 0, 0, 0, 0] → Sell → Buy [6, 4, 0, 1, 0, 0] → Sell → Merch - Magic Cleppsydra → Buy [6, 0, 1, 2, 0, 0] → Sell → Witch - Time is Money → Buy [6, 3, 1, 1, 1, 0] → Sell → Buy [6, 0, 5, 0, 1, 1] → Sell → Buy [12, 0, 0, 0, 0, 3] → Sell → Interest → Interest → Interest

Fang-Mo Magic / Oasis of Sanctifan: Trader → Buy [8, 0, 1, 0, 0, 0] → Sell → Camel → Buy [8, 4, 0, 1, 0, 0] → Sell → Merch - Magic Cleppsydra → Buy [8, 1, 0, 2, 0, 0] → Sell → Buy [4, 0, 0, 2, 1, 0] → Witch - Oasis of Sanctifan → Sell → Buy [0, 6, 0, 0, 0, 2] → Sell → Buy [0, 0, 0, 0, 0, 5] → Sell → Interest → Interest

Fang-Mo Magic / Vertue of Patience: Sell → Camel → Buy [6, 11, 0, 0, 0, 0] → Sell → Buy [6, 4, 0, 1, 0, 0] → Sell → Merch - Magic Cleppsydra → Buy [9, 0, 0, 2, 0, 0] → Sell → Witch - Vertue of Patience → Interest → Buy [6, 3, 0, 0, 0, 1] → Sell → Buy [12, 0, 0, 0, 1, 2] → Sell → Interest → Interest → Interest

Gayawaku Saddle/Midas returned 0, score 41,103 vs global
50,105; 562,153 processed / 1,857,679 generated.
Native/wrapper seconds 308.873/308.892;
external peak RSS 79,604 KiB. search_complete=true,
stop reason None; 80 batches /
795 frontier states, 409,043 beam prunes.
This completes retained work, not exhaustive family coverage. Historical
Saddle family +153 remains below the global record; no promotion.
Full command and route are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Reproducible route: Trader → Buy [15, 0, 0, 0, 0, 0] → Trader → Trader → Buy [39, 0, 0, 0, 0, 0] → Trader → Merch - Sturdy Saddle → Trader → Camel → Witch - Midas was a Trader → Camel → Interest → Buy [18, 26, 0, 0, 0, 3] → Sell → Buy [18, 0, 4, 0, 1, 10] → Sell → Interest

Gayawaku Route/Camelization returned 0, score 38,979 vs global
50,105; 563,152 processed / 1,937,455 generated.
Native/wrapper seconds 394.195/394.214;
external peak RSS 84,508 KiB. search_complete=true,
stop reason None; 73 batches /
727 frontier states, 525,689 beam prunes.
This completes retained work, not exhaustive family coverage. No promotion.
Full command and route are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Camel → Buy [0, 11, 0, 0, 0, 0] → Merch - Route to Mahaji → Sell → Buy [8, 0, 0, 3, 0, 0] → Sell → Buy [8, 12, 0, 0, 0, 1] → Sell → Witch - Camelization → Buy [12, 1, 0, 2, 0, 3] → Sell → Buy [18, 0, 0, 0, 1, 6] → Sell → Interest → Interest

Residual sweep and next plan:

1. Pending: Gayawaku persistent continuation-heavy o0/c80, frontier 18,000,
   300,000 atomic cap and 540 seconds under 384/288 MB limits. Matching
   lane is absent. Revalidate partial graph and terminal metadata, preserve
   graph and run sequentially; record all required metrics.
2. Pending: full route, score, RSS, counts, stop/completion and unique/solved
   graph increments. Current graph has 6,828,359 nodes, 5,733,752 solved,
   815,640 open and 278,967 partial; root partial, best 50,105. No global
   optimality certificate exists.
3. Pending: Gayawaku Hand/Midas, Magic/Oasis and Magic/Time under the
   exercised broad20,000 / split8 / batch10 / suffix1,500 / k0/c0/o100,
   540-second, 512/384 MB profile; matching lanes are absent. This covers
   distinct forced families. Fang-Mo Magic now has untrimmed completed
   runs for all eight Witch choices; its other Merchant/Witch combinations
   remain separate work. The historical audit remains eight Gayawaku
   entries because completed pruned runs are not exhaustive family coverage.
4. Pending if a current town record is exceeded: strict/direct replay,
   consistent promotion, safe build and bounded focused/full tests,
   followed by a fresh residual audit. No candidate exists yet.

This phase completed six bounded searches: one time-stopped mapped run
with 320,812 new unique nodes and 259,680 newly solved nodes, tying
50,105; three untrimmed Fang-Mo Witch alternatives below its town record;
and two Gayawaku families completed their pruned retained work below its
town record. All required metrics and full paths are saved. No native
source changed. Metadata is terminal/completed for Gayawaku
Route/Camelization and no native solver remains running. No additional
global record has been found; the goal remains active and unachieved.

## Continuation-heavy 18,000-state Gayawaku frontier

Previous goal turn made progress: six bounded searches, retained graph
nodes, three untrimmed Fang-Mo Witch alternatives and two completed
pruned Gayawaku families. Current preflight confirms terminal metadata,
no native solver, partial graph with 6,828,359 nodes and 5,733,752 solved.
The o0/c80 frontier 18,000 lane is unrecorded. Launch under 384/288 MB
limits, 300,000 atomic cap and 540 seconds; preserve graph and record
full metrics and path. Then run Gayawaku Hand/Midas, Magic/Oasis and
Magic/Time sequentially under broad20,000 / split8 / batch10 /
suffix1,500, k0/c0/o100, 540 seconds and 512/384 MB limits.
Stop on failure or a global candidate.

Gayawaku o0/c80 frontier 18,000 returned 0, score 50,105 (tie),
78,160 processed / 343,446 generated,
272,595 atomic expansions / 29,968
reused children. Native/wrapper seconds 579.269/579.729;
external peak RSS 254,220 KiB. Stop: persistent search session time budget reached;
search_complete=false, root remains partial.
Graph added 313,478 unique nodes and 259,518 solved nodes. Full command,
limits and before/after counts are in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Pending sequential fixed queue: Gayawaku Hand/Midas, Magic/Oasis and
Magic/Time, each with broad20,000 / split8 / batch10 / suffix1,500,
k0/c0/o100, 540 seconds and 512/384 MB limits. Matching lanes were
checked absent; stop on failure or a global candidate.

Gayawaku Hand/Midas returned 0, score 37,930 vs global
50,105; 506,531 processed / 1,694,606 generated.
Native/wrapper seconds 314.266/314.291;
external peak RSS 79,796 KiB. search_complete=true,
stop reason None; 72 batches /
714 frontier states, 388,608 beam prunes.
This completes retained work, not exhaustive family coverage. Historical
Hand family +84 remains below the global record; no promotion.
Full command and route are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Reproducible route: Trader → Buy [10, 0, 0, 0, 0, 0] → Trader → Trader → Buy [48, 0, 0, 0, 0, 0] → Merch - Hand of Midas → Trader → Trader → Trader → Witch - Midas was a Trader → Camel → Buy [5, 6, 0, 0, 1, 1] → Sell → Buy [27, 0, 0, 0, 0, 5] → Sell → Interest → Interest

Gayawaku Magic/Oasis returned 0, score 24,546 vs global
50,105; 608,736 processed / 2,307,216 generated.
Native/wrapper seconds 117.305/117.325;
external peak RSS 83,560 KiB. search_complete=true,
stop reason None; 67 batches /
661 frontier states, 321,333 beam prunes.
This completes retained work, not exhaustive family coverage. No promotion.
Full command and route are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Reproducible route: Trader → Buy [9, 3, 0, 0, 0, 0] → Trader → Sell → Buy [13, 2, 0, 1, 0, 0] → Sell → Merch - Magic Cleppsydra → Camel → Buy [5, 0, 0, 3, 0, 0] → Witch - Oasis of Sanctifan → Sell → Buy [0, 7, 0, 1, 0, 1] → Sell → Buy [0, 1, 0, 1, 0, 3] → Sell → Buy [0, 0, 0, 0, 0, 5] → Sell → Interest

Gayawaku Magic/Time returned 0, score 23,301 vs global
50,105; 642,949 processed / 2,314,734 generated.
Native/wrapper seconds 438.146/438.164;
external peak RSS 73,400 KiB. search_complete=true,
stop reason None; 67 batches /
661 frontier states, 636,336 beam prunes.
This completes retained work, not exhaustive family coverage. No promotion.
Full command and route are in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Reproducible route: Trader → Buy [4, 6, 0, 0, 0, 0] → Sell → Buy [10, 0, 0, 1, 0, 0] → Sell → Merch - Magic Cleppsydra → Camel → Buy [8, 1, 2, 2, 0, 0] → Sell → Buy [8, 3, 4, 1, 1, 0] → Sell → Witch - Time is Money → Buy [8, 0, 0, 2, 1, 1] → Sell → Buy [16, 0, 1, 0, 2, 3] → Sell → Interest → Interest → Interest

Residual sweep and next plan:

1. Pending: Gayawaku persistent balanced o50/c20, frontier 18,000,
   300,000 atomic cap and 540 seconds under 384/288 MB limits. Matching
   lane is absent. Revalidate partial graph and terminal metadata, preserve
   graph and run sequentially; record all required metrics.
2. Pending: full path, score, RSS, counts, stop/completion and unique/solved
   graph increments. Current graph has 7,141,837 nodes, 5,993,270 solved,
   856,523 open and 292,044 partial; root partial, best 50,105. No global
   optimality certificate exists.
3. Pending: Gayawaku Magic/Patience, Magic/Camelization and Magic/Present's
   Gift, then Magic/Animal Lover and Magic/Stonecutter, under broad20,000 /
   split8 / batch10 / suffix1,500 / k0/c0/o100, 540 seconds, 512/384 MB
   limits. Matching lanes are absent. These cover distinct Witch choices;
   completed pruned families require retention/ranking changes for any
   later repeat, not longer timeout alone. Historical audit remains eight
   Gayawaku entries because its family completions are pruned.
4. Pending if a current town record is exceeded: strict/direct replay,
   consistent promotion, safe build and bounded focused/full tests,
   followed by a fresh residual audit. No candidate exists yet.

This phase completed four bounded searches: one time-stopped mapped run
with 313,478 new unique nodes and 259,518 newly solved nodes, tying
50,105; three Gayawaku families completed their pruned retained work at
37,930 (Hand/Midas), 24,546 (Magic/Oasis) and 23,301 (Magic/Time), all
below the current town record. All required metrics and full paths are
saved. No native source changed. Metadata is terminal/completed for
Gayawaku Magic/Time and no native solver remains running. No additional
global record has been found; the goal remains active and unachieved.

## Gayawaku balanced 18,000 and remaining Magic Witch choices

Previous count-only turn made no search progress. Revalidated terminal/completed
metadata, no native solver, 628 GiB free and absent balanced18k lane.

1. In progress: mapped Gayawaku o50/c20, frontier18,000, atomic cap300,000,
   540-second budget, external/internal384/288 MB, preserving partial graph.
2. Pending: record score, processed/generated, external peak RSS, elapsed,
   stop/completion, full path and graph increments after exit.
3. Pending: Magic/Patience, Camelization, Present's Gift, Animal Lover and
   Stonecutter sequentially, broad20,000/split8/batch10/suffix1,500,
   k0/c0/o100, 540 seconds and 512/384 MB. Stop on failure or global candidate.
4. Pending: verify/promote any global winner with strict/direct replay and
   safe build/tests; otherwise retain unresolved coverage in a fresh plan.

Balanced18k completed with returncode 0; score 50105 ties 50,105. Processed 73,482; generated 312,893; native/wrapper elapsed 579.426/580.418 seconds; external peak RSS 250,216 KiB. search_complete=False; stop=persistent search session time budget reached. Added 281,481 unique nodes and 298,672 solved nodes. Root remains partial.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Full argv, limits and graph counts: `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Next in progress: the five sequential Magic/Witch lanes listed above.

Magic/Patience: returncode 0, score 23,753 <50,105, processed/generated 587,931/2,108,317, native/wrapper elapsed 228.107/228.135 seconds, external peak RSS 82,056 KiB. search_complete=True, stop=None, batches/frontier 67/661, beam prunes 522,877. Retained work complete; pruned coverage is not exhaustive. Full argv/limits in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Reproducible route: Trader → Buy [4, 6, 0, 0, 0, 0] → Sell → Buy [10, 0, 0, 1, 0, 0] → Sell → Merch - Magic Cleppsydra → Camel → Buy [12, 3, 1, 2, 0, 0] → Sell → Witch - Vertue of Patience → Interest → Buy [12, 2, 0, 1, 0, 1] → Sell → Interest → Buy [12, 0, 2, 0, 0, 4] → Sell → Interest → Interest

Magic/Camelization: returncode 0, score 27,249 <50,105, processed/generated 588,109/2,057,789, native/wrapper elapsed 343.965/343.984 seconds, external peak RSS 67,512 KiB. search_complete=True, stop=None, batches/frontier 67/661, beam prunes 538,097. Retained work complete; pruned coverage is not exhaustive. Full argv/limits in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Reproducible route: Trader → Buy [9, 3, 0, 0, 0, 0] → Trader → Sell → Buy [13, 2, 0, 1, 0, 0] → Sell → Merch - Magic Cleppsydra → Camel → Buy [10, 0, 0, 3, 0, 0] → Sell → Witch - Camelization → Buy [14, 15, 1, 0, 0, 1] → Sell → Buy [14, 11, 0, 0, 0, 3] → Sell → Buy [14, 0, 0, 0, 1, 6] → Sell → Interest

Magic/Present's Gift: returncode 0, score 24,252 <50,105, processed/generated 550,431/1,909,290, native/wrapper elapsed 231.645/231.662 seconds, external peak RSS 64,876 KiB. search_complete=True, stop=None, batches/frontier 67/661, beam prunes 490,970. Retained work complete; pruned coverage is not exhaustive. Full argv/limits in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Reproducible route: Trader → Buy [4, 0, 0, 0, 0, 0] → Interest → Buy [10, 2, 0, 1, 0, 0] → Sell → Merch - Magic Cleppsydra → Camel → Buy [8, 1, 3, 2, 0, 0] → Sell → Buy [8, 3, 0, 2, 1, 0] → Sell → Witch - Present's Gift → Buy [8, 7, 0, 0, 0, 2] → Sell → Buy [8, 0, 2, 0, 1, 5] → Sell → Buy [4, 0, 0, 0, 1, 6] → Sell

Magic/Animal Lover: returncode 0, score 24,059 <50,105, processed/generated 490,326/1,644,712, native/wrapper elapsed 137.878/137.896 seconds, external peak RSS 67,616 KiB. search_complete=True, stop=None, batches/frontier 67/661, beam prunes 350,971. Retained work complete; pruned coverage is not exhaustive. Full argv/limits in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Reproducible route: Trader → Buy [4, 6, 0, 0, 0, 0] → Sell → Buy [10, 0, 0, 1, 0, 0] → Sell → Merch - Magic Cleppsydra → Camel → Buy [12, 3, 1, 2, 0, 0] → Sell → Interest → Buy [10, 1, 0, 2, 0, 1] → Witch - Animal Lover → Sell → Buy [12, 1, 0, 4, 0, 3] → Sell → Buy [6, 1, 0, 0, 1, 9] → Sell

Magic/Stonecutter: returncode 0, score 24,220 <50,105, processed/generated 570,046/2,037,064, native/wrapper elapsed 207.271/207.291 seconds, external peak RSS 81,488 KiB. search_complete=True, stop=None, batches/frontier 67/661, beam prunes 507,601. Retained work complete; pruned coverage is not exhaustive. Full argv/limits in `results/more-records-20261009/pair-coverage-summary.jsonl`.

Reproducible route: Trader → Buy [4, 0, 0, 0, 0, 0] → Interest → Buy [10, 2, 0, 1, 0, 0] → Sell → Merch - Magic Cleppsydra → Camel → Buy [8, 1, 3, 2, 0, 0] → Sell → Buy [8, 3, 0, 2, 1, 0] → Sell → Witch - The Stonecutter → Buy [8, 7, 0, 0, 0, 2] → Sell → Buy [12, 0, 0, 0, 0, 5] → Sell → Interest → Interest

Residual sweep and next plan:

1. Completed preflight: Eduming mapped graph is partial: 1,801,608 nodes,
   1,521,136 solved, 201,608 open and 78,864 partial; best81,321.
   Untried o100/c0 b8,000 t540 confirmed absent; prior6,000-state lanes tie.
2. In progress next: Eduming o100/c0 frontier8,000, atomic cap300,000,
   540 seconds under 384/288 MB; preserve graph and record full path, score,
   counts, elapsed, RSS, stop/completion and unique/solved increments.
3. Pending: Eduming c80 and balanced8,000 profiles, after absence checks.
   All Gayawaku Magic Witch choices now have completed retained runs,
   but beam pruning prevents exhaustive-family claims. Its mapped root
   also remains partial; historical eight-family audit remains unresolved.
4. Pending on global candidate: strict/direct replay, promotion and safe
   build/focused/full tests. No candidate exists yet.

Six bounded Gayawaku searches completed without a global improvement.
   Balanced18k added281,481 unique nodes and298,672 solved; five Magic
   Witch families completed pruned retained work below50,105. No native
   code changed. Goal remains active, unachieved.

Eduming optimistic8k returned 0; score 81,321 ties current Computer maximum81,321. Processed/generated 103,610/383,870; native/wrapper elapsed 357.007/357.146 seconds; external peak RSS 184,060 KiB. search_complete=False; stop=None. Added 337,742 unique nodes and 278,804 solved nodes; root=partial. Full argv, limits and graph counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Residual sweep / next execution plan after seven bounded searches:

1. In progress: revalidate Eduming partial graph, terminal metadata, no
   live native process, and absence of continuation80/b8,000 and
   balanced50/20/b8,000 profiles.
2. Pending: run Eduming continuation80/b8,000 then balanced50/20/b8,000
   sequentially, 300,000 atomic cap, 540-second budget, 384/288 MB.
   Record full routes, scores, processed/generated, peak RSS, elapsed,
   stop/completion and graph increments. Stop on failure or global win.
3. Pending: wider Gayawaku frontier20,000 ranking mixes after absence
   checks; root remains partial. Its completed Magic Witch families are
   pruned and do not prove global optimality.
4. Pending on candidate: strict/direct replay, consistent promotion and
   safe build/focused/full tests. None exists.

Seven bounded searches added619,223 unique graph nodes and577,476
solved nodes across Gayawaku and Eduming. Five new Gayawaku Witch
families finished pruned retained work below50,105. Both mapped runs
tied current Computer records. No new global record; native source
unchanged; goal active and unachieved.

Next-plan item1 completed: terminal/completed metadata and no native
solver confirmed; both Eduming8k mixes absent including logs. Current
graph: {"nodes": 2139350, "open": 239350, "partial": 100060, "solved": 1799940, "root_status": "partial", "best_score": 81321, "database_bytes": 1242062848}. Item2 remains pending.

## Eduming remaining8k mixes and wider Gayawaku coverage

Previous goal turn made progress: seven bounded searches and saved paths/metrics.
Current preflight confirms terminal metadata, no solver and absent two Eduming
profiles with626.5 GiB free.

1. In progress: Eduming o0/c80 frontier8,000, t540, atomic cap300,000,
   384/288 MB. Preserve graph and save all required metrics and path.
2. Pending: Eduming o50/c20 frontier8,000 under the same limits, after
   successful terminal result; stop on failure or higher town score.
3. Pending: audit untried Gayawaku20,000-state mapped mixes and start
   highest-priority remaining profile; do not count ties as records.
4. Pending on candidate: strict/direct replay, promotion, safe build and
   focused/full tests. Otherwise retain unresolved scope in residual plan.

Eduming continuation80/b8k: returncode0, score81,321 ties81,321. Processed/generated 103,048/343,531; native/wrapper elapsed 260.228/260.378 seconds; external peak RSS 170,672 KiB. search_complete=False; stop=None; new unique/solved 318,779/298,665; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Balanced8k is now in progress. All three Gayawaku20k mapped mixes were checked absent including logs.

Eduming balanced50/20/b8k: returncode0, score81,321 ties81,321. Processed/generated 85,766/321,187; native/wrapper elapsed 320.218/320.395 seconds; external peak RSS 185,244 KiB. search_complete=False; stop=None; new unique/solved 291,447/303,044; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Residual plan after Eduming8k mixes:

1. In progress: Gayawaku optimistic100/continuation0 frontier20,000,
   540 seconds and300,000 atomic cap under384/288 MB. Profile absent;
   preserved graph preflight: {"nodes": 7423318, "open": 846349, "partial": 285027, "solved": 6291942, "root_status": "partial", "best_score": 50105, "database_bytes": 7692038144}.
2. Pending: record full path, score, counts, elapsed, external RSS,
   stop/completion and graph increments.
3. Pending: Gayawaku continuation80 and balanced50/20 frontier20,000
   profiles, confirmed absent; execute sequentially after successful exits.
4. Pending: inspect Eduming10,000-state mixes for subsequent coverage;
   completed root remains partial and score81,321 has no certificate.
5. Pending on global candidate: strict/direct replay, promotion, safe
   build/focused/full tests. No candidate exists.

Gayawaku optimistic100/b20k: returncode0, score50,105 ties50,105. Processed/generated 81,119/336,939; native/wrapper elapsed 573.988/574.845 seconds; external peak RSS 270,468 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 292,397/236,783; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Saved-entry audit: all22 town sections in Best.txt checked; no unflagged
non-Computer entry exceeds its town Computer maximum. Flagged cheat entries
are excluded. No saved higher route candidate found.

Residual sweep and next plan after three mapped runs:

1. In progress: revalidate terminal metadata/no native solver; inspect
   Gayawaku partial graph and absent continuation80 and balanced50/20
   frontier20,000 lanes including logs.
2. Pending: run continuation80/b20,000 then balanced50/20/b20,000
   sequentially at540 seconds, atomic cap300,000 and384/288 MB. Preserve
   graph and save full paths plus all required metrics; stop on failure or
   global candidate. These increase retention, rather than timeout alone.
3. Pending: Eduming frontier10,000 independent mixes after absence checks.
   Its current mapped root remains partial with best81,321.
4. Pending: Gayawaku historical family audit is still eight pruned/partial
   entries; Magic Witch completed retained searches do not prove optimum.
5. Pending on candidate: strict/direct replay, consistent promotion, safe
   build/focused/full tests and fresh final residual audit. No candidate.

This phase completed three bounded mapped runs, adding902,623 unique
nodes and838,492 solved nodes. All scores tie current town records.
Full routes and required metrics saved; native source unchanged; no new
global record; goal remains active and unachieved.

Next-plan item1 completed: metadata terminal/completed, no native solver,
remaining two Gayawaku20k lanes absent including logs. Current graph:
{"nodes": 7715715, "open": 885619, "partial": 301371, "solved": 6528725, "root_status": "partial", "best_score": 50105, "database_bytes": 7940947968}. Item2 remains pending.

## Gayawaku remaining20k mixes and Eduming10k optimistic

Previous goal turn made progress: three bounded mapped searches and saved
required metrics/routes; no global candidate. Current preflight confirms
terminal metadata, no native solver, partial graphs and615.8 GiB free.
Both Gayawaku20k and all three Eduming10k profiles absent including logs.

1. In progress: Gayawaku o0/c80 frontier20,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Gayawaku o50/c20 frontier20,000 under identical limits,
   after successful terminal exit.
3. Pending: Eduming o100/c0 frontier10,000 under identical limits,
   after successful terminal exit. Stop queue on failure or candidate.
4. Pending: record all score/count/RSS/time/completion/path and graph
   evidence; investigate any higher global score with strict/direct replay,
   promotion and safe build/focused/full tests.
5. Pending residual sweep: remaining Eduming10k ranking mixes and
   wider Gayawaku retained coverage, with no optimality claim from ties.

Gayawaku continuation80/b20k: returncode0, score50,105 ties50,105. Processed/generated 85,218/375,661; native/wrapper elapsed 569.206/569.774 seconds; external peak RSS 267,228 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 338,272/246,274; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Balanced20k is now in progress.

Gayawaku balanced50/20/b20k: returncode0, score50,105 ties50,105. Processed/generated 70,645/296,486; native/wrapper elapsed 578.811/579.537 seconds; external peak RSS 264,380 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 266,485/301,398; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Eduming optimistic10k is now in progress.

Eduming optimistic100/b10k: returncode0, score81,321 ties81,321. Processed/generated 105,681/395,397; native/wrapper elapsed 383.111/383.371 seconds; external peak RSS 186,232 KiB. search_complete=False; stop=None; new unique/solved 337,931/277,858; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Residual sweep and next plan after Gayawaku20k and Eduming10k:

1. In progress: revalidate terminal/completed metadata, no solver,
   partial graph states and absent Eduming continuation80/b10,000 and
   balanced50/20/b10,000 profiles including raw logs.
2. Pending: run those two Eduming lanes sequentially at540 seconds,
   atomic cap300,000 and384/288 MB; preserve graph and record full paths,
   score/count/RSS/time/stop/completion and graph increments. Stop on
   failure or a score above the current town Computer maximum.
3. Pending: Gayawaku frontier22,000 independent mixes after absence
   checks; increasing retention supplies coverage beyond completed20k
   lanes. Its root remains partial and eight historical families pruned.
4. Pending on global candidate: strict/direct replay, promotion and safe
   build/focused/full tests; fresh completion audit. No candidate exists.

Three bounded mapped runs completed, adding942,688 unique nodes and
825,530 solved nodes. Scores tie current town records. All required
metrics and complete paths saved. Native code unchanged; no additional
global record found; goal remains active and unachieved.

Next-plan item1 completed: terminal/completed metadata, no native solver,
remaining Eduming10k lanes absent including logs. Authoritative graph
headers: {"Eduming": {"nodes": 3087507, "open": 287507, "partial": 120493, "solved": 2679507, "root_status": "partial", "best_score": 81321, "database_bytes": 1736339456}, "Gayawaku": {"nodes": 8320472, "open": 927405, "partial": 316670, "solved": 7076397, "root_status": "partial", "best_score": 50105, "database_bytes": 8628416512}}. Item2 remains pending.

## Eduming remaining10k mixes and Gayawaku22k optimistic

Previous turn made progress: three bounded searches with recorded metrics
and paths. Current state confirms terminal metadata, no native solver,
partial graphs and613.5 GiB free. Matching pending lanes absent.

1. In progress: Eduming o0/c80 frontier10,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Eduming o50/c20 frontier10,000 under identical limits,
   after successful terminal exit.
3. Pending: Gayawaku o100/c0 frontier22,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: save all required metrics, full routes and graph increments;
   strict/direct replay, promotion and bounded build/tests on any winner.
5. Pending residual sweep: Gayawaku remaining22k ranking mixes and
   further Eduming retention; no optimum inferred from partial roots.

Eduming continuation80/b10k: returncode0, score81,321 ties81,321. Processed/generated 101,396/348,338; native/wrapper elapsed 264.167/264.368 seconds; external peak RSS 183,140 KiB. search_complete=False; stop=None; new unique/solved 319,772/296,800; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Eduming balanced10k is now in progress.

Eduming balanced50/20/b10k: returncode0, score81,321 ties81,321. Processed/generated 89,913/337,920; native/wrapper elapsed 300.891/301.109 seconds; external peak RSS 199,088 KiB. search_complete=False; stop=None; new unique/solved 308,227/296,051; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Gayawaku optimistic22k is now in progress.

Gayawaku optimistic100/b22k: returncode0, score50,105 ties50,105. Processed/generated 88,828/369,865; native/wrapper elapsed 586.115/587.491 seconds; external peak RSS 273,284 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 321,657/279,822; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Residual sweep and next plan after Eduming10k and Gayawaku22k:

1. In progress: revalidate terminal/completed metadata, no solver and
   partial graph headers; inspect absence of remaining Gayawaku22k
   continuation80 and balanced50/20 profiles including raw logs.
2. Pending: run those Gayawaku lanes sequentially at540 seconds,
   atomic cap300,000 and384/288 MB; preserve graph and record full paths,
   score/count/RSS/time/stop/completion and unique/solved increments.
   Stop on failure or above-current-town Computer score.
3. Pending: Eduming frontier12,000 independent ranking mixes after
   absence checks; its partial root still has no optimality certificate.
4. Pending: Gayawaku historical audit remains eight pruned/partial
   families; completed retained Witch runs do not prove global optimum.
5. Pending on candidate: strict/direct replay, promotion, safe build and
   focused/full tests followed by fresh completion audit. No candidate.

Three bounded mapped runs completed, adding949,656 unique nodes and
872,673 solved nodes. All scores tie current town records. Full paths
and required metrics recorded. No native source changed. No additional
global record; goal remains active and unachieved.

Next-plan item1 completed: terminal/completed metadata, no native solver,
remaining Gayawaku22k profiles absent including logs. Graph headers:
{"Eduming": {"nodes": 3715506, "open": 315506, "partial": 127642, "solved": 3272358, "root_status": "partial", "best_score": 81321, "database_bytes": 2037456896}, "Gayawaku": {"nodes": 8642129, "open": 958114, "partial": 327796, "solved": 7356219, "root_status": "partial", "best_score": 50105, "database_bytes": 8910639104}}. Item2 remains pending.

## Gayawaku remaining22k mixes and Eduming12k optimistic

Previous turn made progress: three bounded mapped runs with all metrics
and paths recorded. Current preflight confirms terminal/completed metadata,
no native solver, partial graphs and612.2 GiB free. Pending profiles absent.

1. In progress: Gayawaku o0/c80 frontier22,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Gayawaku o50/c20 frontier22,000 under identical limits,
   after successful terminal exit.
3. Pending: Eduming o100/c0 frontier12,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: save all required metrics, full paths and graph increments;
   strict/direct replay, promotion and safe build/tests on any winner.
5. Pending residual sweep: remaining Eduming12k mixes and wider
   Gayawaku frontier retention; no optimum inferred from partial roots.

Gayawaku continuation80/b22k: returncode0, score50,105 ties50,105. Processed/generated 80,890/354,102; native/wrapper elapsed 583.348/585.16 seconds; external peak RSS 273,132 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 319,658/248,701; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Gayawaku balanced22k is now in progress.

Gayawaku balanced50/20/b22k: returncode0, score50,105 ties50,105. Processed/generated 77,732/326,512; native/wrapper elapsed 597.125/599.297 seconds; external peak RSS 273,216 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 294,595/283,796; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Eduming optimistic12k is now in progress.

Eduming optimistic100/b12k: returncode0, score81,321 ties81,321. Processed/generated 108,188/410,906; native/wrapper elapsed 367.775/368.039 seconds; external peak RSS 196,260 KiB. search_complete=False; stop=None; new unique/solved 349,465/276,898; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Residual sweep and next plan after Gayawaku22k and Eduming12k:

1. In progress: revalidate terminal/completed metadata, no solver,
   partial graph headers and absence of Eduming12k continuation80 and
   balanced50/20 profiles including raw logs.
2. Pending: run those Eduming profiles sequentially at540 seconds,
   atomic cap300,000 and384/288 MB; preserve graph and record full paths,
   score/count/RSS/time/stop/completion and unique/solved increments.
   Stop on failure or above-current-town Computer score.
3. Pending: Gayawaku frontier24,000 independent mixes after absence
   checks; root remains partial. Historical eight-family audit still pruned.
4. Pending on global candidate: strict/direct replay, promotion, safe
   build/focused/full tests and fresh completion audit. None exists.

Three bounded mapped runs completed, adding963,718 unique nodes and
809,395 solved nodes. All scores tie current town records. Full paths
and required metrics recorded. No native source changed; no additional
global record. Goal remains active and unachieved.

Next-plan item1 completed: terminal/completed metadata, no native solver,
remaining Eduming12k profiles absent including logs. Graph headers:
{"Eduming": {"nodes": 4064971, "open": 364971, "partial": 150744, "solved": 3549256, "root_status": "partial", "best_score": 81321, "database_bytes": 2239803392}, "Gayawaku": {"nodes": 9256382, "open": 1019869, "partial": 347797, "solved": 7888716, "root_status": "partial", "best_score": 50105, "database_bytes": 9608105984}}. Item2 remains pending.

## Eduming remaining12k mixes and Gayawaku24k optimistic

Previous turn made progress: three bounded mapped runs, metrics and paths
recorded. Current preflight confirms terminal/completed metadata, no solver,
partial graphs and609.6 GiB free. Matching pending lanes absent.

1. In progress: Eduming o0/c80 frontier12,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Eduming o50/c20 frontier12,000 under identical limits,
   after successful terminal exit.
3. Pending: Gayawaku o100/c0 frontier24,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record full paths, metrics and graph increments. Diagnose any
   actual failure before another launch; verify/promote a global winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Gayawaku24k mixes and wider
   Eduming retention; all partial roots remain unresolved.

Eduming continuation80/b12k: returncode0, score81,321 ties81,321. Processed/generated 105,834/368,837; native/wrapper elapsed 276.864/277.118 seconds; external peak RSS 195,968 KiB. search_complete=False; stop=None; new unique/solved 337,293/285,534; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Eduming balanced12k is now in progress.

Eduming balanced50/20/b12k: returncode0, score81,321 ties81,321. Processed/generated 88,579/333,574; native/wrapper elapsed 303.263/303.575 seconds; external peak RSS 197,868 KiB. search_complete=False; stop=None; new unique/solved 302,476/301,535; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Gayawaku optimistic24k is now in progress.

Gayawaku optimistic100/b24k: returncode0, score50,105 ties50,105. Processed/generated 88,430/368,016; native/wrapper elapsed 583.572/588.538 seconds; external peak RSS 285,160 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 318,076/275,088; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Residual sweep and next plan after Eduming12k and Gayawaku24k:

1. In progress: revalidate terminal/completed metadata, no solver and
   partial graph headers; check remaining Gayawaku24k continuation80
   and balanced50/20 profiles absent including raw logs.
2. Pending: run those Gayawaku profiles sequentially at540 seconds,
   atomic cap300,000 and384/288 MB; preserve graph and record full paths,
   score/count/RSS/time/stop/completion and unique/solved increments.
   Stop on actual failure or above-current-town Computer score.
3. Pending: Eduming frontier14,000 independent ranking mixes after
   absence checks; partial root remains uncertified.
4. Pending: Gayawaku historical audit remains eight pruned/partial
   families; no global-optimum claim from completed retained Witch runs.
5. Pending on candidate: strict/direct replay, promotion, safe build and
   focused/full tests followed by fresh completion audit. None exists.

Three bounded mapped runs completed, adding957,845 unique nodes and
862,157 solved nodes. All scores tie current town records. Required
metrics and complete paths recorded; native source unchanged. No new
global record. Goal remains active and unachieved.

Next-plan item1 completed: terminal/completed metadata, no native solver,
remaining Gayawaku24k profiles absent including logs. Graph headers:
{"Eduming": {"nodes": 4704740, "open": 404740, "partial": 163675, "solved": 4136325, "root_status": "partial", "best_score": 81321, "database_bytes": 2541219840}, "Gayawaku": {"nodes": 9574458, "open": 1054112, "partial": 356542, "solved": 8163804, "root_status": "partial", "best_score": 50105, "database_bytes": 9889792000}}. Item2 remains pending.

## Gayawaku remaining24k mixes and Eduming14k optimistic

Previous turn made progress: three bounded runs with saved metrics/routes.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and606.8 GiB free. Matching pending profiles absent.

1. In progress: Gayawaku o0/c80 frontier24,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Gayawaku o50/c20 frontier24,000 under identical limits,
   after successful terminal exit.
3. Pending: Eduming o100/c0 frontier14,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record full paths, metrics and graph increments; investigate
   actual failures before another launch. Verify/promote a global winner
   with strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Eduming14k mixes and further
   Gayawaku retention; partial roots remain uncertified.

Gayawaku continuation80/b24k: returncode0, score50,105 ties50,105. Processed/generated 82,487/363,482; native/wrapper elapsed 595.508/596.19 seconds; external peak RSS 287,192 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 325,787/270,487; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Gayawaku balanced24k is now in progress.

Gayawaku balanced50/20/b24k: returncode0, score50,105 ties50,105. Processed/generated 64,476/270,804; native/wrapper elapsed 586.471/589.199 seconds; external peak RSS 286,204 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 239,092/288,242; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Eduming optimistic14k is now in progress.

Eduming optimistic100/b14k: returncode0, score81,321 ties81,321. Processed/generated 100,916/392,917; native/wrapper elapsed 375.982/376.294 seconds; external peak RSS 213,064 KiB. search_complete=False; stop=None; new unique/solved 332,943/278,903; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Residual sweep and next plan after Gayawaku24k and Eduming14k:

1. In progress: revalidate terminal/completed metadata, no solver,
   partial graph headers and absence of remaining Eduming14k
   continuation80 and balanced50/20 profiles including raw logs.
2. Pending: run those Eduming profiles sequentially at540 seconds,
   atomic cap300,000 and384/288 MB; preserve graph and record full paths,
   score/count/RSS/time/stop/completion and unique/solved increments.
   Stop on actual failure or above-current-town Computer score.
3. Pending: Gayawaku frontier26,000 independent mixes after absence
   checks; its root remains partial. Historical eight-family audit pruned.
4. Pending on candidate: strict/direct replay, promotion, safe build and
   focused/full tests followed by fresh completion audit. None exists.

Three bounded mapped runs completed, adding897,822 unique nodes and
837,632 solved nodes. All scores tie current town records. Required
metrics and complete paths saved. Native source unchanged; no additional
global record. Goal remains active and unachieved.

Next-plan item1 completed: terminal/completed metadata, no native solver,
remaining Eduming14k profiles absent including logs. Graph headers:
{"Eduming": {"nodes": 5037683, "open": 437683, "partial": 184772, "solved": 4415228, "root_status": "partial", "best_score": 81321, "database_bytes": 2733654016}, "Gayawaku": {"nodes": 10139337, "open": 1059985, "partial": 356819, "solved": 8722533, "root_status": "partial", "best_score": 50105, "database_bytes": 10531930112}}. Item2 remains pending.

## Eduming remaining14k mixes and Gayawaku26k optimistic

Previous turn made progress: three bounded mapped runs with metrics/routes
recorded. Current preflight confirms terminal/completed metadata, no native
solver, partial graphs and604.7 GiB free. Matching pending lanes absent.

1. In progress: Eduming o0/c80 frontier14,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Eduming o50/c20 frontier14,000 under identical limits,
   after successful terminal exit.
3. Pending: Gayawaku o100/c0 frontier26,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full paths and graph increments; diagnose
   actual failure before another launch. Verify/promote a global winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: Gayawaku remaining26k mixes and further
   Eduming retention; partial roots remain uncertified.

Eduming continuation80/b14k: returncode0, score81,321 ties81,321. Processed/generated 103,297/363,117; native/wrapper elapsed 279.007/279.325 seconds; external peak RSS 207,404 KiB. search_complete=False; stop=None; new unique/solved 334,607/286,422; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Eduming balanced14k is now in progress.

Eduming balanced50/20/b14k: returncode0, score81,321 ties81,321. Processed/generated 88,279/327,785; native/wrapper elapsed 303.126/303.523 seconds; external peak RSS 207,516 KiB. search_complete=False; stop=None; new unique/solved 288,989/306,216; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Gayawaku optimistic26k is now in progress.

Gayawaku optimistic100/b26k: returncode0, score50,105 ties50,105. Processed/generated 81,285/342,911; native/wrapper elapsed 592.503/593.796 seconds; external peak RSS 296,876 KiB. search_complete=False; stop=internal time budget reached after 540 seconds; new unique/solved 298,013/222,731; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Residual sweep and next plan after Eduming14k and Gayawaku26k:

1. In progress: revalidate terminal/completed metadata, no solver and
   partial graphs; inspect remaining Gayawaku26k continuation80 and
   balanced50/20 profiles plus Petrinov8k optimistic profile for absence.
2. Pending: run those two Gayawaku profiles sequentially at540 seconds,
   atomic cap300,000 and384/288 MB. Preserve graph; record full paths,
   score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: broaden coverage to Petrinov optimistic100/continuation0
   frontier8,000 under identical caps and time budget, after absence and
   partial-root checks. Repeated wider Eduming/Gayawaku profiles remain
   tied, so a distinct town supplies another independent search branch.
4. Pending: Eduming16k retention remains available after broader-town
   coverage. Gayawaku historical audit still eight pruned/partial families.
5. Pending on candidate: strict/direct replay, promotion, safe build and
   focused/full tests followed by fresh completion audit. None exists.

Three bounded mapped runs completed, adding921,609 unique nodes and
815,369 solved nodes. All scores tie current town records. Full routes
and required metrics recorded; native source unchanged. No additional
global record. Goal remains active and unachieved.

Next-plan item1 completed: terminal/completed metadata, no native solver,
remaining Gayawaku26k and Petrinov optimistic8k profiles absent including
logs. Graph headers: {"Eduming": {"nodes": 5661279, "open": 461279, "partial": 192134, "solved": 5007866, "root_status": "partial", "best_score": 81321, "database_bytes": 3014348800}, "Gayawaku": {"nodes": 10437350, "open": 1112464, "partial": 379622, "solved": 8945264, "root_status": "partial", "best_score": 50105, "database_bytes": 10798092288}, "Petrinov": {"nodes": 1496448, "open": 196448, "partial": 79462, "solved": 1220538, "root_status": "partial", "best_score": 45578, "database_bytes": 936603648}}. Item2 remains pending.

## Gayawaku remaining26k mixes and Petrinov8k optimistic

Previous turn made progress: three bounded searches with saved metrics and
paths. Current state matches that checkpoint despite goal-accounting jump;
terminal/completed metadata, no solver, partial graphs and601.4 GiB free.
All planned lanes are absent including logs.

1. In progress: Gayawaku o0/c80 frontier26,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Gayawaku o50/c20 frontier26,000 under identical limits,
   after successful terminal exit.
3. Pending: Petrinov o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: save all metrics, complete paths and graph increments;
   diagnose actual failure before further launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: Petrinov remaining8k mixes and wider town
   coverage. Partial roots and pruned Gayawaku families remain unresolved.

Gayawaku continuation80/b26k: returncode0, score50,105 ties50,105. Processed/generated 77,365/338,615; native/wrapper elapsed 589.665/591.399 seconds; external peak RSS 297,752 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 300,091/224,036; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Gayawaku balanced26k is now in progress.

Gayawaku balanced50/20/b26k: returncode0, score50,105 ties50,105. Processed/generated 70,032/294,668; native/wrapper elapsed 581.11/581.953 seconds; external peak RSS 297,712 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 260,838/252,053; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Petrinov optimistic8k is now in progress.

Petrinov optimistic100/b8k: returncode0, score45,578 ties current Computer maximum45,578. Processed/generated 109,452/421,017; native/wrapper elapsed 374.293/374.425 seconds; external peak RSS 190,056 KiB. search_complete=False; stop=None; new unique/solved 345,543/280,107; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [4, 0, 0, 0, 0, 0] → Interest → Buy [15, 0, 0, 0, 0, 0] → Interest → Interest → Merch - Blue Treasure → Interest → Interest → Witch - Oasis of Sanctifan → Interest → Interest → Interest → Interest → Interest → Interest → Interest

Residual sweep and next plan after Gayawaku26k and Petrinov8k:

1. In progress: revalidate terminal/completed metadata, no solver and
   partial graph headers; inspect absence of Petrinov8k continuation80
   and balanced50/20 plus Bajaar8k optimistic profiles including logs.
2. Pending: run the two Petrinov mixes sequentially at540 seconds,
   atomic cap300,000 and384/288 MB. Preserve graph and record full paths,
   score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Bajaar optimistic100/continuation0 frontier8,000 under
   identical limits after successful exits, broadening distinct-town
   coverage. Stop on actual failure or above-current-town Computer score.
4. Pending: Gayawaku28k and Eduming16k retention remain available
   after broader-town coverage; partial roots remain uncertified and
   historical Gayawaku audit remains eight pruned/partial families.
5. Pending on global candidate: strict/direct replay, promotion, safe
   build/focused/full tests and fresh completion audit. None exists.

Three bounded mapped runs completed, adding906,472 unique nodes and
756,196 solved nodes. All scores tie current town records. Full routes
and required metrics saved; native source unchanged. No additional
global record. Goal remains active and unachieved.

Next-plan item1 completed: terminal/completed metadata, no native solver,
remaining Petrinov8k and Bajaar optimistic8k profiles absent including
logs. Graph headers: {"Petrinov": {"nodes": 1841991, "open": 241991, "partial": 99355, "solved": 1500645, "root_status": "partial", "best_score": 45578, "database_bytes": 1116950528}, "Bajaar": {"nodes": 1758699, "open": 219710, "partial": 81063, "solved": 1457926, "root_status": "partial", "best_score": 56147, "database_bytes": 1146425344}, "Gayawaku": {"nodes": 10998279, "open": 1171047, "partial": 405879, "solved": 9421353, "root_status": "partial", "best_score": 50105, "database_bytes": 11438051328}}. Item2 remains pending.

## Petrinov remaining8k mixes and Bajaar8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and598.0 GiB free. Matching pending profiles absent.

1. In progress: Petrinov o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Petrinov o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Bajaar o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: save all metrics, full paths and graph increments; diagnose
   actual failures before another launch. Verify/promote a winner with
   strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Bajaar8k mixes and additional town
   coverage; partial roots remain uncertified.

Petrinov continuation80/b8k: returncode0, score45,578 ties45,578. Processed/generated 123,144/465,152; native/wrapper elapsed 256.522/256.67 seconds; external peak RSS 167,688 KiB. search_complete=False; stop=None; new unique/solved 418,434/255,064; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [4, 0, 0, 0, 0, 0] → Interest → Buy [15, 0, 0, 0, 0, 0] → Interest → Interest → Merch - Blue Treasure → Interest → Interest → Witch - Oasis of Sanctifan → Interest → Interest → Interest → Interest → Interest → Interest → Interest

Petrinov balanced8k is now in progress.

Petrinov balanced50/20/b8k: returncode0, score45,578 ties45,578. Processed/generated 96,927/361,445; native/wrapper elapsed 322.468/322.628 seconds; external peak RSS 192,644 KiB. search_complete=False; stop=None; new unique/solved 299,954/304,414; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [4, 0, 0, 0, 0, 0] → Interest → Buy [15, 0, 0, 0, 0, 0] → Interest → Interest → Merch - Blue Treasure → Interest → Interest → Witch - Oasis of Sanctifan → Interest → Interest → Interest → Interest → Interest → Interest → Interest

Bajaar optimistic8k is now in progress.

Bajaar optimistic100/b8k: returncode0, score56,147 ties current Computer maximum56,147. Processed/generated 95,363/398,358; native/wrapper elapsed 546.87/547.007 seconds; external peak RSS 177,600 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 338,429/275,326; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [7, 0, 0, 2, 0, 0] → Merch - Blue Treasure → Sell → Buy [10, 0, 2, 1, 1, 0] → Sell → Witch - Oasis of Sanctifan → Buy [0, 7, 1, 0, 0, 2] → Sell → Buy [0, 0, 0, 0, 0, 5] → Sell → Interest → Interest → Interest

Residual sweep and next plan after Petrinov8k and Bajaar8k:

1. In progress: revalidate terminal/completed metadata, no solver and
   partial graph headers; inspect absence of Bajaar8k continuation80
   and balanced50/20 plus L'Exquise8k optimistic profiles including logs.
2. Pending: run the two Bajaar mixes sequentially at540 seconds,
   atomic cap300,000 and384/288 MB. Preserve graph and record full paths,
   score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: L'Exquise optimistic100/continuation0 frontier8,000 under
   identical limits after successful exits. Stop on actual failure or
   above-current-town Computer score; broaden independent town coverage.
4. Pending: wider Petrinov/Bajaar retention, Gayawaku28k and Eduming16k
   remain available; partial roots remain uncertified. Historical Gayawaku
   audit still eight pruned/partial families.
5. Pending on global candidate: strict/direct replay, promotion, safe
   build/focused/full tests and fresh completion audit. None exists.

Three bounded mapped runs completed, adding1,056,817 unique nodes and
834,804 solved nodes. All scores tie current town records. Full routes
and required metrics saved; native source unchanged. No additional
global record. Goal remains active and unachieved.

Next-plan item1 completed: terminal/completed metadata, no native solver,
remaining Bajaar8k and L'Exquise optimistic8k profiles absent including
logs. Graph headers: {"Petrinov": {"nodes": 2560379, "open": 360379, "partial": 139877, "solved": 2060123, "root_status": "partial", "best_score": 45578, "database_bytes": 1441808384}, "Bajaar": {"nodes": 2097128, "open": 265553, "partial": 98323, "solved": 1733252, "root_status": "partial", "best_score": 56147, "database_bytes": 1341050880}, "L'Exquise": {"nodes": 1446751, "open": 257394, "partial": 85763, "solved": 1103594, "root_status": "partial", "best_score": 56817, "database_bytes": 965128192}}. Item2 remains pending.

## Bajaar remaining8k mixes and L'Exquise8k optimistic

Previous turn made progress: three bounded runs with saved metrics/routes.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and597.3 GiB free. Matching pending profiles absent.

1. In progress: Bajaar o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Bajaar o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: L'Exquise o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining L'Exquise8k mixes and additional
   town coverage; partial roots remain uncertified.

Bajaar continuation80/b8k: returncode0, score56,147 ties56,147. Processed/generated 114,967/458,389; native/wrapper elapsed 361.012/361.166 seconds; external peak RSS 179,712 KiB. search_complete=False; stop=None; new unique/solved 419,752/255,073; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [7, 0, 0, 2, 0, 0] → Merch - Blue Treasure → Sell → Buy [10, 0, 2, 1, 1, 0] → Sell → Witch - Oasis of Sanctifan → Buy [0, 7, 1, 0, 0, 2] → Sell → Buy [0, 0, 0, 0, 0, 5] → Sell → Interest → Interest → Interest

Bajaar balanced8k is now in progress.

Bajaar balanced50/20/b8k: returncode0, score56,147 ties56,147. Processed/generated 81,382/320,136; native/wrapper elapsed 441.191/441.45 seconds; external peak RSS 185,244 KiB. search_complete=False; stop=None; new unique/solved 286,006/311,396; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [7, 0, 0, 2, 0, 0] → Merch - Blue Treasure → Sell → Buy [10, 0, 2, 1, 1, 0] → Sell → Witch - Oasis of Sanctifan → Buy [0, 7, 1, 0, 0, 2] → Sell → Buy [0, 0, 0, 0, 0, 5] → Sell → Interest → Interest → Interest

L'Exquise optimistic8k is now in progress.

L'Exquise optimistic100/b8k: returncode0, score56,817 ties current Computer maximum56,817. Processed/generated 95,658/408,980; native/wrapper elapsed 545.156/545.286 seconds; external peak RSS 175,244 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 344,094/276,421; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Trader → Trader → Trader → Trader → Buy [14, 0, 0, 0, 0, 0] → Merch - Blue Treasure → Trader → Trader → Witch - Midas was a Trader → Trader → Camel → Buy [0, 0, 0, 0, 1, 3] → Sell → Interest → Interest → Interest

Residual sweep and next plan after Bajaar8k and L'Exquise8k:

1. In progress: revalidate terminal/completed metadata, no solver and
   partial graph headers; inspect absence of L'Exquise8k continuation80
   and balanced50/20 plus Mehaz8k optimistic profiles including logs.
2. Pending: run the two L'Exquise mixes sequentially at540 seconds,
   atomic cap300,000 and384/288 MB. Preserve graph and record full paths,
   score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Mehaz optimistic100/continuation0 frontier8,000 under
   identical limits after successful exits. Stop on actual failure or
   above-current-town Computer score; broaden independent town coverage.
4. Pending: wider Petrinov/Bajaar/L'Exquise retention, Gayawaku28k and
   Eduming16k remain available. Partial roots remain uncertified and
   historical Gayawaku audit still eight pruned/partial families.
5. Pending on global candidate: strict/direct replay, promotion, safe
   build/focused/full tests and fresh completion audit. None exists.

Three bounded mapped runs completed, adding1,049,852 unique nodes and
842,890 solved nodes. All scores tie current town records. Full routes
and required metrics saved; native source unchanged. No additional
global record. Goal remains active and unachieved.

Next-plan item1 completed: terminal/completed metadata, no native solver,
remaining L'Exquise8k and Mehaz optimistic8k profiles absent including
logs. Graph headers: {"Bajaar": {"nodes": 2802886, "open": 371311, "partial": 131854, "solved": 2299721, "root_status": "partial", "best_score": 56147, "database_bytes": 1681170432}, "L'Exquise": {"nodes": 1790845, "open": 309822, "partial": 101008, "solved": 1380015, "root_status": "partial", "best_score": 56817, "database_bytes": 1168908288}, "Mehaz": {"nodes": 1548579, "open": 248579, "partial": 88813, "solved": 1211187, "root_status": "partial", "best_score": 48738, "database_bytes": 1048330240}}. Item2 remains pending.

## L'Exquise remaining8k mixes and Mehaz8k optimistic

Previous turn made progress: three bounded runs with saved metrics/routes.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and594.1 GiB free. Matching pending profiles absent.

1. In progress: L'Exquise o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: L'Exquise o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Mehaz o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Mehaz8k mixes and additional
   town coverage; partial roots remain uncertified.

L'Exquise continuation80/b8k: returncode0, score56,817 ties56,817. Processed/generated 107,218/493,863; native/wrapper elapsed 426.951/427.11 seconds; external peak RSS 184,748 KiB. search_complete=False; stop=None; new unique/solved 454,912/249,294; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Trader → Trader → Trader → Trader → Buy [14, 0, 0, 0, 0, 0] → Merch - Blue Treasure → Trader → Trader → Witch - Midas was a Trader → Trader → Camel → Buy [0, 0, 0, 0, 1, 3] → Sell → Interest → Interest → Interest

L'Exquise balanced8k is now in progress.

L'Exquise balanced50/20/b8k: returncode0, score56,817 ties56,817. Processed/generated 77,203/333,011; native/wrapper elapsed 505.237/505.396 seconds; external peak RSS 182,200 KiB. search_complete=False; stop=None; new unique/solved 291,135/308,618; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Trader → Trader → Trader → Trader → Buy [14, 0, 0, 0, 0, 0] → Merch - Blue Treasure → Trader → Trader → Witch - Midas was a Trader → Trader → Camel → Buy [0, 0, 0, 0, 1, 3] → Sell → Interest → Interest → Interest

Mehaz optimistic8k is now in progress.

Mehaz optimistic100/b8k: returncode0, score48,738 ties current Computer maximum48,738. Processed/generated 101,038/437,592; native/wrapper elapsed 513.745/513.983 seconds; external peak RSS 196,284 KiB. search_complete=False; stop=None; new unique/solved 355,568/279,032; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Camel → Buy [5, 8, 0, 0, 0, 0] → Camel → Merch - Route to Mahaji → Sell → Buy [4, 10, 0, 2, 0, 0] → Sell → Buy [4, 17, 0, 0, 0, 1] → Witch - Camelization → Sell → Buy [4, 4, 0, 3, 0, 3] → Sell → Buy [6, 0, 0, 0, 0, 9] → Sell → Interest → Interest

Residual sweep and next plan after L'Exquise8k and Mehaz8k:

1. In progress: revalidate terminal/completed metadata, no solver and
   partial graph headers; inspect absence of Mehaz8k continuation80 and
   balanced50/20 plus Jilliqo8k optimistic profiles including logs.
2. Pending: run the two Mehaz mixes sequentially at540 seconds, atomic
   cap300,000 and384/288 MB. Preserve graph and record full paths,
   score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Jilliqo optimistic100/continuation0 frontier8,000 under
   identical limits after successful exits. Stop on actual failure or
   above-current-town Computer score; broaden independent town coverage.
4. Pending: wider retained profiles remain available for towns already
   covered at8k, Gayawaku28k and Eduming16k. Partial roots remain
   uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on global candidate: strict/direct replay, promotion, safe
   build/focused/full tests and fresh completion audit. None exists.

Three bounded mapped runs completed, adding1,101,615 unique nodes and
836,944 solved nodes. All scores tie current town records. Full routes
and required metrics saved; native source unchanged. No additional
global record. Goal remains active and unachieved.

Next-plan item1 completed: terminal/completed metadata, no native solver,
remaining Mehaz8k and Jilliqo optimistic8k profiles absent including logs.
Graph headers: {"L'Exquise": {"nodes": 2536892, "open": 455869, "partial": 143096, "solved": 1937927, "root_status": "partial", "best_score": 56817, "database_bytes": 1518055424}, "Mehaz": {"nodes": 1904147, "open": 304147, "partial": 109781, "solved": 1490219, "root_status": "partial", "best_score": 48738, "database_bytes": 1226235904}, "Jilliqo": {"nodes": 1527530, "open": 227530, "partial": 86897, "solved": 1213103, "root_status": "partial", "best_score": 37819, "database_bytes": 974082048}}. Item2 remains pending.

## Mehaz remaining8k mixes and Jilliqo8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and592.0 GiB free. Matching pending profiles absent.

1. In progress: Mehaz o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Mehaz o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Jilliqo o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, complete routes and graph increments;
   diagnose actual failures before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Jilliqo8k mixes and additional
   town coverage; partial roots remain uncertified.

Mehaz continuation80/b8k: returncode0, score48,738 ties48,738. Processed/generated 113,943/496,981; native/wrapper elapsed 374.628/374.859 seconds; external peak RSS 180,744 KiB. search_complete=False; stop=None; new unique/solved 454,319/247,685; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Camel → Buy [5, 8, 0, 0, 0, 0] → Camel → Merch - Route to Mahaji → Sell → Buy [4, 10, 0, 2, 0, 0] → Sell → Buy [4, 17, 0, 0, 0, 1] → Witch - Camelization → Sell → Buy [4, 4, 0, 3, 0, 3] → Sell → Buy [6, 0, 0, 0, 0, 9] → Sell → Interest → Interest

Mehaz balanced8k is now in progress.

Mehaz balanced50/20/b8k: returncode0, score48,738 ties48,738. Processed/generated 90,259/387,479; native/wrapper elapsed 446.225/446.403 seconds; external peak RSS 183,436 KiB. search_complete=False; stop=None; new unique/solved 314,289/292,488; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Camel → Buy [5, 8, 0, 0, 0, 0] → Camel → Merch - Route to Mahaji → Sell → Buy [4, 10, 0, 2, 0, 0] → Sell → Buy [4, 17, 0, 0, 0, 1] → Witch - Camelization → Sell → Buy [4, 4, 0, 3, 0, 3] → Sell → Buy [6, 0, 0, 0, 0, 9] → Sell → Interest → Interest

Jilliqo optimistic8k is now in progress.

Jilliqo optimistic100/b8k: returncode0, score37,819 ties current Computer maximum37,819. Processed/generated 101,402/413,625; native/wrapper elapsed 535.77/535.998 seconds; external peak RSS 193,640 KiB. search_complete=False; stop=None; new unique/solved 343,648/281,579; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 0, 15, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Residual sweep and next plan after Mehaz8k and Jilliqo8k:

1. In progress: revalidate terminal/completed metadata, no solver and
   partial graph headers; inspect absence of Jilliqo8k continuation80 and
   balanced50/20 plus Port-Willam8k optimistic profiles including logs.
2. Pending: run the two Jilliqo mixes sequentially at540 seconds, atomic
   cap300,000 and384/288 MB. Preserve graph and record full paths,
   score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Port-Willam optimistic100/continuation0 frontier8,000 under
   identical limits after successful exits. Stop on actual failure or
   above-current-town Computer score; broaden independent town coverage.
4. Pending: wider retained profiles remain available for towns already
   covered at8k, Gayawaku28k and Eduming16k. Partial roots remain
   uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on global candidate: strict/direct replay, promotion, safe
   build/focused/full tests and fresh completion audit. None exists.

Three bounded mapped runs completed, adding1,112,256 unique nodes and
821,752 solved nodes. All scores tie current town records. Full routes
and required metrics saved; native source unchanged. No additional
global record. Goal remains active and unachieved.

Next-plan item1 completed: terminal/completed metadata, no native solver,
remaining Jilliqo8k and Port-Willam optimistic8k profiles absent including
logs. Graph headers: {"Mehaz": {"nodes": 2672755, "open": 472755, "partial": 169608, "solved": 2030392, "root_status": "partial", "best_score": 48738, "database_bytes": 1593421824}, "Jilliqo": {"nodes": 1871178, "open": 271178, "partial": 105318, "solved": 1494682, "root_status": "partial", "best_score": 37819, "database_bytes": 1154777088}, "Port-Willam": {"nodes": 1922818, "open": 327503, "partial": 120901, "solved": 1474414, "root_status": "partial", "best_score": 42296, "database_bytes": 1143996416}}. Item2 remains pending.

## Jilliqo remaining8k mixes and Port-Willam8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and590.9 GiB free. Matching pending profiles absent.

1. In progress: Jilliqo o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Jilliqo o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Port-Willam o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Port-Willam8k mixes and additional
   town coverage; partial roots remain uncertified.

Jilliqo continuation80/b8k: returncode0, score37,819 ties37,819. Processed/generated 123,035/490,153; native/wrapper elapsed 288.859/289.005 seconds; external peak RSS 179,948 KiB. search_complete=False; stop=None; new unique/solved 451,798/246,304; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 0, 15, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Jilliqo balanced8k is now in progress.

Jilliqo balanced50/20/b8k: returncode0, score37,819 ties37,819. Processed/generated 106,316/384,082; native/wrapper elapsed 397.692/397.861 seconds; external peak RSS 194,424 KiB. search_complete=False; stop=None; new unique/solved 314,765/289,482; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 0, 15, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Port-Willam optimistic8k is now in progress.

Port-Willam optimistic100/b8k: returncode0, score42,296 ties current Computer maximum42,296. Processed/generated 102,560/412,566; native/wrapper elapsed 487.145/487.344 seconds; external peak RSS 196,532 KiB. search_complete=False; stop=None; new unique/solved 353,307/280,489; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [15, 0, 0, 0, 0, 0] → Trader → Camel → Buy [31, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [17, 8, 0, 0, 0, 1] → Sell → Buy [18, 7, 3, 0, 1, 4] → Sell → Buy [18, 0, 0, 0, 0, 8] → Sell → Interest

Three previous bounded mapped runs completed, adding1,119,870 unique nodes and816,275 solved nodes. All scores tie current records. Goal remains active; no additional global record.

Residual sweep and next plan: Port-Willam remaining8k mixes and Skjollird8k optimistic

1. Completed: inspect terminal/completed metadata, absence of live native solver, pending town+lane rows and logs, partial graph headers and588.1 GiB free. All three profiles below are absent. Re-read native safety and deep-campaign guidance including Gayawaku profile.
2. In progress: Port-Willam o0/c80 frontier8,000, atomic cap300,000, 540-second solver budget, external/internal384/288 MB; preserve the graph and record required metrics and complete action path.
3. Pending: Port-Willam o50/c20 under identical limits after successful terminal exit.
4. Pending: Skjollird o100/c0 under identical limits after successful terminal exit. Stop on failure or a score above the current town Computer maximum.
5. Pending: record all three outcomes, external RSS, counts, actual completion/stop, routes and graph increments. Verify a global candidate with strict/direct replay, promotion and safe build/focused/full tests.
6. Pending fresh residual sweep: inspect remaining Skjollird8k mixes and Cornecopea8k optimistic; wider Gayawaku28k and Eduming16k remain available. Partial roots are uncertified, and eight historical Gayawaku families remain pruned/partial.

Port-Willam continuation80/b8k: returncode0, score42,296 ties42,296. Processed/generated 97,361/373,177; native/wrapper elapsed 296.165/296.324 seconds; external peak RSS 179,104 KiB. search_complete=False; stop=None; new unique/solved 344,523/286,977; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [15, 0, 0, 0, 0, 0] → Trader → Camel → Buy [31, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [17, 8, 0, 0, 0, 1] → Sell → Buy [18, 7, 3, 0, 1, 4] → Sell → Buy [18, 0, 0, 0, 0, 8] → Sell → Interest

Port-Willam balanced8k is now in progress.

Port-Willam balanced50/20/b8k: returncode0, score42,296 ties42,296. Processed/generated 97,005/371,736; native/wrapper elapsed 360.227/360.668 seconds; external peak RSS 192,816 KiB. search_complete=False; stop=None; new unique/solved 321,032/288,689; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [15, 0, 0, 0, 0, 0] → Trader → Camel → Buy [31, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [17, 8, 0, 0, 0, 1] → Sell → Buy [18, 7, 3, 0, 1, 4] → Sell → Buy [18, 0, 0, 0, 0, 8] → Sell → Interest

Skjollird optimistic8k is now in progress.

Skjollird optimistic100/b8k: returncode0, score47,065 ties current Computer maximum47,065. Processed/generated 98,269/408,863; native/wrapper elapsed 518.861/519.006 seconds; external peak RSS 190,380 KiB. search_complete=False; stop=None; new unique/solved 355,006/278,644; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [15, 0, 0, 0, 0, 0] → Trader → Camel → Buy [30, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [18, 5, 0, 0, 0, 1] → Sell → Buy [18, 13, 0, 0, 1, 4] → Sell → Buy [18, 1, 0, 0, 0, 10] → Sell → Interest

Three bounded mapped runs completed, adding1,020,561 unique nodes and854,310 solved nodes. All scores tie current town Computer records. Full routes and required metrics saved; native source unchanged. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Port-Willam8k and Skjollird8k:

1. In progress: revalidate terminal/completed metadata, no native solver and partial graph headers; inspect absence of Skjollird8k continuation80 and balanced50/20 plus Cornecopea8k optimistic profiles, including logs.
2. Pending: run the two Skjollird mixes sequentially at540 seconds, atomic cap300,000 and384/288 MB. Preserve graph and record full paths, score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Cornecopea optimistic100/continuation0 frontier8,000 under identical limits after successful exits. Stop on actual failure or above-current-town Computer score; broaden independent town coverage.
4. Pending: Cornecopea remaining8k mixes and Sanctifan8k remain available, as do wider retained profiles for previously covered towns, Gayawaku28k and Eduming16k. Partial roots remain uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: terminal/completed metadata, no native solver, remaining Skjollird8k and Cornecopea optimistic8k profiles absent including logs. Graph headers: {"Port-Willam": {"nodes": 2941680, "open": 446365, "partial": 164746, "solved": 2330569, "root_status": "partial", "best_score": 42296, "database_bytes": 1643429888}, "Skjollird": {"nodes": 2287467, "open": 387467, "partial": 142507, "solved": 1757493, "root_status": "partial", "best_score": 47065, "database_bytes": 1350250496}, "Cornecopea": {"nodes": 1915851, "open": 315851, "partial": 120243, "solved": 1479757, "root_status": "partial", "best_score": 43881, "database_bytes": 925364224}}. Free space 586.8 GiB. Item2 remains pending.

## Skjollird remaining8k mixes and Cornecopea8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and586.8 GiB free. Matching pending profiles absent.
Current Best.txt Computer maxima verified: Skjollird47,065; Cornecopea43,881.
Deep-campaign guidance re-read, including Gayawaku-specific recovery limits.

1. In progress: Skjollird o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Skjollird o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Cornecopea o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Cornecopea8k mixes and Sanctifan8k
   coverage; partial roots remain uncertified.

Skjollird continuation80/b8k: returncode0, score47,065 ties47,065. Processed/generated 95,280/375,609; native/wrapper elapsed 312.749/312.909 seconds; external peak RSS 181,896 KiB. search_complete=False; stop=None; new unique/solved 340,217/284,562; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [15, 0, 0, 0, 0, 0] → Trader → Camel → Buy [30, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [18, 5, 0, 0, 0, 1] → Sell → Buy [18, 13, 0, 0, 1, 4] → Sell → Buy [18, 1, 0, 0, 0, 10] → Sell → Interest

Skjollird balanced8k is now in progress.

Skjollird balanced50/20/b8k: returncode0, score47,065 ties47,065. Processed/generated 96,987/380,412; native/wrapper elapsed 452.898/453.187 seconds; external peak RSS 194,492 KiB. search_complete=False; stop=None; new unique/solved 331,378/280,923; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [15, 0, 0, 0, 0, 0] → Trader → Camel → Buy [30, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [18, 5, 0, 0, 0, 1] → Sell → Buy [18, 13, 0, 0, 1, 4] → Sell → Buy [18, 1, 0, 0, 0, 10] → Sell → Interest

Cornecopea optimistic8k is now in progress.

Cornecopea optimistic100/b8k: returncode0, score43,881 ties current Computer maximum43,881. Processed/generated 102,864/401,911; native/wrapper elapsed 326.184/326.374 seconds; external peak RSS 201,804 KiB. search_complete=False; stop=None; new unique/solved 342,965/287,899; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Camel → Buy [23, 0, 0, 0, 0, 0] → Trader → Trader → Trader → Merch - Magic Cleppsydra → Camel → Buy [34, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [20, 18, 0, 0, 0, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Three bounded mapped runs completed, adding1,014,560 unique nodes and853,384 solved nodes. All scores tie current town Computer records. Full routes and required metrics saved; native source unchanged. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Skjollird8k and Cornecopea8k:

1. In progress: revalidate terminal/completed metadata, no native solver and partial graph headers; inspect absence of Cornecopea8k continuation80 and balanced50/20 plus Sanctifan8k optimistic profiles, including logs.
2. Pending: run the two Cornecopea mixes sequentially at540 seconds, atomic cap300,000 and384/288 MB. Preserve graph and record full paths, score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Sanctifan optimistic100/continuation0 frontier8,000 under identical limits after successful exits. Stop on actual failure or above-current-town Computer score; broaden independent town coverage.
4. Pending: Sanctifan remaining8k mixes and additional town coverage, plus wider retained profiles for previously covered towns, Gayawaku28k and Eduming16k. Partial roots remain uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: terminal/completed metadata, no native solver, remaining Cornecopea8k and Sanctifan optimistic8k profiles absent including logs. Graph headers: {"Skjollird": {"nodes": 2959062, "open": 459062, "partial": 177022, "solved": 2322978, "root_status": "partial", "best_score": 47065, "database_bytes": 1661493248}, "Cornecopea": {"nodes": 2258816, "open": 358816, "partial": 132344, "solved": 1767656, "root_status": "partial", "best_score": 43881, "database_bytes": 1050169344}, "Sanctifan": {"nodes": 1663745, "open": 263745, "partial": 94741, "solved": 1305259, "root_status": "partial", "best_score": 55728, "database_bytes": 1012387840}}. Free space 586.3 GiB. Item2 remains pending.

## Cornecopea remaining8k mixes and Sanctifan8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and586.3 GiB free. Matching pending profiles absent.
Current Best.txt Computer maxima verified: Cornecopea43,881; Sanctifan55,728.
Deep-campaign guidance re-read, including Gayawaku-specific recovery limits.

1. In progress: Cornecopea o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Cornecopea o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Sanctifan o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Sanctifan8k mixes and additional
   town8k coverage; partial roots remain uncertified.

Cornecopea continuation80/b8k: returncode0, score43,881 ties43,881. Processed/generated 98,363/372,258; native/wrapper elapsed 265.243/265.402 seconds; external peak RSS 180,728 KiB. search_complete=False; stop=None; new unique/solved 339,785/284,611; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Camel → Buy [23, 0, 0, 0, 0, 0] → Trader → Trader → Trader → Merch - Magic Cleppsydra → Camel → Buy [34, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [20, 18, 0, 0, 0, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Cornecopea balanced8k is now in progress.

Cornecopea balanced50/20/b8k: returncode0, score43,881 ties43,881. Processed/generated 96,535/365,062; native/wrapper elapsed 284.762/284.932 seconds; external peak RSS 196,836 KiB. search_complete=False; stop=None; new unique/solved 314,541/291,447; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Camel → Buy [23, 0, 0, 0, 0, 0] → Trader → Trader → Trader → Merch - Magic Cleppsydra → Camel → Buy [34, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [20, 18, 0, 0, 0, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Sanctifan optimistic8k is now in progress.

Sanctifan optimistic100/b8k: returncode0, score55,728 ties current Computer maximum55,728. Processed/generated 106,950/408,908; native/wrapper elapsed 451.746/451.887 seconds; external peak RSS 185,604 KiB. search_complete=False; stop=None; new unique/solved 362,561/276,791; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 8, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest → Interest

Three bounded mapped runs completed, adding1,016,887 unique nodes and852,849 solved nodes. All scores tie current town Computer records. Full routes and required metrics saved; native source unchanged. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Cornecopea8k and Sanctifan8k:

1. In progress: revalidate terminal/completed metadata, no native solver and partial graph headers; inspect absence of Sanctifan8k continuation80 and balanced50/20 plus Tocaccialli8k optimistic profiles, including logs.
2. Pending: run the two Sanctifan mixes sequentially at540 seconds, atomic cap300,000 and384/288 MB. Preserve graph and record full paths, score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Tocaccialli optimistic100/continuation0 frontier8,000 under identical limits after successful exits. Stop on actual failure or above-current-town Computer score; broaden independent town coverage.
4. Pending: Tocaccialli remaining8k mixes and additional town coverage, plus wider retained profiles for previously covered towns, Gayawaku28k and Eduming16k. Partial roots remain uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: terminal/completed metadata, no native solver, remaining Sanctifan8k and Tocaccialli optimistic8k profiles absent including logs. Graph headers: {"Cornecopea": {"nodes": 2913142, "open": 413142, "partial": 156286, "solved": 2343714, "root_status": "partial", "best_score": 43881, "database_bytes": 1283366912}, "Sanctifan": {"nodes": 2026306, "open": 326306, "partial": 117950, "solved": 1582050, "root_status": "partial", "best_score": 55728, "database_bytes": 1198305280}, "Tocaccialli": {"nodes": 1629707, "open": 229707, "partial": 86785, "solved": 1313215, "root_status": "partial", "best_score": 47621, "database_bytes": 1014689792}}. Free space 585.9 GiB. Item2 remains pending.

## Sanctifan remaining8k mixes and Tocaccialli8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and585.9 GiB free. Matching pending profiles absent.
Current Best.txt Computer maxima match frozen comparison records:
Sanctifan55,728; Tocaccialli49,634. Tocaccialli graph best47,621 is below
its saved record, so recovering a score above47,621 alone is not a global win.
Deep-campaign guidance re-read, including Gayawaku-specific recovery limits.

1. In progress: Sanctifan o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Sanctifan o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Tocaccialli o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Tocaccialli8k mixes and additional
   town8k coverage; partial roots remain uncertified.

Sanctifan continuation80/b8k: returncode0, score55,728 ties55,728. Processed/generated 119,575/480,121; native/wrapper elapsed 301.349/301.507 seconds; external peak RSS 179,512 KiB. search_complete=False; stop=None; new unique/solved 454,799/246,984; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 8, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest → Interest

Sanctifan balanced8k is now in progress.

Sanctifan balanced50/20/b8k: returncode0, score55,728 ties55,728. Processed/generated 94,062/357,752; native/wrapper elapsed 389.828/389.993 seconds; external peak RSS 198,152 KiB. search_complete=False; stop=None; new unique/solved 312,564/296,324; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 8, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest → Interest

Tocaccialli optimistic8k is now in progress.

Tocaccialli optimistic100/b8k: returncode0, score49,634 ties current Computer maximum49,634. Graph best increased47,621→49,634, recovering the existing global record rather than finding a new one. Processed/generated 97,650/386,512; native/wrapper elapsed 419.083/419.209 seconds; external peak RSS 196,196 KiB. search_complete=False; stop=None; new unique/solved 348,956/280,983; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Sturdy Saddle → Buy [13, 0, 0, 0, 0, 0] → Camel → Buy [43, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Trader → Camel → Buy [1, 2, 16, 0, 0, 2] → Sell → Buy [27, 0, 0, 0, 0, 8] → Sell → Interest → Interest

Three bounded mapped runs completed, adding1,116,319 unique nodes and824,291 solved nodes. All scores tie current town Computer records. Full routes and required metrics saved; native source unchanged. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Sanctifan8k and Tocaccialli8k:

1. In progress: revalidate terminal/completed metadata, no native solver and partial graph headers; inspect absence of Tocaccialli8k continuation80 and balanced50/20 plus Normalia8k optimistic profiles, including logs.
2. Pending: run the two Tocaccialli mixes sequentially at540 seconds, atomic cap300,000 and384/288 MB. Preserve graph and record full paths, score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Normalia optimistic100/continuation0 frontier8,000 under identical limits after successful exits. Stop on actual failure or above-current-town Computer score; broaden independent town coverage.
4. Pending: Normalia remaining8k mixes and additional town coverage, plus wider retained profiles for previously covered towns, Gayawaku28k and Eduming16k. Partial roots remain uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: terminal/completed metadata, no native solver, remaining Tocaccialli8k and Normalia optimistic8k profiles absent including logs. Graph headers: {"Sanctifan": {"nodes": 2793669, "open": 493669, "partial": 174642, "solved": 2125358, "root_status": "partial", "best_score": 55728, "database_bytes": 1548816384}, "Tocaccialli": {"nodes": 1978663, "open": 278663, "partial": 105802, "solved": 1594198, "root_status": "partial", "best_score": 49634, "database_bytes": 1191317504}, "Normalia": {"nodes": 1485109, "open": 219886, "partial": 83158, "solved": 1182065, "root_status": "partial", "best_score": 37527, "database_bytes": 919494656}}. Free space 585.2 GiB. Item2 remains pending.

## Tocaccialli remaining8k mixes and Normalia8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes,
including recovery of Tocaccialli49,634 in its graph.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and585.2 GiB free. Matching pending profiles absent.
Current Best.txt Computer maxima match frozen comparison records:
Tocaccialli49,634; Normalia37,527. Normalia's cheat-flagged78,073 is excluded.
Deep-campaign guidance re-read, including Gayawaku-specific recovery limits.

1. In progress: Tocaccialli o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Tocaccialli o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Normalia o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Normalia8k mixes and additional
   town8k coverage; partial roots remain uncertified.

Tocaccialli continuation80/b8k: returncode0, score49,634 ties49,634. Processed/generated 115,378/462,207; native/wrapper elapsed 386.033/386.294 seconds; external peak RSS 179,308 KiB. search_complete=False; stop=None; new unique/solved 435,931/246,743; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Sturdy Saddle → Buy [13, 0, 0, 0, 0, 0] → Camel → Buy [43, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Trader → Camel → Buy [1, 2, 16, 0, 0, 2] → Sell → Buy [27, 0, 0, 0, 0, 8] → Sell → Interest → Interest

Tocaccialli balanced8k is now in progress.

Tocaccialli balanced50/20/b8k: returncode0, score49,634 ties49,634. Processed/generated 84,500/321,784; native/wrapper elapsed 430.747/430.918 seconds; external peak RSS 191,100 KiB. search_complete=False; stop=None; new unique/solved 289,021/309,644; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Sturdy Saddle → Buy [13, 0, 0, 0, 0, 0] → Camel → Buy [43, 0, 0, 0, 0, 0] → Trader → Witch - Midas was a Trader → Trader → Camel → Buy [1, 2, 16, 0, 0, 2] → Sell → Buy [27, 0, 0, 0, 0, 8] → Sell → Interest → Interest

Normalia optimistic8k is now in progress.

Normalia optimistic100/b8k: returncode0, score37,527 ties current Computer maximum37,527. Processed/generated 92,334/390,542; native/wrapper elapsed 439.416/439.531 seconds; external peak RSS 187,908 KiB. search_complete=False; stop=None; new unique/solved 347,009/283,609; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 8, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Three bounded mapped runs completed, adding1,071,961 unique nodes and839,996 solved nodes. All scores tie current town Computer records. Full routes and required metrics saved; native source unchanged. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Tocaccialli8k and Normalia8k:

1. In progress: revalidate terminal/completed metadata, no native solver and partial graph headers; inspect absence of Normalia8k continuation80 and balanced50/20 plus Huangoz8k optimistic profiles, including logs.
2. Pending: run the two Normalia mixes sequentially at540 seconds, atomic cap300,000 and384/288 MB. Preserve graph and record full paths, score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Huangoz optimistic100/continuation0 frontier8,000 under identical limits after successful exits. Stop on actual failure or above-current-town Computer score; broaden independent town coverage.
4. Pending: Huangoz remaining8k mixes and additional town coverage, plus wider retained profiles for previously covered towns, Gayawaku28k and Eduming16k. Partial roots remain uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: terminal/completed metadata, no native solver, remaining Normalia8k and Huangoz optimistic8k profiles absent including logs. Graph headers: {"Tocaccialli": {"nodes": 2703615, "open": 403615, "partial": 149415, "solved": 2150585, "root_status": "partial", "best_score": 49634, "database_bytes": 1530433536}, "Normalia": {"nodes": 1832118, "open": 266895, "partial": 99549, "solved": 1465674, "root_status": "partial", "best_score": 37527, "database_bytes": 1090707456}, "Huangoz": {"nodes": 1519547, "open": 219547, "partial": 81587, "solved": 1218413, "root_status": "partial", "best_score": 37527, "database_bytes": 943321088}}. Free space 584.2 GiB. Item2 remains pending.

## Normalia remaining8k mixes and Huangoz8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and584.2 GiB free. Matching pending profiles absent.
Current Best.txt Computer maxima match frozen comparison records:
Normalia37,527; Huangoz37,527. Normalia's cheat-flagged78,073 is excluded.
Deep-campaign guidance re-read, including Gayawaku-specific recovery limits.

1. In progress: Normalia o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Normalia o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Huangoz o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Huangoz8k mixes and additional
   town8k coverage; partial roots remain uncertified.

Normalia continuation80/b8k: returncode0, score37,527 ties37,527. Processed/generated 123,197/476,610; native/wrapper elapsed 281.069/281.218 seconds; external peak RSS 176,836 KiB. search_complete=False; stop=None; new unique/solved 432,895/250,713; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 8, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Normalia balanced8k is now in progress.

Normalia balanced50/20/b8k: returncode0, score37,527 ties37,527. Processed/generated 100,860/360,603; native/wrapper elapsed 360.451/360.696 seconds; external peak RSS 179,936 KiB. search_complete=False; stop=None; new unique/solved 313,509/293,267; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 8, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Huangoz optimistic8k is now in progress.

Huangoz optimistic100/b8k: returncode0, score37,527 ties current Computer maximum37,527. Processed/generated 94,613/401,264; native/wrapper elapsed 460.011/460.14 seconds; external peak RSS 189,704 KiB. search_complete=False; stop=None; new unique/solved 356,598/280,188; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 8, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Three bounded mapped runs completed, adding1,103,002 unique nodes and824,168 solved nodes. All scores tie current town Computer records. Full routes and required metrics saved; native source unchanged. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Normalia8k and Huangoz8k:

1. In progress: revalidate terminal/completed metadata, no native solver and partial graph headers; inspect absence of Huangoz8k continuation80 and balanced50/20 plus Fang-Mo8k optimistic profiles, including logs.
2. Pending: run the two Huangoz mixes sequentially at540 seconds, atomic cap300,000 and384/288 MB. Preserve graph and record full paths, score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Fang-Mo optimistic100/continuation0 frontier8,000 under identical limits after successful exits. Stop on actual failure or above-current-town Computer score; broaden independent town coverage.
4. Pending: Fang-Mo remaining8k mixes and additional town coverage, plus wider retained profiles for previously covered towns, Gayawaku28k and Eduming16k. Partial roots remain uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: terminal/completed metadata, no native solver, remaining Huangoz8k and Fang-Mo optimistic8k profiles absent including logs. Graph headers: {"Normalia": {"nodes": 2578522, "open": 413299, "partial": 155569, "solved": 2009654, "root_status": "partial", "best_score": 37527, "database_bytes": 1424625664}, "Huangoz": {"nodes": 1876145, "open": 276145, "partial": 101399, "solved": 1498601, "root_status": "partial", "best_score": 37527, "database_bytes": 1119027200}, "Fang-Mo": {"nodes": 1525489, "open": 224489, "partial": 85744, "solved": 1215256, "root_status": "partial", "best_score": 37569, "database_bytes": 945430528}}. Free space 583.2 GiB. Item2 remains pending.

## Huangoz remaining8k mixes and Fang-Mo8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and583.2 GiB free. Matching pending profiles absent.
Current Best.txt Computer maxima match frozen comparison records:
Huangoz37,527; Fang-Mo37,569. Fang-Mo cheat-flagged entries are excluded,
and its weighted starting pottery is retained without rule/model changes.
Deep-campaign guidance re-read, including Gayawaku-specific recovery limits.

1. In progress: Huangoz o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Huangoz o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Fang-Mo o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Fang-Mo8k mixes and additional
   town8k coverage; partial roots remain uncertified.

Huangoz continuation80/b8k: returncode0, score37,527 ties37,527. Processed/generated 123,807/480,510; native/wrapper elapsed 281.624/281.765 seconds; external peak RSS 175,572 KiB. search_complete=False; stop=None; new unique/solved 436,434/249,309; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 8, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Huangoz balanced8k is now in progress.

Huangoz balanced50/20/b8k: returncode0, score37,527 ties37,527. Processed/generated 98,349/352,939; native/wrapper elapsed 334.285/334.439 seconds; external peak RSS 189,700 KiB. search_complete=False; stop=None; new unique/solved 305,080/298,123; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 8, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Fang-Mo optimistic8k is now in progress.

Fang-Mo optimistic100/b8k: returncode0, score37,569 ties current Computer maximum37,569. Processed/generated 95,936/403,257; native/wrapper elapsed 495.77/495.955 seconds; external peak RSS 190,072 KiB. search_complete=False; stop=None; new unique/solved 351,222/281,117; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [10, 0, 0, 0, 0, 0] → Trader → Camel → Buy [27, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [41, 0, 0, 0, 0, 0] → Camel → Camel → Buy [5, 7, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Three bounded mapped runs completed, adding1,092,736 unique nodes and828,549 solved nodes. All scores tie current town Computer records. Full routes and required metrics saved; native source unchanged. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Huangoz8k and Fang-Mo8k:

1. In progress: revalidate terminal/completed metadata, no native solver and partial graph headers; inspect absence of Fang-Mo8k continuation80 and balanced50/20 plus Octoyashi8k optimistic profiles, including logs.
2. Pending: run the two Fang-Mo mixes sequentially at540 seconds, atomic cap300,000 and384/288 MB. Preserve graph and record full paths, score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Octoyashi optimistic100/continuation0 frontier8,000 under identical limits after successful exits. Stop on actual failure or above-current-town Computer score; broaden independent town coverage.
4. Pending: Octoyashi remaining8k mixes and additional town coverage, plus wider retained profiles for previously covered towns, Gayawaku28k and Eduming16k. Partial roots remain uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: terminal/completed metadata, no native solver, remaining Fang-Mo8k and Octoyashi optimistic8k profiles absent including logs. Graph headers: {"Huangoz": {"nodes": 2617659, "open": 417659, "partial": 153967, "solved": 2046033, "root_status": "partial", "best_score": 37527, "database_bytes": 1452355584}, "Fang-Mo": {"nodes": 1876711, "open": 275711, "partial": 104627, "solved": 1496373, "root_status": "partial", "best_score": 37569, "database_bytes": 1124634624}, "Octoyashi": {"nodes": 1524624, "open": 224624, "partial": 85604, "solved": 1214396, "root_status": "partial", "best_score": 40056, "database_bytes": 961179648}}. Free space 581.9 GiB. Item2 remains pending.

## Fang-Mo remaining8k mixes and Octoyashi8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and582.0 GiB free. Matching pending profiles absent.
Current Best.txt Computer maxima match frozen comparison records:
Fang-Mo37,569; Octoyashi40,056. Fang-Mo cheat-flagged entries are excluded,
and its weighted starting pottery remains unchanged.
Deep-campaign guidance re-read, including Gayawaku-specific recovery limits.

1. In progress: Fang-Mo o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Fang-Mo o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Octoyashi o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Octoyashi8k mixes and additional
   town8k coverage; partial roots remain uncertified.

Fang-Mo continuation80/b8k: returncode0, score37,569 ties37,569. Processed/generated 120,923/476,188; native/wrapper elapsed 278.204/278.346 seconds; external peak RSS 186,812 KiB. search_complete=False; stop=None; new unique/solved 440,252/251,311; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [10, 0, 0, 0, 0, 0] → Trader → Camel → Buy [27, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [41, 0, 0, 0, 0, 0] → Camel → Camel → Buy [5, 7, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Fang-Mo balanced8k is now in progress.

Fang-Mo balanced50/20/b8k: returncode0, score37,569 ties37,569. Processed/generated 98,278/357,210; native/wrapper elapsed 352.066/352.229 seconds; external peak RSS 189,516 KiB. search_complete=False; stop=None; new unique/solved 310,004/292,920; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [10, 0, 0, 0, 0, 0] → Trader → Camel → Buy [27, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [41, 0, 0, 0, 0, 0] → Camel → Camel → Buy [5, 7, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Octoyashi optimistic8k is now in progress.

Octoyashi optimistic100/b8k: returncode0, score40,056 ties current Computer maximum40,056. Processed/generated 100,372/404,328; native/wrapper elapsed 468.738/468.877 seconds; external peak RSS 189,664 KiB. search_complete=False; stop=None; new unique/solved 347,383/282,455; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Trader → Buy [14, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Camel → Buy [30, 0, 0, 0, 0, 0] → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [18, 16, 1, 0, 0, 1] → Sell → Buy [18, 0, 5, 0, 1, 4] → Sell → Buy [18, 0, 0, 0, 0, 7] → Sell → Interest

Three bounded mapped runs completed, adding1,097,639 unique nodes and826,686 solved nodes. All scores tie current town Computer records. Full routes and required metrics saved; native source unchanged. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Fang-Mo8k and Octoyashi8k:

1. In progress: revalidate terminal/completed metadata, no native solver and partial graph headers; inspect absence of Octoyashi8k continuation80 and balanced50/20 plus Kifuai8k optimistic profiles, including logs.
2. Pending: run the two Octoyashi mixes sequentially at540 seconds, atomic cap300,000 and384/288 MB. Preserve graph and record full paths, score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Kifuai optimistic100/continuation0 frontier8,000 under identical limits after successful exits. Stop on actual failure or above-current-town Computer score; broaden independent town coverage.
4. Pending: Kifuai remaining8k mixes and additional town coverage, plus wider retained profiles for previously covered towns, Gayawaku28k and Eduming16k. Partial roots remain uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: terminal/completed metadata, no native solver, remaining Octoyashi8k and Kifuai optimistic8k profiles absent including logs. Graph headers: {"Fang-Mo": {"nodes": 2626967, "open": 425967, "partial": 160396, "solved": 2040604, "root_status": "partial", "best_score": 37569, "database_bytes": 1459265536}, "Octoyashi": {"nodes": 1872007, "open": 272007, "partial": 103149, "solved": 1496851, "root_status": "partial", "best_score": 40056, "database_bytes": 1135919104}, "Kifuai": {"nodes": 1515946, "open": 215946, "partial": 83608, "solved": 1216392, "root_status": "partial", "best_score": 37547, "database_bytes": 952336384}}. Free space 580.6 GiB. Item2 remains pending.

## Octoyashi remaining8k mixes and Kifuai8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and580.6 GiB free. Matching pending profiles absent.
Current Best.txt Computer maxima match frozen comparison records:
Octoyashi40,056; Kifuai37,547.
Deep-campaign guidance re-read, including Gayawaku-specific recovery limits.

1. In progress: Octoyashi o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Octoyashi o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Kifuai o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Kifuai8k mixes and additional
   town8k coverage; partial roots remain uncertified.

Octoyashi continuation80/b8k: returncode0, score40,056 ties40,056. Processed/generated 119,485/472,766; native/wrapper elapsed 326.831/326.998 seconds; external peak RSS 174,464 KiB. search_complete=False; stop=None; new unique/solved 436,274/251,521; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Trader → Buy [14, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Camel → Buy [30, 0, 0, 0, 0, 0] → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [18, 16, 1, 0, 0, 1] → Sell → Buy [18, 0, 5, 0, 1, 4] → Sell → Buy [18, 0, 0, 0, 0, 7] → Sell → Interest

Octoyashi balanced8k is now in progress.

Octoyashi balanced50/20/b8k: returncode0, score40,056 ties40,056. Processed/generated 96,386/351,540; native/wrapper elapsed 373.047/373.238 seconds; external peak RSS 190,980 KiB. search_complete=False; stop=None; new unique/solved 304,110/300,419; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Trader → Buy [14, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Camel → Buy [30, 0, 0, 0, 0, 0] → Trader → Camel → Witch - Midas was a Trader → Camel → Buy [18, 16, 1, 0, 0, 1] → Sell → Buy [18, 0, 5, 0, 1, 4] → Sell → Buy [18, 0, 0, 0, 0, 7] → Sell → Interest

Kifuai optimistic8k is now in progress.

Kifuai optimistic100/b8k: returncode0, score37,547 ties current Computer maximum37,547. Processed/generated 100,753/416,262; native/wrapper elapsed 461.336/461.481 seconds; external peak RSS 193,092 KiB. search_complete=False; stop=None; new unique/solved 359,343/279,139; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 6, 10, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Three bounded mapped runs completed, adding1,099,727 unique nodes and831,079 solved nodes. All scores tie current town Computer records. Full routes and required metrics saved; native source unchanged. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Octoyashi8k and Kifuai8k:

1. In progress: revalidate terminal/completed metadata, no native solver and partial graph headers; inspect absence of Kifuai8k continuation80 and balanced50/20 plus Ipartus8k optimistic profiles, including logs.
2. Pending: run the two Kifuai mixes sequentially at540 seconds, atomic cap300,000 and384/288 MB. Preserve graph and record full paths, score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Ipartus optimistic100/continuation0 frontier8,000 under identical limits after successful exits. Stop on actual failure or above-current-town Computer score; broaden independent town coverage.
4. Pending: Ipartus remaining8k mixes and additional town coverage, plus wider retained profiles for previously covered towns, Gayawaku28k and Eduming16k. Partial roots remain uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: terminal/completed metadata, no native solver, remaining Kifuai8k and Ipartus optimistic8k profiles absent including logs. Graph headers: {"Octoyashi": {"nodes": 2612391, "open": 412391, "partial": 151209, "solved": 2048791, "root_status": "partial", "best_score": 40056, "database_bytes": 1472737280}, "Kifuai": {"nodes": 1875289, "open": 275289, "partial": 104469, "solved": 1495531, "root_status": "partial", "best_score": 37547, "database_bytes": 1138466816}, "Ipartus": {"nodes": 1518579, "open": 218579, "partial": 82168, "solved": 1217832, "root_status": "partial", "best_score": 37734, "database_bytes": 944926720}}. Free space 577.6 GiB. Item2 remains pending.

## Kifuai remaining8k mixes and Ipartus8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and577.6 GiB free. Matching pending profiles absent.
Current Best.txt Computer maxima match frozen comparison records:
Kifuai37,547; Ipartus37,734.
Deep-campaign guidance re-read, including Gayawaku-specific recovery limits.

1. In progress: Kifuai o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Kifuai o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Ipartus o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Ipartus8k mixes and additional
   town8k coverage; partial roots remain uncertified.

Kifuai continuation80/b8k: returncode0, score37,547 ties37,547. Processed/generated 125,580/490,071; native/wrapper elapsed 302.837/302.982 seconds; external peak RSS 177,776 KiB. search_complete=False; stop=None; new unique/solved 442,812/246,736; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 6, 10, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Kifuai balanced8k is now in progress.

Kifuai balanced50/20/b8k: returncode0, score37,547 ties37,547. Processed/generated 102,259/366,809; native/wrapper elapsed 482.286/482.615 seconds; external peak RSS 194,780 KiB. search_complete=False; stop=None; new unique/solved 313,205/293,962; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 6, 10, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Ipartus optimistic8k is now in progress.

Ipartus optimistic100/b8k: returncode0, score37,734 ties current Computer maximum37,734. Processed/generated 82,065/348,177; native/wrapper elapsed 546.253/546.523 seconds; external peak RSS 191,624 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 308,634/233,834; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 0, 15, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Three bounded mapped runs exited successfully, adding1,064,651 unique nodes and774,532 solved nodes. All scores tie current town Computer records. Full routes and required metrics saved; native source unchanged. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Kifuai8k and Ipartus8k:

1. In progress: revalidate terminal/completed metadata, no native solver and partial graph headers; inspect absence of Ipartus8k continuation80 and balanced50/20 plus Doba Lao8k optimistic profiles, including logs.
2. Pending: run the two Ipartus mixes sequentially at540 seconds, atomic cap300,000 and384/288 MB. Preserve graph and record full paths, score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Doba Lao optimistic100/continuation0 frontier8,000 under identical limits after successful exits. Stop on actual failure or above-current-town Computer score; broaden independent town coverage.
4. Pending: Doba Lao remaining8k mixes and additional town coverage, plus wider retained profiles for previously covered towns, Gayawaku28k and Eduming16k. Partial roots remain uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: terminal/completed metadata, no native solver, remaining Ipartus8k and Doba Lao optimistic8k profiles absent including logs. Graph headers: {"Kifuai": {"nodes": 2631306, "open": 431306, "partial": 163771, "solved": 2036229, "root_status": "partial", "best_score": 37547, "database_bytes": 1478463488}, "Ipartus": {"nodes": 1827213, "open": 274032, "partial": 101515, "solved": 1451666, "root_status": "partial", "best_score": 37734, "database_bytes": 1098514432}, "Doba Lao": {"nodes": 1495846, "open": 195846, "partial": 77076, "solved": 1222924, "root_status": "partial", "best_score": 45543, "database_bytes": 919789568}}. Free space 576.4 GiB. Ipartus optimistic8k stopped at its solver time budget with native exit0 and saved graph/route; no external timeout or failure. Item2 remains pending.

## Ipartus remaining8k mixes and Doba Lao8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Ipartus optimistic8k saved a partial graph at the solver time budget, exit0.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and576.4 GiB free. Matching pending profiles absent.
Current Best.txt Computer maxima match frozen comparison records:
Ipartus37,734; Doba Lao45,543.
Deep-campaign guidance re-read, including Gayawaku-specific recovery limits.

1. In progress: Ipartus o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Ipartus o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Doba Lao o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Doba Lao8k mixes and additional
   town8k coverage; partial roots remain uncertified.

Ipartus continuation80/b8k: returncode0, score37,734 ties37,734. Processed/generated 120,583/469,541; native/wrapper elapsed 426.028/426.402 seconds; external peak RSS 183,676 KiB. search_complete=False; stop=None; new unique/solved 427,569/255,076; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 0, 15, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Ipartus balanced8k is now in progress.

Ipartus balanced50/20/b8k: returncode0, score37,734 ties37,734. Processed/generated 97,940/353,635; native/wrapper elapsed 522.897/523.21 seconds; external peak RSS 194,668 KiB. search_complete=False; stop=None; new unique/solved 309,075/294,569; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 0, 15, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Doba Lao optimistic8k is now in progress.

Doba Lao optimistic100/b8k: returncode0, score45,543 ties current Computer maximum45,543. Processed/generated 93,005/366,309; native/wrapper elapsed 547.116/547.332 seconds; external peak RSS 192,860 KiB. search_complete=False; stop=internal time budget reached after 540 seconds; new unique/solved 319,055/259,625; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Camel → Camel → Buy [37, 1, 0, 0, 0, 3] → Sell → Interest → Interest → Interest

Three bounded mapped runs exited successfully, adding1,055,699 unique nodes and809,270 solved nodes. All scores tie current town Computer records. Full routes and required metrics saved; native source unchanged. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Ipartus8k and Doba Lao8k:

1. In progress: revalidate terminal/completed metadata, no native solver and partial graph headers; inspect absence of Doba Lao8k continuation80 and balanced50/20 plus Mahaji8k optimistic profiles, including logs.
2. Pending: run the two Doba Lao mixes sequentially at540 seconds, atomic cap300,000 and384/288 MB. Preserve graph and record full paths, score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Mahaji optimistic100/continuation0 frontier8,000 under identical limits after successful exits. Stop on actual failure or above-current-town Computer score; broaden independent town coverage.
4. Pending: Mahaji remaining8k mixes and additional town coverage, plus wider retained profiles for previously covered towns, Gayawaku28k and Eduming16k. Partial roots remain uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: terminal/completed metadata, no native solver, remaining Doba Lao8k and Mahaji optimistic8k profiles absent including logs. Graph headers: {"Ipartus": {"nodes": 2563857, "open": 410676, "partial": 151870, "solved": 2001311, "root_status": "partial", "best_score": 37734, "database_bytes": 1428287488}, "Doba Lao": {"nodes": 1814901, "open": 239097, "partial": 93255, "solved": 1482549, "root_status": "partial", "best_score": 45543, "database_bytes": 1091432448}, "Mahaji": {"nodes": 1518016, "open": 218016, "partial": 81307, "solved": 1218693, "root_status": "partial", "best_score": 37589, "database_bytes": 945086464}}. Free space 574.0 GiB. Doba Lao optimistic8k stopped at its internal540-second budget with native exit0 and saved graph/route; no external timeout or failure. Item2 remains pending.

## Doba Lao remaining8k mixes and Mahaji8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Doba Lao optimistic8k saved a partial graph at the internal time budget, exit0.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and574.0 GiB free. Matching pending profiles absent.
Current Best.txt Computer maxima match frozen comparison records:
Doba Lao45,543; Mahaji37,589.
Deep-campaign guidance re-read, including Gayawaku-specific recovery limits.

1. In progress: Doba Lao o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Doba Lao o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Mahaji o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Mahaji8k mixes and additional
   town8k coverage; partial roots remain uncertified.

Doba Lao continuation80/b8k: returncode0, score45,543 ties45,543. Processed/generated 115,909/431,852; native/wrapper elapsed 414.441/414.778 seconds; external peak RSS 174,388 KiB. search_complete=False; stop=None; new unique/solved 410,418/262,354; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Camel → Camel → Buy [37, 1, 0, 0, 0, 3] → Sell → Interest → Interest → Interest

Doba Lao balanced8k is now in progress.

Doba Lao balanced50/20/b8k: returncode0, score45,543 ties45,543. Processed/generated 86,689/318,653; native/wrapper elapsed 549.472/549.834 seconds; external peak RSS 193,328 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 281,671/295,097; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Camel → Camel → Buy [37, 1, 0, 0, 0, 3] → Sell → Interest → Interest → Interest

Mahaji optimistic8k is now in progress.

Mahaji optimistic100/b8k: returncode0, score37,589 ties current Computer maximum37,589. Processed/generated 76,823/323,323; native/wrapper elapsed 546.31/546.501 seconds; external peak RSS 181,040 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 286,871/206,411; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 12, 6, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Three bounded mapped runs exited successfully, adding978,960 unique nodes and763,862 solved nodes. All scores tie current town Computer records. Full routes and required metrics saved; native source unchanged. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Doba Lao8k and Mahaji8k:

1. In progress: revalidate terminal/completed metadata, no native solver and partial graph headers; inspect absence of Mahaji8k continuation80 and balanced50/20 plus Vilparino8k optimistic profiles, including logs.
2. Pending: run the two Mahaji mixes sequentially at540 seconds, atomic cap300,000 and384/288 MB. Preserve graph and record full paths, score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Vilparino optimistic100/continuation0 frontier8,000 under identical limits after successful exits. Stop on actual failure or above-current-town Computer score; broaden independent town coverage.
4. Pending: Vilparino remaining8k mixes and Moonin8k coverage, plus wider retained profiles for previously covered towns, Gayawaku28k and Eduming16k. Partial roots remain uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: terminal/completed metadata, no native solver, remaining Mahaji8k and Vilparino optimistic8k profiles absent including logs. Graph headers: {"Doba Lao": {"nodes": 2506990, "open": 342278, "partial": 124712, "solved": 2040000, "root_status": "partial", "best_score": 45543, "database_bytes": 1397665792}, "Mahaji": {"nodes": 1804887, "open": 277606, "partial": 102177, "solved": 1425104, "root_status": "partial", "best_score": 37589, "database_bytes": 1087488000}, "Vilparino": {"nodes": 1514149, "open": 214149, "partial": 82854, "solved": 1217146, "root_status": "partial", "best_score": 37569, "database_bytes": 949411840}}. Free space 571.5 GiB. Doba Lao balanced8k and Mahaji optimistic8k reached their solver time budgets with native exit0 and saved graphs/routes; no external timeout or failure. Item2 remains pending.

## Mahaji remaining8k mixes and Vilparino8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Mahaji optimistic8k saved a partial graph at its solver time budget, exit0.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and571.5 GiB free. Matching pending profiles absent.
Current Best.txt Computer maxima match frozen comparison records:
Mahaji37,589; Vilparino37,569.
Deep-campaign guidance re-read, including Gayawaku-specific recovery limits.

1. In progress: Mahaji o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Mahaji o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Vilparino o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Vilparino8k mixes and Moonin8k
   coverage; partial roots remain uncertified.

Mahaji continuation80/b8k: returncode0, score37,589 ties37,589. Processed/generated 120,671/474,740; native/wrapper elapsed 428.665/428.998 seconds; external peak RSS 164,940 KiB. search_complete=False; stop=None; new unique/solved 432,885/251,523; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 12, 6, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Mahaji balanced8k is now in progress.

Mahaji balanced50/20/b8k: returncode0, score37,589 ties37,589. Processed/generated 97,171/351,925; native/wrapper elapsed 521.557/521.856 seconds; external peak RSS 169,396 KiB. search_complete=False; stop=None; new unique/solved 304,451/297,373; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 12, 6, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Vilparino optimistic8k is now in progress.

Vilparino optimistic100/b8k: returncode0, score37,569 ties current Computer maximum37,569. Processed/generated 75,886/310,133; native/wrapper elapsed 547.34/547.622 seconds; external peak RSS 180,920 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 270,333/195,311; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 7, 10, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Three bounded mapped runs exited successfully, adding1,007,669 unique nodes and744,207 solved nodes. All scores tie current town Computer records. Full routes and required metrics saved; native source unchanged. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Mahaji8k and Vilparino8k:

1. In progress: revalidate terminal/completed metadata, no native solver and partial graph headers; inspect absence of Vilparino8k continuation80 and balanced50/20 plus Moonin8k optimistic profiles, including logs.
2. Pending: run the two Vilparino mixes sequentially at540 seconds, atomic cap300,000 and384/288 MB. Preserve graph and record full paths, score/count/RSS/time/stop/completion and unique/solved increments.
3. Pending: Moonin optimistic100/continuation0 frontier8,000 under identical limits after successful exits. Compare against current37,584, not historical37,527; stop on actual failure or above-current-town Computer score.
4. Pending: Moonin remaining8k mixes and full22-town8k inventory, plus wider retained profiles for previously covered towns, Gayawaku28k and Eduming16k. Partial roots remain uncertified; historical Gayawaku audit still eight pruned/partial families.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: terminal/completed metadata, no native solver, remaining Vilparino8k and Moonin optimistic8k profiles absent including logs. Graph headers: {"Mahaji": {"nodes": 2542223, "open": 414942, "partial": 153281, "solved": 1974000, "root_status": "partial", "best_score": 37589, "database_bytes": 1419149312}, "Vilparino": {"nodes": 1784482, "open": 268973, "partial": 103052, "solved": 1412457, "root_status": "partial", "best_score": 37569, "database_bytes": 1087021056}, "Moonin": {"nodes": 1694128, "open": 305586, "partial": 107828, "solved": 1280714, "root_status": "partial", "best_score": 37527, "database_bytes": 1051934720}}. Free space 570.0 GiB. Vilparino optimistic8k reached its solver time budget with native exit0 and saved graph/route; no external timeout or failure. Item2 remains pending.

## Vilparino remaining8k mixes and Moonin8k optimistic

Previous turn made progress: three bounded searches with saved metrics/routes.
Vilparino optimistic8k saved a partial graph at its solver time budget, exit0.
Current preflight confirms terminal/completed metadata, no native solver,
partial graphs and569.5 GiB free. Matching pending profiles absent.
Current Best.txt Computer maxima match frozen comparison records:
Vilparino37,569; Moonin37,584. Moonin graph best37,527 is below its current
record; rediscovering or recovering an existing record is not a new global win.
Deep-campaign guidance re-read, including Gayawaku-specific recovery limits.

1. In progress: Vilparino o0/c80 frontier8,000, atomic cap300,000,
   540-second solver budget, external/internal384/288 MB. Preserve graph.
2. Pending: Vilparino o50/c20 frontier8,000 under identical limits,
   after successful terminal exit.
3. Pending: Moonin o100/c0 frontier8,000 under identical limits,
   after successful terminal exit. Stop queue on failure or global candidate.
4. Pending: record all metrics, full routes and graph increments;
   diagnose actual failure before another launch. Verify/promote a winner
   through strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: remaining Moonin8k mixes and full22-town8k
   inventory; partial roots remain uncertified.

Vilparino continuation80/b8k: returncode0, score37,569 ties37,569. Processed/generated 123,305/479,377; native/wrapper elapsed 412.399/412.646 seconds; external peak RSS 186,868 KiB. search_complete=False; stop=None; new unique/solved 435,623/249,247; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 7, 10, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Vilparino balanced8k is now in progress.

Vilparino balanced50/20/b8k: returncode0, score37,569 ties37,569. Processed/generated 95,795/352,489; native/wrapper elapsed 462.756/463.011 seconds; external peak RSS 180,488 KiB. search_complete=False; stop=None; new unique/solved 299,645/299,862; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 7, 10, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Moonin optimistic8k is now in progress; compare against current37,584.

Moonin optimistic100/b8k: returncode0, score37,527, below current Computer maximum37,584 by57. Processed/generated 92,896/375,659; native/wrapper elapsed 545.133/545.36 seconds; external peak RSS 200,592 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 316,246/209,689; root remains partial. Full command, limits and counts in `results/more-records-20261009/persistent-ranking-summary.jsonl`. This is a saved partial result at the internal time budget, not an external timeout.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Magic Cleppsydra → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 8, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Three bounded mapped runs exited successfully, adding1,051,514 unique nodes and758,798 solved nodes. Both Vilparino scores tie its record; Moonin remains57 below its existing record. Full routes and required metrics saved; native source unchanged. No additional global record. Total new town records remains one: Moonin37,584 versus37,527 (+57). Goal remains active and unachieved.

Residual sweep and next plan after Vilparino8k and Moonin optimistic8k:

1. In progress: verify terminal/completed metadata and no native solver; audit exact22-town × three ranking mixes at frontier8,000/t540 and identify remaining lanes, including preexisting logs. Read Moonin/Vilparino headers and free space.
2. Pending: run Moonin o0/c80 and o50/c20 frontier8,000 sequentially, atomic cap300,000,540-second solver budget, external/internal384/288 MB. Compare against current37,584. Record full metrics and paths.
3. Pending: re-audit completed8k inventory, then preflight an untried wider profile: Petrinov10k, Gayawaku28k or Eduming16k. Independent width/ranking changes add coverage; partial roots remain uncertified.
4. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: {"metadata_status": "completed", "native_processes": [], "completed_8k_lanes": 64, "missing_8k_lanes": [{"town": "Moonin", "lane": "persistent-beam-o0-c80-n300k-timed540-b8000", "log_exists": false}, {"town": "Moonin", "lane": "persistent-beam-o50-c20-n300k-timed540-b8000", "log_exists": false}], "headers": {"Moonin": {"nodes": 2010374, "open": 382186, "partial": 137785, "solved": 1490403, "root_status": "partial", "best_score": 37527, "database_bytes": 1216679936}, "Vilparino": {"nodes": 2519750, "open": 404241, "partial": 153943, "solved": 1961566, "root_status": "partial", "best_score": 37569, "database_bytes": 1419608064}}, "free_gib": 567.5}. Remaining Moonin8k mixes are pending. Search exits are successful partial work; no optimality certificate or additional global record.

## Moonin remaining8k mixes and Petrinov10k optimistic

Previous goal turn made progress: observed the existing Moonin optimistic run to terminal exit0, recorded metrics/full route, and audited64/66 completed8k lanes. No global improvement.

Current preflight: terminal/completed metadata, no native solver, all three pending profiles and logs absent; current Best.txt maxima match frozen records: Moonin37,584, Petrinov45,578. Deep-campaign guidance re-read including Gayawaku profile. Graph headers: {"Moonin": {"nodes": 2010374, "open": 382186, "partial": 137785, "solved": 1490403, "root_status": "partial", "best_score": 37527, "database_bytes": 1216679936}, "Petrinov": {"nodes": 2560379, "open": 360379, "partial": 139877, "solved": 2060123, "root_status": "partial", "best_score": 45578, "database_bytes": 1441808384}}. Free space567.5 GiB.

1. In progress: Moonin o0/c80 b8k t540, cap300,000 atomic nodes,384/288 MB external/internal.
2. Pending: Moonin o50/c20 b8k t540 under identical limits, sequentially.
3. Pending: Petrinov o100/c0 b10k t540 under identical limits; wider frontier adds coverage. Stop on actual failure or global candidate.
4. Pending: record full paths, actual scores, native completion/stop status, timings, processed/generated states, external peak RSS, graph increments. Validate/promote any record with strict/direct replay and safe build/focused/full tests.
5. Pending residual sweep: verify66/66 all-town8k lanes, preflight remaining Petrinov10k mixes and further wider retained profiles. All partial roots remain uncertified.

Moonin continuation80/b8k: returncode0, score37,527, below current Computer maximum37,584 by57. Processed/generated 123,632/484,137; native/wrapper elapsed 312.806/312.981 seconds; external peak RSS 178,256 KiB. search_complete=False; stop=None; new unique/solved 438,860/247,934; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Magic Cleppsydra → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 8, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Moonin balanced8k is now in progress.

Moonin balanced50/20/b8k: returncode0, score37,527, below current Computer maximum37,584 by57. Processed/generated 109,533/390,138; native/wrapper elapsed 471.05/471.286 seconds; external peak RSS 186,556 KiB. search_complete=False; stop=None; new unique/solved 324,267/284,978; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Trader → Merch - Magic Cleppsydra → Camel → Witch - Midas was a Trader → Buy [43, 0, 0, 0, 0, 0] → Camel → Camel → Buy [2, 8, 9, 0, 1, 2] → Sell → Buy [20, 0, 0, 0, 0, 9] → Sell → Interest

Petrinov optimistic10k is now in progress.

8k inventory audit:66/66 exact town/ranking keys have native exit0 plus saved result and full route. Every persistent root remains uncertified; successful wrapper exit does not prove exhaustive completion. Petrinov10k optimistic remains active.

Petrinov optimistic100/b10k: returncode0, score45,578 ties current Computer maximum45,578. Processed/generated 108,507/419,550; native/wrapper elapsed 424.754/425.004 seconds; external peak RSS 206,036 KiB. search_complete=False; stop=None; new unique/solved 332,019/281,211; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [4, 0, 0, 0, 0, 0] → Interest → Buy [15, 0, 0, 0, 0, 0] → Interest → Interest → Merch - Blue Treasure → Interest → Interest → Witch - Oasis of Sanctifan → Interest → Interest → Interest → Interest → Interest → Interest → Interest

Three bounded mapped runs exited successfully, adding1,095,146 unique nodes and814,123 solved nodes. Both Moonin runs scored37,527, below its37,584 record; Petrinov tied45,578. Full routes and required metrics saved; native source unchanged. All66 town/ranking8k profiles now have successful saved results. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Moonin8k and Petrinov10k optimistic:

1. In progress: verify terminal/completed metadata, no native solver and partial graph headers; preflight exact absence of Petrinov10k continuation80 and balanced50/20 plus Gayawaku28k optimistic profiles and logs. Reconfirm current records and free space.
2. Pending: run Petrinov o0/c80 and o50/c20 frontier10,000 sequentially at540 seconds, cap300,000 atomic nodes,384/288 MB external/internal. Save score/count/RSS/time/completion/stop/full route and graph increments.
3. Pending: Gayawaku optimistic100/continuation0 frontier28,000 under identical bounds after successful exits; its wider retained mapped frontier adds coverage, unlike a timeout-only repetition. Read Gayawaku-specific deep-campaign guidance before launching.
4. Pending: remaining Gayawaku28k mixes, Eduming16k and wider independent town profiles; partial roots remain uncertified and historic Gayawaku families remain pruned/partial.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: {"metadata_status": "completed", "native_processes": [], "completed_8k_lanes": 66, "pending_profiles": [{"town": "Petrinov", "lane": "persistent-beam-o0-c80-n300k-timed540-b10000", "log_exists": false}, {"town": "Petrinov", "lane": "persistent-beam-o50-c20-n300k-timed540-b10000", "log_exists": false}, {"town": "Gayawaku", "lane": "persistent-beam-o100-c0-n300k-timed540-b28000", "log_exists": false}], "headers": {"Moonin": {"nodes": 2773501, "open": 545313, "partial": 204873, "solved": 2023315, "root_status": "partial", "best_score": 37527, "database_bytes": 1562877952}, "Petrinov": {"nodes": 2892398, "open": 392398, "partial": 158666, "solved": 2341334, "root_status": "partial", "best_score": 45578, "database_bytes": 1618628608}, "Gayawaku": {"nodes": 10998279, "open": 1171047, "partial": 405879, "solved": 9421353, "root_status": "partial", "best_score": 50105, "database_bytes": 11438051328}}, "free_gib": 563.5}. All roots remain partial. Current Best.txt Computer maxima: Moonin37,584, Petrinov45,578, Gayawaku50,105; no native model or record changes in this phase. Items2–5 remain pending.

## Petrinov remaining10k mixes and Gayawaku28k optimistic

Previous goal turn made progress: three bounded searches saved metrics/routes, all66/66 town/ranking8k keys audited, no global win. Preflight revalidated: terminal/completed metadata, no native solver, all pending profile rows/logs absent, current Best.txt Computer records match frozen Petrinov45,578 and Gayawaku50,105. Deep-campaign guidance re-read including Gayawaku hybrid memory recovery; this phase uses existing bounded persistent mapped search. Graph headers: {"Petrinov": {"nodes": 2892398, "open": 392398, "partial": 158666, "solved": 2341334, "root_status": "partial", "best_score": 45578, "database_bytes": 1618628608}, "Gayawaku": {"nodes": 10998279, "open": 1171047, "partial": 405879, "solved": 9421353, "root_status": "partial", "best_score": 50105, "database_bytes": 11438051328}}. Free space563.4 GiB.

1. In progress: Petrinov o0/c80 b10k t540, cap300,000 atomic nodes,384/288 MB external/internal.
2. Pending: Petrinov o50/c20 b10k t540 under identical limits, sequentially.
3. Pending: Gayawaku o100/c0 b28k t540 under identical limits; new width adds coverage. Stop queue on actual failure or above-current-town record.
4. Pending: record full paths, score, processed/generated states, external RSS, native/wrapper times, actual completion/stop status and graph increments. Replay/promote any winner through strict/direct checks and safe build/focused/full tests.
5. Pending residual sweep: remaining Gayawaku28k mixes, Eduming16k and wider independent town profiles. Partial roots remain uncertified.

Petrinov continuation80/b10k: returncode0, score45,578 ties current Computer maximum45,578. Processed/generated 114,534/398,461; native/wrapper elapsed 288.27/288.476 seconds; external peak RSS 182,932 KiB. search_complete=False; stop=None; new unique/solved 336,725/282,546; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [4, 0, 0, 0, 0, 0] → Interest → Buy [15, 0, 0, 0, 0, 0] → Interest → Interest → Merch - Blue Treasure → Interest → Interest → Witch - Oasis of Sanctifan → Interest → Interest → Interest → Interest → Interest → Interest → Interest

Petrinov balanced10k is now in progress.

Petrinov balanced50/20/b10k: returncode0, score45,578 ties current Computer maximum45,578. Processed/generated 91,221/347,519; native/wrapper elapsed 354.447/354.699 seconds; external peak RSS 200,440 KiB. search_complete=False; stop=None; new unique/solved 292,191/308,901; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [4, 0, 0, 0, 0, 0] → Interest → Buy [15, 0, 0, 0, 0, 0] → Interest → Interest → Merch - Blue Treasure → Interest → Interest → Witch - Oasis of Sanctifan → Interest → Interest → Interest → Interest → Interest → Interest → Interest

Gayawaku optimistic28k is now in progress.

Gayawaku optimistic100/b28k: returncode0, score50,105 ties current Computer maximum50,105. Processed/generated 71,107/303,392; native/wrapper elapsed 585.219/586.373 seconds; external peak RSS 310,912 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 260,567/231,788; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`. Observed run exceeded the requested internal540-second budget before saving; actual successful exit and elapsed figures above, no external timeout.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Three bounded mapped runs exited successfully, adding889,483 unique nodes and823,235 solved nodes. Petrinov45,578 and Gayawaku50,105 tie current Computer records. Full routes and required metrics saved; native source unchanged. No additional global record. Goal remains active and unachieved.

Residual sweep and next plan after Petrinov10k and Gayawaku28k optimistic:

1. In progress: verify terminal/completed metadata, no native solver and partial graph headers; preflight exact absence of Gayawaku28k continuation80 and balanced50/20 plus Eduming16k optimistic profiles and logs; inspect Gayawaku time-budget overshoot in saved output. Reconfirm records and free space.
2. Pending: run Gayawaku o0/c80 and o50/c20 frontier28,000 sequentially at540 seconds, cap300,000 atomic nodes,384/288 MB external/internal. Save all required metrics and full paths; retain outer602/604-second ceilings and stop on actual failure.
3. Pending: Eduming optimistic100/continuation0 frontier16,000 under identical bounds after successful exits. Wider retention adds coverage.
4. Pending: remaining Eduming16k mixes and wider independent town profiles; partial roots remain uncertified and historic Gayawaku families remain pruned/partial.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: {"metadata_status": "completed", "native_processes": [], "pending_profiles": [{"town": "Gayawaku", "lane": "persistent-beam-o0-c80-n300k-timed540-b28000", "log_exists": false}, {"town": "Gayawaku", "lane": "persistent-beam-o50-c20-n300k-timed540-b28000", "log_exists": false}, {"town": "Eduming", "lane": "persistent-beam-o100-c0-n300k-timed540-b16000", "log_exists": false}], "current_records": {"Gayawaku": 50105, "Eduming": 81321}, "headers": {"Eduming": {"nodes": 5661279, "open": 461279, "partial": 192134, "solved": 5007866, "root_status": "partial", "best_score": 81321, "database_bytes": 3014348800}, "Petrinov": {"nodes": 3521314, "open": 421314, "partial": 167219, "solved": 2932781, "root_status": "partial", "best_score": 45578, "database_bytes": 1915310080}, "Gayawaku": {"nodes": 11258846, "open": 1192724, "partial": 412981, "solved": 9653141, "root_status": "partial", "best_score": 50105, "database_bytes": 11665375232}}, "free_gib": 557.9, "gayawaku_stop_output": []}. Gayawaku saved stop reason is persistent search session time budget reached; exit0, native585.219/wrapper586.373 seconds, within enforced602/604-second external ceilings. Retain those safety ceilings for pending profiles. No failure diagnosis or native change required. Items2–5 remain pending.

## Gayawaku remaining28k mixes and Eduming16k optimistic

Previous goal turn made progress: three successful bounded searches saved full metrics/routes; Gayawaku optimistic28k saved partial work at its time budget after585.219 native seconds, within602/604-second enforced wrappers. No global win. Current preflight confirms terminal/completed metadata, no native solver, pending profiles/logs absent and current Best.txt Computer records match frozen Gayawaku50,105 and Eduming81,321. Deep-campaign guidance re-read including Gayawaku memory profile. Graph headers: {"Eduming": {"nodes": 5661279, "open": 461279, "partial": 192134, "solved": 5007866, "root_status": "partial", "best_score": 81321, "database_bytes": 3014348800}, "Gayawaku": {"nodes": 11258846, "open": 1192724, "partial": 412981, "solved": 9653141, "root_status": "partial", "best_score": 50105, "database_bytes": 11665375232}}. Free space558.0 GiB.

1. In progress: Gayawaku o0/c80 b28k t540, cap300,000 atomic nodes,384/288 MB external/internal; preserve602/604-second wrapper ceilings.
2. Pending: Gayawaku o50/c20 b28k t540 under identical bounds, sequentially.
3. Pending: Eduming o100/c0 b16k t540 under identical bounds. Stop on actual failure or above-current-town record.
4. Pending: record actual native completion/stop status, score, processed/generated counts, external RSS, native/wrapper times, full route and unique/solved graph increments. Strict/direct replay and safe build/focused/full tests for any winner.
5. Pending residual sweep: remaining Eduming16k mixes and additional wider independent town coverage. Partial roots and historic pruned Gayawaku families remain uncertified.

Gayawaku continuation80/b28k: returncode0, score50,105 ties current Computer maximum50,105. Processed/generated 76,746/336,819; native/wrapper elapsed 585.046/585.717 seconds; external peak RSS 310,420 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 301,401/255,034; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`. Successful saved partial work, no external timeout.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Gayawaku balanced28k is now in progress.

Gayawaku balanced50/20/b28k: returncode0, score50,105 ties current Computer maximum50,105. Processed/generated 61,815/265,237; native/wrapper elapsed 596.978/598.48 seconds; external peak RSS 310,568 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 235,994/246,447; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`. Successful saved partial work, no external timeout.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Trader → Buy [45, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Camel → Camel → Buy [17, 1, 10, 0, 1, 2] → Sell → Buy [30, 0, 1, 0, 0, 7] → Sell → Interest → Interest

Eduming optimistic16k is now in progress.

Eduming optimistic100/b16k: returncode0, score81,321 ties current Computer maximum81,321. Processed/generated 102,351/398,352; native/wrapper elapsed 448.16/449.46 seconds; external peak RSS 221,672 KiB. search_complete=False; stop=None; new unique/solved 334,520/280,717; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Three bounded mapped runs exited successfully, adding871,915 unique nodes and782,198 solved nodes. Gayawaku50,105 and Eduming81,321 tie current Computer records. Gayawaku mixes saved partial work at their internal time budgets with native exit0, within enforced wrapper ceilings; Eduming partial with stop=None. Full routes and required metrics saved; native source unchanged. No additional global record. Total new town records remains one: Moonin+57. Goal remains active and unachieved.

Residual sweep and next plan after Gayawaku28k and Eduming16k optimistic:

1. In progress: verify terminal/completed metadata, no native solver and partial graph headers; preflight exact absence of Eduming16k continuation80 and balanced50/20 plus Mehaz10k optimistic profiles and logs. Reconfirm current records and free space.
2. Pending: run Eduming o0/c80 and o50/c20 frontier16,000 sequentially at540 seconds, cap300,000 atomic nodes,384/288 MB external/internal; save full metrics/routes.
3. Pending: Mehaz optimistic100/continuation0 frontier10,000 under identical bounds after successful exits. Its wider retention adds independent town coverage.
4. Pending: remaining Mehaz10k mixes, further wider independent towns, and Gayawaku30k profiles after time/retention preflight. All roots remain partial, without optimality certificates.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: {"metadata_status": "completed", "native_processes": [], "pending_profiles": [{"town": "Eduming", "lane": "persistent-beam-o0-c80-n300k-timed540-b16000", "log_exists": false}, {"town": "Eduming", "lane": "persistent-beam-o50-c20-n300k-timed540-b16000", "log_exists": false}, {"town": "Mehaz", "lane": "persistent-beam-o100-c0-n300k-timed540-b10000", "log_exists": false}], "current_records": {"Eduming": 81321, "Mehaz": 48738}, "headers": {"Eduming": {"nodes": 5995799, "open": 495799, "partial": 211417, "solved": 5288583, "root_status": "partial", "best_score": 81321, "database_bytes": 3211603968}, "Gayawaku": {"nodes": 11796241, "open": 1223244, "partial": 418375, "solved": 10154622, "root_status": "partial", "best_score": 50105, "database_bytes": 12283215872}, "Mehaz": {"nodes": 2672755, "open": 472755, "partial": 169608, "solved": 2030392, "root_status": "partial", "best_score": 48738, "database_bytes": 1593421824}}, "free_gib": 553.4}. All roots remain partial. Gayawaku has all three28k profiles saved, with successful partial exits. Items2–5 remain pending.

## Eduming remaining16k mixes and Mehaz10k optimistic

Previous goal turn made progress: three bounded searches saved full metrics/routes, completing all Gayawaku28k mixes and Eduming optimistic16k. No global win. Current preflight revalidates terminal/completed metadata, no native solver, pending profile rows/logs absent and current Best.txt Computer records equal frozen Eduming81,321 and Mehaz48,738. Deep-campaign guidance re-read including Gayawaku profile. Graph headers: {"Eduming": {"nodes": 5995799, "open": 495799, "partial": 211417, "solved": 5288583, "root_status": "partial", "best_score": 81321, "database_bytes": 3211603968}, "Mehaz": {"nodes": 2672755, "open": 472755, "partial": 169608, "solved": 2030392, "root_status": "partial", "best_score": 48738, "database_bytes": 1593421824}}. Free space553.3 GiB.

1. In progress: Eduming o0/c80 b16k t540, cap300,000 atomic nodes,384/288 MB external/internal.
2. Pending: Eduming o50/c20 b16k t540 under identical bounds, sequentially.
3. Pending: Mehaz o100/c0 b10k t540 under identical bounds. Stop on actual failure or above-current-town Computer score.
4. Pending: save full route, score, processed/generated counts, external RSS, native/wrapper times, actual completion/stop status, unique/solved graph increments. Replay/promote any winner through strict/direct checks and safe build/focused/full tests.
5. Pending residual sweep: remaining Mehaz10k mixes and wider independent town coverage; all partial roots remain uncertified.

Eduming continuation80/b16k: returncode0, score81,321 ties current Computer maximum81,321. Processed/generated 102,223/362,888; native/wrapper elapsed 482.942/484.442 seconds; external peak RSS 216,652 KiB. search_complete=False; stop=None; new unique/solved 331,100/287,806; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Eduming balanced16k is now in progress.

Eduming balanced50/20/b16k: returncode0, score81,321 ties current Computer maximum81,321. Processed/generated 95,577/353,558; native/wrapper elapsed 488.526/491.061 seconds; external peak RSS 218,024 KiB. search_complete=False; stop=None; new unique/solved 307,515/297,488; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [12, 0, 0, 0, 0, 0] → Trader → Camel → Buy [26, 0, 0, 0, 0, 0] → Trader → Merch - Magic Cleppsydra → Trader → Trader → Witch - Midas was a Trader → Buy [28, 0, 0, 0, 0, 0] → Trader → Camel → Buy [37, 0, 0, 0, 0, 0] → Interest → Interest → Interest → Interest

Mehaz optimistic10k is now in progress.

Mehaz optimistic100/b10k: returncode0, score48,738 ties current Computer maximum48,738. Processed/generated 73,418/319,042; native/wrapper elapsed 563.98/564.607 seconds; external peak RSS 214,904 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 253,600/188,690; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Camel → Buy [5, 8, 0, 0, 0, 0] → Camel → Merch - Route to Mahaji → Sell → Buy [4, 10, 0, 2, 0, 0] → Sell → Buy [4, 17, 0, 0, 0, 1] → Witch - Camelization → Sell → Buy [4, 4, 0, 3, 0, 3] → Sell → Buy [6, 0, 0, 0, 0, 9] → Sell → Interest → Interest

Three bounded mapped runs exited successfully, adding892,215 unique nodes and773,984 solved nodes. Eduming81,321 and Mehaz48,738 tie current Computer records. Full routes and required metrics saved; native source unchanged. All three Eduming16k mixes saved. No additional global record. Total new town records remains one: Moonin+57. Goal remains active and unachieved.

Residual sweep and next plan after Eduming16k and Mehaz10k optimistic:

1. In progress: verify terminal/completed metadata, no native solver and partial graph headers; preflight exact absence of Mehaz10k continuation80 and balanced50/20 plus Bajaar10k optimistic profiles and logs. Reconfirm current records and free space.
2. Pending: run Mehaz o0/c80 and o50/c20 frontier10,000 sequentially at540 seconds, cap300,000 atomic nodes,384/288 MB external/internal; save full metrics/routes.
3. Pending: Bajaar optimistic100/continuation0 frontier10,000 under identical bounds after successful exits. Wider retention adds independent town coverage.
4. Pending: remaining Bajaar10k mixes and wider independent town profiles; Eduming18k and Gayawaku30k after time/retention preflight. All roots remain partial, without optimality certificates.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: {"metadata_status": "completed", "native_processes": [], "pending_profiles": [{"town": "Mehaz", "lane": "persistent-beam-o0-c80-n300k-timed540-b10000", "log_exists": false}, {"town": "Mehaz", "lane": "persistent-beam-o50-c20-n300k-timed540-b10000", "log_exists": false}, {"town": "Bajaar", "lane": "persistent-beam-o100-c0-n300k-timed540-b10000", "log_exists": false}], "current_records": {"Mehaz": 48738, "Bajaar": 56147}, "headers": {"Bajaar": {"nodes": 2802886, "open": 371311, "partial": 131854, "solved": 2299721, "root_status": "partial", "best_score": 56147, "database_bytes": 1681170432}, "Eduming": {"nodes": 6634414, "open": 534414, "partial": 226123, "solved": 5873877, "root_status": "partial", "best_score": 81321, "database_bytes": 3496751104}, "Mehaz": {"nodes": 2926355, "open": 521553, "partial": 185720, "solved": 2219082, "root_status": "partial", "best_score": 48738, "database_bytes": 1728147456}}, "free_gib": 552.0}. All roots remain partial. Mehaz optimistic10k saved partial work at its internal time budget with native exit0; native563.98/wrapper564.607 seconds, within enforced602/604-second ceilings. Items2–5 remain pending.

## Mehaz remaining10k mixes and Bajaar10k optimistic

Previous goal turn made progress: three successful bounded searches saved required metrics/full routes, all Eduming16k mixes completed. No global win. Current preflight confirms terminal/completed metadata, no native solver, pending profiles/logs absent and current Best.txt Computer records equal frozen Mehaz48,738 and Bajaar56,147. Deep-campaign guidance re-read including Gayawaku profile. Graph headers: {"Bajaar": {"nodes": 2802886, "open": 371311, "partial": 131854, "solved": 2299721, "root_status": "partial", "best_score": 56147, "database_bytes": 1681170432}, "Mehaz": {"nodes": 2926355, "open": 521553, "partial": 185720, "solved": 2219082, "root_status": "partial", "best_score": 48738, "database_bytes": 1728147456}}. Free space552.0 GiB.

1. In progress: Mehaz o0/c80 b10k t540, cap300,000 atomic nodes,384/288 MB external/internal.
2. Pending: Mehaz o50/c20 b10k t540 under identical bounds, sequentially.
3. Pending: Bajaar o100/c0 b10k t540 under identical bounds. Stop on actual failure or above-current-town Computer score.
4. Pending: save full route, score, processed/generated counts, external RSS, native/wrapper times, actual completion/stop status, unique/solved graph increments. Replay/promote any winner through strict/direct checks and safe build/focused/full tests.
5. Pending residual sweep: remaining Bajaar10k mixes and wider independent town coverage; all partial roots remain uncertified.

Mehaz continuation80/b10k: returncode0, score48,738 ties current Computer maximum48,738. Processed/generated 92,072/381,825; native/wrapper elapsed 442.214/442.476 seconds; external peak RSS 196,508 KiB. search_complete=False; stop=None; new unique/solved 339,822/284,514; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Camel → Buy [5, 8, 0, 0, 0, 0] → Camel → Merch - Route to Mahaji → Sell → Buy [4, 10, 0, 2, 0, 0] → Sell → Buy [4, 17, 0, 0, 0, 1] → Witch - Camelization → Sell → Buy [4, 4, 0, 3, 0, 3] → Sell → Buy [6, 0, 0, 0, 0, 9] → Sell → Interest → Interest

Mehaz balanced10k is now in progress.

Mehaz balanced50/20/b10k: returncode0, score48,738 ties current Computer maximum48,738. Processed/generated 88,919/381,556; native/wrapper elapsed 554.906/555.276 seconds; external peak RSS 200,100 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 307,857/296,601; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Camel → Buy [5, 8, 0, 0, 0, 0] → Camel → Merch - Route to Mahaji → Sell → Buy [4, 10, 0, 2, 0, 0] → Sell → Buy [4, 17, 0, 0, 0, 1] → Witch - Camelization → Sell → Buy [4, 4, 0, 3, 0, 3] → Sell → Buy [6, 0, 0, 0, 0, 9] → Sell → Interest → Interest

Bajaar optimistic10k is now in progress.

Bajaar optimistic100/b10k: returncode0, score56,147 ties current Computer maximum56,147. Processed/generated 70,154/291,561; native/wrapper elapsed 555.144/555.417 seconds; external peak RSS 193,564 KiB. search_complete=False; stop=persistent search session time budget reached; new unique/solved 244,666/173,403; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [7, 0, 0, 2, 0, 0] → Merch - Blue Treasure → Sell → Buy [10, 0, 2, 1, 1, 0] → Sell → Witch - Oasis of Sanctifan → Buy [0, 7, 1, 0, 0, 2] → Sell → Buy [0, 0, 0, 0, 0, 5] → Sell → Interest → Interest → Interest

Three bounded mapped runs exited successfully, adding892,345 unique nodes and754,518 solved nodes. Mehaz48,738 and Bajaar56,147 tie current Computer records. Full routes and required metrics saved; native source unchanged. All three Mehaz10k mixes saved. No additional global record. Total new town records remains one: Moonin+57. Goal remains active and unachieved.

Residual sweep and next plan after Mehaz10k and Bajaar10k optimistic:

1. In progress: verify terminal/completed metadata, no native solver and partial graph headers; preflight exact absence of Bajaar10k continuation80 and balanced50/20 plus L'Exquise10k optimistic profiles and logs. Reconfirm current records and free space.
2. Pending: run Bajaar o0/c80 and o50/c20 frontier10,000 sequentially at540 seconds, cap300,000 atomic nodes,384/288 MB external/internal; save full metrics/routes.
3. Pending: L'Exquise optimistic100/continuation0 frontier10,000 under identical bounds after successful exits. Wider retention adds independent town coverage.
4. Pending: remaining L'Exquise10k mixes and wider independent towns; Eduming18k and Gayawaku30k after time/retention preflight. All roots remain partial, without optimality certificates.
5. Pending on a global candidate: strict/direct replay, promotion, safe build/focused/full tests and fresh completion audit. None exists.

Next-plan item1 completed: {"metadata_status": "completed", "native_processes": [], "pending_profiles": [{"town": "Bajaar", "lane": "persistent-beam-o0-c80-n300k-timed540-b10000", "log_exists": false}, {"town": "Bajaar", "lane": "persistent-beam-o50-c20-n300k-timed540-b10000", "log_exists": false}, {"town": "L'Exquise", "lane": "persistent-beam-o100-c0-n300k-timed540-b10000", "log_exists": false}], "current_records": {"Bajaar": 56147, "L'Exquise": 56817}, "headers": {"L'Exquise": {"nodes": 2536892, "open": 455869, "partial": 143096, "solved": 1937927, "root_status": "partial", "best_score": 56817, "database_bytes": 1518055424}, "Bajaar": {"nodes": 3047552, "open": 422015, "partial": 152413, "solved": 2473124, "root_status": "partial", "best_score": 56147, "database_bytes": 1824649216}, "Mehaz": {"nodes": 3574034, "open": 569406, "partial": 204431, "solved": 2800197, "root_status": "partial", "best_score": 48738, "database_bytes": 2046750720}}, "free_gib": 551.3}. Best.txt uses L_Exquise for L'Exquise; normalized explicitly after the read-only preflight parser rejected the unmatched name. All roots remain partial. Mehaz balanced10k and Bajaar optimistic10k saved partial work at internal time budgets with native exit0; no external timeout. Items2–5 remain pending.

## Bajaar remaining10k mixes and L'Exquise10k optimistic

Previous goal turn made progress: three successful bounded searches saved full metrics/routes, all Mehaz10k mixes completed. No global win. Current preflight confirms terminal/completed metadata, no native solver, pending profiles/logs absent and current Best.txt Computer records equal frozen Bajaar56,147 and L'Exquise56,817. Best.txt L_Exquise header normalized explicitly. Deep-campaign guidance re-read including Gayawaku profile. Graph headers: {"L'Exquise": {"nodes": 2536892, "open": 455869, "partial": 143096, "solved": 1937927, "root_status": "partial", "best_score": 56817, "database_bytes": 1518055424}, "Bajaar": {"nodes": 3047552, "open": 422015, "partial": 152413, "solved": 2473124, "root_status": "partial", "best_score": 56147, "database_bytes": 1824649216}}. Free space551.3 GiB.

1. In progress: Bajaar o0/c80 b10k t540, cap300,000 atomic nodes,384/288 MB external/internal.
2. Pending: Bajaar o50/c20 b10k t540 under identical bounds, sequentially.
3. Pending: L'Exquise o100/c0 b10k t540 under identical bounds. Stop on actual failure or above-current-town Computer score.
4. Pending: save full route, score, processed/generated counts, external RSS, native/wrapper times, actual completion/stop status, unique/solved graph increments. Replay/promote any winner through strict/direct checks and safe build/focused/full tests.
5. Pending residual sweep: remaining L'Exquise10k mixes and wider independent town coverage; all partial roots remain uncertified.

Bajaar continuation80/b10k: returncode0, score56,147 ties current Computer maximum56,147. Processed/generated 102,792/390,117; native/wrapper elapsed 549.943/550.193 seconds; external peak RSS 195,896 KiB. search_complete=False; stop=None; new unique/solved 347,383/280,934; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [7, 0, 0, 2, 0, 0] → Merch - Blue Treasure → Sell → Buy [10, 0, 2, 1, 1, 0] → Sell → Witch - Oasis of Sanctifan → Buy [0, 7, 1, 0, 0, 2] → Sell → Buy [0, 0, 0, 0, 0, 5] → Sell → Interest → Interest → Interest

Bajaar balanced10k is now in progress.

Bajaar balanced50/20/b10k: returncode0, score56,147 ties current Computer maximum56,147. Processed/generated 82,483/325,824; native/wrapper elapsed 494.843/495.086 seconds; external peak RSS 205,816 KiB. search_complete=False; stop=None; new unique/solved 286,817/308,896; root remains partial. Full command and limits in `results/more-records-20261009/persistent-ranking-summary.jsonl`.

Reproducible route: Trader → Buy [13, 0, 0, 0, 0, 0] → Trader → Camel → Buy [7, 0, 0, 2, 0, 0] → Merch - Blue Treasure → Sell → Buy [10, 0, 2, 1, 1, 0] → Sell → Witch - Oasis of Sanctifan → Buy [0, 7, 1, 0, 0, 2] → Sell → Buy [0, 0, 0, 0, 0, 5] → Sell → Interest → Interest → Interest

L'Exquise optimistic10k is now in progress.
