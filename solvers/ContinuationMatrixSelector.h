#ifndef TRADING_CARAVAN_SOLVER_CONTINUATIONMATRIXSELECTOR_H
#define TRADING_CARAVAN_SOLVER_CONTINUATIONMATRIXSELECTOR_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

enum class ContinuationLane : uint8_t {
    ImmediateSell = 0,
    Trader,
    Camel,
    Interest,
    FoodOnlyBuy,
    Count,
};

constexpr size_t CONTINUATION_LANE_COUNT =
    static_cast<size_t>(ContinuationLane::Count);

struct ContinuationCell {
    bool valid = false;
    long double realistic = 0.0L;
    long double optimistic = 0.0L;
};

struct ContinuationMatrixRow {
    long double baseline_realistic = 0.0L;
    long double baseline_optimistic = 0.0L;
    std::array<ContinuationCell, CONTINUATION_LANE_COUNT> lanes{};
    int gold = 0;
    int food = 0;
    uint32_t stable_id = 0;

    [[nodiscard]] long double best_realistic() const noexcept;
    [[nodiscard]] long double best_optimistic() const noexcept;
};

struct ContinuationMatrixSelection {
    std::vector<size_t> owner_indices;
    std::array<size_t, CONTINUATION_LANE_COUNT> lane_owner_counts{};
    size_t lane_owner_total = 0;
    size_t duplicate_ticket_skips = 0;
};

class ContinuationMatrixSelector {
public:
    [[nodiscard]] static ContinuationMatrixSelection select(
        const std::vector<ContinuationMatrixRow>& rows,
        size_t capacity,
        uint8_t lane_share_percent,
        uint8_t optimistic_share_percent
    );
};

#endif // TRADING_CARAVAN_SOLVER_CONTINUATIONMATRIXSELECTOR_H
