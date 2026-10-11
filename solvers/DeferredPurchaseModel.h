#ifndef TRADING_CARAVAN_SOLVER_DEFERREDPURCHASEMODEL_H
#define TRADING_CARAVAN_SOLVER_DEFERREDPURCHASEMODEL_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../player/player.h"
#include "../purchaseHandler/BestPurchaseHandler.h"
#include "DeferredGoldFunction.h"
#include "EarlyActionSkeleton.h"
#include "DeferredTraceRecipe.h"

class SolverStateCodecAccess;

struct DeferredPurchaseSeed {
    int gold = 0;
    int food = 0;
    uint32_t parent_trace = 0;
};

struct DeferredPurchaseResolution {
    Player player;
    buy_list concrete_buy = buy_list();
    uint32_t parent_trace = 0;
    std::vector<SolverAction> actions;
};

enum class DeferredBuyCoverage : uint8_t {
    Missing,
    DominatingReplacement,
    Exact,
};

class DeferredPurchaseModel {
public:
    [[nodiscard]] static DeferredPurchaseModel open(
        const Player& before_buy,
        const std::vector<DeferredPurchaseSeed>& seeds
    );

    [[nodiscard]] const Player& representative() const noexcept;
    [[nodiscard]] size_t input_seed_count() const noexcept;
    [[nodiscard]] size_t origin_count() const noexcept;
    [[nodiscard]] size_t memory_bytes() const noexcept;
    [[nodiscard]] size_t dynamic_memory_bytes(
        std::unordered_set<const void*>& counted_shared
    ) const noexcept;
    [[nodiscard]] bool semantically_equal(const DeferredPurchaseModel& other) const noexcept;
    [[nodiscard]] size_t semantic_hash() const noexcept;
    [[nodiscard]] size_t dominance_partition_hash() const noexcept;
    [[nodiscard]] bool dominates(
        const DeferredPurchaseModel& other,
        uint64_t* gold_function_comparisons = nullptr,
        uint64_t* exact_spend_evaluations = nullptr,
        uint64_t* origin_coverage_checks = nullptr,
        uint64_t* successful_cross_shape_checks = nullptr
    ) const noexcept;
    [[nodiscard]] DeferredBuyCoverage classify_exact_buy(
        const buy_list& buy
    ) const noexcept;
    [[nodiscard]] const DeferredTraceRecipe& trace_recipe() const noexcept;
    [[nodiscard]] EarlyActionSkeleton extend_skeleton(EarlyActionSkeleton skeleton) const noexcept;
    [[nodiscard]] std::vector<uint32_t> provenances() const;
    [[nodiscard]] DeferredPurchaseModel remap_provenances(
        const std::unordered_map<uint32_t, uint32_t>& remap
    ) const;

    [[nodiscard]] std::optional<DeferredPurchaseModel> after_start_of_day() const;
    [[nodiscard]] DeferredPurchaseModel after_resume_same_day() const;
    [[nodiscard]] std::optional<DeferredPurchaseModel> after_hire(npcs selection) const;
    [[nodiscard]] std::optional<DeferredPurchaseModel> after_interest() const;
    [[nodiscard]] std::optional<DeferredPurchaseModel> after_merchant(merch_options selection) const;
    [[nodiscard]] DeferredPurchaseModel after_witch(witch_options selection) const;

    [[nodiscard]] std::vector<DeferredPurchaseResolution> resolve_food_only_for_next_buy() const;
    [[nodiscard]] std::vector<DeferredPurchaseResolution> resolve_sell(BestPurchaseHandler& handler) const;
    [[nodiscard]] std::optional<DeferredPurchaseResolution> resolve_sell_exact_remaining_food(
        BestPurchaseHandler& handler,
        int remaining_food
    ) const;
    [[nodiscard]] std::optional<DeferredPurchaseResolution> resolve_sell_best_at_least(
        BestPurchaseHandler& handler,
        int minimum_remaining_food
    ) const;
    [[nodiscard]] std::vector<DeferredPurchaseResolution> resolve_sell_best_at_least(
        BestPurchaseHandler& handler,
        int minimum_remaining_food,
        size_t limit
    ) const;
    [[nodiscard]] std::vector<DeferredPurchaseResolution> resolve_finish(BestPurchaseHandler& handler) const;

private:
    friend class SolverStateCodecAccess;
    struct Origin {
        DeferredGoldFunction gold = DeferredGoldFunction::open(0, 0, 0);
        int food_before_buy = 0;
        int free_weight_before_buy = 0;
        uint32_t parent_trace = 0;
    };

    std::shared_ptr<const Player> buy_market;
    Player common;
    std::shared_ptr<std::vector<Origin>> origins;
    size_t seeds_before_compaction = 0;
    int cumulative_food_used = 0;
    int minimum_food_coverage = 0;
    DeferredTraceRecipe trace;

    DeferredPurchaseModel(
        std::shared_ptr<const Player> buy_snapshot,
        Player common_player,
        std::vector<Origin> origin_rows,
        size_t input_seed_count
    );

    [[nodiscard]] int minimum_food_units(const Origin& origin) const noexcept;
    [[nodiscard]] int maximum_food_units(const Origin& origin) const noexcept;
    [[nodiscard]] int maximum_useful_remaining_food() const noexcept;
    [[nodiscard]] int maximum_useful_food_units(const Origin& origin) const noexcept;
    [[nodiscard]] bool origin_is_feasible(const Origin& origin) const noexcept;
    [[nodiscard]] const std::vector<Origin>& origin_rows() const noexcept;
    [[nodiscard]] std::vector<Origin>& mutable_origins();
    [[nodiscard]] std::optional<DeferredPurchaseResolution> resolve_sell_candidate(
        const Origin& origin,
        int food,
        const std::vector<ExactPurchaseChoice>& cargo_frontier
    ) const;
    void constrain_remaining_food(int required_remaining_food);
    void prune_infeasible_origins();
    void add_gold_to_origins(int amount);
    void synchronize_representative();
    [[nodiscard]] static int end_of_day_income(const Player& player) noexcept;
    [[nodiscard]] static int merchant_cost(const Player& player, merch_options selection) noexcept;
    [[nodiscard]] static std::vector<DeferredPurchaseResolution> physical_frontier(
        std::vector<DeferredPurchaseResolution> rows,
        bool food_is_weightless
    );
};

#endif // TRADING_CARAVAN_SOLVER_DEFERREDPURCHASEMODEL_H
