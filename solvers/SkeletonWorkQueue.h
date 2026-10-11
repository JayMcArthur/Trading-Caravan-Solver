#ifndef TRADING_CARAVAN_SOLVER_SKELETONWORKQUEUE_H
#define TRADING_CARAVAN_SOLVER_SKELETONWORKQUEUE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "ContinuationMatrixSelector.h"

enum class SkeletonQueueTicket : uint8_t {
    Realistic = 0,
    Optimistic,
    ImmediateSell,
    Trader,
    Camel,
    Interest,
    FoodOnlyBuy,
    Count,
};

constexpr size_t SKELETON_QUEUE_TICKET_COUNT =
    static_cast<size_t>(SkeletonQueueTicket::Count);

struct SkeletonQueueCandidate {
    uint64_t skeleton_key = 0;
    ContinuationMatrixRow continuation;
    size_t source_index = 0;
};

struct SkeletonQueueGroup {
    uint64_t skeleton_key = 0;
    ContinuationMatrixRow continuation;
    SkeletonQueueTicket first_ticket = SkeletonQueueTicket::Realistic;
    long double scheduling_rank = 0.0L;
    std::vector<size_t> source_indices;
};

struct SkeletonQueueSchedule {
    // Groups and members are best-first. A solver may reverse them for
    // vector::pop_back() queue semantics.
    std::vector<SkeletonQueueGroup> groups;
    std::array<size_t, SKELETON_QUEUE_TICKET_COUNT> head_ticket_counts{};
    size_t duplicate_ticket_skips = 0;
};

class SkeletonWorkQueue {
public:
    [[nodiscard]] static SkeletonQueueSchedule schedule(
        const std::vector<SkeletonQueueCandidate>& candidates,
        uint8_t lane_share_percent,
        uint8_t optimistic_share_percent
    );
};

#endif // TRADING_CARAVAN_SOLVER_SKELETONWORKQUEUE_H
