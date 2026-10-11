#ifndef TRADING_CARAVAN_SOLVER_DEFERREDSEARCHSTATE_H
#define TRADING_CARAVAN_SOLVER_DEFERREDSEARCHSTATE_H

#include <cstddef>
#include <cstdint>
#include <variant>
#include <unordered_map>
#include <unordered_set>

#include "DeferredPurchaseModel.h"
#include "DeferredSoldModel.h"

enum class DeferredContinuationKind : uint8_t {
    ImmediateSell,
    Trader,
    Camel,
    Interest,
    FoodOnlyBuy,
};

struct DeferredContinuationView {
    DeferredContinuationKind kind = DeferredContinuationKind::ImmediateSell;
    Player player;
};

struct DeferredDominanceMetrics {
    uint64_t gold_function_comparisons = 0;
    uint64_t exact_spend_evaluations = 0;
    uint64_t origin_coverage_checks = 0;
    uint64_t successful_cross_shape_checks = 0;
};

class DeferredSearchState {
public:
    explicit DeferredSearchState(DeferredPurchaseModel purchase);
    explicit DeferredSearchState(DeferredSoldModel sold);

    [[nodiscard]] bool has_open_purchase() const noexcept;
    [[nodiscard]] bool has_sold_ledger() const noexcept;
    [[nodiscard]] const DeferredPurchaseModel& purchase() const;
    [[nodiscard]] const DeferredSoldModel& sold() const;
    [[nodiscard]] const Player& representative() const noexcept;
    [[nodiscard]] size_t row_count() const noexcept;
    [[nodiscard]] size_t memory_bytes() const noexcept;
    [[nodiscard]] size_t memory_bytes(
        std::unordered_set<const void*>& counted_shared
    ) const noexcept;
    [[nodiscard]] uint64_t ranking_identity() const noexcept;
    [[nodiscard]] std::vector<DeferredContinuationView> continuation_views(
        BestPurchaseHandler& handler,
        bool player_action_reachable = true
    ) const;
    [[nodiscard]] bool semantically_equal(const DeferredSearchState& other) const noexcept;
    [[nodiscard]] size_t semantic_hash() const noexcept;
    [[nodiscard]] size_t dominance_partition_hash() const noexcept;
    [[nodiscard]] bool dominates(
        const DeferredSearchState& other,
        DeferredDominanceMetrics* metrics = nullptr
    ) const noexcept;
    [[nodiscard]] EarlyActionSkeleton extend_skeleton(EarlyActionSkeleton skeleton) const noexcept;
    [[nodiscard]] std::vector<uint32_t> provenances() const;
    [[nodiscard]] DeferredSearchState remap_provenances(
        const std::unordered_map<uint32_t, uint32_t>& remap
    ) const;

private:
    uint64_t identity = 0;
    std::variant<DeferredPurchaseModel, DeferredSoldModel> state;
};

#endif // TRADING_CARAVAN_SOLVER_DEFERREDSEARCHSTATE_H
