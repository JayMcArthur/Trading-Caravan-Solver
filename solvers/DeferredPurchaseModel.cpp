#include "DeferredPurchaseModel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <functional>
#include <queue>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include "../game/Game.h"

namespace {
constexpr int MAX_BUY_COUNT = std::numeric_limits<uint8_t>::max();

}

DeferredPurchaseModel::DeferredPurchaseModel(
    std::shared_ptr<const Player> buy_snapshot,
    Player common_player,
    std::vector<Origin> origin_rows,
    const size_t input_seed_count
) : buy_market(std::move(buy_snapshot)),
    common(std::move(common_player)),
    origins(std::make_shared<std::vector<Origin>>(std::move(origin_rows))),
    seeds_before_compaction(input_seed_count) {}

DeferredPurchaseModel DeferredPurchaseModel::open(
    const Player& before_buy,
    const std::vector<DeferredPurchaseSeed>& seeds
) {
    if (seeds.empty()) {
        throw std::invalid_argument("Deferred purchase requires at least one origin");
    }

    for (const DeferredPurchaseSeed& seed : seeds) {
        if (seed.gold < 0 || seed.food < 0) {
            throw std::invalid_argument("Deferred purchase origin cannot have negative resources");
        }
    }

    std::vector<size_t> selected_seed_indices;
    if (seeds.size() == 1) {
        selected_seed_indices.push_back(0);
    } else {
        const int food_price = before_buy.item_shop[i_Food][id_Buy];
        const int food_weight = before_buy.item_shop[i_Food][id_Weight];
        std::vector<int> maximum_total_food(seeds.size(), 0);
        int largest_total_food = 0;
        for (size_t index = 0; index < seeds.size(); ++index) {
            Player concrete = before_buy;
            concrete.gold = seeds[index].gold;
            concrete.food = seeds[index].food;
            const int free_weight = Game::check_weight_left(concrete);
            const int by_gold = food_price <= 0
                ? MAX_BUY_COUNT
                : seeds[index].gold / food_price;
            const int by_weight = food_weight <= 0
                ? MAX_BUY_COUNT
                : std::max(0, free_weight) / food_weight;
            const int new_food = std::max(0, std::min({MAX_BUY_COUNT, by_gold, by_weight}));
            maximum_total_food[index] = seeds[index].food + new_food;
            largest_total_food = std::max(largest_total_food, maximum_total_food[index]);
        }

        struct FoodInterval {
            int start = 0;
            int end = 0;
            int64_t adjusted_gold = 0;
            size_t seed_index = 0;
        };
        std::vector<FoodInterval> intervals;
        std::vector<int> breakpoints;
        intervals.reserve(seeds.size());
        breakpoints.reserve(seeds.size() * 2U);
        for (size_t index = 0; index < seeds.size(); ++index) {
            intervals.push_back(FoodInterval{
                seeds[index].food,
                maximum_total_food[index],
                static_cast<int64_t>(seeds[index].gold) +
                    static_cast<int64_t>(food_price) * seeds[index].food,
                index,
            });
            breakpoints.push_back(seeds[index].food);
            if (maximum_total_food[index] < largest_total_food) {
                breakpoints.push_back(maximum_total_food[index] + 1);
            }
        }
        std::sort(intervals.begin(), intervals.end(), [](const FoodInterval& lhs, const FoodInterval& rhs) {
            return std::tie(lhs.start, lhs.seed_index) < std::tie(rhs.start, rhs.seed_index);
        });
        std::sort(breakpoints.begin(), breakpoints.end());
        breakpoints.erase(std::unique(breakpoints.begin(), breakpoints.end()), breakpoints.end());

        struct BetterInterval {
            bool operator()(const FoodInterval* lhs, const FoodInterval* rhs) const noexcept {
                if (lhs->adjusted_gold != rhs->adjusted_gold) {
                    return lhs->adjusted_gold < rhs->adjusted_gold;
                }
                return lhs->seed_index > rhs->seed_index;
            }
        };
        std::priority_queue<const FoodInterval*, std::vector<const FoodInterval*>, BetterInterval> active;
        std::vector<uint8_t> selected(seeds.size(), 0);
        size_t next_interval = 0;
        for (const int total_food : breakpoints) {
            while (next_interval < intervals.size() && intervals[next_interval].start <= total_food) {
                active.push(&intervals[next_interval]);
                ++next_interval;
            }
            while (!active.empty() && active.top()->end < total_food) {
                active.pop();
            }
            if (!active.empty()) {
                selected[active.top()->seed_index] = 1;
            }
        }
        for (size_t index = 0; index < seeds.size(); ++index) {
            if (selected[index]) {
                selected_seed_indices.push_back(index);
            }
        }
    }

    const int income = end_of_day_income(before_buy);
    std::vector<Origin> origins;
    origins.reserve(selected_seed_indices.size());
    for (const size_t seed_index : selected_seed_indices) {
        const DeferredPurchaseSeed& seed = seeds[seed_index];
        Player concrete = before_buy;
        concrete.gold = seed.gold;
        concrete.food = seed.food;
        origins.push_back(Origin{
            DeferredGoldFunction::open(seed.gold, income, seed.gold),
            seed.food,
            Game::check_weight_left(concrete),
            seed.parent_trace,
        });
    }

    Player common = before_buy;
    common.gold = seeds.front().gold;
    common.food = seeds.front().food;
    Game::event_buy_items(common, buy_list());
    DeferredPurchaseModel model(
        std::make_shared<const Player>(before_buy),
        std::move(common),
        std::move(origins),
        seeds.size()
    );
    model.prune_infeasible_origins();
    if (model.origin_rows().empty()) {
        throw std::invalid_argument("Deferred purchase has no feasible allocation");
    }
    model.synchronize_representative();
    return model;
}

