#include "RouteSurvivalAudit.h"

#include <algorithm>
#include <ostream>

namespace {
const char* action_name(const game_actions action) noexcept {
    switch (action) {
        case _a_buy:
            return "Buy";
        case _a_sell:
            return "Sell";
        case _a_npc:
            return "Crew";
        case _a_interest:
            return "Interest";
        case _a_merchant:
            return "Merchant";
        case _a_witch:
            return "Witch";
        case _a_wait:
            return "Wait";
        case _a_disaster:
            return "Disaster";
    }
    return "Unknown";
}
}

RouteSurvivalAudit::RouteSurvivalAudit(std::vector<SolverAction> route)
    : target_route(std::move(route)),
      stats(target_route.size() + 1U) {}

bool RouteSurvivalAudit::enabled() const noexcept {
    return !target_route.empty();
}

size_t RouteSurvivalAudit::target_action_count() const noexcept {
    return target_route.size();
}

bool RouteSurvivalAudit::action_matches(
    const SolverAction& expected,
    const SolverAction& generated,
    const bool exact_buy
) noexcept {
    if (expected.action != generated.action) {
        return false;
    }
    switch (expected.action) {
        case _a_npc:
        case _a_merchant:
        case _a_witch:
        case _a_disaster:
            return expected.option == generated.option;
        case _a_buy:
            return !exact_buy ||
                (expected.buy.food == generated.buy.food &&
                 expected.buy.spice == generated.buy.spice &&
                 expected.buy.pottery == generated.buy.pottery &&
                 expected.buy.marble == generated.buy.marble &&
                 expected.buy.silk == generated.buy.silk &&
                 expected.buy.jewelry == generated.buy.jewelry);
        case _a_sell:
        case _a_interest:
        case _a_wait:
            return true;
    }
    return false;
}

int16_t RouteSurvivalAudit::advance(
    const int16_t progress,
    const SolverAction& action
) const noexcept {
    if (!enabled() || progress < 0 || static_cast<size_t>(progress) >= target_route.size()) {
        return NO_MATCH;
    }
    return action_matches(target_route[static_cast<size_t>(progress)], action, false)
        ? static_cast<int16_t>(progress + 1)
        : NO_MATCH;
}

int16_t RouteSurvivalAudit::advance_exact(
    const int16_t progress,
    const SolverAction& action
) const noexcept {
    if (!enabled() || progress < 0 || static_cast<size_t>(progress) >= target_route.size()) {
        return NO_MATCH;
    }
    return action_matches(target_route[static_cast<size_t>(progress)], action, true)
        ? static_cast<int16_t>(progress + 1)
        : NO_MATCH;
}

int16_t RouteSurvivalAudit::match_prefix(
    const std::vector<SolverAction>& actions
) const noexcept {
    if (!enabled()) {
        return NO_MATCH;
    }
    int16_t progress = 0;
    for (const SolverAction& action : actions) {
        progress = advance(progress, action);
        if (progress == NO_MATCH) {
            return NO_MATCH;
        }
    }
    return progress;
}

int16_t RouteSurvivalAudit::match_exact_prefix(
    const std::vector<SolverAction>& actions
) const noexcept {
    if (!enabled()) {
        return NO_MATCH;
    }
    int16_t progress = 0;
    for (const SolverAction& action : actions) {
        progress = advance_exact(progress, action);
        if (progress == NO_MATCH) {
            return NO_MATCH;
        }
    }
    return progress;
}

const buy_list* RouteSurvivalAudit::expected_buy_before(
    const int16_t progress,
    const bool require_sell_after_buy
) const noexcept {
    if (progress <= 0 || static_cast<size_t>(progress) > target_route.size()) {
        return nullptr;
    }
    bool saw_sell = false;
    for (size_t index = static_cast<size_t>(progress); index > 0; --index) {
        const SolverAction& action = target_route[index - 1];
        if (action.action == _a_sell) {
            if (!require_sell_after_buy) {
                return nullptr;
            }
            saw_sell = true;
            continue;
        }
        if (action.action == _a_buy) {
            if (require_sell_after_buy != saw_sell) {
                return nullptr;
            }
            return &action.buy;
        }
    }
    return nullptr;
}

const buy_list* RouteSurvivalAudit::expected_open_buy(const int16_t progress) const noexcept {
    return expected_buy_before(progress, false);
}

const buy_list* RouteSurvivalAudit::expected_sold_buy(const int16_t progress) const noexcept {
    return expected_buy_before(progress, true);
}

void RouteSurvivalAudit::note_quantity(
    RouteAuditQuantityStats& entry,
    const RouteAuditQuantityStatus quantity
) noexcept {
    switch (quantity) {
        case RouteAuditQuantityStatus::Exact:
            ++entry.exact;
            break;
        case RouteAuditQuantityStatus::DominatingReplacement:
            ++entry.dominating_replacement;
            break;
        case RouteAuditQuantityStatus::Missing:
            ++entry.missing;
            break;
        case RouteAuditQuantityStatus::NotApplicable:
            break;
    }
}

void RouteSurvivalAudit::note_generated(
    const int16_t progress,
    const RouteAuditQuantityStatus quantity
) noexcept {
    if (progress >= 0 && static_cast<size_t>(progress) < stats.size()) {
        RouteAuditPrefixStats& entry = stats[static_cast<size_t>(progress)];
        ++entry.generated;
        note_quantity(entry.quantity_generated, quantity);
    }
}

