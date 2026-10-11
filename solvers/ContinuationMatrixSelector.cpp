#include "ContinuationMatrixSelector.h"

#include <algorithm>
#include <numeric>

namespace {
enum class Ranking : uint8_t {
    Realistic,
    Optimistic,
};

long double lane_score(
    const ContinuationMatrixRow& row,
    const size_t lane,
    const Ranking ranking
) {
    const ContinuationCell& cell = row.lanes[lane];
    return ranking == Ranking::Realistic ? cell.realistic : cell.optimistic;
}

long double global_score(const ContinuationMatrixRow& row, const Ranking ranking) {
    return ranking == Ranking::Realistic
        ? row.best_realistic()
        : row.best_optimistic();
}

template <typename Score>
bool ranked_before(
    const ContinuationMatrixRow& lhs,
    const ContinuationMatrixRow& rhs,
    Score&& score
) {
    const long double lhs_score = score(lhs);
    const long double rhs_score = score(rhs);
    if (lhs_score != rhs_score) {
        return lhs_score > rhs_score;
    }
    if (lhs.gold != rhs.gold) {
        return lhs.gold > rhs.gold;
    }
    if (lhs.food != rhs.food) {
        return lhs.food > rhs.food;
    }
    return lhs.stable_id < rhs.stable_id;
}
}

long double ContinuationMatrixRow::best_realistic() const noexcept {
    long double best = baseline_realistic;
    for (const ContinuationCell& cell : lanes) {
        if (cell.valid) {
            best = std::max(best, cell.realistic);
        }
    }
    return best;
}

long double ContinuationMatrixRow::best_optimistic() const noexcept {
    long double best = baseline_optimistic;
    for (const ContinuationCell& cell : lanes) {
        if (cell.valid) {
            best = std::max(best, cell.optimistic);
        }
    }
    return best;
}

ContinuationMatrixSelection ContinuationMatrixSelector::select(
    const std::vector<ContinuationMatrixRow>& rows,
    const size_t capacity,
    const uint8_t lane_share_percent,
    const uint8_t optimistic_share_percent
) {
    ContinuationMatrixSelection result;
    if (rows.empty() || capacity == 0) {
        return result;
    }
    const size_t keep = std::min(capacity, rows.size());
    result.owner_indices.reserve(keep);
    std::vector<uint8_t> selected(rows.size(), 0);

    std::array<std::vector<size_t>, CONTINUATION_LANE_COUNT> realistic_lanes;
    std::array<std::vector<size_t>, CONTINUATION_LANE_COUNT> optimistic_lanes;
    for (size_t lane = 0; lane < CONTINUATION_LANE_COUNT; ++lane) {
        auto& realistic = realistic_lanes[lane];
        auto& optimistic = optimistic_lanes[lane];
        for (size_t index = 0; index < rows.size(); ++index) {
            if (rows[index].lanes[lane].valid) {
                realistic.push_back(index);
                optimistic.push_back(index);
            }
        }
        std::sort(realistic.begin(), realistic.end(), [&](const size_t lhs, const size_t rhs) {
            return ranked_before(rows[lhs], rows[rhs], [&](const ContinuationMatrixRow& row) {
                return lane_score(row, lane, Ranking::Realistic);
            });
        });
        std::sort(optimistic.begin(), optimistic.end(), [&](const size_t lhs, const size_t rhs) {
            return ranked_before(rows[lhs], rows[rhs], [&](const ContinuationMatrixRow& row) {
                return lane_score(row, lane, Ranking::Optimistic);
            });
        });
    }

    const auto select_lane_round_robin = [&](
        const auto& ranked_lanes,
        const size_t target,
        std::vector<size_t>& cursors
    ) {
        while (result.owner_indices.size() < target) {
            bool found = false;
            for (size_t lane = 0; lane < CONTINUATION_LANE_COUNT &&
                 result.owner_indices.size() < target; ++lane) {
                const auto& ranked = ranked_lanes[lane];
                while (cursors[lane] < ranked.size() && selected[ranked[cursors[lane]]]) {
                    ++cursors[lane];
                    ++result.duplicate_ticket_skips;
                }
                if (cursors[lane] >= ranked.size()) {
                    continue;
                }
                const size_t owner = ranked[cursors[lane]++];
                selected[owner] = 1;
                result.owner_indices.push_back(owner);
                ++result.lane_owner_counts[lane];
                ++result.lane_owner_total;
                found = true;
            }
            if (!found) {
                break;
            }
        }
    };

    const size_t lane_capacity = std::min(
        keep,
        (keep * std::min<uint8_t>(lane_share_percent, 100)) / 100
    );
    const size_t optimistic_lane_capacity =
        (lane_capacity * std::min<uint8_t>(optimistic_share_percent, 100)) / 100;
    std::vector<size_t> optimistic_cursors(CONTINUATION_LANE_COUNT, 0);
    select_lane_round_robin(
        optimistic_lanes,
        optimistic_lane_capacity,
        optimistic_cursors
    );
    std::vector<size_t> realistic_cursors(CONTINUATION_LANE_COUNT, 0);
    select_lane_round_robin(realistic_lanes, lane_capacity, realistic_cursors);

    std::vector<size_t> global_realistic(rows.size());
    std::iota(global_realistic.begin(), global_realistic.end(), 0);
    std::vector<size_t> global_optimistic = global_realistic;
    std::sort(global_realistic.begin(), global_realistic.end(), [&](const size_t lhs, const size_t rhs) {
        return ranked_before(rows[lhs], rows[rhs], [](const ContinuationMatrixRow& row) {
            return global_score(row, Ranking::Realistic);
        });
    });
    std::sort(global_optimistic.begin(), global_optimistic.end(), [&](const size_t lhs, const size_t rhs) {
        return ranked_before(rows[lhs], rows[rhs], [](const ContinuationMatrixRow& row) {
            return global_score(row, Ranking::Optimistic);
        });
    });

    const auto select_global = [&](const std::vector<size_t>& ranked, const size_t target) {
        for (const size_t owner : ranked) {
            if (result.owner_indices.size() >= target) {
                break;
            }
            if (selected[owner]) {
                ++result.duplicate_ticket_skips;
                continue;
            }
            selected[owner] = 1;
            result.owner_indices.push_back(owner);
        }
    };

    const size_t remaining_after_lanes = keep - result.owner_indices.size();
    const size_t optimistic_global =
        (remaining_after_lanes * std::min<uint8_t>(optimistic_share_percent, 100)) / 100;
    select_global(global_optimistic, result.owner_indices.size() + optimistic_global);
    select_global(global_realistic, keep);
    return result;
}