const Player& DeferredPurchaseModel::representative() const noexcept {
    return common;
}

size_t DeferredPurchaseModel::input_seed_count() const noexcept {
    return seeds_before_compaction;
}

size_t DeferredPurchaseModel::origin_count() const noexcept {
    return origin_rows().size();
}

size_t DeferredPurchaseModel::memory_bytes() const noexcept {
    std::unordered_set<const void*> counted;
    return sizeof(*this) + dynamic_memory_bytes(counted);
}

size_t DeferredPurchaseModel::dynamic_memory_bytes(
    std::unordered_set<const void*>& counted_shared
) const noexcept {
    size_t bytes = 0;
    if (counted_shared.insert(buy_market.get()).second) {
        bytes += sizeof(Player) + 2U * sizeof(void*);
    }
    if (counted_shared.insert(origins.get()).second) {
        bytes += sizeof(std::vector<Origin>) +
            origin_rows().capacity() * sizeof(Origin) + 2U * sizeof(void*);
        for (const Origin& origin : origin_rows()) {
            bytes += origin.gold.memory_bytes() - sizeof(DeferredGoldFunction);
        }
    }
    bytes += trace.dynamic_memory_bytes(counted_shared);
    return bytes;
}

bool DeferredPurchaseModel::semantically_equal(const DeferredPurchaseModel& other) const noexcept {
    if (cumulative_food_used != other.cumulative_food_used ||
        minimum_food_coverage != other.minimum_food_coverage ||
        origin_rows().size() != other.origin_rows().size()) {
        return false;
    }
    for (int item = 0; item <= i_Jewelry; ++item) {
        if (buy_market->item_shop[item][id_Buy] != other.buy_market->item_shop[item][id_Buy] ||
            buy_market->item_shop[item][id_Weight] != other.buy_market->item_shop[item][id_Weight]) {
            return false;
        }
    }
    for (size_t index = 0; index < origin_rows().size(); ++index) {
        const Origin& lhs = origin_rows()[index];
        const Origin& rhs = other.origin_rows()[index];
        if (!(lhs.gold == rhs.gold) || lhs.food_before_buy != rhs.food_before_buy ||
            lhs.free_weight_before_buy != rhs.free_weight_before_buy) {
            return false;
        }
    }
    return true;
}

size_t DeferredPurchaseModel::semantic_hash() const noexcept {
    size_t seed = static_cast<size_t>(cumulative_food_used);
    const auto combine = [&seed](const auto value) {
        using Value = std::decay_t<decltype(value)>;
        seed ^= std::hash<Value>{}(value) + 0x9e3779b97f4a7c15ULL +
                (seed << 6U) + (seed >> 2U);
    };
    combine(minimum_food_coverage);
    for (int item = 0; item <= i_Jewelry; ++item) {
        combine(buy_market->item_shop[item][id_Buy]);
        combine(buy_market->item_shop[item][id_Weight]);
    }
    for (const Origin& origin : origin_rows()) {
        combine(origin.gold.semantic_hash());
        combine(origin.food_before_buy);
        combine(origin.free_weight_before_buy);
    }
    return seed;
}

size_t DeferredPurchaseModel::dominance_partition_hash() const noexcept {
    size_t seed = static_cast<size_t>(cumulative_food_used);
    const auto combine = [&seed](const auto value) {
        using Value = std::decay_t<decltype(value)>;
        seed ^= std::hash<Value>{}(value) + 0x9e3779b97f4a7c15ULL +
                (seed << 6U) + (seed >> 2U);
    };
    for (int item = 0; item <= i_Jewelry; ++item) {
        combine(buy_market->item_shop[item][id_Buy]);
        combine(buy_market->item_shop[item][id_Weight]);
    }
    combine(buy_market->spice);
    combine(buy_market->pottery);
    combine(buy_market->marble);
    combine(buy_market->silk);
    combine(buy_market->jewelry);
    return seed;
}

