#include "EarlyActionSkeleton.h"

#include <algorithm>

EarlyActionSkeleton EarlyActionSkeleton::extended(const SolverAction& action) const noexcept {
    EarlyActionSkeleton result = *this;
    const uint8_t code = action_code(action);
    if (code == 0 || result.counted_actions >= MAX_ACTIONS) {
        return result;
    }
    result.packed_actions |= static_cast<uint32_t>(code) << (result.counted_actions * 4U);
    ++result.counted_actions;
    return result;
}

uint64_t EarlyActionSkeleton::key(const uint8_t maximum_actions) const noexcept {
    const uint8_t retained_actions = std::min<uint8_t>(counted_actions, maximum_actions);
    const uint32_t mask = retained_actions >= 8
        ? 0xffffffffU
        : (retained_actions == 0 ? 0U : (uint32_t{1} << (retained_actions * 4U)) - 1U);
    return static_cast<uint64_t>(packed_actions & mask) |
           (static_cast<uint64_t>(retained_actions) << 32U);
}

uint8_t EarlyActionSkeleton::action_count() const noexcept {
    return counted_actions;
}

uint8_t EarlyActionSkeleton::prefix_mismatches(
    const EarlyActionSkeleton& target,
    const uint8_t maximum_actions
) const noexcept {
    const uint8_t compared_actions = std::min({
        counted_actions,
        target.counted_actions,
        maximum_actions,
        MAX_ACTIONS,
    });
    uint8_t mismatches = 0;
    for (uint8_t index = 0; index < compared_actions; ++index) {
        const uint32_t shift = static_cast<uint32_t>(index) * 4U;
        const uint8_t lhs = static_cast<uint8_t>((packed_actions >> shift) & 0x0fU);
        const uint8_t rhs = static_cast<uint8_t>((target.packed_actions >> shift) & 0x0fU);
        mismatches += lhs == rhs ? 0 : 1;
    }
    return mismatches;
}

uint8_t EarlyActionSkeleton::action_code(const SolverAction& action) noexcept {
    switch (action.action) {
        case _a_buy:
            return 1;
        case _a_sell:
            return 2;
        case _a_npc:
            return action.option == static_cast<uint8_t>(n_Trader) ? 3 : 4;
        case _a_interest:
            return 5;
        case _a_wait:
            return 6;
        case _a_merchant:
        case _a_witch:
        case _a_disaster:
            return 0;
    }
    return 0;
}
