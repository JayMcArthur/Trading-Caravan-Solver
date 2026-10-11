# Normal-mode record campaign

Targets are the highest non-cheat historical/local scores in `Best.txt`.
Search results are bounded discoveries, not proofs of global optimality. A bot
route is promoted only after the solver's independent `Game` replay agrees with
the symbolic result.

| Town | Target | Best bot | Gap | Status |
| --- | ---: | ---: | ---: | --- |
| Normalia | 35,114 | 37,527 | +2,413 | beaten |
| Fang-Mo | 34,918 | 37,569 | +2,651 | beaten; weighted starting pottery |
| L'Exquise | 56,817 | 56,817 | 0 | met |
| Mahaji | 34,859 | 37,589 | +2,730 | beaten |
| Tocaccialli | 46,083 | 49,634 | +3,551 | beaten |
| Bajaar | 56,146 | 56,147 | +1 | beaten; verified no-continue route |
| Doba Lao | 44,631 | 45,543 | +912 | beaten |
| Huangoz | 34,848 | 37,527 | +2,679 | beaten |
| Port-Willam | 42,296 | 42,296 | 0 | tied |
| Moonin | 36,732 | 37,584 | +852 | beaten; late Cleppsydra polish |
| Cornecopea | 43,696 | 43,881 | +185 | beaten |
| Eduming | 80,043 | 81,321 | +1,278 | beaten |
| Octoyashi | 40,056 | 40,056 | 0 | met |
| Kifuai | 36,589 | 37,547 | +958 | beaten; fixture saved |
| Skjollird | 43,446 | 47,065 | +3,619 | beaten; fixture saved |
| Petrinov | 44,447 | 45,578 | +1,131 | beaten |
| Jilliqo | 37,291 | 37,819 | +528 | beaten |
| Gayawaku | 50,104 | 50,105 | +1 | beaten; fixture saved |
| Vilparino | 36,631 | 37,569 | +938 | beaten |
| Sanctifan | 51,130 | 55,728 | +4,598 | beaten; polished fixture saved |
| Mehaz | 41,853 | 48,738 | +6,885 | beaten |
| Ipartus | 36,295 | 37,734 | +1,439 | beaten |

## Chronological reproducible findings

The current scores are in the table above. Values below describe the campaign
stage when each experiment ran and may have been superseded by later sweeps.

- A sequential exact-replay polishing screen on 2026-10-09 improved Moonin
  from 37,527 to 37,584 by delaying Magic Cleppsydra until after the day-12
  cargo Buy. The winning run evaluated 1,000,000 candidates at 179 MB RSS under
  a 384 MB hard cap. See `benchmarks/LightRecordSearch2026-10-09.md`.

- The Magic Cleppsydra-only all-town hybrid sweep produced 11 new records and
  tied six more saved bests while staying below 104 MB RSS. Its complete matrix,
  configuration, and five non-Cleppsydra winners are in
  `benchmarks/ClepsydraAllTownSweep2026-09-01.md`.
- Sequential Sturdy Saddle and Blue Treasure sweeps then tested the strongest
  alternative Merchant lanes at the same width. Sturdy tied only Tocaccialli;
  Blue tied L'Exquise, Bajaar, and Petrinov. Neither produced a new record.
  See `benchmarks/AlternativeMerchantAllTownSweeps2026-09-01.md`.
- Route to Mahaji subsequently tied only Mehaz in an all-town pinned sweep.
  Focused Blue Sanctifan widening, ranking, diversity, and learned-refinement
  policies all plateaued at 55,445. The driver now supports configurable
  learned refinement on open-timing pinned lanes. See
  `benchmarks/NextSearchExperiments2026-09-01.md`.
- Exact-replay route polishing subsequently tested local food, cargo, crew,
  encounter, and action-order mutations without admitting illegal routes.
  Sanctifan improved from 55,602 to 55,728. Normalia remained at 37,527 after
  1,797,262 candidates across three seeds; its Merchant and Witch are already
  on their earliest useful days. See `benchmarks/RoutePolishing2026-09-02.md`.

- The sold-demand ledger made an all-town beam-5,000 screen practical. It
  discovered replay-verified improvements for Fang-Mo (35,163), Tocaccialli
  (49,634), Huangoz (35,114), Cornecopea (43,713), and Jilliqo (37,759).
- A beam-20,000 screen repeated every established ordinary-town floor and found
  a structurally different Mehaz route scoring 48,738. It uses Route to Mahaji,
  Camelization, and five cargo sales rather than the common Cleppsydra/Midas
  route.
- Port-Willam's day matrix tied the human 42,296 with Magic Cleppsydra on day 7
  and Midas on day 9 at beam 5,000.
- Petrinov's Witch-option screen beat the human record with 45,578: Blue
  Treasure on day 7, Oasis of Sanctifan on day 9, then seven consecutive
  Interest actions. The independent `Game` replay matches exactly.
