#include "DeferredSoldModel.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

#include "../game/Game.h"

namespace {
bool same_buy(const buy_list& lhs, const buy_list& rhs) noexcept {
    return lhs.food == rhs.food && lhs.spice == rhs.spice &&
           lhs.pottery == rhs.pottery && lhs.marble == rhs.marble &&
           lhs.silk == rhs.silk && lhs.jewelry == rhs.jewelry;
}

bool buy_less(const buy_list& lhs, const buy_list& rhs) noexcept {
    return std::tie(
        lhs.food,
        lhs.spice,
        lhs.pottery,
        lhs.marble,
        lhs.silk,
        lhs.jewelry
    ) < std::tie(
        rhs.food,
        rhs.spice,
        rhs.pottery,
        rhs.marble,
        rhs.silk,
        rhs.jewelry
    );
}

bool same_context(const Player& lhs, const Player& rhs) noexcept {
    if (lhs.trader != rhs.trader || lhs.camel != rhs.camel ||
        lhs.food_consumption != rhs.food_consumption ||
        lhs.bonus_food_consumption != rhs.bonus_food_consumption ||
        lhs.backpack != rhs.backpack || lhs.daily_income != rhs.daily_income ||
        std::lround(lhs.interest_rate * 100.0) !=
            std::lround(rhs.interest_rate * 100.0) ||
        lhs.set_interest != rhs.set_interest ||
        lhs.quick_interest != rhs.quick_interest || lhs.statue != rhs.statue ||
        lhs.hand_of_midas != rhs.hand_of_midas ||
        lhs.merch_discount != rhs.merch_discount ||
        lhs.trader_interest != rhs.trader_interest || lhs.day != rhs.day ||
        lhs.max_day != rhs.max_day || lhs.traveler_mode != rhs.traveler_mode ||
        lhs.merchant_happened != rhs.merchant_happened ||
        lhs.merchant_day != rhs.merchant_day ||
        lhs.call_merchant != rhs.call_merchant ||
        lhs.witch_happened != rhs.witch_happened ||
        lhs.witch_day != rhs.witch_day ||
        lhs.disaster_happened != rhs.disaster_happened ||
        lhs.disaster_day != rhs.disaster_day ||
        lhs.scheduled_disaster != rhs.scheduled_disaster ||
        lhs.day_start_skip != rhs.day_start_skip ||
        lhs.bought_last != rhs.bought_last ||
        lhs.cargo_buy_locked != rhs.cargo_buy_locked ||
        lhs.player_world.town != rhs.player_world.town ||
        lhs.player_world.merch != rhs.player_world.merch ||
        lhs.player_world.witch != rhs.player_world.witch ||
        lhs.player_world.disaster != rhs.player_world.disaster) {
        return false;
    }
    for (int item = i_Food; item <= i_Jewelry; ++item) {
        for (int datum = id_Buy; datum <= id_Weight; ++datum) {
            if (lhs.item_shop[item][datum] != rhs.item_shop[item][datum]) {
                return false;
            }
        }
    }
    for (int npc = n_Trader; npc <= n_Camel; ++npc) {
        for (int datum = nd_Cost; datum <= nd_Eat; ++datum) {
            if (lhs.npc_shop[npc][datum] != rhs.npc_shop[npc][datum]) {
                return false;
            }
        }
    }
    return true;
}

size_t context_hash(const Player& player) noexcept {
    size_t seed = 0x43616e6f6eULL;
    const auto combine = [&seed](const auto value) {
        using Value = std::decay_t<decltype(value)>;
        seed ^= std::hash<Value>{}(value) + 0x9e3779b97f4a7c15ULL +
                (seed << 6U) + (seed >> 2U);
    };
    combine(player.trader);
    combine(player.camel);
    combine(player.food_consumption);
    combine(player.bonus_food_consumption);
    combine(player.backpack);
    combine(player.daily_income);
    combine(std::lround(player.interest_rate * 100.0));
    combine(player.set_interest);
    combine(player.quick_interest);
    combine(player.statue);
    combine(player.hand_of_midas);
    combine(player.merch_discount);
    combine(player.trader_interest);
    combine(player.day);
    combine(player.max_day);
    combine(player.traveler_mode);
    combine(player.merchant_happened);
    combine(player.merchant_day);
    combine(player.call_merchant);
    combine(player.witch_happened);
    combine(player.witch_day);
    combine(player.disaster_happened);
    combine(player.disaster_day);
    combine(static_cast<uint8_t>(player.scheduled_disaster));
    combine(player.day_start_skip);
    combine(player.bought_last);
    combine(player.cargo_buy_locked);
    combine(static_cast<uint8_t>(player.player_world.town));
    combine(static_cast<uint8_t>(player.player_world.merch));
    combine(static_cast<uint8_t>(player.player_world.witch));
    combine(static_cast<uint8_t>(player.player_world.disaster));
    for (int item = i_Food; item <= i_Jewelry; ++item) {
        for (int datum = id_Buy; datum <= id_Weight; ++datum) {
            combine(player.item_shop[item][datum]);
        }
    }
    for (int npc = n_Trader; npc <= n_Camel; ++npc) {
        for (int datum = nd_Cost; datum <= nd_Eat; ++datum) {
            combine(player.npc_shop[npc][datum]);
        }
    }
    return seed;
}
}

