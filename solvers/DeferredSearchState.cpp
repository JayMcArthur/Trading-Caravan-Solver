#include "DeferredSearchState.h"

#include <algorithm>
#include <atomic>
#include <stdexcept>
#include <utility>

#include "../game/Game.h"

namespace {
std::atomic<uint64_t> next_deferred_identity{1};
}

DeferredSearchState::DeferredSearchState(DeferredPurchaseModel purchase)
    : identity(next_deferred_identity.fetch_add(1, std::memory_order_relaxed)),
      state(std::move(purchase)) {}

DeferredSearchState::DeferredSearchState(DeferredSoldModel sold)
    : identity(next_deferred_identity.fetch_add(1, std::memory_order_relaxed)),
      state(std::move(sold)) {}

bool DeferredSearchState::has_open_purchase() const noexcept {
    return std::holds_alternative<DeferredPurchaseModel>(state);
}

bool DeferredSearchState::has_sold_ledger() const noexcept {
    return std::holds_alternative<DeferredSoldModel>(state);
}

const DeferredPurchaseModel& DeferredSearchState::purchase() const {
    if (!has_open_purchase()) {
        throw std::logic_error("Deferred search state has no open purchase");
    }
    return std::get<DeferredPurchaseModel>(state);
}

const DeferredSoldModel& DeferredSearchState::sold() const {
    if (!has_sold_ledger()) {
        throw std::logic_error("Deferred search state has no sold demand ledger");
    }
    return std::get<DeferredSoldModel>(state);
}

const Player& DeferredSearchState::representative() const noexcept {
    if (has_open_purchase()) {
        return std::get<DeferredPurchaseModel>(state).representative();
    }
    return std::get<DeferredSoldModel>(state).representative();
}

size_t DeferredSearchState::row_count() const noexcept {
    if (has_open_purchase()) {
        return std::get<DeferredPurchaseModel>(state).origin_count();
    }
    return std::get<DeferredSoldModel>(state).source_count();
}

size_t DeferredSearchState::memory_bytes() const noexcept {
    std::unordered_set<const void*> counted;
    return memory_bytes(counted);
}

size_t DeferredSearchState::memory_bytes(
    std::unordered_set<const void*>& counted_shared
) const noexcept {
    if (has_open_purchase()) {
        return sizeof(*this) +
            std::get<DeferredPurchaseModel>(state).dynamic_memory_bytes(counted_shared);
    }
    return sizeof(*this) +
        std::get<DeferredSoldModel>(state).dynamic_memory_bytes(counted_shared);
}

uint64_t DeferredSearchState::ranking_identity() const noexcept {
    return identity;
}

std::vector<DeferredContinuationView> DeferredSearchState::continuation_views(
    BestPurchaseHandler& handler,
    const bool player_action_reachable
) const {
    std::vector<DeferredContinuationView> views;
    if (!player_action_reachable) {
        return views;
    }
    if (has_sold_ledger()) {
        const DeferredSoldModel& unresolved = sold();
        std::optional<DeferredSoldModel> working = unresolved.representative().day_start_skip
            ? unresolved.after_resume_same_day()
            : unresolved.after_start_of_day();
        if (!working.has_value()) {
            return views;
        }
        if (auto trader = working->after_hire(n_Trader); trader.has_value()) {
            views.push_back(DeferredContinuationView{
                DeferredContinuationKind::Trader,
                trader->representative(),
            });
        }
        if (auto camel = working->after_hire(n_Camel); camel.has_value()) {
            views.push_back(DeferredContinuationView{
                DeferredContinuationKind::Camel,
                camel->representative(),
            });
        }
        if (auto interest = working->after_interest(); interest.has_value()) {
            views.push_back(DeferredContinuationView{
                DeferredContinuationKind::Interest,
                interest->representative(),
            });
        }
        return views;
    }

    const DeferredPurchaseModel& unresolved = purchase();
    std::optional<DeferredPurchaseModel> working = unresolved.representative().day_start_skip
        ? std::optional<DeferredPurchaseModel>(unresolved.after_resume_same_day())
        : unresolved.after_start_of_day();
    if (!working.has_value()) {
        return views;
    }

    const Player& current = working->representative();
    const int required_after_sell = current.day == current.max_day
        ? 0
        : current.food_consumption;
    if (auto sold = working->resolve_sell_best_at_least(handler, required_after_sell);
        sold.has_value()) {
        views.push_back(DeferredContinuationView{
            DeferredContinuationKind::ImmediateSell,
            std::move(sold->player),
        });
    }
    if (auto trader = working->after_hire(n_Trader); trader.has_value()) {
        views.push_back(DeferredContinuationView{
            DeferredContinuationKind::Trader,
            trader->representative(),
        });
    }
    if (auto camel = working->after_hire(n_Camel); camel.has_value()) {
        views.push_back(DeferredContinuationView{
            DeferredContinuationKind::Camel,
            camel->representative(),
        });
    }
    if (auto interest = working->after_interest(); interest.has_value()) {
        views.push_back(DeferredContinuationView{
            DeferredContinuationKind::Interest,
            interest->representative(),
        });
    }

    if (current.day < current.max_day && !current.bought_last &&
        Game::check_can_buy_items(current)) {
        const auto resolutions = working->resolve_food_only_for_next_buy();
        std::vector<DeferredPurchaseSeed> seeds;
        seeds.reserve(resolutions.size());
        for (const DeferredPurchaseResolution& resolution : resolutions) {
            seeds.push_back(DeferredPurchaseSeed{
                resolution.player.gold,
                resolution.player.food,
                resolution.parent_trace,
            });
        }
        if (!seeds.empty()) {
            DeferredPurchaseModel next = DeferredPurchaseModel::open(
                resolutions.front().player,
                seeds
            );
            views.push_back(DeferredContinuationView{
                DeferredContinuationKind::FoodOnlyBuy,
                next.representative(),
            });
        }
    }
    return views;
}