bool DeferredPurchaseModel::dominates(
    const DeferredPurchaseModel& other,
    uint64_t* gold_function_comparisons,
    uint64_t* exact_spend_evaluations,
    uint64_t* origin_coverage_checks,
    uint64_t* successful_cross_shape_checks
) const noexcept {
    if (dominance_partition_hash() != other.dominance_partition_hash() ||
        cumulative_food_used != other.cumulative_food_used ||
        minimum_food_coverage > other.minimum_food_coverage) {
        return false;
    }
    for (int item = 0; item <= i_Jewelry; ++item) {
        if (buy_market->item_shop[item][id_Buy] != other.buy_market->item_shop[item][id_Buy] ||
            buy_market->item_shop[item][id_Weight] != other.buy_market->item_shop[item][id_Weight]) {
            return false;
        }
    }
    if (buy_market->spice != other.buy_market->spice ||
        buy_market->pottery != other.buy_market->pottery ||
        buy_market->marble != other.buy_market->marble ||
        buy_market->silk != other.buy_market->silk ||
        buy_market->jewelry != other.buy_market->jewelry) {
        return false;
    }
    bool identical_shape = origin_rows().size() == other.origin_rows().size();
    if (identical_shape) {
        for (size_t index = 0; index < origin_rows().size(); ++index) {
            if (origin_rows()[index].food_before_buy !=
                    other.origin_rows()[index].food_before_buy ||
                origin_rows()[index].free_weight_before_buy !=
                    other.origin_rows()[index].free_weight_before_buy) {
                identical_shape = false;
                break;
            }
        }
    }

    const int food_price = buy_market->item_shop[i_Food][id_Buy];
    const int food_weight = buy_market->item_shop[i_Food][id_Weight];
    for (const Origin& weaker : other.origin_rows()) {
        bool covered = false;
        const int weaker_first_food = other.minimum_food_units(weaker);
        const int weaker_last_food = other.maximum_useful_food_units(weaker);
        for (const Origin& stronger : origin_rows()) {
            if (origin_coverage_checks != nullptr) {
                ++*origin_coverage_checks;
            }
            const int food_shift = weaker.food_before_buy -
                stronger.food_before_buy;
            const int stronger_first_food = weaker_first_food + food_shift;
            const int stronger_last_food = weaker_last_food + food_shift;
            if (stronger_first_food < minimum_food_units(stronger) ||
                stronger_last_food > maximum_useful_food_units(stronger) ||
                stronger_first_food < 0 ||
                stronger.free_weight_before_buy - food_shift * food_weight <
                    weaker.free_weight_before_buy) {
                continue;
            }

            const int spend_shift = food_shift * food_price;
            const int weaker_minimum_spend = weaker_first_food * food_price;
            if (weaker_minimum_spend + spend_shift < 0 ||
                weaker.gold.maximum_original_spend() + spend_shift >
                    stronger.gold.maximum_original_spend()) {
                continue;
            }

            if (gold_function_comparisons != nullptr) {
                ++*gold_function_comparisons;
            }
            bool gold_covered = true;
            for (int spend = weaker_minimum_spend;
                 spend <= weaker.gold.maximum_original_spend();
                 ++spend) {
                if (exact_spend_evaluations != nullptr) {
                    ++*exact_spend_evaluations;
                }
                if (stronger.gold.evaluate(spend + spend_shift) <
                    weaker.gold.evaluate(spend)) {
                    gold_covered = false;
                    break;
                }
            }
            if (gold_covered) {
                covered = true;
                break;
            }
        }
        if (!covered) {
            return false;
        }
    }
    if (!identical_shape && successful_cross_shape_checks != nullptr) {
        ++*successful_cross_shape_checks;
    }
    return true;
}

DeferredBuyCoverage DeferredPurchaseModel::classify_exact_buy(
    const buy_list& buy
) const noexcept {
    const std::array<int, 6> quantities{
        buy.food,
        buy.spice,
        buy.pottery,
        buy.marble,
        buy.silk,
        buy.jewelry,
    };
    int total_cost = 0;
    int total_weight = 0;
    for (int item = i_Food; item <= i_Jewelry; ++item) {
        total_cost += quantities[static_cast<size_t>(item)] *
            buy_market->item_shop[item][id_Buy];
        total_weight += quantities[static_cast<size_t>(item)] *
            buy_market->item_shop[item][id_Weight];
    }

    bool safely_replaced = false;
    for (const Origin& origin : origin_rows()) {
        const int food = static_cast<int>(buy.food);
        if (food < minimum_food_units(origin) ||
            food > maximum_food_units(origin) ||
            total_cost > origin.gold.maximum_original_spend() ||
            total_weight > origin.free_weight_before_buy) {
            continue;
        }
        if (food <= maximum_useful_food_units(origin)) {
            return DeferredBuyCoverage::Exact;
        }
        // Food beyond the maximum future demand has no score value and can
        // only consume historical gold/capacity. The model intentionally
        // represents it by the otherwise-identical lower-food allocation.
        safely_replaced = true;
    }
    return safely_replaced
        ? DeferredBuyCoverage::DominatingReplacement
        : DeferredBuyCoverage::Missing;
}

const DeferredTraceRecipe& DeferredPurchaseModel::trace_recipe() const noexcept {
    return trace;
}

EarlyActionSkeleton DeferredPurchaseModel::extend_skeleton(EarlyActionSkeleton skeleton) const noexcept {
    skeleton = skeleton.extended(SolverAction{_a_buy, buy_list(), 0});
    for (const SolverAction& action : trace.actions_after_buy()) {
        skeleton = skeleton.extended(action);
    }
    return skeleton;
}

std::vector<uint32_t> DeferredPurchaseModel::provenances() const {
    std::vector<uint32_t> result;
    result.reserve(origin_rows().size());
    for (const Origin& origin : origin_rows()) {
        result.push_back(origin.parent_trace);
    }
    return result;
}

DeferredPurchaseModel DeferredPurchaseModel::remap_provenances(
    const std::unordered_map<uint32_t, uint32_t>& remap
) const {
    DeferredPurchaseModel result = *this;
    for (Origin& origin : result.mutable_origins()) {
        const auto found = remap.find(origin.parent_trace);
        origin.parent_trace = found == remap.end() ? 0 : found->second;
    }
    return result;
}

std::optional<DeferredPurchaseModel> DeferredPurchaseModel::after_start_of_day() const {
    DeferredPurchaseModel next = *this;
    if (next.common.day != 0) {
        next.cumulative_food_used += next.common.food_consumption;
        next.minimum_food_coverage = std::max(
            next.minimum_food_coverage,
            next.cumulative_food_used
        );
    }
    ++next.common.day;
    next.prune_infeasible_origins();
    if (next.origin_rows().empty()) {
        return std::nullopt;
    }
    next.synchronize_representative();
    return next;
}

DeferredPurchaseModel DeferredPurchaseModel::after_resume_same_day() const {
    DeferredPurchaseModel next = *this;
    next.common.day_start_skip = false;
    next.synchronize_representative();
    return next;
}