DeferredSoldModel::DeferredSoldModel(
    Player representative_player,
    std::vector<EnvelopeRow> envelope,
    DeferredTraceRecipe trace
) : common(std::move(representative_player)),
    rows(std::make_shared<std::vector<EnvelopeRow>>(std::move(envelope))),
    purchase_trace(std::move(trace)) {
    canonicalize_rows();
    synchronize_representative();
}

std::optional<DeferredSoldModel> DeferredSoldModel::from_purchase(
    const DeferredPurchaseModel& purchase,
    BestPurchaseHandler& handler
) {
    std::vector<DeferredPurchaseResolution> resolutions =
        purchase.resolve_sell(handler);
    if (resolutions.empty()) {
        return std::nullopt;
    }

    std::vector<EnvelopeRow> envelope;
    envelope.reserve(resolutions.size());
    for (const DeferredPurchaseResolution& resolution : resolutions) {
        envelope.push_back(EnvelopeRow{
            resolution.player.food,
            resolution.player.gold,
            resolution.concrete_buy,
            resolution.parent_trace,
        });
    }
    return DeferredSoldModel(
        std::move(resolutions.front().player),
        std::move(envelope),
        purchase.trace_recipe()
    );
}

const Player& DeferredSoldModel::representative() const noexcept {
    return common;
}

size_t DeferredSoldModel::source_count() const noexcept {
    return rows->size();
}

size_t DeferredSoldModel::memory_bytes() const noexcept {
    std::unordered_set<const void*> counted;
    return sizeof(*this) + dynamic_memory_bytes(counted);
}

size_t DeferredSoldModel::dynamic_memory_bytes(
    std::unordered_set<const void*>& counted_shared
) const noexcept {
    size_t bytes = actions_after_sell.capacity() * sizeof(SolverAction) +
        purchase_trace.dynamic_memory_bytes(counted_shared);
    if (counted_shared.insert(rows.get()).second) {
        bytes += sizeof(std::vector<EnvelopeRow>) +
            rows->capacity() * sizeof(EnvelopeRow) + 2U * sizeof(void*);
    }
    return bytes;
}

bool DeferredSoldModel::semantically_equal(
    const DeferredSoldModel& other
) const noexcept {
    if (!same_context(common, other.common) || rows->size() != other.rows->size()) {
        return false;
    }
    for (size_t index = 0; index < rows->size(); ++index) {
        if ((*rows)[index].food != (*other.rows)[index].food ||
            (*rows)[index].gold != (*other.rows)[index].gold) {
            return false;
        }
    }
    return true;
}

