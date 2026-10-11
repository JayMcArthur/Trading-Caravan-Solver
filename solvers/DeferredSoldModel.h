#ifndef TRADING_CARAVAN_SOLVER_DEFERREDSOLDMODEL_H
#define TRADING_CARAVAN_SOLVER_DEFERREDSOLDMODEL_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "DeferredPurchaseModel.h"

class SolverStateCodecAccess;

class DeferredSoldModel {
public:
    [[nodiscard]] static std::optional<DeferredSoldModel> from_purchase(
        const DeferredPurchaseModel& purchase,
        BestPurchaseHandler& handler
    );

    [[nodiscard]] const Player& representative() const noexcept;
    [[nodiscard]] size_t source_count() const noexcept;
    [[nodiscard]] size_t memory_bytes() const noexcept;
    [[nodiscard]] size_t dynamic_memory_bytes(
        std::unordered_set<const void*>& counted_shared
    ) const noexcept;
    [[nodiscard]] bool semantically_equal(const DeferredSoldModel& other) const noexcept;
    [[nodiscard]] size_t semantic_hash() const noexcept;
    [[nodiscard]] size_t dominance_partition_hash() const noexcept;
    [[nodiscard]] bool dominates(
        const DeferredSoldModel& other,
        uint64_t* gold_function_comparisons = nullptr,
        uint64_t* exact_spend_evaluations = nullptr
    ) const noexcept;
    [[nodiscard]] DeferredBuyCoverage classify_exact_buy(
        const buy_list& buy
    ) const noexcept;
    [[nodiscard]] EarlyActionSkeleton extend_skeleton(
        EarlyActionSkeleton skeleton
    ) const noexcept;
    [[nodiscard]] std::vector<uint32_t> provenances() const;
    [[nodiscard]] DeferredSoldModel remap_provenances(
        const std::unordered_map<uint32_t, uint32_t>& remap
    ) const;

    [[nodiscard]] std::optional<DeferredSoldModel> after_start_of_day() const;
    [[nodiscard]] std::optional<DeferredSoldModel> after_resume_same_day() const;
    [[nodiscard]] std::optional<DeferredSoldModel> after_hire(npcs selection) const;
    [[nodiscard]] std::optional<DeferredSoldModel> after_interest() const;
    [[nodiscard]] std::optional<DeferredSoldModel> after_merchant(
        merch_options selection
    ) const;
    [[nodiscard]] std::optional<DeferredSoldModel> after_witch(
        witch_options selection
    ) const;
    [[nodiscard]] std::optional<DeferredSoldModel> after_disaster(
        disaster_options selection
    ) const;

    [[nodiscard]] std::vector<DeferredPurchaseResolution> materialize() const;
    [[nodiscard]] std::vector<DeferredPurchaseResolution> resolve_finish(
        size_t limit
    ) const;

private:
    friend class SolverStateCodecAccess;
    struct EnvelopeRow {
        int food = 0;
        int gold = 0;
        buy_list concrete_buy = buy_list();
        uint32_t parent_trace = 0;
    };

    Player common;
    std::shared_ptr<std::vector<EnvelopeRow>> rows;
    DeferredTraceRecipe purchase_trace;
    std::vector<SolverAction> actions_after_sell;

    DeferredSoldModel(
        Player representative_player,
        std::vector<EnvelopeRow> envelope,
        DeferredTraceRecipe trace
    );

    [[nodiscard]] std::vector<EnvelopeRow>& mutable_rows();
    [[nodiscard]] Player player_for(const EnvelopeRow& row) const;
    [[nodiscard]] bool transform_rows(
        const std::function<bool(Player&)>& transform
    );
    void canonicalize_rows();
    void synchronize_representative();
};

#endif // TRADING_CARAVAN_SOLVER_DEFERREDSOLDMODEL_H