std::optional<DeferredPurchaseModel> DeferredPurchaseModel::after_hire(const npcs selection) const {
    DeferredPurchaseModel next = *this;
    if (next.common.day >= next.common.max_day) {
        return std::nullopt;
    }
    next.constrain_remaining_food(
        next.common.food_consumption + next.common.npc_shop[selection][nd_Eat]
    );
    const int cost = next.common.npc_shop[selection][nd_Cost];
    std::vector<Origin> affordable;
    affordable.reserve(next.origin_rows().size());
    for (Origin origin : next.origin_rows()) {
        if (origin.gold.spend_if_affordable(cost)) {
            affordable.push_back(std::move(origin));
        }
    }
    next.origins = std::make_shared<std::vector<Origin>>(std::move(affordable));

    Player mutated = next.common;
    mutated.gold = std::max(cost, 0);
    mutated.food = std::numeric_limits<int>::max() / 4;
    Game::event_buy_npc(mutated, selection);
    next.common = std::move(mutated);
    next.add_gold_to_origins(end_of_day_income(next.common));
    next.prune_infeasible_origins();
    if (next.origin_rows().empty()) {
        return std::nullopt;
    }
    next.trace.append(SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(selection)});
    next.synchronize_representative();
    return next;
}

std::optional<DeferredPurchaseModel> DeferredPurchaseModel::after_interest() const {
    DeferredPurchaseModel next = *this;
    if (next.common.day != next.common.max_day) {
        next.constrain_remaining_food(next.common.food_consumption);
    }
    next.prune_infeasible_origins();
    if (next.origin_rows().empty()) {
        return std::nullopt;
    }

    const int rate_hundredths = static_cast<int>(std::lround(next.common.interest_rate * 100.0));
    for (Origin& origin : next.mutable_origins()) {
        origin.gold.apply_interest(rate_hundredths);
    }
    const bool quick_interest = next.common.quick_interest;
    Player mutated = next.common;
    mutated.gold = 0;
    Game::event_interest(mutated);
    next.common = std::move(mutated);
    if (!quick_interest) {
        next.add_gold_to_origins(end_of_day_income(next.common));
    }
    next.trace.append(SolverAction{_a_interest, buy_list(), 0});
    next.synchronize_representative();
    return next;
}

std::optional<DeferredPurchaseModel> DeferredPurchaseModel::after_merchant(
    const merch_options selection
) const {
    DeferredPurchaseModel next = *this;
    const int cost = merchant_cost(next.common, selection);
    std::vector<Origin> affordable;
    affordable.reserve(next.origin_rows().size());
    for (Origin origin : next.origin_rows()) {
        if (cost == 0 || origin.gold.spend_if_affordable(cost)) {
            affordable.push_back(std::move(origin));
        }
    }
    next.origins = std::make_shared<std::vector<Origin>>(std::move(affordable));
    next.prune_infeasible_origins();
    if (next.origin_rows().empty()) {
        return std::nullopt;
    }

    Player mutated = next.common;
    mutated.gold = std::numeric_limits<int>::max() / 4;
    if (!Game::event_merchant(mutated, selection)) {
        return std::nullopt;
    }
    next.common = std::move(mutated);
    next.trace.append(SolverAction{_a_merchant, buy_list(), static_cast<uint8_t>(selection)});
    next.synchronize_representative();
    return next;
}

DeferredPurchaseModel DeferredPurchaseModel::after_witch(const witch_options selection) const {
    DeferredPurchaseModel next = *this;
    Game::event_witch(next.common, selection);
    next.trace.append(SolverAction{_a_witch, buy_list(), static_cast<uint8_t>(selection)});
    next.synchronize_representative();
    return next;
}

std::vector<DeferredPurchaseResolution> DeferredPurchaseModel::resolve_food_only_for_next_buy() const {
    std::vector<DeferredPurchaseResolution> rows;
    for (const Origin& origin : origin_rows()) {
        const int minimum_food = minimum_food_units(origin);
        const int maximum_food = maximum_useful_food_units(origin);
        for (int food = minimum_food; food <= maximum_food; ++food) {
            buy_list buy(static_cast<uint8_t>(food), 0, 0, 0, 0, 0);
            const int spend = food * buy_market->item_shop[i_Food][id_Buy];
            Player player = common;
            player.gold = origin.gold.evaluate(spend);
            player.food = origin.food_before_buy + food - cumulative_food_used;
            player.cargo_buy_locked = false;
            rows.push_back(DeferredPurchaseResolution{
                std::move(player),
                buy,
                origin.parent_trace,
                trace.resolve(buy),
            });
        }
    }
    return physical_frontier(
        std::move(rows), buy_market->item_shop[i_Food][id_Weight] == 0
    );
}

