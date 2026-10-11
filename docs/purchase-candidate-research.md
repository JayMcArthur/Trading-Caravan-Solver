# Purchase-candidate generation: exact storage before pruning

## Recommendation

Yes: first build a canonical, inspectable store of strategically different purchase baskets, then use tests to decide how aggressively to reduce it. The strongest design for this solver is a hybrid:

1. generate baskets once, by item type, so permutations are impossible;
2. retain exact nondominated labels over spend, weight, and each reachable future-price outcome;
3. at query time select a small set of named anchors; and
4. only after measuring frontier size and route quality, add epsilon-box compression or a hard beam width.

This ordering gives pruning experiments an exact oracle. It also separates two questions that the current single-best query blends together: which baskets are genuinely redundant, and how many useful alternatives the main game search can afford.

## Problem model

For the five non-food item counts \(x\in\mathbb Z_{\ge 0}^5\):

\[
c\cdot x\le G\le 50{,}000,\qquad w\cdot x\le W\le 360.
\]

For every reachable future-price scenario \(s\), store an additive outcome such as liquidation gain

\[
p_s(x)=\sum_i (sell_{s,i}-buy_i)x_i.
\]

Unspent gold is already represented by minimizing `cost`; at a particular query it is `reserve = G - cost`. A label `A` safely dominates `B` when `A.cost <= B.cost`, `A.weight <= B.weight`, and `A.p_s >= B.p_s` for every retained scenario, with at least one strict comparison. Because all quantities and objectives are additive, every extension feasible from `B` is feasible from `A`. If later rules introduce history-dependent bonuses or item synergies, their sufficient state must be added before this dominance remains safe.

The current resource grid has `(50,001 * 361) = 18,050,361` cells per valuation before storing baskets or multiple labels. A label frontier can be much smaller in practice, but its worst case is output-sensitive and should be measured rather than assumed.

## What the primary sources imply