- At that stage, Bajaar was the only below-target town at 49,827 versus 56,146. Beam
  20,000, all Merchant days 5-7, Witch days 9-11, all Merchant choices at the
  strongest later schedule, all Witch choices at the strongest Merchant lane,
  optimism shares 0/25/50/75/100, and both legacy/Sell-aware scoring did not
  improve it. Beam 50,000 stopped safely on its internal memory projection,
  and 20%/40% action-structure diversity at beam 20,000 also repeated 49,827.

- Skjollird 43,475: beam 500, 25% optimistic, 5.6 seconds, 48 MB.
- Kifuai 36,589: beam 500, 50% optimistic, 1.8 seconds, 34 MB.
- Sanctifan 51,130: prefix-guided search after the first 11 saved actions,
  beam 50,000, with no beam pruning in the suffix. The independent replay
  exactly matches the human record. Its late route uses two cargo sales and
  three Interest actions; Magic Cleppsydra supplies the extra day and
  Sanctifan makes the first Interest action free.
- Bajaar 49,827: beam 5,000, 25% optimistic, 23.5 seconds, 119 MB. This
  beats both 49,621 historical entries but remains below 56,146.
- Port-Willam's former 41,344 day-6 floor was superseded by the day-7 route
  above.
- Petrinov's former 41,672 Blue Treasure/Midas floor was superseded by the
  Blue Treasure/Oasis route above.
- Gayawaku 50,105: Merchant day 7/Witch day 9, isolated Magic/Midas, beam 500,
  25% optimistic, 29.0 seconds, 78 MB. This beats the recorded 50,104 by one.
- Removing the forced opening Trader at beam 500 did not improve Bajaar,
  Port-Willam, Kifuai, Petrinov, or Gayawaku. Sanctifan changed only from
  36,652 to 36,654 under the default 25% split. The shared route structure was
  therefore a search result, not merely a hidden prefix restriction.
- The experimental Sell-aware scorer matched the legacy scorer for all five
  non-Gayawaku gap towns at beam 500 and 25%.
- A 20-40% structural-diversity slice did not improve the sampled gap towns,
  so it remains opt-in and disabled by default.
- First-eligible Merchant-isolated beam-1,000 searches reproduced Bajaar 49,827,
  Port-Willam 38,300, and Petrinov 41,672 with much smaller beams. Alternative
  merchant choices sampled for Bajaar and Port-Willam remained below those
  floors; Gayawaku's isolated Magic lane produced the 46,325 improvement.
- At first-eligible timing, fixing both the best Merchant and Midas Witch choices let beam-5,000 runs
  reproduce Bajaar, Port-Willam, and Petrinov without improvement. Gayawaku
  beam 2,000 also repeated 46,325 at 183 MB; beam 5,000 stopped safely at the
  internal memory estimate before producing a finalist.
- All nine Merchant choices were screened at Port-Willam's winning day-6/day-9
  schedule with beam 200. Magic Cleppsydra remained best at 41,344; the next
  closest sampled choice was Wooden Statuette at 36,027.
- Bajaar beam 10,000 repeated 49,827 at 182 MB. Beam 20,000 was stopped safely
  by the 512 MB internal estimate before finishing; the process was at 234 MB
  resident and did not crash.
- Hybrid frontier draining later completed 20,000- and 50,000-prefix Bajaar
  lanes at bounded memory. A 1.70-million-state run still tied 49,827, so the
  next quality fork is distinct early action skeletons rather than more
  undifferentiated width. See `benchmarks/HybridFrontierResults.md`.
- Archived unrestricted-cargo runs do not explain the former five gaps. Their best
  stored results were below the current locked/deferred floors: Bajaar 45,978,
  Port-Willam 37,535, Gayawaku 38,724, and Sanctifan 34,373. No archived
  unrestricted Petrinov result exceeded the current 41,672. Reopening repeated
  cargo Buys is therefore not the next supported experiment.

Raw JSONL is in `benchmarks/results/record-gaps-sell-aware-500.jsonl`,
`record-gaps-open-500.jsonl`, `record-gaps-share-500.jsonl`, and
`sanctifan-focused.jsonl`. The complete 36-scenario timing screen is in
`encounter-days-b200.jsonl`.

## Next hypotheses

1. Build a bounded early-action-skeleton bank for Bajaar, then use exact
   prefix-guided suffix search from each survivor. This targets crew/order and
   early-interest diversity directly instead of widening the same scalar beam.
2. Keep 49,827 as a replay fixture and require every candidate skeleton to beat
   it before promotion.
3. Do not introduce pruning DFS until Buy coverage and the terminal upper bound
   are proven admissible; the current layered beam is memory-bounded, but a
   pruning DFS could silently discard the winner.