std::vector<DeferredPurchaseResolution> DeferredPurchaseModel::resolve_sell(
    BestPurchaseHandler& handler
) const {
    DeferredPurchaseModel legal = *this;
    if (legal.common.day != legal.common.max_day) {
        legal.constrain_remaining_food(legal.common.food_consumption);
    }
    legal.prune_infeasible_origins();

    struct Candidate {
        const Origin* origin = nullptr;
        int food = 0;
        int cargo_cost_limit = 0;
        int cargo_weight_limit = 0;
    };
    std::vector<Candidate> candidates;
    std::unordered_map<int, int> maximum_cost_by_weight;
    for (const Origin& origin : legal.origin_rows()) {
        const int minimum_food = legal.minimum_food_units(origin);
        // Food has no final score. On the final day, every unit above the
        // accumulated survival minimum only spends historical gold and, when
        // weighted, consumes cargo capacity. Deferred gold operations are
        // monotone, so extra food cannot improve the terminal Sell result.
        const int maximum_food = legal.common.day == legal.common.max_day
            ? minimum_food
            : legal.maximum_useful_food_units(origin);
        for (int food = minimum_food; food <= maximum_food; ++food) {
            const int food_cost = food * legal.buy_market->item_shop[i_Food][id_Buy];
            const int food_weight = food * legal.buy_market->item_shop[i_Food][id_Weight];
            const int cargo_cost_limit = origin.gold.maximum_original_spend() - food_cost;
            const int cargo_weight_limit = origin.free_weight_before_buy - food_weight;
            if (cargo_cost_limit < 0 || cargo_weight_limit < 0) {
                continue;
            }
            candidates.push_back(Candidate{
                &origin,
                food,
                cargo_cost_limit,
                cargo_weight_limit,
            });
            const int weight_key = std::min(cargo_weight_limit, MAX_WEIGHT);
            maximum_cost_by_weight[weight_key] = std::max(
                maximum_cost_by_weight[weight_key],
                cargo_cost_limit
            );
        }
    }

    const auto sale_market = handler.prepareExactSaleMarket(
        *legal.buy_market,
        legal.common
    );
    std::unordered_map<
        int,
        std::shared_ptr<const std::vector<ExactPurchaseChoice>>
    > frontier_by_weight;
    frontier_by_weight.reserve(maximum_cost_by_weight.size());
    for (const auto& [weight, maximum_cost] : maximum_cost_by_weight) {
        frontier_by_weight.emplace(
            weight,
            sale_market.bucket_frontier(
                static_cast<uint16_t>(std::min(maximum_cost, MAX_GOLD)),
                static_cast<uint16_t>(weight)
            )
        );
    }

    std::vector<DeferredPurchaseResolution> rows;
    rows.reserve(MAX_BUY_COUNT + 1);
    std::unordered_map<int, size_t> row_by_remaining_food;
    row_by_remaining_food.reserve(MAX_BUY_COUNT + 1);
    const SolverAction sell_action{_a_sell, buy_list(), 0};
    for (const Candidate& input : candidates) {
        const auto frontier = frontier_by_weight.find(
            std::min(input.cargo_weight_limit, MAX_WEIGHT)
        );
        if (frontier == frontier_by_weight.end() || !frontier->second) {
            continue;
        }
        auto choice = legal.resolve_sell_candidate(
            *input.origin,
            input.food,
            *frontier->second
        );
        if (!choice.has_value()) {
            continue;
        }
        const auto existing = row_by_remaining_food.find(choice->player.food);
        if (existing != row_by_remaining_food.end() &&
            rows[existing->second].player.gold >= choice->player.gold) {
            continue;
        }
        choice->actions = legal.trace.resolve(choice->concrete_buy, &sell_action);
        if (existing == row_by_remaining_food.end()) {
            row_by_remaining_food.emplace(choice->player.food, rows.size());
            rows.push_back(std::move(*choice));
        } else {
            rows[existing->second] = std::move(*choice);
        }
    }
    return physical_frontier(
        std::move(rows), legal.common.item_shop[i_Food][id_Weight] == 0
    );
}

std::optional<DeferredPurchaseResolution>
DeferredPurchaseModel::resolve_sell_exact_remaining_food(
    BestPurchaseHandler& handler,
    const int remaining_food
) const {
    if (remaining_food < 0) {
        return std::nullopt;
    }
    struct Candidate {
        const Origin* origin = nullptr;
        int food = 0;
        int cargo_cost_limit = 0;
        int cargo_weight_limit = 0;
    };
    std::vector<Candidate> candidates;
    std::unordered_map<int, int> maximum_cost_by_weight;
    for (const Origin& origin : origin_rows()) {
        const int food = remaining_food + cumulative_food_used - origin.food_before_buy;
        if (food < minimum_food_units(origin) || food > maximum_useful_food_units(origin)) {
            continue;
        }
        const int cargo_cost_limit = origin.gold.maximum_original_spend() -
            food * buy_market->item_shop[i_Food][id_Buy];
        const int cargo_weight_limit = origin.free_weight_before_buy -
            food * buy_market->item_shop[i_Food][id_Weight];
        if (cargo_cost_limit < 0 || cargo_weight_limit < 0) {
            continue;
        }
        candidates.push_back(Candidate{&origin, food, cargo_cost_limit, cargo_weight_limit});
        const int weight_key = std::min(cargo_weight_limit, MAX_WEIGHT);
        maximum_cost_by_weight[weight_key] = std::max(
            maximum_cost_by_weight[weight_key],
            cargo_cost_limit
        );
    }

    const auto sale_market = handler.prepareExactSaleMarket(*buy_market, common);
    std::unordered_map<int, std::shared_ptr<const std::vector<ExactPurchaseChoice>>> frontier_by_weight;
    for (const auto& [weight, maximum_cost] : maximum_cost_by_weight) {
        frontier_by_weight.emplace(
            weight,
            sale_market.bucket_frontier(
                static_cast<uint16_t>(std::min(maximum_cost, MAX_GOLD)),
                static_cast<uint16_t>(weight)
            )
        );
    }

    std::optional<DeferredPurchaseResolution> best;
    for (const Candidate& input : candidates) {
        const auto found = frontier_by_weight.find(std::min(input.cargo_weight_limit, MAX_WEIGHT));
        if (found == frontier_by_weight.end()) {
            continue;
        }
        auto candidate = resolve_sell_candidate(*input.origin, input.food, *found->second);
        if (candidate.has_value() &&
            (!best.has_value() || candidate->player.gold > best->player.gold)) {
            best = std::move(candidate);
        }
    }
    if (best.has_value()) {
        const SolverAction sell_action{_a_sell, buy_list(), 0};
        best->actions = trace.resolve(best->concrete_buy, &sell_action);
    }
    return best;
}