void RouteSurvivalAudit::note_reduction(
    const RouteAuditReductionStage stage,
    const int16_t progress,
    const RouteAuditQuantityStatus quantity
) noexcept {
    if (progress < 0 || static_cast<size_t>(progress) >= stats.size()) {
        return;
    }
    RouteAuditPrefixStats& entry = stats[static_cast<size_t>(progress)];
    switch (stage) {
        case RouteAuditReductionStage::BeforeExact:
            ++entry.before_exact;
            note_quantity(entry.quantity_before_exact, quantity);
            break;
        case RouteAuditReductionStage::AfterExact:
            ++entry.after_exact;
            note_quantity(entry.quantity_after_exact, quantity);
            break;
        case RouteAuditReductionStage::AfterDominance:
            ++entry.after_dominance;
            note_quantity(entry.quantity_after_dominance, quantity);
            break;
        case RouteAuditReductionStage::AfterBeam:
            ++entry.after_beam;
            note_quantity(entry.quantity_after_beam, quantity);
            break;
    }
}

void RouteSurvivalAudit::note_materialized(
    const std::vector<SolverAction>& actions
) noexcept {
    const int16_t progress = match_exact_prefix(actions);
    if (progress >= 0 && static_cast<size_t>(progress) < stats.size()) {
        ++stats[static_cast<size_t>(progress)].quantity_materialized;
    }
}

void RouteSurvivalAudit::note_finished(const int16_t progress) noexcept {
    if (progress >= 0 && static_cast<size_t>(progress) < stats.size()) {
        ++stats[static_cast<size_t>(progress)].finished;
    }
}

void RouteSurvivalAudit::note_ranks(
    const int16_t progress,
    const size_t realistic_rank,
    const size_t optimistic_rank,
    const std::array<size_t, CONTINUATION_LANE_COUNT>& lane_ranks
) noexcept {
    if (progress < 0 || static_cast<size_t>(progress) >= stats.size()) {
        return;
    }
    RouteAuditPrefixStats& entry = stats[static_cast<size_t>(progress)];
    const auto retain_best = [](size_t& destination, const size_t value) {
        if (value > 0 && (destination == 0 || value < destination)) {
            destination = value;
        }
    };
    retain_best(entry.best_realistic_rank, realistic_rank);
    retain_best(entry.best_optimistic_rank, optimistic_rank);
    for (size_t lane = 0; lane < CONTINUATION_LANE_COUNT; ++lane) {
        retain_best(entry.best_lane_ranks[lane], lane_ranks[lane]);
    }
}

int16_t RouteSurvivalAudit::highest_with(
    uint64_t RouteAuditPrefixStats::*field
) const noexcept {
    for (size_t index = stats.size(); index > 0; --index) {
        if (stats[index - 1].*field > 0) {
            return static_cast<int16_t>(index - 1);
        }
    }
    return NO_MATCH;
}

int16_t RouteSurvivalAudit::highest_generated_prefix() const noexcept {
    return highest_with(&RouteAuditPrefixStats::generated);
}

int16_t RouteSurvivalAudit::highest_retained_prefix() const noexcept {
    return std::max(
        highest_with(&RouteAuditPrefixStats::after_beam),
        highest_with(&RouteAuditPrefixStats::finished)
    );
}

const std::vector<RouteAuditPrefixStats>& RouteSurvivalAudit::prefix_stats() const noexcept {
    return stats;
}

const std::vector<SolverAction>& RouteSurvivalAudit::route() const noexcept {
    return target_route;
}

void RouteSurvivalAudit::write_json(std::ostream& out) const {
    if (!enabled()) {
        out << "null";
        return;
    }
    out << '{'
        << "\"mode\":\"symbolic-exact-buy\""
        << ",\"buy_quantities_deferred\":true"
        << ",\"target_actions\":" << target_route.size()
        << ",\"highest_generated_prefix\":" << highest_generated_prefix()
        << ",\"highest_retained_prefix\":" << highest_retained_prefix()
        << ",\"prefixes\":[";
    for (size_t prefix = 0; prefix < stats.size(); ++prefix) {
        if (prefix != 0) {
            out << ',';
        }
        const RouteAuditPrefixStats& entry = stats[prefix];
        out << '{'
            << "\"prefix\":" << prefix
            << ",\"next_action\":";
        if (prefix < target_route.size()) {
            out << '"' << action_name(target_route[prefix].action) << '"';
        } else {
            out << "null";
        }
        out << ",\"generated\":" << entry.generated
            << ",\"before_exact\":" << entry.before_exact
            << ",\"after_exact\":" << entry.after_exact
            << ",\"after_dominance\":" << entry.after_dominance
            << ",\"after_beam\":" << entry.after_beam
            << ",\"finished\":" << entry.finished
            << ",\"best_realistic_rank\":" << entry.best_realistic_rank
            << ",\"best_optimistic_rank\":" << entry.best_optimistic_rank;
        const auto write_quantity = [&out](
            const char* name,
            const RouteAuditQuantityStats& quantity
        ) {
            out << ",\"" << name << "\":{" 
                << "\"exact\":" << quantity.exact
                << ",\"dominating_replacement\":" << quantity.dominating_replacement
                << ",\"missing\":" << quantity.missing
                << '}';
        };
        write_quantity("quantity_generated", entry.quantity_generated);
        write_quantity("quantity_before_exact", entry.quantity_before_exact);
        write_quantity("quantity_after_exact", entry.quantity_after_exact);
        write_quantity("quantity_after_dominance", entry.quantity_after_dominance);
        write_quantity("quantity_after_beam", entry.quantity_after_beam);
        out << ",\"quantity_materialized\":" << entry.quantity_materialized
            << ",\"best_lane_ranks\":[";
        for (size_t lane = 0; lane < CONTINUATION_LANE_COUNT; ++lane) {
            if (lane != 0) {
                out << ',';
            }
            out << entry.best_lane_ranks[lane];
        }
        out << "]}";
    }
    out << "]}";
}
