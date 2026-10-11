#ifndef TRADING_CARAVAN_SOLVER_ROUTESURVIVALAUDIT_H
#define TRADING_CARAVAN_SOLVER_ROUTESURVIVALAUDIT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "ContinuationMatrixSelector.h"
#include "SolverAction.h"

enum class RouteAuditReductionStage : uint8_t {
    BeforeExact,
    AfterExact,
    AfterDominance,
    AfterBeam,
};

enum class RouteAuditQuantityStatus : uint8_t {
    NotApplicable,
    Missing,
    DominatingReplacement,
    Exact,
};

struct RouteAuditQuantityStats {
    uint64_t exact = 0;
    uint64_t dominating_replacement = 0;
    uint64_t missing = 0;
};

struct RouteAuditPrefixStats {
    uint64_t generated = 0;
    uint64_t before_exact = 0;
    uint64_t after_exact = 0;
    uint64_t after_dominance = 0;
    uint64_t after_beam = 0;
    uint64_t finished = 0;
    size_t best_realistic_rank = 0;
    size_t best_optimistic_rank = 0;
    std::array<size_t, CONTINUATION_LANE_COUNT> best_lane_ranks{};
    RouteAuditQuantityStats quantity_generated;
    RouteAuditQuantityStats quantity_before_exact;
    RouteAuditQuantityStats quantity_after_exact;
    RouteAuditQuantityStats quantity_after_dominance;
    RouteAuditQuantityStats quantity_after_beam;
    uint64_t quantity_materialized = 0;
};

class RouteSurvivalAudit {
public:
    static constexpr int16_t NO_MATCH = -1;

    explicit RouteSurvivalAudit(std::vector<SolverAction> route = {});

    [[nodiscard]] bool enabled() const noexcept;
    [[nodiscard]] size_t target_action_count() const noexcept;
    [[nodiscard]] int16_t advance(int16_t progress, const SolverAction& action) const noexcept;
    [[nodiscard]] int16_t advance_exact(
        int16_t progress,
        const SolverAction& action
    ) const noexcept;
    [[nodiscard]] int16_t match_prefix(const std::vector<SolverAction>& actions) const noexcept;
    [[nodiscard]] int16_t match_exact_prefix(
        const std::vector<SolverAction>& actions
    ) const noexcept;
    [[nodiscard]] const buy_list* expected_open_buy(int16_t progress) const noexcept;
    [[nodiscard]] const buy_list* expected_sold_buy(int16_t progress) const noexcept;
    void note_generated(
        int16_t progress,
        RouteAuditQuantityStatus quantity = RouteAuditQuantityStatus::NotApplicable
    ) noexcept;
    void note_reduction(
        RouteAuditReductionStage stage,
        int16_t progress,
        RouteAuditQuantityStatus quantity = RouteAuditQuantityStatus::NotApplicable
    ) noexcept;
    void note_materialized(const std::vector<SolverAction>& actions) noexcept;
    void note_finished(int16_t progress) noexcept;
    void note_ranks(
        int16_t progress,
        size_t realistic_rank,
        size_t optimistic_rank,
        const std::array<size_t, CONTINUATION_LANE_COUNT>& lane_ranks
    ) noexcept;

    [[nodiscard]] int16_t highest_generated_prefix() const noexcept;
    [[nodiscard]] int16_t highest_retained_prefix() const noexcept;
    [[nodiscard]] const std::vector<RouteAuditPrefixStats>& prefix_stats() const noexcept;
    [[nodiscard]] const std::vector<SolverAction>& route() const noexcept;
    void write_json(std::ostream& out) const;

private:
    std::vector<SolverAction> target_route;
    std::vector<RouteAuditPrefixStats> stats;

    [[nodiscard]] static bool action_matches(
        const SolverAction& expected,
        const SolverAction& generated,
        bool exact_buy
    ) noexcept;
    [[nodiscard]] const buy_list* expected_buy_before(
        int16_t progress,
        bool require_sell_after_buy
    ) const noexcept;
    static void note_quantity(
        RouteAuditQuantityStats& stats,
        RouteAuditQuantityStatus quantity
    ) noexcept;
    [[nodiscard]] int16_t highest_with(
        uint64_t RouteAuditPrefixStats::*field
    ) const noexcept;
};

#endif // TRADING_CARAVAN_SOLVER_ROUTESURVIVALAUDIT_H