std::optional<DeferredPurchaseResolution> DeferredPurchaseModel::resolve_sell_best_at_least(
    BestPurchaseHandler& handler,
    const int minimum_remaining_food
) const {
    auto best = resolve_sell_best_at_least(handler, minimum_remaining_food, 1);
    if (best.empty()) {
        return std::nullopt;
    }
    return std::move(best.front());
}

std::vector<DeferredPurchaseResolution> DeferredPurchaseModel::resolve_sell_best_at_least(
    BestPurchaseHandler& handler,
    const int minimum_remaining_food,
    const size_t limit
) const {
    if (minimum_remaining_food < 0 || limit == 0) {
        return {};
    }
    struct Candidate {
        const Origin* origin = nullptr;
        int food = 0;
        int cargo_cost_limit = 0;
        int cargo_weight_limit = 0;
    };
    std::vector<Candidate> candidates;
    std::unordered_map<int, int> maximum_cost_by_weight;
    const int samples_per_origin = static_cast<int>(std::min<size_t>(
        limit,
        static_cast<size_t>(MAX_BUY_COUNT) + 1
    ));
    for (const Origin& origin : origin_rows()) {
        const int first_food = std::max(
            minimum_food_units(origin),
            minimum_remaining_food + cumulative_food_used - origin.food_before_buy
        );
        const int last_food = std::min(
            maximum_useful_food_units(origin),
            first_food + samples_per_origin - 1
        );
        if (first_food > last_food) {
            continue;
        }
        for (int food = first_food; food <= last_food; ++food) {
            const int cargo_cost_limit = origin.gold.maximum_original_spend() -
                food * buy_market->item_shop[i_Food][id_Buy];
            const int cargo_weight_limit = origin.free_weight_before_buy -
                food * buy_market->item_shop[i_Food][id_Weight];
            if (cargo_cost_limit < 0 || cargo_weight_limit < 0) {
                continue;
            }
            candidates.push_back(Candidate{&origin, food, cargo_cost_limit, cargo_weight_limit});
            const int weight_key = std::min(cargo_weight_limit, MAX_WEIGHT);
            maximum_cost_by_weight[weight_key] = std::max(
                maximum_cost_by_weight[weight_key],
                cargo_cost_limit
            );
        }
    }

    const auto sale_market = handler.prepareExactSaleMarket(*buy_market, common);
    std::unordered_map<int, std::shared_ptr<const std::vector<ExactPurchaseChoice>>> frontier_by_weight;
    for (const auto& [weight, maximum_cost] : maximum_cost_by_weight) {
        frontier_by_weight.emplace(
            weight,
            sale_market.bucket_frontier(
                static_cast<uint16_t>(std::min(maximum_cost, MAX_GOLD)),
                static_cast<uint16_t>(weight)
            )
        );
    }

    std::vector<DeferredPurchaseResolution> best;
    best.reserve(std::min(limit, candidates.size()));
    for (const Candidate& input : candidates) {
        const auto found = frontier_by_weight.find(std::min(input.cargo_weight_limit, MAX_WEIGHT));
        if (found == frontier_by_weight.end()) {
            continue;
        }
        auto candidate = resolve_sell_candidate(*input.origin, input.food, *found->second);
        if (candidate.has_value()) {
            best.push_back(std::move(*candidate));
        }
    }
    const size_t keep = std::min(limit, best.size());
    std::partial_sort(
        best.begin(),
        best.begin() + static_cast<std::ptrdiff_t>(keep),
        best.end(),
        [](const DeferredPurchaseResolution& lhs, const DeferredPurchaseResolution& rhs) {
            if (lhs.player.gold != rhs.player.gold) {
                return lhs.player.gold > rhs.player.gold;
            }
            return lhs.player.food > rhs.player.food;
        }
    );
    best.resize(keep);
    const SolverAction sell_action{_a_sell, buy_list(), 0};
    for (DeferredPurchaseResolution& resolution : best) {
        resolution.actions = trace.resolve(resolution.concrete_buy, &sell_action);
    }
    return best;
}

std::optional<DeferredPurchaseResolution>
DeferredPurchaseModel::resolve_sell_candidate(
    const Origin& origin,
    const int food,
    const std::vector<ExactPurchaseChoice>& cargo_frontier
) const {
    const int food_cost = food * buy_market->item_shop[i_Food][id_Buy];
    const ExactPurchaseChoice* best_cargo = nullptr;
    int best_candidate_gold = std::numeric_limits<int>::min();
    for (const ExactPurchaseChoice& cargo : cargo_frontier) {
        const int total_spend = food_cost + cargo.actual_cost;
        if (total_spend > origin.gold.maximum_original_spend()) {
            break;
        }
        const int candidate_gold =
            origin.gold.evaluate(total_spend) + static_cast<int>(cargo.sale_value);
        if (best_cargo == nullptr || candidate_gold > best_candidate_gold) {
            best_cargo = &cargo;
            best_candidate_gold = candidate_gold;
        }
    }
    if (best_cargo == nullptr) {
        return std::nullopt;
    }

    buy_list buy = best_cargo->buys;
    buy.food = static_cast<uint8_t>(food);
    Player player = common;
    player.gold = best_candidate_gold - static_cast<int>(best_cargo->sale_value);
    player.food = origin.food_before_buy + food - cumulative_food_used;
    player.spice += buy.spice;
    player.pottery += buy.pottery;
    player.marble += buy.marble;
    player.silk += buy.silk;
    player.jewelry += buy.jewelry;
    player.cargo_buy_locked = true;
    Game::event_sell_items(player);
    return DeferredPurchaseResolution{
        std::move(player),
        buy,
        origin.parent_trace,
        {},
    };
}