- Exact Pareto dynamic programming is established for integer/unbounded knapsack. Rong and Figueira generate the complete bi-objective integer-knapsack frontier; their earlier reduction DP explicitly models each decision variable as a nonnegative item count. Klamroth and Wiecek cover multiple criteria and multiple constraints. These support type-staged labels plus dominance reduction, instead of storing purchase-order paths. [Rong & Figueira 2014](https://doi.org/10.1016/j.ejor.2013.11.032), [Rong & Figueira 2013](https://doi.org/10.1016/j.ejor.2013.05.045), [Klamroth & Wiecek 2000](https://doi.org/10.1002/%28SICI%291520-6750%28200002%2947%3A1%3C57%3A%3AAID-NAV4%3E3.0.CO%3B2-4)

- Forward DP also applies to multidimensional unbounded knapsack, although He, Hartman, and Pardalos combine it with valid inequalities for speed. For this project, such bounds should come after an exact oracle because an unsafe bound can erase a strategic basket. [He, Hartman & Pardalos 2016](https://doi.org/10.3233/INF-2016-1098)

- Exact Pareto sets may be too large; epsilon-dominance supplies a principled bounded representation. Laumanns et al. partition objective space into boxes, retain representatives of nondominated boxes, allow a different epsilon per coordinate, and discuss additive tolerances. Their archive has bounded size and controlled spread. Papadimitriou and Yannakakis prove the broader result that polynomially succinct epsilon-approximate Pareto curves exist under bounded-value and solvability conditions. [Laumanns et al. 2002, author PDF](https://www.sop.tik.ee.ethz.ch/publicationListFiles/ltdz2002a.pdf), [DOI](https://doi.org/10.1162/106365602760234108), [Papadimitriou & Yannakakis 2000](https://doi.org/10.1109/SFCS.2000.892068)

- A bounded multiobjective beam should have a vector-valued oracle and a Pareto-efficient filter. Barthelemy et al. formalize this oracle/filter split, show that width controls runtime, and apply it to integer multiobjective knapsack. This maps cleanly to five item-type levels, with quantity branches at each level. [Barthelemy et al. 2015, primary manuscript](https://optimization-online.org/wp-content/uploads/2015/04/4879.pdf)

- Future prices should remain separate scenario axes until final selection. Botte and Schobel formalize multi-scenario efficiency by treating each scenario as an objective. Kleywegt and Papastavrou's stochastic-knapsack DP gives remaining capacity a continuation value and derives a marginal reservation threshold; the analogous solver principle is to value retained gold/capacity by what it enables later, rather than by a fixed penalty. [Botte & Schobel 2019](https://doi.org/10.1016/j.ejor.2018.08.020), [Kleywegt & Papastavrou 1998](https://doi.org/10.1287/opre.46.1.17)

- Objective-space spread alone does not ensure strategically different inventories: different decisions can map to the same objective vector. After objective pruning, preserve composition diversity using normalized five-item count shares. [Liu et al. 2020, author PDF](https://yiping0liu.github.io/pdf/CPDEA.pdf), [DOI](https://doi.org/10.1109/TEVC.2019.2938557)

## Ranked designs for this codebase

The estimates below are engineering bounds for the current limits, not claims from the cited papers.

| Rank | Design | Guarantee and output | Expected runtime | Expected memory |
| ---: | --- | --- | --- | --- |
| **1** | **Scenario x reserve query portfolio over the existing compact tables** | For each reachable price world, query exact scalar optima at 6-10 meaningful reserve thresholds: zero, next-action affordability shortfalls, and a few gap-filling thresholds. Union, deduplicate, then always retain scenario maxima, a reserve-rich near-best basket, a maximin/minimax-regret basket, and 1-3 composition-farthest alternatives. Exact for every issued scalar query, but can miss unsupported multi-scenario compromises. | Usually 36-60 existing tree lookups per food choice; lookup cost is small compared with solver-state expansion. | Almost no new persistent memory; tens of candidate records. Reuses the current 3-8 MB compact tables per loaded world. |
| **2** | **Exact type-staged, all-scenario Pareto store** | Process the five item types once. At stage `i`, branch quantity `0..min(gold/c_i, weight/w_i)`, merge identical labels, and Pareto-prune on `(min cost, min weight, max p_1..p_S)`. This is the best correctness oracle and preserves unsupported trade-offs. Query-time output is the named 5-16 anchors above. | With current positive weights, a type has at most about 91 quantity choices. Runtime is output-sensitive and can grow sharply with scenario count; use a sorted skyline/index, not quadratic survivor scans. | Roughly 36-48 bytes per compact six-scenario label: about 4 MB per 100k labels or 40 MB per 1M, plus temporary merge space. Exact high-dimensional fronts can be much larger, so instrument frontier size per stage/world. |
| **3** | **Type-staged epsilon-Pareto beam** | Same five-level generator, but exact-dominance first, then one representative per additive/multiplicative epsilon box; explicitly protect every scenario extreme, reserve milestone, and composition niche. Bounded and deterministic, with a clear approximation knob, but no exact route guarantee once boxes or width discard labels. | For beam `B`, at most roughly `5 * 91 * B` raw extensions: about 58k at `B=128`, 233k at `B=512`, and 466k at `B=1024`, before cheap hash-box rejection. | Active labels are tiny; transient extensions are normally under a few MB for `B<=1024`. Suitable once exact-oracle comparisons establish tolerances. |
| **4** | **Recourse-scored frontier** | Score a basket in scenario `s` as immediate value plus `Q_s(gold_left, weight_left, game_context)`. This directly prices cash reserve and spare capacity instead of preserving arbitrary percentages. It is strategically strongest if a reliable continuation-value cache exists, but it couples purchase generation to the main solver. | Approximately `candidate_count * scenario_count` continuation lookups; prohibitively expensive if each `Q_s` launches search rather than hitting a memoized table. | Potentially large because the cache key includes future game context, not only gold and weight. Use initially as an offline evaluator of Designs 1-3, not the generator. |

## Concrete storage and selection details

- Canonical basket: five `uint8` counts are sufficient while capacity is 360 and all supported non-food weights remain positive; keep cost/weight in `uint16` and each scenario outcome in signed `int32`.
- Preserve raw scenario outcomes in saved records. Do not save only a weighted average: later experiments can change robust/expected/regret selection without rebuilding baskets.
- Use additive boxes for reserve and zero-crossing profit coordinates (for example, $25 gold and a measured payoff tolerance). Multiplicative log boxes are appropriate only for strictly positive relative objectives.
- Within the same epsilon box, prefer ordinary dominance; otherwise keep the better worst-scenario value, then the composition farther from already selected baskets. Always pin extremes before applying a global cap.
- Evaluate pruning by recall against the exact Design 2 oracle on small and representative states: scenario-optimum recall, reserve-milestone recall, composition spread, emitted candidate count, downstream best score, runtime, and peak RSS.

## Suggested implementation order

1. Add a `PurchaseCandidate` record and vector-returning interface without removing the existing single-best path.
2. Implement Design 1 as the low-risk baseline and log why each basket was retained.
3. Build Design 2 behind an offline/test flag for small and then full states; serialize its exact labels if sizes are acceptable.
4. Compare Design 1 against the exact oracle, then introduce Design 3 and sweep epsilon/beam sizes.
5. Prune only after route-quality tests identify which anchors, scenarios, and reserve levels are redundant.