bool DeferredSearchState::semantically_equal(const DeferredSearchState& other) const noexcept {
    if (state.index() != other.state.index()) {
        return false;
    }
    if (has_open_purchase()) {
        return purchase().semantically_equal(other.purchase());
    }
    return sold().semantically_equal(other.sold());
}

size_t DeferredSearchState::semantic_hash() const noexcept {
    const size_t mode = has_open_purchase() ? 0x51d7348dU : 0xd7ac631fU;
    const size_t value = has_open_purchase() ? purchase().semantic_hash() : sold().semantic_hash();
    return value ^ (mode + 0x9e3779b97f4a7c15ULL + (value << 6U) + (value >> 2U));
}

size_t DeferredSearchState::dominance_partition_hash() const noexcept {
    const size_t mode = has_open_purchase() ? 0x724f88a1U : 0xa36b70d5U;
    const size_t value = has_open_purchase()
        ? purchase().dominance_partition_hash()
        : sold().dominance_partition_hash();
    return value ^ (mode + 0x9e3779b97f4a7c15ULL + (value << 6U) + (value >> 2U));
}

bool DeferredSearchState::dominates(
    const DeferredSearchState& other,
    DeferredDominanceMetrics* metrics
) const noexcept {
    if (state.index() != other.state.index() ||
        dominance_partition_hash() != other.dominance_partition_hash()) {
        return false;
    }
    uint64_t* gold_comparisons = metrics == nullptr
        ? nullptr
        : &metrics->gold_function_comparisons;
    uint64_t* spend_evaluations = metrics == nullptr
        ? nullptr
        : &metrics->exact_spend_evaluations;
    if (has_open_purchase()) {
        return purchase().dominates(
            other.purchase(),
            gold_comparisons,
            spend_evaluations,
            metrics == nullptr ? nullptr : &metrics->origin_coverage_checks,
            metrics == nullptr ? nullptr : &metrics->successful_cross_shape_checks
        );
    }
    return sold().dominates(
        other.sold(),
        gold_comparisons,
        spend_evaluations
    );
}

EarlyActionSkeleton DeferredSearchState::extend_skeleton(EarlyActionSkeleton skeleton) const noexcept {
    if (has_open_purchase()) {
        return purchase().extend_skeleton(skeleton);
    }
    return sold().extend_skeleton(skeleton);
}

std::vector<uint32_t> DeferredSearchState::provenances() const {
    if (has_open_purchase()) {
        return purchase().provenances();
    }
    return sold().provenances();
}

DeferredSearchState DeferredSearchState::remap_provenances(
    const std::unordered_map<uint32_t, uint32_t>& remap
) const {
    if (has_open_purchase()) {
        return DeferredSearchState(purchase().remap_provenances(remap));
    }
    return DeferredSearchState(sold().remap_provenances(remap));
}