std::vector<DeferredPurchaseResolution> DeferredPurchaseModel::resolve_finish(
    BestPurchaseHandler& handler
) const {
    constexpr size_t MAX_MODEL_FINALISTS = 3;
    std::vector<DeferredPurchaseResolution> finalists;
    Player final_value_market = *buy_market;
    for (int item = i_Spice; item <= i_Jewelry; ++item) {
        final_value_market.item_shop[item][id_Sell] = common.item_shop[item][id_Buy];
    }
    const auto final_market = handler.prepareExactSaleMarket(*buy_market, final_value_market);
    for (const Origin& origin : origin_rows()) {
        const int food = minimum_food_units(origin);
        if (food > maximum_food_units(origin)) {
            continue;
        }
        // Food has no final point value. Once the accumulated coverage
        // constraint is met, every additional unit only spends gold (and may
        // consume weight), so it cannot improve any final cargo choice.
        const int food_cost = food * buy_market->item_shop[i_Food][id_Buy];
        const int food_weight = food * buy_market->item_shop[i_Food][id_Weight];
        const int cargo_cost_limit = origin.gold.maximum_original_spend() - food_cost;
        const int cargo_weight_limit = origin.free_weight_before_buy - food_weight;
        if (cargo_cost_limit < 0 || cargo_weight_limit < 0) {
            continue;
        }
        const auto cargo_choices = final_market.bucket_frontier(
            static_cast<uint16_t>(std::min(cargo_cost_limit, MAX_GOLD)),
            static_cast<uint16_t>(std::min(cargo_weight_limit, MAX_WEIGHT))
        );
        const auto consider = [&](const ExactPurchaseChoice& cargo) {
            const int total_spend = food_cost + cargo.actual_cost;
            if (total_spend > origin.gold.maximum_original_spend()) {
                return;
            }
            buy_list buy = cargo.buys;
            buy.food = static_cast<uint8_t>(food);
            Player player = common;
            player.gold = origin.gold.evaluate(total_spend);
            player.food = origin.food_before_buy + food - cumulative_food_used;
            player.spice += buy.spice;
            player.pottery += buy.pottery;
            player.marble += buy.marble;
            player.silk += buy.silk;
            player.jewelry += buy.jewelry;
            player.cargo_buy_locked =
                buy.spice > 0 || buy.pottery > 0 || buy.marble > 0 ||
                buy.silk > 0 || buy.jewelry > 0;
            const int points = Game::check_points(player, "Final");
            if (finalists.size() == MAX_MODEL_FINALISTS &&
                Game::check_points(finalists.back().player, "Final") >= points) {
                return;
            }
            finalists.push_back(DeferredPurchaseResolution{
                std::move(player), buy, origin.parent_trace, trace.resolve(buy),
            });
            std::sort(finalists.begin(), finalists.end(), [](const auto& lhs, const auto& rhs) {
                return Game::check_points(lhs.player, "Final") >
                       Game::check_points(rhs.player, "Final");
            });
            if (finalists.size() > MAX_MODEL_FINALISTS) {
                finalists.pop_back();
            }
        };
        for (const ExactPurchaseChoice& cargo : *cargo_choices) {
            if (food_cost + cargo.actual_cost > origin.gold.maximum_original_spend()) {
                break;
            }
            consider(cargo);
        }
        consider(ExactPurchaseChoice{});
    }
    return finalists;
}

int DeferredPurchaseModel::minimum_food_units(const Origin& origin) const noexcept {
    return std::max(0, minimum_food_coverage - origin.food_before_buy);
}

int DeferredPurchaseModel::maximum_food_units(const Origin& origin) const noexcept {
    const int food_cost = buy_market->item_shop[i_Food][id_Buy];
    const int food_weight = buy_market->item_shop[i_Food][id_Weight];
    const int by_gold = food_cost <= 0
        ? MAX_BUY_COUNT
        : origin.gold.maximum_original_spend() / food_cost;
    const int by_weight = food_weight <= 0
        ? MAX_BUY_COUNT
        : origin.free_weight_before_buy / food_weight;
    return std::max(0, std::min({MAX_BUY_COUNT, by_gold, by_weight}));
}

int DeferredPurchaseModel::maximum_useful_remaining_food() const noexcept {
    int remaining_days = std::max(0, common.max_day - common.day);
    if (!common.merchant_happened) {
        ++remaining_days; // Magic Clepsydra can add one trading day.
    }
    if (!common.witch_happened) {
        ++remaining_days; // Time is Money can add one trading day.
    }

    const int trader_eat = std::max(0, common.npc_shop[n_Trader][nd_Eat]);
    const int camel_eat = std::max(0, common.npc_shop[n_Camel][nd_Eat]);
    const int maximum_camel_eat = common.witch_happened
        ? camel_eat
        : camel_eat * 2; // Camelization is the largest normal-mode increase.
    const int maximum_hire_eat = std::max(trader_eat, maximum_camel_eat);
    int maximum_rate = std::max(0, common.bonus_food_consumption) +
        std::max(0, common.trader) * trader_eat +
        std::max(0, common.camel) * maximum_camel_eat;
    if (!common.merchant_happened) {
        maximum_rate += trader_eat; // Leader's Necklace.
    }
    if (!common.witch_happened) {
        maximum_rate += 2 * maximum_camel_eat; // Animal Lover, over-approximated with Camelization.
    }

    const int64_t future_consumption =
        static_cast<int64_t>(maximum_rate) * remaining_days +
        static_cast<int64_t>(maximum_hire_eat) * remaining_days * (remaining_days + 1) / 2;
    return static_cast<int>(std::min<int64_t>(MAX_BUY_COUNT, std::max<int64_t>(0, future_consumption)));
}