size_t DeferredSoldModel::semantic_hash() const noexcept {
    size_t seed = context_hash(common);
    const auto combine = [&seed](const auto value) {
        using Value = std::decay_t<decltype(value)>;
        seed ^= std::hash<Value>{}(value) + 0x9e3779b97f4a7c15ULL +
                (seed << 6U) + (seed >> 2U);
    };
    for (const EnvelopeRow& row : *rows) {
        combine(row.food);
        combine(row.gold);
    }
    return seed;
}

size_t DeferredSoldModel::dominance_partition_hash() const noexcept {
    // Envelope shape deliberately stays out so different Sell histories can
    // prove containment. The future context remains part of the interface,
    // even though FrontierReducer also partitions it at the outer state seam.
    return context_hash(common);
}

bool DeferredSoldModel::dominates(
    const DeferredSoldModel& other,
    uint64_t* gold_function_comparisons,
    uint64_t* exact_spend_evaluations
) const noexcept {
    (void)exact_spend_evaluations;
    if (!same_context(common, other.common)) {
        return false;
    }
    const bool weightless = common.item_shop[i_Food][id_Weight] == 0;
    size_t same_food_index = 0;
    for (const EnvelopeRow& weaker : *other.rows) {
        bool covered = false;
        if (weightless) {
            for (const EnvelopeRow& stronger : *rows) {
                if (gold_function_comparisons != nullptr) {
                    ++*gold_function_comparisons;
                }
                if (stronger.food >= weaker.food && stronger.gold >= weaker.gold) {
                    covered = true;
                    break;
                }
            }
        } else {
            while (same_food_index < rows->size() &&
                   (*rows)[same_food_index].food < weaker.food) {
                ++same_food_index;
            }
            if (same_food_index < rows->size()) {
                if (gold_function_comparisons != nullptr) {
                    ++*gold_function_comparisons;
                }
                const EnvelopeRow& stronger = (*rows)[same_food_index];
                covered = stronger.food == weaker.food &&
                    stronger.gold >= weaker.gold;
            }
        }
        if (!covered) {
            return false;
        }
    }
    return true;
}

DeferredBuyCoverage DeferredSoldModel::classify_exact_buy(
    const buy_list& buy
) const noexcept {
    const bool exact = std::any_of(rows->begin(), rows->end(), [&](const auto& row) {
        return same_buy(row.concrete_buy, buy);
    });
    if (exact) {
        return DeferredBuyCoverage::Exact;
    }
    return rows->empty() ? DeferredBuyCoverage::Missing
                         : DeferredBuyCoverage::DominatingReplacement;
}

EarlyActionSkeleton DeferredSoldModel::extend_skeleton(
    EarlyActionSkeleton skeleton
) const noexcept {
    skeleton = skeleton.extended(SolverAction{_a_buy, buy_list(), 0});
    for (const SolverAction& action : purchase_trace.actions_after_buy()) {
        skeleton = skeleton.extended(action);
    }
    skeleton = skeleton.extended(SolverAction{_a_sell, buy_list(), 0});
    for (const SolverAction& action : actions_after_sell) {
        skeleton = skeleton.extended(action);
    }
    return skeleton;
}

