#ifndef TRADING_CARAVAN_SOLVER_EARLYACTIONSKELETON_H
#define TRADING_CARAVAN_SOLVER_EARLYACTIONSKELETON_H

#include <cstdint>

#include "SolverAction.h"

class EarlyActionSkeleton {
public:
    [[nodiscard]] EarlyActionSkeleton extended(const SolverAction& action) const noexcept;
    [[nodiscard]] uint64_t key(uint8_t maximum_actions = 8) const noexcept;
    [[nodiscard]] uint8_t action_count() const noexcept;
    [[nodiscard]] uint8_t prefix_mismatches(
        const EarlyActionSkeleton& target,
        uint8_t maximum_actions = 8
    ) const noexcept;

private:
    static constexpr uint8_t MAX_ACTIONS = 8;
    uint32_t packed_actions = 0;
    uint8_t counted_actions = 0;

    [[nodiscard]] static uint8_t action_code(const SolverAction& action) noexcept;
};

#endif // TRADING_CARAVAN_SOLVER_EARLYACTIONSKELETON_H