int DeferredPurchaseModel::maximum_useful_food_units(const Origin& origin) const noexcept {
    const int useful_by_future_demand = maximum_useful_remaining_food() +
        cumulative_food_used - origin.food_before_buy;
    return std::min(maximum_food_units(origin), std::max(0, useful_by_future_demand));
}

bool DeferredPurchaseModel::origin_is_feasible(const Origin& origin) const noexcept {
    return minimum_food_units(origin) <= maximum_food_units(origin);
}

const std::vector<DeferredPurchaseModel::Origin>&
DeferredPurchaseModel::origin_rows() const noexcept {
    return *origins;
}

std::vector<DeferredPurchaseModel::Origin>& DeferredPurchaseModel::mutable_origins() {
    if (!origins.unique()) {
        origins = std::make_shared<std::vector<Origin>>(*origins);
    }
    return *origins;
}

void DeferredPurchaseModel::constrain_remaining_food(const int required_remaining_food) {
    minimum_food_coverage = std::max(
        minimum_food_coverage,
        cumulative_food_used + required_remaining_food
    );
}

void DeferredPurchaseModel::prune_infeasible_origins() {
    const bool has_infeasible = std::any_of(origin_rows().begin(), origin_rows().end(), [&](const Origin& origin) {
        return !origin_is_feasible(origin);
    });
    if (!has_infeasible) {
        return;
    }
    std::vector<Origin>& rows = mutable_origins();
    rows.erase(
        std::remove_if(rows.begin(), rows.end(), [&](const Origin& origin) {
            return !origin_is_feasible(origin);
        }),
        rows.end()
    );
}

void DeferredPurchaseModel::add_gold_to_origins(const int amount) {
    if (amount == 0) {
        return;
    }
    for (Origin& origin : mutable_origins()) {
        origin.gold.add_gold(amount);
    }
}

void DeferredPurchaseModel::synchronize_representative() {
    if (origin_rows().empty()) {
        return;
    }
    const Origin* representative = nullptr;
    int representative_gold = std::numeric_limits<int>::min();
    int representative_food = std::numeric_limits<int>::min();
    for (const Origin& origin : origin_rows()) {
        const int food_to_buy = minimum_food_units(origin);
        const int spend = food_to_buy * buy_market->item_shop[i_Food][id_Buy];
        const int gold = origin.gold.evaluate(spend);
        const int food = origin.food_before_buy + food_to_buy - cumulative_food_used;
        if (representative == nullptr || gold > representative_gold ||
            (gold == representative_gold && food > representative_food)) {
            representative = &origin;
            representative_gold = gold;
            representative_food = food;
        }
    }
    common.gold = representative_gold;
    common.food = representative_food;
    common.spice = buy_market->spice;
    common.pottery = buy_market->pottery;
    common.marble = buy_market->marble;
    common.silk = buy_market->silk;
    common.jewelry = buy_market->jewelry;
    common.cargo_buy_locked = false;
}

int DeferredPurchaseModel::end_of_day_income(const Player& player) noexcept {
    return player.daily_income + player.trader * player.npc_shop[n_Trader][nd_Effect];
}

int DeferredPurchaseModel::merchant_cost(const Player& player, const merch_options selection) noexcept {
    const int discount = player.merch_discount ? 4 : 5;
    switch (selection) {
        case m_No_Merchant:
            return 0;
        case m_Cornucopia:
        case m_Route_to_Mahaji:
        case m_Wooden_Statuette:
            return discount * 5;
        case m_Canvas_Bag:
        case m_Leaders_Necklace:
        case m_Hand_of_Midas:
            return discount * 10;
        case m_Sturdy_Saddle:
        case m_Magic_Cleppsydra:
        case m_Blue_Treasure:
            return discount * 20;
    }
    return 0;
}

std::vector<DeferredPurchaseResolution> DeferredPurchaseModel::physical_frontier(
    std::vector<DeferredPurchaseResolution> rows,
    const bool food_is_weightless
) {
    if (food_is_weightless) {
        std::sort(rows.begin(), rows.end(), [](const DeferredPurchaseResolution& lhs, const DeferredPurchaseResolution& rhs) {
            return std::tie(lhs.player.gold, lhs.player.food) >
                   std::tie(rhs.player.gold, rhs.player.food);
        });
        std::vector<DeferredPurchaseResolution> frontier;
        frontier.reserve(rows.size());
        int greatest_food = std::numeric_limits<int>::min();
        for (DeferredPurchaseResolution& candidate : rows) {
            if (candidate.player.food > greatest_food) {
                greatest_food = candidate.player.food;
                frontier.push_back(std::move(candidate));
            }
        }
        std::sort(frontier.begin(), frontier.end(), [](const auto& lhs, const auto& rhs) {
            return std::tie(lhs.player.gold, lhs.player.food) <
                   std::tie(rhs.player.gold, rhs.player.food);
        });
        return frontier;
    }

    std::sort(rows.begin(), rows.end(), [](const DeferredPurchaseResolution& lhs, const DeferredPurchaseResolution& rhs) {
        if (lhs.player.food != rhs.player.food) {
            return lhs.player.food < rhs.player.food;
        }
        return lhs.player.gold > rhs.player.gold;
    });
    std::vector<DeferredPurchaseResolution> frontier;
    frontier.reserve(rows.size());
    for (DeferredPurchaseResolution& candidate : rows) {
        if (frontier.empty() || frontier.back().player.food != candidate.player.food) {
            frontier.push_back(std::move(candidate));
        }
    }
    std::sort(frontier.begin(), frontier.end(), [](const DeferredPurchaseResolution& lhs, const DeferredPurchaseResolution& rhs) {
        return std::tie(lhs.player.gold, lhs.player.food) < std::tie(rhs.player.gold, rhs.player.food);
    });
    return frontier;
}