std::vector<uint32_t> DeferredSoldModel::provenances() const {
    std::vector<uint32_t> result;
    result.reserve(rows->size());
    for (const EnvelopeRow& row : *rows) {
        result.push_back(row.parent_trace);
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

DeferredSoldModel DeferredSoldModel::remap_provenances(
    const std::unordered_map<uint32_t, uint32_t>& remap
) const {
    DeferredSoldModel result = *this;
    for (EnvelopeRow& row : result.mutable_rows()) {
        const auto found = remap.find(row.parent_trace);
        row.parent_trace = found == remap.end() ? 0 : found->second;
    }
    return result;
}

std::optional<DeferredSoldModel> DeferredSoldModel::after_start_of_day() const {
    DeferredSoldModel next = *this;
    if (!next.transform_rows([](Player& player) {
            Game::start_of_day(player);
            return player.food >= 0;
        })) {
        return std::nullopt;
    }
    return next;
}

std::optional<DeferredSoldModel> DeferredSoldModel::after_resume_same_day() const {
    DeferredSoldModel next = *this;
    if (!next.transform_rows([](Player& player) {
            player.day_start_skip = false;
            return true;
        })) {
        return std::nullopt;
    }
    return next;
}

std::optional<DeferredSoldModel> DeferredSoldModel::after_hire(
    const npcs selection
) const {
    DeferredSoldModel next = *this;
    if (!next.transform_rows([selection](Player& player) {
            if (player.day >= player.max_day ||
                player.gold < player.npc_shop[selection][nd_Cost] ||
                player.food < player.food_consumption +
                    player.npc_shop[selection][nd_Eat]) {
                return false;
            }
            Game::event_buy_npc(player, selection);
            return true;
        })) {
        return std::nullopt;
    }
    next.actions_after_sell.push_back(
        SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(selection)}
    );
    return next;
}

std::optional<DeferredSoldModel> DeferredSoldModel::after_interest() const {
    DeferredSoldModel next = *this;
    if (!next.transform_rows([](Player& player) {
            if (player.day != player.max_day &&
                player.food < player.food_consumption) {
                return false;
            }
            Game::event_interest(player);
            return true;
        })) {
        return std::nullopt;
    }
    next.actions_after_sell.push_back(SolverAction{_a_interest, buy_list(), 0});
    return next;
}

std::optional<DeferredSoldModel> DeferredSoldModel::after_merchant(
    const merch_options selection
) const {
    DeferredSoldModel next = *this;
    if (!next.transform_rows([selection](Player& player) {
            if (selection != m_No_Merchant &&
                !Game::check__afford_merchant(player, selection)) {
                return false;
            }
            return Game::event_merchant(player, selection);
        })) {
        return std::nullopt;
    }
    next.actions_after_sell.push_back(
        SolverAction{_a_merchant, buy_list(), static_cast<uint8_t>(selection)}
    );
    return next;
}

std::optional<DeferredSoldModel> DeferredSoldModel::after_witch(
    const witch_options selection
) const {
    DeferredSoldModel next = *this;
    if (!next.transform_rows([selection](Player& player) {
            Game::event_witch(player, selection);
            return true;
        })) {
        return std::nullopt;
    }
    next.actions_after_sell.push_back(
        SolverAction{_a_witch, buy_list(), static_cast<uint8_t>(selection)}
    );
    return next;
}

std::optional<DeferredSoldModel> DeferredSoldModel::after_disaster(
    const disaster_options selection
) const {
    DeferredSoldModel next = *this;
    if (!next.transform_rows([selection](Player& player) {
            Game::event_disaster(player, selection);
            return true;
        })) {
        return std::nullopt;
    }
    next.actions_after_sell.push_back(
        SolverAction{_a_disaster, buy_list(), static_cast<uint8_t>(selection)}
    );
    return next;
}

std::vector<DeferredPurchaseResolution> DeferredSoldModel::materialize() const {
    const SolverAction sell_action{_a_sell, buy_list(), 0};
    std::vector<DeferredPurchaseResolution> result;
    result.reserve(rows->size());
    for (const EnvelopeRow& row : *rows) {
        std::vector<SolverAction> actions = purchase_trace.resolve(
            row.concrete_buy,
            &sell_action
        );
        actions.insert(
            actions.end(),
            actions_after_sell.begin(),
            actions_after_sell.end()
        );
        result.push_back(DeferredPurchaseResolution{
            player_for(row),
            row.concrete_buy,
            row.parent_trace,
            std::move(actions),
        });
    }
    return result;
}

std::vector<DeferredPurchaseResolution> DeferredSoldModel::resolve_finish(
    const size_t limit
) const {
    std::vector<DeferredPurchaseResolution> finalists = materialize();
    std::sort(finalists.begin(), finalists.end(), [](const auto& lhs, const auto& rhs) {
        const int lhs_points = Game::check_points(lhs.player, "Final");
        const int rhs_points = Game::check_points(rhs.player, "Final");
        if (lhs_points != rhs_points) {
            return lhs_points > rhs_points;
        }
        return lhs.parent_trace < rhs.parent_trace;
    });
    if (finalists.size() > limit) {
        finalists.resize(limit);
    }
    return finalists;
}

std::vector<DeferredSoldModel::EnvelopeRow>& DeferredSoldModel::mutable_rows() {
    if (!rows.unique()) {
        rows = std::make_shared<std::vector<EnvelopeRow>>(*rows);
    }
    return *rows;
}

Player DeferredSoldModel::player_for(const EnvelopeRow& row) const {
    Player player = common;
    player.gold = row.gold;
    player.food = row.food;
    player.points = 0;
    return player;
}

bool DeferredSoldModel::transform_rows(
    const std::function<bool(Player&)>& transform
) {
    std::vector<EnvelopeRow> transformed;
    transformed.reserve(rows->size());
    std::optional<Player> transformed_template;
    for (EnvelopeRow row : *rows) {
        Player player = player_for(row);
        if (!transform(player)) {
            continue;
        }
        row.food = player.food;
        row.gold = player.gold;
        transformed.push_back(std::move(row));
        if (!transformed_template.has_value()) {
            transformed_template = std::move(player);
        }
    }
    if (transformed.empty() || !transformed_template.has_value()) {
        return false;
    }
    common = std::move(*transformed_template);
    rows = std::make_shared<std::vector<EnvelopeRow>>(std::move(transformed));
    canonicalize_rows();
    synchronize_representative();
    return true;
}

void DeferredSoldModel::canonicalize_rows() {
    std::vector<EnvelopeRow>& envelope = mutable_rows();
    std::sort(envelope.begin(), envelope.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.food != rhs.food) {
            return lhs.food < rhs.food;
        }
        if (lhs.gold != rhs.gold) {
            return lhs.gold > rhs.gold;
        }
        if (lhs.parent_trace != rhs.parent_trace) {
            return lhs.parent_trace < rhs.parent_trace;
        }
        return buy_less(lhs.concrete_buy, rhs.concrete_buy);
    });
    envelope.erase(
        std::unique(envelope.begin(), envelope.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.food == rhs.food;
        }),
        envelope.end()
    );

    if (common.item_shop[i_Food][id_Weight] == 0) {
        std::sort(envelope.begin(), envelope.end(), [](const auto& lhs, const auto& rhs) {
            if (lhs.gold != rhs.gold) {
                return lhs.gold > rhs.gold;
            }
            return lhs.food > rhs.food;
        });
        int greatest_food = std::numeric_limits<int>::min();
        envelope.erase(
            std::remove_if(envelope.begin(), envelope.end(), [&](const auto& row) {
                if (row.food <= greatest_food) {
                    return true;
                }
                greatest_food = row.food;
                return false;
            }),
            envelope.end()
        );
        std::sort(envelope.begin(), envelope.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.food < rhs.food;
        });
    }
}

void DeferredSoldModel::synchronize_representative() {
    if (rows->empty()) {
        return;
    }
    const EnvelopeRow& best = *std::max_element(
        rows->begin(),
        rows->end(),
        [](const auto& lhs, const auto& rhs) {
            if (lhs.gold != rhs.gold) {
                return lhs.gold < rhs.gold;
            }
            return lhs.food < rhs.food;
        }
    );
    common.gold = best.gold;
    common.food = best.food;
    common.points = 0;
}
