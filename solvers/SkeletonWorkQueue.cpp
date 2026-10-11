#include "SkeletonWorkQueue.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace {
size_t lane_index(const SkeletonQueueTicket ticket) noexcept {
    return static_cast<size_t>(ticket) -
        static_cast<size_t>(SkeletonQueueTicket::ImmediateSell);
}

long double ticket_score(
    const SkeletonQueueGroup& group,
    const SkeletonQueueTicket ticket
) noexcept {
    if (ticket == SkeletonQueueTicket::Realistic) {
        return group.continuation.best_realistic();
    }
    if (ticket == SkeletonQueueTicket::Optimistic) {
        return group.continuation.best_optimistic();
    }
    const ContinuationCell& cell = group.continuation.lanes[lane_index(ticket)];
    return cell.valid ? cell.realistic : -std::numeric_limits<long double>::infinity();
}

bool group_before(
    const SkeletonQueueGroup& lhs,
    const SkeletonQueueGroup& rhs,
    const SkeletonQueueTicket ticket
) noexcept {
    const long double lhs_score = ticket_score(lhs, ticket);
    const long double rhs_score = ticket_score(rhs, ticket);
    if (lhs_score != rhs_score) {
        return lhs_score > rhs_score;
    }
    if (lhs.continuation.gold != rhs.continuation.gold) {
        return lhs.continuation.gold > rhs.continuation.gold;
    }
    if (lhs.continuation.food != rhs.continuation.food) {
        return lhs.continuation.food > rhs.continuation.food;
    }
    if (lhs.continuation.stable_id != rhs.continuation.stable_id) {
        return lhs.continuation.stable_id < rhs.continuation.stable_id;
    }
    return lhs.skeleton_key < rhs.skeleton_key;
}

void merge_row(ContinuationMatrixRow& aggregate, const ContinuationMatrixRow& row) {
    aggregate.baseline_realistic = std::max(
        aggregate.baseline_realistic,
        row.baseline_realistic
    );
    aggregate.baseline_optimistic = std::max(
        aggregate.baseline_optimistic,
        row.baseline_optimistic
    );
    for (size_t lane = 0; lane < CONTINUATION_LANE_COUNT; ++lane) {
        if (!row.lanes[lane].valid) {
            continue;
        }
        ContinuationCell& target = aggregate.lanes[lane];
        if (!target.valid) {
            target = row.lanes[lane];
            continue;
        }
        target.realistic = std::max(target.realistic, row.lanes[lane].realistic);
        target.optimistic = std::max(target.optimistic, row.lanes[lane].optimistic);
    }
}
}

SkeletonQueueSchedule SkeletonWorkQueue::schedule(
    const std::vector<SkeletonQueueCandidate>& candidates,
    const uint8_t lane_share_percent,
    const uint8_t optimistic_share_percent
) {
    SkeletonQueueSchedule result;
    if (candidates.empty()) {
        return result;
    }

    std::unordered_map<uint64_t, std::vector<const SkeletonQueueCandidate*>> grouped;
    grouped.reserve(candidates.size());
    for (const SkeletonQueueCandidate& candidate : candidates) {
        grouped[candidate.skeleton_key].push_back(&candidate);
    }

    result.groups.reserve(grouped.size());
    for (auto& [key, members] : grouped) {
        std::sort(members.begin(), members.end(), [](const auto* lhs, const auto* rhs) {
            const long double lhs_realistic = lhs->continuation.best_realistic();
            const long double rhs_realistic = rhs->continuation.best_realistic();
            if (lhs_realistic != rhs_realistic) {
                return lhs_realistic > rhs_realistic;
            }
            const long double lhs_optimistic = lhs->continuation.best_optimistic();
            const long double rhs_optimistic = rhs->continuation.best_optimistic();
            if (lhs_optimistic != rhs_optimistic) {
                return lhs_optimistic > rhs_optimistic;
            }
            return lhs->continuation.stable_id < rhs->continuation.stable_id;
        });

        SkeletonQueueGroup group;
        group.skeleton_key = key;
        group.continuation.baseline_realistic =
            -std::numeric_limits<long double>::infinity();
        group.continuation.baseline_optimistic =
            -std::numeric_limits<long double>::infinity();
        group.source_indices.reserve(members.size());
        for (const SkeletonQueueCandidate* member : members) {
            merge_row(group.continuation, member->continuation);
            group.source_indices.push_back(member->source_index);
        }
        group.continuation.gold = members.front()->continuation.gold;
        group.continuation.food = members.front()->continuation.food;
        group.continuation.stable_id = members.front()->continuation.stable_id;
        result.groups.emplace_back(std::move(group));
    }

    std::array<std::vector<size_t>, SKELETON_QUEUE_TICKET_COUNT> rankings;
    std::array<std::vector<size_t>, SKELETON_QUEUE_TICKET_COUNT> rank_positions;
    for (size_t ticket_index = 0; ticket_index < SKELETON_QUEUE_TICKET_COUNT;
         ++ticket_index) {
        const auto ticket = static_cast<SkeletonQueueTicket>(ticket_index);
        auto& ranking = rankings[ticket_index];
        for (size_t group_index = 0; group_index < result.groups.size(); ++group_index) {
            if (std::isfinite(ticket_score(result.groups[group_index], ticket))) {
                ranking.push_back(group_index);
            }
        }
        std::sort(ranking.begin(), ranking.end(), [&](const size_t lhs, const size_t rhs) {
            return group_before(result.groups[lhs], result.groups[rhs], ticket);
        });
        auto& positions = rank_positions[ticket_index];
        positions.assign(result.groups.size(), std::numeric_limits<size_t>::max());
        for (size_t rank = 0; rank < ranking.size(); ++rank) {
            positions[ranking[rank]] = rank;
        }
    }

    const long double lane_total = std::min<uint8_t>(lane_share_percent, 100);
    const long double non_lane = 100.0L - lane_total;
    const long double optimistic_weight = non_lane *
        static_cast<long double>(std::min<uint8_t>(optimistic_share_percent, 100)) /
        100.0L;
    const long double realistic_weight = non_lane - optimistic_weight;
    std::array<long double, SKELETON_QUEUE_TICKET_COUNT> weights{};
    weights[static_cast<size_t>(SkeletonQueueTicket::Realistic)] = realistic_weight;
    weights[static_cast<size_t>(SkeletonQueueTicket::Optimistic)] = optimistic_weight;
    const long double one_lane_weight = lane_total /
        static_cast<long double>(CONTINUATION_LANE_COUNT);
    for (size_t ticket_index = static_cast<size_t>(SkeletonQueueTicket::ImmediateSell);
         ticket_index < SKELETON_QUEUE_TICKET_COUNT; ++ticket_index) {
        weights[ticket_index] = one_lane_weight;
    }

    std::vector<uint8_t> selected(result.groups.size(), 0);
    std::vector<size_t> ordered;
    ordered.reserve(result.groups.size());
    // A time-limited queue sees the best family for every available view
    // before weighted ranking takes over.
    for (size_t ticket_index = 0; ticket_index < SKELETON_QUEUE_TICKET_COUNT;
         ++ticket_index) {
        if (weights[ticket_index] <= 0.0L) {
            continue;
        }
        for (const size_t group_index : rankings[ticket_index]) {
            if (selected[group_index]) {
                ++result.duplicate_ticket_skips;
                continue;
            }
            selected[group_index] = 1;
            result.groups[group_index].first_ticket =
                static_cast<SkeletonQueueTicket>(ticket_index);
            result.groups[group_index].scheduling_rank = 0.0L;
            ordered.push_back(group_index);
            ++result.head_ticket_counts[ticket_index];
            break;
        }
    }
    const size_t head_size = ordered.size();

    for (size_t group_index = 0; group_index < result.groups.size(); ++group_index) {
        if (selected[group_index]) {
            continue;
        }
        long double best_rank = std::numeric_limits<long double>::infinity();
        SkeletonQueueTicket best_ticket = SkeletonQueueTicket::Realistic;
        for (size_t ticket_index = 0; ticket_index < SKELETON_QUEUE_TICKET_COUNT;
             ++ticket_index) {
            if (weights[ticket_index] <= 0.0L) {
                continue;
            }
            const size_t position = rank_positions[ticket_index][group_index];
            if (position == std::numeric_limits<size_t>::max()) {
                continue;
            }
            const long double rank = static_cast<long double>(position + 1) /
                weights[ticket_index];
            if (rank < best_rank) {
                best_rank = rank;
                best_ticket = static_cast<SkeletonQueueTicket>(ticket_index);
            }
        }
        result.groups[group_index].first_ticket = best_ticket;
        result.groups[group_index].scheduling_rank = best_rank;
        ordered.push_back(group_index);
    }

    std::sort(
        ordered.begin() + static_cast<std::ptrdiff_t>(head_size),
        ordered.end(),
        [&](const size_t lhs, const size_t rhs) {
            const SkeletonQueueGroup& left = result.groups[lhs];
            const SkeletonQueueGroup& right = result.groups[rhs];
            if (left.scheduling_rank != right.scheduling_rank) {
                return left.scheduling_rank < right.scheduling_rank;
            }
            if (group_before(left, right, SkeletonQueueTicket::Realistic)) {
                return true;
            }
            if (group_before(right, left, SkeletonQueueTicket::Realistic)) {
                return false;
            }
            return left.skeleton_key < right.skeleton_key;
        }
    );

    std::vector<SkeletonQueueGroup> scheduled;
    scheduled.reserve(result.groups.size());
    for (const size_t group_index : ordered) {
        scheduled.emplace_back(std::move(result.groups[group_index]));
    }
    result.groups = std::move(scheduled);
    return result;
}
