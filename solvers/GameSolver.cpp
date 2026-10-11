#include "GameSolver.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#ifdef __linux__
#include <unistd.h>
#endif
#include "../game/Game.h"
#include "DeferredPurchaseModel.h"
#include "DeferredSearchState.h"
#include "DeferredSoldModel.h"
#include "EncounterTiming.h"
#include "FrontierReducer.h"
#include "AdmissibleUpperBound.h"
#include "PersistentSearchGraph.h"
#include "SolverStateCodec.h"
#include "helpers.h"

namespace {
class ScopedNanosecondTimer {
public:
    explicit ScopedNanosecondTimer(uint64_t& destination) noexcept
        : destination_(destination), started_at_(std::chrono::steady_clock::now()) {}

    ~ScopedNanosecondTimer() {
        destination_ += static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - started_at_
            ).count()
        );
    }

    ScopedNanosecondTimer(const ScopedNanosecondTimer&) = delete;
    ScopedNanosecondTimer& operator=(const ScopedNanosecondTimer&) = delete;

private:
    uint64_t& destination_;
    std::chrono::steady_clock::time_point started_at_;
};

template <typename T>
void hash_combine(size_t& seed, const T& value) {
    seed ^= std::hash<T>{}(value) + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
}

struct FractionalItemBound {
    long double profit = 0.0L;
    long double cost = 0.0L;
    long double weight = 0.0L;
    long double density = 0.0L;
};

std::string json_escape(const std::string& value) {
    std::ostringstream escaped;
    for (const unsigned char character : value) {
        switch (character) {
            case '\\':
                escaped << "\\\\";
                break;
            case '"':
                escaped << "\\\"";
                break;
            case '\n':
                escaped << "\\n";
                break;
            case '\r':
                escaped << "\\r";
                break;
            case '\t':
                escaped << "\\t";
                break;
            default:
                if (character < 0x20U) {
                    escaped << "\\u"
                            << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(character)
                            << std::dec << std::setfill(' ');
                } else {
                    escaped << static_cast<char>(character);
                }
        }
    }
    return escaped.str();
}

const char* hybrid_skeleton_order_name(const HybridSkeletonOrder order) noexcept {
    switch (order) {
        case HybridSkeletonOrder::Score:
            return "score";
        case HybridSkeletonOrder::Cohort:
            return "cohort";
        case HybridSkeletonOrder::Interleave:
            return "interleave";
        case HybridSkeletonOrder::StructureInterleave:
            return "structure-interleave";
    }
    return "score";
}

ga_data replay_event(const SolverAction& action) {
    switch (action.action) {
        case _a_buy:
            return ga_data(
                _a_buy,
                action.buy.food,
                action.buy.spice,
                action.buy.pottery,
                action.buy.marble,
                action.buy.silk,
                action.buy.jewelry
            );
        case _a_npc:
            return ga_data(_a_npc, static_cast<npcs>(action.option));
        case _a_merchant:
            return ga_data(_a_merchant, static_cast<merch_options>(action.option));
        case _a_witch:
            return ga_data(_a_witch, static_cast<witch_options>(action.option));
        case _a_disaster:
            return ga_data(_a_disaster, static_cast<disaster_options>(action.option));
        case _a_sell:
        case _a_interest:
        case _a_wait:
            return ga_data(action.action);
    }
    throw std::logic_error("Unknown solver action in route replay");
}
}

bool SolverState::operator==(const SolverState& other) const noexcept {
    const bool buckets_equal =
        (!deferred && !other.deferred) ||
        (deferred && other.deferred && deferred->semantically_equal(*other.deferred));
    return buckets_equal &&
           gold == other.gold &&
           food == other.food &&
           spice == other.spice &&
           pottery == other.pottery &&
           marble == other.marble &&
           silk == other.silk &&
           jewelry == other.jewelry &&
           trader == other.trader &&
           camel == other.camel &&
           food_consumption == other.food_consumption &&
           bonus_food_consumption == other.bonus_food_consumption &&
           backpack == other.backpack &&
           daily_income == other.daily_income &&
           day == other.day &&
           max_day == other.max_day &&
           interest_hundredths == other.interest_hundredths &&
           disaster_day == other.disaster_day &&
           scheduled_disaster == other.scheduled_disaster &&
           merchant_day == other.merchant_day &&
           witch_day == other.witch_day &&
           player_world.town == other.player_world.town &&
           player_world.merch == other.player_world.merch &&
           player_world.witch == other.player_world.witch &&
           player_world.disaster == other.player_world.disaster &&
           flags == other.flags &&
           item_shop == other.item_shop &&
           npc_shop == other.npc_shop;
}

bool SolverState::has_flag(const uint16_t flag) const noexcept {
    return (flags & flag) != 0;
}

void SolverState::set_flag(const uint16_t flag, const bool value) noexcept {
    if (value) {
        flags |= flag;
    } else {
        flags &= static_cast<uint16_t>(~flag);
    }
}

Player SolverState::to_player() const {
    Player player;
    player.gold = gold;
    player.food = food;
    player.spice = spice;
    player.pottery = pottery;
    player.marble = marble;
    player.silk = silk;
    player.jewelry = jewelry;
    player.trader = trader;
    player.camel = camel;
    player.food_consumption = food_consumption;
    player.bonus_food_consumption = bonus_food_consumption;
    player.backpack = backpack;
    player.daily_income = daily_income;
    player.interest_rate = static_cast<double>(interest_hundredths) / 100.0;
    player.set_interest = has_flag(SSF_SET_INTEREST);
    player.quick_interest = has_flag(SSF_QUICK_INTEREST);
    player.statue = has_flag(SSF_STATUE);
    player.hand_of_midas = has_flag(SSF_HAND_OF_MIDAS);
    player.merch_discount = has_flag(SSF_MERCH_DISCOUNT);
    player.trader_interest = has_flag(SSF_TRADER_INTEREST);
    player.day = day;
    player.max_day = max_day;
    player.traveler_mode = has_flag(SSF_TRAVELER_MODE);
    player.merchant_happened = has_flag(SSF_MERCHANT_HAPPENED);
    player.merchant_day = merchant_day;
    player.call_merchant = has_flag(SSF_CALL_MERCHANT);
    player.witch_happened = has_flag(SSF_WITCH_HAPPENED);
    player.witch_day = witch_day;
    player.disaster_happened = has_flag(SSF_DISASTER_HAPPENED);
    player.disaster_day = disaster_day;
    player.scheduled_disaster = static_cast<disaster_options>(scheduled_disaster);
    player.day_start_skip = has_flag(SSF_DAY_START_SKIP);
    player.bought_last = has_flag(SSF_BOUGHT_LAST);
    player.cargo_buy_locked = has_flag(SSF_CARGO_BUY_LOCKED);
    player.player_world = player_world;
    for (size_t index = 0; index < item_shop.size(); ++index) {
        reinterpret_cast<int*>(player.item_shop)[index] = item_shop[index];
    }
    for (size_t index = 0; index < npc_shop.size(); ++index) {
        reinterpret_cast<int*>(player.npc_shop)[index] = npc_shop[index];
    }
    player.trace_id = trace_id;
    player.last_action.clear();
    player.available_actions.clear();
    player.points = 0;
    return player;
}

SolverState SolverState::from_player(const Player& player, const uint32_t trace_id_value) {
    SolverState state;
    state.gold = player.gold;
    state.food = player.food;
    state.spice = player.spice;
    state.pottery = player.pottery;
    state.marble = player.marble;
    state.silk = player.silk;
    state.jewelry = player.jewelry;
    state.trader = player.trader;
    state.camel = player.camel;
    state.food_consumption = player.food_consumption;
    state.bonus_food_consumption = player.bonus_food_consumption;
    state.backpack = player.backpack;
    state.daily_income = player.daily_income;
    state.day = player.day;
    state.max_day = player.max_day;
    state.interest_hundredths = static_cast<int16_t>(std::lround(player.interest_rate * 100.0));
    state.disaster_day = static_cast<uint8_t>(player.disaster_day);
    state.scheduled_disaster = static_cast<uint8_t>(player.scheduled_disaster);
    state.merchant_day = static_cast<uint8_t>(player.merchant_day);
    state.witch_day = static_cast<uint8_t>(player.witch_day);
    state.player_world = player.player_world;
    state.set_flag(SSF_SET_INTEREST, player.set_interest);
    state.set_flag(SSF_QUICK_INTEREST, player.quick_interest);
    state.set_flag(SSF_STATUE, player.statue);
    state.set_flag(SSF_HAND_OF_MIDAS, player.hand_of_midas);
    state.set_flag(SSF_MERCH_DISCOUNT, player.merch_discount);
    state.set_flag(SSF_TRADER_INTEREST, player.trader_interest);
    state.set_flag(SSF_MERCHANT_HAPPENED, player.merchant_happened);
    state.set_flag(SSF_CALL_MERCHANT, player.call_merchant);
    state.set_flag(SSF_WITCH_HAPPENED, player.witch_happened);
    state.set_flag(SSF_DISASTER_HAPPENED, player.disaster_happened);
    state.set_flag(SSF_TRAVELER_MODE, player.traveler_mode);
    state.set_flag(SSF_DAY_START_SKIP, player.day_start_skip);
    state.set_flag(SSF_BOUGHT_LAST, player.bought_last);
    state.set_flag(SSF_CARGO_BUY_LOCKED, player.cargo_buy_locked);
    for (size_t index = 0; index < state.item_shop.size(); ++index) {
        state.item_shop[index] = static_cast<int16_t>(reinterpret_cast<const int*>(player.item_shop)[index]);
    }
    for (size_t index = 0; index < state.npc_shop.size(); ++index) {
        state.npc_shop[index] = static_cast<int16_t>(reinterpret_cast<const int*>(player.npc_shop)[index]);
    }
    state.trace_id = trace_id_value;
    return state;
}

size_t SolverStateHash::operator()(const SolverState& state) const noexcept {
    size_t seed = 0;
    hash_combine(seed, state.gold);
    hash_combine(seed, state.food);
    hash_combine(seed, state.spice);
    hash_combine(seed, state.pottery);
    hash_combine(seed, state.marble);
    hash_combine(seed, state.silk);
    hash_combine(seed, state.jewelry);
    hash_combine(seed, state.trader);
    hash_combine(seed, state.camel);
    hash_combine(seed, state.food_consumption);
    hash_combine(seed, state.bonus_food_consumption);
    hash_combine(seed, state.backpack);
    hash_combine(seed, state.daily_income);
    hash_combine(seed, state.day);
    hash_combine(seed, state.max_day);
    hash_combine(seed, state.interest_hundredths);
    hash_combine(seed, state.disaster_day);
    hash_combine(seed, state.scheduled_disaster);
    hash_combine(seed, state.merchant_day);
    hash_combine(seed, state.witch_day);
    hash_combine(seed, state.player_world.town);
    hash_combine(seed, state.player_world.merch);
    hash_combine(seed, state.player_world.witch);
    hash_combine(seed, state.player_world.disaster);
    hash_combine(seed, state.flags);
    hash_combine(seed, state.deferred ? state.deferred->semantic_hash() : 0U);
    for (int value : state.item_shop) {
        hash_combine(seed, value);
    }
    for (int value : state.npc_shop) {
        hash_combine(seed, value);
    }
    return seed;
}

bool SolverContextKey::operator==(const SolverContextKey& other) const noexcept {
    return trader == other.trader &&
           camel == other.camel &&
           food_consumption == other.food_consumption &&
           bonus_food_consumption == other.bonus_food_consumption &&
           backpack == other.backpack &&
           daily_income == other.daily_income &&
           day == other.day &&
           max_day == other.max_day &&
           interest_hundredths == other.interest_hundredths &&
           disaster_day == other.disaster_day &&
           scheduled_disaster == other.scheduled_disaster &&
           merchant_day == other.merchant_day &&
           witch_day == other.witch_day &&
           player_world.town == other.player_world.town &&
           player_world.merch == other.player_world.merch &&
           player_world.witch == other.player_world.witch &&
           player_world.disaster == other.player_world.disaster &&
           flags == other.flags &&
           item_shop == other.item_shop &&
           npc_shop == other.npc_shop;
}

SolverContextKey SolverContextKey::from_state(const SolverState& state) {
    SolverContextKey key;
    key.trader = state.trader;
    key.camel = state.camel;
    key.food_consumption = state.food_consumption;
    key.bonus_food_consumption = state.bonus_food_consumption;
    key.backpack = state.backpack;
    key.daily_income = state.daily_income;
    key.day = state.day;
    key.max_day = state.max_day;
    key.interest_hundredths = state.interest_hundredths;
    key.disaster_day = state.disaster_day;
    key.scheduled_disaster = state.scheduled_disaster;
    key.merchant_day = state.merchant_day;
    key.witch_day = state.witch_day;
    key.player_world = state.player_world;
    key.flags = state.flags;
    key.item_shop = state.item_shop;
    key.npc_shop = state.npc_shop;
    return key;
}

size_t SolverContextKeyHash::operator()(const SolverContextKey& key) const noexcept {
    size_t seed = 0;
    hash_combine(seed, key.trader);
    hash_combine(seed, key.camel);
    hash_combine(seed, key.food_consumption);
    hash_combine(seed, key.bonus_food_consumption);
    hash_combine(seed, key.backpack);
    hash_combine(seed, key.daily_income);
    hash_combine(seed, key.day);
    hash_combine(seed, key.max_day);
    hash_combine(seed, key.interest_hundredths);
    hash_combine(seed, key.disaster_day);
    hash_combine(seed, key.scheduled_disaster);
    hash_combine(seed, key.merchant_day);
    hash_combine(seed, key.witch_day);
    hash_combine(seed, key.player_world.town);
    hash_combine(seed, key.player_world.merch);
    hash_combine(seed, key.player_world.witch);
    hash_combine(seed, key.player_world.disaster);
    hash_combine(seed, key.flags);
    for (int value : key.item_shop) {
        hash_combine(seed, value);
    }
    for (int value : key.npc_shop) {
        hash_combine(seed, value);
    }
    return seed;
}

void GameSolver::initialize(const Player& start_player, const uint64_t memory_limit_mb_override) {
    if (memory_limit_mb_override > 0) {
        memory_limit_bytes = memory_limit_mb_override * 1024ULL * 1024ULL;
    } else {
        const uint64_t available_memory = detect_available_memory_bytes();
        if (available_memory > 0) {
            memory_limit_bytes = std::min<uint64_t>(available_memory / 4ULL, 1024ULL * 1024ULL * 1024ULL);
        } else {
            memory_limit_bytes = 512ULL * 1024ULL * 1024ULL;
        }
    }
    traces.clear();
    traces.push_back(TraceNode{});
    start_state = SolverState::from_player(start_player, 0);
    current_layer = {start_state};
    started_at = std::chrono::steady_clock::now();
    last_status_at = started_at;
    last_resident_bytes = resident_memory_bytes();
    last_memory_estimated_bytes = 0;
    memory_checks_since_exact_scan = 0;
    memory_checks_since_resident_sample = 0;
}

GameSolver::GameSolver(
    const Player& start_player,
    const bool _merch_on,
    const bool _witch_on,
    const int _array_limit,
    const int _print_counter,
    std::string checkpoint_file,
    const uint64_t _memory_limit_mb,
    const PurchaseProfile _purchase_profile,
    const uint8_t _optimistic_share_percent,
    const bool _sell_action_aware_beam,
    const uint8_t _diversity_share_percent
) : merch_on(_merch_on),
    witch_on(_witch_on),
    purchase_profile(_purchase_profile),
    optimistic_share_percent(_optimistic_share_percent),
    diversity_share_percent(_diversity_share_percent),
    sell_action_aware_beam(_sell_action_aware_beam),
    array_limit(_array_limit),
    print_counter(_print_counter),
    checkpoint_path(std::move(checkpoint_file)) {
    prefix_actions = 0;
    initialize(start_player, _memory_limit_mb);
}

GameSolver::GameSolver(
    const town_options town,
    const bool _merch_on,
    const bool _witch_on,
    const int _array_limit,
    const int _print_counter,
    const std::vector<ga_data>& que,
    std::string checkpoint_file,
    const uint64_t _memory_limit_mb,
    const PurchaseProfile _purchase_profile,
    const uint8_t _optimistic_share_percent,
    const bool _sell_action_aware_beam,
    const uint8_t _diversity_share_percent,
    const int _merchant_day,
    const int _witch_day
) : merch_on(_merch_on),
    witch_on(_witch_on),
    purchase_profile(_purchase_profile),
    optimistic_share_percent(_optimistic_share_percent),
    diversity_share_percent(_diversity_share_percent),
    sell_action_aware_beam(_sell_action_aware_beam),
    array_limit(_array_limit),
    print_counter(_print_counter),
    checkpoint_path(std::move(checkpoint_file)) {
    if ((_merchant_day != 0 && (_merchant_day < 5 || _merchant_day > 7)) ||
        (_witch_day != 0 && (_witch_day < 9 || _witch_day > 11))) {
        throw std::invalid_argument(
            "Classical encounter days must be Merchant 5-7 and Witch 9-11"
        );
    }
    prefix_actions = que.size();
    Player start_player;
    Game::apply_town(start_player, town);
    start_player.merchant_day = _merchant_day;
    start_player.witch_day = _witch_day;
    if (!que.empty()) {
        Game::auto_game_loop(start_player, que);
    }
    initialize(start_player, _memory_limit_mb);
}

uint64_t GameSolver::detect_available_memory_bytes() {
    uint64_t available_bytes = 0;
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo) {
        std::string label;
        uint64_t value_kb = 0;
        std::string unit;
        while (meminfo >> label >> value_kb >> unit) {
            if (label == "MemAvailable:") {
                available_bytes = value_kb * 1024ULL;
                break;
            }
        }
    }

    std::ifstream cgroup_limit("/sys/fs/cgroup/memory.max");
    std::ifstream cgroup_current("/sys/fs/cgroup/memory.current");
    std::string limit_text;
    uint64_t current_bytes = 0;
    if (cgroup_limit >> limit_text && limit_text != "max" && cgroup_current >> current_bytes) {
        try {
            const uint64_t limit_bytes = std::stoull(limit_text);
            const uint64_t cgroup_available = limit_bytes > current_bytes ? limit_bytes - current_bytes : 0;
            if (cgroup_available > 0) {
                available_bytes = available_bytes == 0 ? cgroup_available : std::min(available_bytes, cgroup_available);
            }
        } catch (const std::exception&) {
            // Fall back to /proc/meminfo when the cgroup value is not numeric.
        }
    }
    return available_bytes;
}

uint64_t GameSolver::resident_memory_bytes() {
#ifdef __linux__
    std::ifstream statm("/proc/self/statm");
    uint64_t ignored_pages = 0;
    uint64_t resident_pages = 0;
    if (statm >> ignored_pages >> resident_pages) {
        return resident_pages * static_cast<uint64_t>(::sysconf(_SC_PAGESIZE));
    }
#endif
    return 0;
}

MemoryCheckCadence memory_check_cadence(
    const uint64_t observed_bytes,
    const uint64_t working_limit_bytes
) noexcept {
    if (working_limit_bytes == 0 || observed_bytes >= working_limit_bytes) {
        return MemoryCheckCadence{1, 1};
    }
    if (observed_bytes >= working_limit_bytes - working_limit_bytes / 10ULL) {
        return MemoryCheckCadence{1, 4096};
    }
    if (observed_bytes >= working_limit_bytes * 3ULL / 4ULL) {
        return MemoryCheckCadence{16, 4096};
    }
    if (observed_bytes >= working_limit_bytes / 2ULL) {
        return MemoryCheckCadence{32, 4096};
    }
    return MemoryCheckCadence{64, 4096};
}

int GameSolver::active_day(const SolverState& state) {
    return state.day + (state.has_flag(SSF_DAY_START_SKIP) ? 0 : 1);
}

std::string GameSolver::format_action(const SolverAction& action) {
    std::ostringstream out;
    switch (action.action) {
        case _a_buy:
            out << "Buy [" << static_cast<int>(action.buy.food)
                << ", " << static_cast<int>(action.buy.spice)
                << ", " << static_cast<int>(action.buy.pottery)
                << ", " << static_cast<int>(action.buy.marble)
                << ", " << static_cast<int>(action.buy.silk)
                << ", " << static_cast<int>(action.buy.jewelry) << "]";
            break;
        case _a_wait:
            out << "Wait (empty Buy)";
            break;
        case _a_sell:
            out << "Sell";
            break;
        case _a_npc:
            out << (action.option == static_cast<uint8_t>(n_Trader) ? "Trader" : "Camel");
            break;
        case _a_interest:
            out << "Interest";
            break;
        case _a_merchant:
            out << "Merch - " << mo_conversion.at(static_cast<merch_options>(action.option));
            break;
        case _a_witch:
            out << "Witch - " << wo_conversion.at(static_cast<witch_options>(action.option));
            break;
        case _a_disaster:
            out << "Disaster - " << do_conversion.at(static_cast<disaster_options>(action.option));
            break;
    }
    return out.str();
}

std::vector<SolverAction> GameSolver::rebuild_actions(const uint32_t trace_id) const {
    std::vector<SolverAction> actions;
    uint32_t current = trace_id;
    while (current != 0 && current < traces.size()) {
        actions.emplace_back(traces[current].action);
        current = traces[current].parent;
    }
    std::reverse(actions.begin(), actions.end());
    return actions;
}

uint64_t GameSolver::estimated_memory_bytes() const {
    return estimated_memory_bytes(nullptr, nullptr);
}

uint64_t GameSolver::estimated_memory_bytes(
    const std::vector<SolverState>* same_day_candidates,
    const std::vector<SolverState>* next_day_candidates,
    const size_t projected_concrete_states
) const {
    uint64_t bytes = static_cast<uint64_t>(traces.capacity()) * sizeof(TraceNode) +
        static_cast<uint64_t>(finished.capacity()) * sizeof(FinishedSolve) +
        handler.loadedTreeMemoryBytes() +
        static_cast<uint64_t>(continuation_score_cache.bucket_count()) * sizeof(void*) +
        static_cast<uint64_t>(continuation_score_cache.size()) *
            sizeof(decltype(continuation_score_cache)::value_type) +
        static_cast<uint64_t>(projected_concrete_states) * sizeof(SolverState);
    for (const FinishedSolve& solve : finished) {
        bytes += static_cast<uint64_t>(solve.actions.capacity()) * sizeof(SolverAction);
    }

    std::unordered_set<const std::vector<SolverState>*> counted_vectors;
    std::unordered_set<const DeferredSearchState*> counted_deferred;
    std::unordered_set<const void*> counted_shared;
    const auto add_states = [&](const std::vector<SolverState>* states) {
        if (states == nullptr || !counted_vectors.insert(states).second) {
            return;
        }
        bytes += static_cast<uint64_t>(states->capacity()) * sizeof(SolverState);
        for (const SolverState& state : *states) {
            if (state.deferred && counted_deferred.insert(state.deferred.get()).second) {
                bytes += state.deferred->memory_bytes(counted_shared);
            }
        }
    };

    add_states(&current_layer);
    add_states(pending_states_for_budget);
    add_states(waiting_states_for_budget);
    add_states(same_day_candidates);
    add_states(next_day_candidates);
    return bytes;
}

bool GameSolver::can_still_reach_merchant_bonus(const SolverState& state) const {
    if (!merch_on || state.has_flag(SSF_MERCHANT_HAPPENED)) {
        return false;
    }
    const int next_day_value = active_day(state);
    if (state.has_flag(SSF_CALL_MERCHANT)) {
        return next_day_value <= state.max_day;
    }
    if (state.merchant_day > 0) {
        return next_day_value <= state.merchant_day && state.merchant_day <= state.max_day;
    }
    return Helpers::day_in_array(merch_days, next_day_value) && next_day_value <= state.max_day;
}

std::array<int, 6> GameSolver::best_future_sell_prices(const SolverState& state) const {
    std::array<int, 6> prices{};
    for (int item = 0; item <= i_Jewelry; ++item) {
        prices[item] = state.item_shop[item * 3 + id_Sell];
    }
    if (can_still_reach_merchant_bonus(state)) {
        prices[i_Spice] = std::max(prices[i_Spice], state.item_shop[i_Spice * 3 + id_Sell] + 20);
    }
    return prices;
}

size_t GameSolver::estimated_buy_branch_states(const Player& current) const {
    const int current_food_cost = current.food_consumption;
    const int days_left = current.max_day - current.day;
    const int food_multi = days_left > 1 ? 2 : 1;
    const int max_weight = current.item_shop[i_Food][id_Weight] == 0 ? 100 : Game::check_weight_left(current) / current.item_shop[i_Food][id_Weight];
    const int max_afford = current.item_shop[i_Food][id_Buy] == 0 ? 100 : current.gold / current.item_shop[i_Food][id_Buy];
    const int max_eat = std::max(current.npc_shop[n_Trader][nd_Eat], current.npc_shop[n_Camel][nd_Eat]);
    const int future_days = std::max(days_left - 1, 0);
    const int max_want = (current_food_cost * days_left) + (max_eat * (future_days * (future_days + 1)) / 2);
    const int max_food = std::min({max_weight, max_afford, max_want});
    const int needed_food = std::max(0, (food_multi * current_food_cost) - current.food);
    if (max_food < needed_food) {
        return 0;
    }
    return static_cast<size_t>(max_food - needed_food + 1) * 2ULL;
}

long double GameSolver::beam_score(const SolverState& state) const {
    if (state.deferred) {
        SolverState representative = SolverState::from_player(
            state.deferred->representative(),
            state.trace_id
        );
        return beam_score(representative);
    }
    const int remaining_days = std::max(0, state.max_day - active_day(state) + 1);
    const auto sell_prices = best_future_sell_prices(state);
    long double wealth = static_cast<long double>(state.gold) +
        static_cast<long double>(state.spice) * sell_prices[i_Spice] +
        static_cast<long double>(state.pottery) * sell_prices[i_Pottery] +
        static_cast<long double>(state.marble) * sell_prices[i_Marble] +
        static_cast<long double>(state.silk) * sell_prices[i_Silk] +
        static_cast<long double>(state.jewelry) * sell_prices[i_Jewelry] +
        (state.has_flag(SSF_HAND_OF_MIDAS) ? 100.0L : 0.0L);

    const long double interest_rate = std::max<long double>(static_cast<long double>(state.interest_hundredths) / 100.0L, 1.0L);
    const long double passive_income = static_cast<long double>(state.daily_income + state.trader * state.npc_shop[n_Trader * 3 + nd_Effect]);
    int actions_left = remaining_days;
    const bool carrying_items =
        state.spice > 0 ||
        state.pottery > 0 ||
        state.marble > 0 ||
        state.silk > 0 ||
        state.jewelry > 0;
    if (sell_action_aware_beam && carrying_items && actions_left > 0) {
        wealth += passive_income;
        --actions_left;
    }
    while (actions_left > 0) {
        const long double one_interest_profit = wealth * (interest_rate - 1.0L);
        if (actions_left >= 2) {
            const long double trade_profit = optimistic_trade_profit_for_day(state, wealth, sell_prices);
            const long double interest_wealth_after_one = wealth + passive_income + one_interest_profit;
            const long double two_interest_profit = one_interest_profit + interest_wealth_after_one * (interest_rate - 1.0L);
            if (trade_profit > two_interest_profit) {
                wealth += (2.0L * passive_income) + trade_profit;
                actions_left -= 2;
                continue;
            }
        }
        wealth += passive_income + one_interest_profit;
        --actions_left;
    }

    if (state.has_flag(SSF_STATUE)) {
        const int maximum_future_hires = remaining_days;
        wealth += static_cast<long double>(
            state.trader + state.camel + maximum_future_hires
        ) * 500.0L;
    }
    return wealth;
}

void GameSolver::request_stop(const std::string& reason) {
    if (stop_requested) {
        return;
    }
    stop_requested = true;
    stop_reason = reason;
}

bool GameSolver::enforce_memory_budget(
    const std::vector<SolverState>& same_day_candidates,
    const std::vector<SolverState>& next_day_candidates,
    const size_t projected_concrete_states
) {
    ++profiling.memory_budget_checks;
    if (time_limit_seconds > 0 &&
        std::chrono::steady_clock::now() - started_at >= std::chrono::seconds(time_limit_seconds)) {
        request_stop("internal time budget reached after " + std::to_string(time_limit_seconds) + " seconds");
        return false;
    }
    if (memory_limit_bytes == 0) {
        return true;
    }
    const uint64_t working_limit = memory_limit_bytes - (memory_limit_bytes / 10ULL);
    ++memory_checks_since_exact_scan;
    ++memory_checks_since_resident_sample;
    MemoryCheckCadence cadence = memory_check_cadence(
        std::max(last_memory_estimated_bytes, last_resident_bytes),
        working_limit
    );
    bool resident_sampled = false;
    if (last_resident_bytes == 0 ||
        memory_checks_since_resident_sample >= cadence.resident_sample_interval) {
        last_resident_bytes = resident_memory_bytes();
        memory_checks_since_resident_sample = 0;
        resident_sampled = true;
        ++profiling.memory_budget_resident_samples;
        profiling.peak_resident_bytes = std::max(
            profiling.peak_resident_bytes,
            last_resident_bytes
        );
    }
    const uint64_t cached_observed = std::max(
        last_memory_estimated_bytes,
        last_resident_bytes
    );
    cadence = memory_check_cadence(cached_observed, working_limit);
    const bool exact_scan_required =
        last_memory_estimated_bytes == 0 ||
        memory_checks_since_exact_scan >= cadence.exact_scan_interval ||
        projected_concrete_states > 0;
    if (!exact_scan_required) {
        return true;
    }

    const auto scan_started = std::chrono::steady_clock::now();
    const uint64_t estimated = estimated_memory_bytes(
        &same_day_candidates,
        &next_day_candidates,
        projected_concrete_states
    );
    last_memory_estimated_bytes = estimated;
    memory_checks_since_exact_scan = 0;
    ++profiling.memory_budget_exact_scans;
    profiling.peak_estimated_bytes = std::max(profiling.peak_estimated_bytes, estimated);
    if (!resident_sampled) {
        last_resident_bytes = resident_memory_bytes();
        memory_checks_since_resident_sample = 0;
        ++profiling.memory_budget_resident_samples;
    }
    const uint64_t resident = last_resident_bytes;
    profiling.peak_resident_bytes = std::max(profiling.peak_resident_bytes, resident);
    const uint64_t observed = std::max(estimated, resident);
    profiling.memory_budget_scan_nanoseconds += static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - scan_started
        ).count()
    );
    if (observed <= working_limit) {
        return true;
    }

    std::ostringstream out;
    out << "memory budget reached"
        << " | est_mem_mb=" << (estimated / (1024ULL * 1024ULL))
        << " | rss_mb=" << (resident / (1024ULL * 1024ULL))
        << " | mem_limit_mb=" << (memory_limit_bytes / (1024ULL * 1024ULL))
        << " | pending=" << (pending_states_for_budget == nullptr ? 0 : pending_states_for_budget->size())
        << " | traces=" << traces.size()
        << " | same_day=" << same_day_candidates.size()
        << " | next_day=" << next_day_candidates.size();
    request_stop(out.str());
    return false;
}

long double GameSolver::optimistic_final_bound(const SolverState& state) const {
    if (state.deferred) {
        SolverState representative = SolverState::from_player(
            state.deferred->representative(),
            state.trace_id
        );
        return optimistic_final_bound(representative);
    }
    const int next_day_value = active_day(state);
    const int remaining_days = std::max(0, state.max_day - next_day_value + 1);
    const auto sell_prices = best_future_sell_prices(state);

    long double liquidated = static_cast<long double>(state.gold) +
        static_cast<long double>(state.spice) * sell_prices[i_Spice] +
        static_cast<long double>(state.pottery) * sell_prices[i_Pottery] +
        static_cast<long double>(state.marble) * sell_prices[i_Marble] +
        static_cast<long double>(state.silk) * sell_prices[i_Silk] +
        static_cast<long double>(state.jewelry) * sell_prices[i_Jewelry] +
        (state.has_flag(SSF_HAND_OF_MIDAS) ? 100.0L : 0.0L);

    const long double current_interest_rate = static_cast<long double>(state.interest_hundredths) / 100.0L;
    const long double max_interest_rate =
        std::max<long double>(current_interest_rate, current_interest_rate + (can_still_reach_merchant_bonus(state) ? 0.2L : 0.0L));
    const long double passive_income_per_day =
        static_cast<long double>(state.daily_income + state.trader * state.npc_shop[n_Trader * 3 + nd_Effect]);

    long double wealth = liquidated;
    for (int day_index = 0; day_index < remaining_days; ++day_index) {
        const long double interest_profit = std::max<long double>(0.0L, wealth * (max_interest_rate - 1.0L));
        const long double trade_profit = optimistic_trade_profit_for_day(state, wealth, sell_prices);
        wealth += passive_income_per_day + std::max(interest_profit, trade_profit);
        if (wealth > static_cast<long double>(std::numeric_limits<int>::max())) {
            wealth = static_cast<long double>(std::numeric_limits<int>::max());
            break;
        }
    }

    long double bound = wealth;
    if (state.has_flag(SSF_STATUE)) {
        const int maximum_future_hires = remaining_days;
        bound += static_cast<long double>(
            state.trader + state.camel + maximum_future_hires
        ) * 500.0L;
    }
    return bound;
}

long double GameSolver::optimistic_trade_profit_for_day(
    const SolverState& state,
    const long double available_gold,
    const std::array<int, 6>& sell_prices
) const {
    const long double total_capacity =
        static_cast<long double>(state.camel * state.npc_shop[n_Camel * 3 + nd_Effect] + state.backpack);
    if (available_gold <= 0.0L || total_capacity <= 0.0L) {
        return 0.0L;
    }

    std::array<FractionalItemBound, 5> options{};
    size_t option_count = 0;
    for (int item = i_Spice; item <= i_Jewelry; ++item) {
        const int buy_cost = state.item_shop[item * 3 + id_Buy];
        const int buy_weight = state.item_shop[item * 3 + id_Weight];
        const int profit = sell_prices[item] - buy_cost;
        if (buy_cost <= 0 || profit <= 0) {
            continue;
        }
        if (buy_weight <= 0) {
            return std::max<long double>(available_gold * (static_cast<long double>(profit) / static_cast<long double>(buy_cost)), 0.0L);
        }
        options[option_count++] = FractionalItemBound{
            static_cast<long double>(profit),
            static_cast<long double>(buy_cost),
            static_cast<long double>(buy_weight),
            static_cast<long double>(profit) / static_cast<long double>(buy_weight)
        };
    }

    std::sort(options.begin(), options.begin() + static_cast<long long>(option_count), [](const FractionalItemBound& lhs, const FractionalItemBound& rhs) {
        if (lhs.density != rhs.density) {
            return lhs.density > rhs.density;
        }
        return lhs.cost < rhs.cost;
    });

    long double remaining_gold = available_gold;
    long double remaining_weight = total_capacity;
    long double total_profit = 0.0L;
    for (size_t index = 0; index < option_count; ++index) {
        const auto& item = options[index];
        const long double max_by_gold = remaining_gold / item.cost;
        const long double max_by_weight = remaining_weight / item.weight;
        const long double units = std::min(max_by_gold, max_by_weight);
        if (units <= 0.0L) {
            continue;
        }
        total_profit += units * item.profit;
        remaining_gold -= units * item.cost;
        remaining_weight -= units * item.weight;
    }
    return std::max(total_profit, 0.0L);
}

void GameSolver::print_status(const int active_day_value, const size_t pending_same_day, const size_t next_day_candidates) const {
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started_at).count();
    const double rate = elapsed > 0.0 ? static_cast<double>(profiling.processed_states) / elapsed : 0.0;
    const auto purchase_profile = handler.getProfilingSummary();
    std::cout << "Layer " << active_day_value
              << " | processed=" << profiling.processed_states
              << " | generated=" << profiling.generated_states
              << " | buy=" << profiling.generated_buy_states
              << " | wait=" << profiling.generated_wait_states
              << " | sell=" << profiling.generated_sell_states
              << " | trader=" << profiling.generated_trader_states
              << " | camel=" << profiling.generated_camel_states
              << " | interest=" << profiling.generated_interest_states
              << " | merchant=" << profiling.generated_merchant_states
              << " | witch=" << profiling.generated_witch_states
              << " | disaster=" << profiling.generated_disaster_states
              << " | merchant_deferred=" << profiling.merchant_deferral_continuations
              << " | witch_deferred=" << profiling.witch_deferral_continuations
              << " | duplicates=" << profiling.duplicate_prunes
              << " | dominated=" << profiling.dominance_prunes
              << " | beam_pruned=" << profiling.beam_prunes
              << " | same_day=" << pending_same_day
              << " | next_day=" << next_day_candidates
              << " | frontier=" << current_layer.size()
              << " | traces=" << traces.size()
              << " | est_mem_mb=" << (estimated_memory_bytes() / (1024ULL * 1024ULL))
              << " | rss_mb=" << (resident_memory_bytes() / (1024ULL * 1024ULL))
              << " | tree_misses=" << purchase_profile.cache_misses
              << " | states/sec=" << std::fixed << std::setprecision(1) << rate
              << std::endl;
}

void GameSolver::print_solves() const {
    for (size_t index = 0; index < finished.size(); ++index) {
        std::cout << "Place " << (index + 1) << ": " << finished[index].points
                  << (finished[index].origin == FinishedSolve::Origin::Incumbent ? " (incumbent)" : " (discovered)")
                  << "\n";
        for (size_t action_index = 0; action_index < finished[index].actions.size(); ++action_index) {
            if (action_index != 0) {
                std::cout << ", ";
            }
            std::cout << format_action(finished[index].actions[action_index]);
        }
        std::cout << "\n\n";
    }
}

void GameSolver::set_time_limit_seconds(const uint64_t seconds) noexcept {
    time_limit_seconds = seconds;
}

void GameSolver::set_forced_merchant_option(const std::optional<merch_options> option) noexcept {
    forced_merchant_option = option;
}

bool GameSolver::merchant_option_allowed(const merch_options option) const noexcept {
    return !forced_merchant_option.has_value() || *forced_merchant_option == option;
}

void GameSolver::set_forced_witch_option(const std::optional<witch_options> option) noexcept {
    forced_witch_option = option;
}

void GameSolver::set_hybrid_batch_size(const size_t states) noexcept {
    hybrid_batch_size = states;
}

void GameSolver::set_hybrid_suffix_limit(const size_t states) noexcept {
    hybrid_suffix_limit = states;
}

void GameSolver::set_hybrid_split_day(const int day) noexcept {
    hybrid_split_day = day;
}

void GameSolver::set_hybrid_skeleton_order(const HybridSkeletonOrder order) noexcept {
    hybrid_skeleton_order = order;
}

void GameSolver::set_skeleton_share_percent(const uint8_t percent) noexcept {
    skeleton_share_percent = percent;
}

void GameSolver::set_skeleton_depth(const uint8_t actions) noexcept {
    skeleton_depth = std::clamp<uint8_t>(actions, 1, 8);
}

void GameSolver::set_continuation_lane_share_percent(const uint8_t percent) noexcept {
    continuation_lane_share_percent = std::min<uint8_t>(percent, 100);
}

void GameSolver::set_skeleton_queue_day(const int day) noexcept {
    skeleton_queue_day = std::clamp(day, -1, 255);
}

void GameSolver::set_refinement_template(
    const std::vector<SolverAction>& actions,
    const uint8_t share_percent,
    const uint8_t maximum_mismatches
) {
    EarlyActionSkeleton target;
    for (const SolverAction& action : actions) {
        target = target.extended(action);
    }
    if (target.action_count() == 0) {
        throw std::invalid_argument("Refinement template requires at least one player action");
    }
    refinement_template = target;
    refinement_share_percent = std::clamp<uint8_t>(share_percent, 1, 100);
    refinement_maximum_mismatches = std::min<uint8_t>(maximum_mismatches, 8);
}

void GameSolver::set_route_survival_audit(std::vector<SolverAction> route) {
    survival_audit.emplace(std::move(route));
    if (!survival_audit->enabled()) {
        survival_audit.reset();
        return;
    }
    for (SolverState& state : current_layer) {
        state.route_audit_progress = 0;
        state.route_audit_quantity = RouteAuditQuantityStatus::NotApplicable;
        survival_audit->note_generated(0, state.route_audit_quantity);
        survival_audit->note_reduction(
            RouteAuditReductionStage::AfterBeam,
            0,
            state.route_audit_quantity
        );
    }
}

RouteAuditQuantityStatus GameSolver::classify_route_audit_quantity(
    const int16_t progress,
    const DeferredSearchState& deferred
) {
    if (!survival_audit || progress == RouteSurvivalAudit::NO_MATCH) {
        return RouteAuditQuantityStatus::NotApplicable;
    }
    const buy_list* expected = deferred.has_open_purchase()
        ? survival_audit->expected_open_buy(progress)
        : survival_audit->expected_sold_buy(progress);
    if (expected == nullptr) {
        return RouteAuditQuantityStatus::NotApplicable;
    }
    const DeferredBuyCoverage coverage = deferred.has_open_purchase()
        ? deferred.purchase().classify_exact_buy(*expected)
        : deferred.sold().classify_exact_buy(*expected);
    switch (coverage) {
        case DeferredBuyCoverage::Exact:
            return RouteAuditQuantityStatus::Exact;
        case DeferredBuyCoverage::DominatingReplacement:
            return RouteAuditQuantityStatus::DominatingReplacement;
        case DeferredBuyCoverage::Missing:
            return RouteAuditQuantityStatus::Missing;
    }
    return RouteAuditQuantityStatus::Missing;
}

const RouteSurvivalAudit* GameSolver::route_survival_audit() const noexcept {
    return survival_audit ? &*survival_audit : nullptr;
}

bool GameSolver::witch_option_allowed(const witch_options option) const noexcept {
    return !forced_witch_option.has_value() || *forced_witch_option == option;
}

bool GameSolver::deferred_player_action_reachable(const SolverState& state) const noexcept {
    if (!state.deferred || state.has_flag(SSF_DAY_START_SKIP)) {
        return true;
    }

    Player upcoming = state.deferred->representative();
    ++upcoming.day;

    const EncounterOpportunity merchant_timing = EncounterTiming::merchant(upcoming);
    if (merch_on && !upcoming.merchant_happened && merchant_timing.due &&
        !merchant_timing.may_defer) {
        const bool has_configured_option = !forced_merchant_option.has_value() ||
            *forced_merchant_option != m_No_Merchant || upcoming.merchant_day > 0;
        if (has_configured_option) {
            // Affordability is origin-dependent. Suppressing a ticket when all
            // origins are poor is conservative; scoring an action before a
            // mandatory encounter would be an impossible continuation.
            return false;
        }
    }

    const EncounterOpportunity witch_timing = EncounterTiming::witch(upcoming);
    if (witch_on && !upcoming.witch_happened && witch_timing.due &&
        !witch_timing.may_defer &&
        (!forced_witch_option.has_value() || *forced_witch_option != w_No_Witch)) {
        return false;
    }
    return true;
}

void GameSolver::print_benchmark_json(
    std::ostream& out,
    const double campaign_elapsed_seconds
) const {
    const double elapsed_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started_at).count();
    const double states_per_second = elapsed_seconds > 0.0
        ? static_cast<double>(profiling.processed_states) / elapsed_seconds
        : 0.0;
    const auto purchase_summary = handler.getProfilingSummary();
    const auto town = static_cast<town_options>(start_state.player_world.town);
    const int discovered_points = best_discovered_points();
    const int seeded_points = incumbent_points();
    const bool best_is_discovered = !finished.empty() && finished.front().origin == FinishedSolve::Origin::Discovered;
    out << '{'
        << "\"town\":\"" << json_escape(to_conversion.at(town)) << '"'
        << ",\"best_score\":";
    if (finished.empty()) {
        out << "null";
    } else {
        out << best_points();
    }
    out << ",\"best_source\":";
    if (finished.empty()) {
        out << "null";
    } else {
        out << '"' << (best_is_discovered ? "discovered" : "incumbent") << '"';
    }
    const bool mapped_search = profiling.mapped_nodes > 0;
    out << ",\"search_complete\":"
        << ((mapped_search ? profiling.proof_search_completed : !stop_requested)
            ? "true" : "false")
        << ",\"stop_reason\":";
    if (stop_requested) {
        out << '"' << json_escape(stop_reason) << '"';
    } else {
        out << "null";
    }
    out << ",\"prefix_actions\":" << prefix_actions
        << ",\"merchant_day\":" << static_cast<int>(start_state.merchant_day)
        << ",\"witch_day\":" << static_cast<int>(start_state.witch_day)
        << ",\"discovered_score\":";
    if (discovered_points == std::numeric_limits<int>::min()) {
        out << "null";
    } else {
        out << discovered_points;
    }
    out << ",\"incumbent_score\":";
    if (seeded_points == std::numeric_limits<int>::min()) {
        out << "null";
    } else {
        out << seeded_points;
    }
    out
        << ",\"purchase_profile\":\"" << purchase_profile_name(purchase_profile) << '"'
        << ",\"merchant_option\":";
    if (forced_merchant_option.has_value()) {
        out << '"' << json_escape(mo_conversion.at(*forced_merchant_option)) << '"';
    } else {
        out << "\"any\"";
    }
    out << ",\"witch_option\":";
    if (forced_witch_option.has_value()) {
        out << '"' << json_escape(wo_conversion.at(*forced_witch_option)) << '"';
    } else {
        out << "\"any\"";
    }
    out
        << ",\"cargo_buy_rule\":\"one-cargo-buy-until-sell\""
        << ",\"sell_action_aware_beam\":" << (sell_action_aware_beam ? "true" : "false")
        << ",\"optimistic_share_percent\":" << static_cast<int>(optimistic_share_percent)
        << ",\"diversity_share_percent\":" << static_cast<int>(diversity_share_percent)
        << ",\"skeleton_share_percent\":" << static_cast<int>(skeleton_share_percent)
        << ",\"skeleton_depth\":" << static_cast<int>(skeleton_depth)
        << ",\"continuation_lane_share_percent\":"
        << static_cast<int>(continuation_lane_share_percent)
        << ",\"skeleton_queue_day_config\":" << skeleton_queue_day
        << ",\"refinement_share_percent\":" << static_cast<int>(refinement_share_percent)
        << ",\"refinement_maximum_mismatches\":" << static_cast<int>(refinement_maximum_mismatches)
        << ",\"search_pass\":\"" << (refinement_template.has_value() ? "refinement" : "baseline") << '"'
        << ",\"search_mode\":\"" << (mapped_search ? "persistent-exact" : "transient") << '"'
        << ",\"mapped_order\":";
    if (mapped_search) {
        out << '"' << (mapped_search_order == PersistentSearchOrder::Beam ? "beam" : "depth") << '"';
    } else {
        out << "null";
    }
    out << ",\"mapped_beam_width\":";
    if (mapped_search && mapped_search_order == PersistentSearchOrder::Beam) {
        out << array_limit;
    } else {
        out << "null";
    }
    out
        << ",\"hybrid_batch_size\":" << hybrid_batch_size
        << ",\"hybrid_suffix_limit_config\":" << hybrid_suffix_limit
        << ",\"hybrid_suffix_limit\":"
        << (hybrid_batch_size == 0
            ? 0
            : std::min<size_t>(
                static_cast<size_t>(array_limit),
                hybrid_suffix_limit == 0 ? hybrid_batch_size : hybrid_suffix_limit
            ))
        << ",\"hybrid_split_day_config\":" << hybrid_split_day
        << ",\"hybrid_skeleton_order\":\"" << hybrid_skeleton_order_name(hybrid_skeleton_order) << '"'
        << ",\"elapsed_seconds\":" << std::fixed << std::setprecision(3) << elapsed_seconds
        << ",\"campaign_elapsed_seconds\":" << std::fixed << std::setprecision(3)
        << (campaign_elapsed_seconds < 0.0 ? elapsed_seconds : campaign_elapsed_seconds)
        << ",\"states_per_second\":" << std::fixed << std::setprecision(1) << states_per_second
        << ",\"peak_rss_mb\":" << (profiling.peak_resident_bytes / (1024ULL * 1024ULL))
        << ",\"processed_states\":" << profiling.processed_states
        << ",\"generated_states\":" << profiling.generated_states
        << ",\"generated_buy_states\":" << profiling.generated_buy_states
        << ",\"generated_wait_states\":" << profiling.generated_wait_states
        << ",\"generated_sell_states\":" << profiling.generated_sell_states
        << ",\"generated_trader_states\":" << profiling.generated_trader_states
        << ",\"generated_camel_states\":" << profiling.generated_camel_states
        << ",\"generated_interest_states\":" << profiling.generated_interest_states
        << ",\"generated_merchant_states\":" << profiling.generated_merchant_states
        << ",\"generated_witch_states\":" << profiling.generated_witch_states
        << ",\"merchant_deferral_continuations\":" << profiling.merchant_deferral_continuations
        << ",\"witch_deferral_continuations\":" << profiling.witch_deferral_continuations
        << ",\"finished_states\":" << profiling.finished_states
        << ",\"duplicate_prunes\":" << profiling.duplicate_prunes
        << ",\"dominance_prunes\":" << profiling.dominance_prunes
        << ",\"pre_beam_duplicate_prunes\":" << profiling.pre_beam_duplicate_prunes
        << ",\"pre_beam_dominance_prunes\":" << profiling.pre_beam_dominance_prunes
        << ",\"deferred_dominance_checks\":" << profiling.deferred_dominance_checks
        << ",\"deferred_partition_rejections\":" << profiling.deferred_partition_rejections
        << ",\"deferred_purchase_dominance_prunes\":"
        << profiling.deferred_purchase_dominance_prunes
        << ",\"deferred_sold_dominance_prunes\":"
        << profiling.deferred_sold_dominance_prunes
        << ",\"deferred_gold_function_comparisons\":"
        << profiling.deferred_gold_function_comparisons
        << ",\"deferred_exact_spend_evaluations\":"
        << profiling.deferred_exact_spend_evaluations
        << ",\"deferred_origin_coverage_checks\":"
        << profiling.deferred_origin_coverage_checks
        << ",\"deferred_cross_shape_dominance_prunes\":"
        << profiling.deferred_cross_shape_dominance_prunes
        << ",\"beam_prunes\":" << profiling.beam_prunes
        << ",\"peak_candidate_states\":" << profiling.peak_candidate_states
        << ",\"peak_estimated_mb\":"
        << (profiling.peak_estimated_bytes / (1024ULL * 1024ULL))
        << ",\"memory_budget_checks\":" << profiling.memory_budget_checks
        << ",\"memory_budget_exact_scans\":" << profiling.memory_budget_exact_scans
        << ",\"memory_budget_resident_samples\":"
        << profiling.memory_budget_resident_samples
        << ",\"memory_budget_scan_ms\":" << std::fixed << std::setprecision(3)
        << (static_cast<double>(profiling.memory_budget_scan_nanoseconds) / 1000000.0)
        << ",\"hybrid_batches\":" << profiling.hybrid_batches
        << ",\"hybrid_frontier_states\":" << profiling.hybrid_frontier_states
        << ",\"hybrid_peak_waiting_states\":" << profiling.hybrid_peak_waiting_states
        << ",\"hybrid_split_day\":" << profiling.hybrid_split_day
        << ",\"hybrid_trigger_estimated_mb\":"
        << (profiling.hybrid_trigger_estimated_bytes / (1024ULL * 1024ULL))
        << ",\"hybrid_split_skeleton_groups\":" << profiling.hybrid_split_skeleton_groups
        << ",\"hybrid_effective_batch_size\":" << profiling.hybrid_effective_batch_size
        << ",\"skeleton_queue_groups\":" << profiling.skeleton_queue_groups
        << ",\"skeleton_queue_states\":" << profiling.skeleton_queue_states
        << ",\"skeleton_queue_completed_groups\":"
        << profiling.skeleton_queue_completed_groups
        << ",\"skeleton_queue_split_day\":" << profiling.skeleton_queue_split_day
        << ",\"skeleton_queue_matrix_rows\":" << profiling.skeleton_queue_matrix_rows
        << ",\"skeleton_queue_duplicate_ticket_skips\":"
        << profiling.skeleton_queue_duplicate_ticket_skips
        << ",\"skeleton_queue_head_realistic\":"
        << profiling.skeleton_queue_head_ticket_counts[
            static_cast<size_t>(SkeletonQueueTicket::Realistic)]
        << ",\"skeleton_queue_head_optimistic\":"
        << profiling.skeleton_queue_head_ticket_counts[
            static_cast<size_t>(SkeletonQueueTicket::Optimistic)]
        << ",\"skeleton_queue_head_sell\":"
        << profiling.skeleton_queue_head_ticket_counts[
            static_cast<size_t>(SkeletonQueueTicket::ImmediateSell)]
        << ",\"skeleton_queue_head_trader\":"
        << profiling.skeleton_queue_head_ticket_counts[
            static_cast<size_t>(SkeletonQueueTicket::Trader)]
        << ",\"skeleton_queue_head_camel\":"
        << profiling.skeleton_queue_head_ticket_counts[
            static_cast<size_t>(SkeletonQueueTicket::Camel)]
        << ",\"skeleton_queue_head_interest\":"
        << profiling.skeleton_queue_head_ticket_counts[
            static_cast<size_t>(SkeletonQueueTicket::Interest)]
        << ",\"skeleton_queue_head_food_buy\":"
        << profiling.skeleton_queue_head_ticket_counts[
            static_cast<size_t>(SkeletonQueueTicket::FoodOnlyBuy)]
        << ",\"skeleton_queue_ceiling_checked_groups\":"
        << profiling.skeleton_queue_ceiling_checked_groups
        << ",\"skeleton_queue_ceiling_violations\":"
        << profiling.skeleton_queue_ceiling_violations
        << ",\"skeleton_queue_ceiling_max_overrun\":"
        << profiling.skeleton_queue_ceiling_max_overrun
        << ",\"skeleton_queue_ceiling_below_current_best\":"
        << profiling.skeleton_queue_ceiling_below_current_best
        << ",\"skeleton_queue_ceiling_pruning\":false"
        << ",\"skeleton_group_candidates\":" << profiling.skeleton_group_candidates
        << ",\"skeleton_reserved_states\":" << profiling.skeleton_reserved_states
        << ",\"skeleton_max_groups\":" << profiling.skeleton_max_groups
        << ",\"refinement_group_candidates\":" << profiling.refinement_group_candidates
        << ",\"refinement_reserved_states\":" << profiling.refinement_reserved_states
        << ",\"refinement_max_groups\":" << profiling.refinement_max_groups
        << ",\"proof_states_popped\":" << profiling.proof_states_popped
        << ",\"proof_transposition_hits\":" << profiling.proof_transposition_hits
        << ",\"proof_transposition_evictions\":" << profiling.proof_transposition_evictions
        << ",\"proof_bound_prunes\":" << profiling.proof_bound_prunes
        << ",\"proof_peak_stack_states\":" << profiling.proof_peak_stack_states
        << ",\"proof_peak_transposition_states\":" << profiling.proof_peak_transposition_states
        << ",\"mapped_expanded_this_run\":" << profiling.mapped_expanded_this_run
        << ",\"mapped_reused_children_this_run\":" << profiling.mapped_reused_children_this_run
        << ",\"mapped_nodes\":" << profiling.mapped_nodes
        << ",\"mapped_open_nodes\":" << profiling.mapped_open_nodes
        << ",\"mapped_partial_nodes\":" << profiling.mapped_partial_nodes
        << ",\"mapped_solved_nodes\":" << profiling.mapped_solved_nodes
        << ",\"mapped_database_bytes\":" << profiling.mapped_database_bytes
        << ",\"mapped_beam_sweeps\":" << profiling.mapped_beam_sweeps
        << ",\"mapped_beam_candidate_visits\":"
        << profiling.mapped_beam_candidate_visits
        << ",\"mapped_beam_parked_visits\":"
        << profiling.mapped_beam_parked_visits
        << ",\"proof_search_completed\":" << (profiling.proof_search_completed ? "true" : "false")
        << ",\"proof_top_three_certified\":" << (profiling.proof_top_three_certified ? "true" : "false")
        << ",\"purchase_valuation_worlds\":" << profiling.purchase_valuation_worlds
        << ",\"purchase_food_range_total\":" << profiling.purchase_food_range_total
        << ",\"purchase_food_only_checks\":" << profiling.purchase_food_only_checks
        << ",\"max_purchase_food_range\":" << profiling.max_purchase_food_range
        << ",\"purchase_queries\":" << profiling.purchase_queries
        << ",\"purchase_total_ms\":" << std::fixed << std::setprecision(3) << purchase_summary.total_ms
        << ",\"purchase_query_ms\":" << std::fixed << std::setprecision(3) << purchase_summary.query_ms
        << ",\"purchase_ensure_ms\":" << std::fixed << std::setprecision(3) << purchase_summary.ensure_ms
        << ",\"exact_sale_calls\":" << purchase_summary.exact_sale_calls
        << ",\"exact_sale_cache_hits\":" << purchase_summary.exact_sale_cache_hits
        << ",\"exact_sale_hot_cache_hits\":" << purchase_summary.exact_sale_hot_cache_hits
        << ",\"exact_sale_cache_entries\":" << purchase_summary.exact_sale_cache_entries
        << ",\"exact_sale_ms\":" << std::fixed << std::setprecision(3) << purchase_summary.exact_sale_ms
        << ",\"purchase_portfolio_choices\":" << profiling.purchase_portfolio_choices
        << ",\"purchase_unique_buys\":" << profiling.purchase_unique_buys
        << ",\"purchase_duplicate_buys\":" << profiling.purchase_duplicate_buys
        << ",\"purchase_dominance_prunes\":" << profiling.purchase_dominance_prunes
        << ",\"deferred_purchase_states\":" << profiling.deferred_purchase_states
        << ",\"deferred_sold_states\":" << profiling.deferred_sold_states
        << ",\"deferred_buy_seed_inputs\":" << profiling.deferred_buy_seed_inputs
        << ",\"deferred_buy_origins_kept\":" << profiling.deferred_buy_origins_kept
        << ",\"deferred_rows_total\":" << profiling.deferred_rows_total
        << ",\"deferred_max_rows\":" << profiling.deferred_max_rows
        << ",\"deferred_expansions\":" << profiling.deferred_expansions
        << ",\"deferred_purchase_expansions\":" << profiling.deferred_purchase_expansions
        << ",\"deferred_sold_expansions\":" << profiling.deferred_sold_expansions
        << ",\"continuation_view_owners\":" << profiling.continuation_view_owners
        << ",\"continuation_views_evaluated\":" << profiling.continuation_views_evaluated
        << ",\"continuation_view_score_wins\":" << profiling.continuation_view_score_wins
        << ",\"continuation_view_bound_wins\":" << profiling.continuation_view_bound_wins
        << ",\"continuation_matrix_lane_owners\":" << profiling.continuation_matrix_lane_owners
        << ",\"continuation_matrix_duplicate_ticket_skips\":"
        << profiling.continuation_matrix_duplicate_ticket_skips
        << ",\"continuation_matrix_sell_owners\":"
        << profiling.continuation_matrix_lane_counts[static_cast<size_t>(ContinuationLane::ImmediateSell)]
        << ",\"continuation_matrix_trader_owners\":"
        << profiling.continuation_matrix_lane_counts[static_cast<size_t>(ContinuationLane::Trader)]
        << ",\"continuation_matrix_camel_owners\":"
        << profiling.continuation_matrix_lane_counts[static_cast<size_t>(ContinuationLane::Camel)]
        << ",\"continuation_matrix_interest_owners\":"
        << profiling.continuation_matrix_lane_counts[static_cast<size_t>(ContinuationLane::Interest)]
        << ",\"continuation_matrix_food_buy_owners\":"
        << profiling.continuation_matrix_lane_counts[static_cast<size_t>(ContinuationLane::FoodOnlyBuy)]
        << ",\"continuation_matrix_scoring_ms\":" << std::fixed << std::setprecision(3)
        << (static_cast<double>(profiling.continuation_matrix_scoring_nanoseconds) / 1000000.0)
        << ",\"continuation_matrix_selection_ms\":" << std::fixed << std::setprecision(3)
        << (static_cast<double>(profiling.continuation_matrix_selection_nanoseconds) / 1000000.0)
        << ",\"beam_trim_ms\":" << std::fixed << std::setprecision(3)
        << (static_cast<double>(profiling.beam_trim_nanoseconds) / 1000000.0)
        << ",\"deferred_purchase_expansion_ms\":" << std::fixed << std::setprecision(3)
        << (static_cast<double>(profiling.deferred_purchase_expansion_nanoseconds) / 1000000.0)
        << ",\"deferred_sold_expansion_ms\":" << std::fixed << std::setprecision(3)
        << (static_cast<double>(profiling.deferred_sold_expansion_nanoseconds) / 1000000.0)
        << ",\"concrete_expansion_ms\":" << std::fixed << std::setprecision(3)
        << (static_cast<double>(profiling.concrete_expansion_nanoseconds) / 1000000.0)
        << ",\"frontier_reduction_ms\":" << std::fixed << std::setprecision(3)
        << (static_cast<double>(profiling.frontier_reduction_nanoseconds) / 1000000.0)
        << ",\"layer_compression_ms\":" << std::fixed << std::setprecision(3)
        << (static_cast<double>(profiling.layer_compression_nanoseconds) / 1000000.0)
        << ",\"skeleton_queue_schedule_ms\":" << std::fixed << std::setprecision(3)
        << (static_cast<double>(profiling.skeleton_queue_schedule_nanoseconds) / 1000000.0)
        << ",\"tree_misses\":" << purchase_summary.cache_misses
        << ",\"route_survival_audit\":";
    if (survival_audit) {
        survival_audit->write_json(out);
    } else {
        out << "null";
    }
    out << ",\"route\":[";
    if (!finished.empty()) {
        for (size_t index = 0; index < finished.front().actions.size(); ++index) {
            if (index != 0) {
                out << ',';
            }
            out << '"' << json_escape(format_action(finished.front().actions[index])) << '"';
        }
    }
    out << "]}\n";
}

void GameSolver::seed_incumbent(const int points, std::vector<SolverAction> actions) {
    const auto existing = std::find_if(finished.begin(), finished.end(), [](const FinishedSolve& solve) {
        return solve.origin == FinishedSolve::Origin::Incumbent;
    });
    if (existing != finished.end() && existing->points >= points) {
        return;
    }
    if (existing != finished.end()) {
        finished.erase(existing);
    }
    finished.push_back(FinishedSolve{
        points, std::move(actions), 0, FinishedSolve::Origin::Incumbent
    });
    sort_and_limit_finished();
}

void GameSolver::sort_and_limit_finished() {
    std::sort(finished.begin(), finished.end(), [](const FinishedSolve& lhs, const FinishedSolve& rhs) {
        if (lhs.points != rhs.points) {
            return lhs.points > rhs.points;
        }
        return lhs.origin == FinishedSolve::Origin::Discovered && rhs.origin == FinishedSolve::Origin::Incumbent;
    });
    if (finished.size() > MAX_FINISHED_TO_KEEP) {
        const auto incumbent = std::find_if(finished.begin(), finished.end(), [](const FinishedSolve& solve) {
            return solve.origin == FinishedSolve::Origin::Incumbent;
        });
        if (incumbent != finished.end() && static_cast<size_t>(std::distance(finished.begin(), incumbent)) >= MAX_FINISHED_TO_KEEP) {
            FinishedSolve retained_incumbent = std::move(*incumbent);
            finished.resize(MAX_FINISHED_TO_KEEP - 1);
            finished.emplace_back(std::move(retained_incumbent));
        } else {
            finished.resize(MAX_FINISHED_TO_KEEP);
        }
    }
}

void GameSolver::record_finished(const SolverState& state) {
    ++profiling.finished_states;
    if (survival_audit) {
        const int16_t progress = survival_audit->match_prefix(
            rebuild_actions(state.trace_id)
        );
        survival_audit->note_generated(progress, state.route_audit_quantity);
        survival_audit->note_materialized(rebuild_actions(state.trace_id));
        survival_audit->note_finished(progress);
    }
    const int points = Game::check_points(state.to_player(), "Final");
    if (measuring_skeleton_queue_family) {
        active_skeleton_queue_best_points = std::max(
            active_skeleton_queue_best_points,
            points
        );
    }
    const bool has_incumbent = std::any_of(finished.begin(), finished.end(), [](const FinishedSolve& solve) {
        return solve.origin == FinishedSolve::Origin::Incumbent;
    });
    const size_t discovered_capacity = MAX_FINISHED_TO_KEEP - (has_incumbent ? 1U : 0U);
    size_t discovered_count = 0;
    int lowest_discovered_points = std::numeric_limits<int>::max();
    for (const FinishedSolve& solve : finished) {
        if (solve.origin != FinishedSolve::Origin::Discovered) {
            continue;
        }
        ++discovered_count;
        lowest_discovered_points = std::min(lowest_discovered_points, solve.points);
    }
    const bool keep_in_global_top =
        discovered_count < discovered_capacity || points > lowest_discovered_points;
    if (!keep_in_global_top && active_expansion_finishes == nullptr) {
        return;
    }

    FinishedSolve solve;
    solve.points = points;
    solve.actions = rebuild_actions(state.trace_id);
    solve.trace_id = state.trace_id;
    Player replayed = start_state.to_player();
    std::vector<ga_data> replay_events;
    replay_events.reserve(solve.actions.size());
    for (const SolverAction& action : solve.actions) {
        replay_events.push_back(replay_event(action));
    }
    Game::auto_game_loop(replayed, replay_events);
    const int replayed_points = Game::check_points(replayed, "Final");
    if (replayed_points != solve.points) {
        std::ostringstream error;
        error << "Deferred route replay mismatch: symbolic=" << solve.points
              << " replayed=" << replayed_points;
        throw std::logic_error(error.str());
    }
    solve.origin = FinishedSolve::Origin::Discovered;
    if (active_expansion_finishes != nullptr) {
        active_expansion_finishes->push_back(solve);
    }
    if (!keep_in_global_top) {
        return;
    }
    finished.emplace_back(std::move(solve));
    sort_and_limit_finished();
}

void GameSolver::count_generated_action(const SolverAction& action) {
    ++profiling.generated_states;
    switch (action.action) {
        case _a_buy:
            ++profiling.generated_buy_states;
            break;
        case _a_wait:
            ++profiling.generated_wait_states;
            break;
        case _a_sell:
            ++profiling.generated_sell_states;
            break;
        case _a_npc:
            if (action.option == static_cast<uint8_t>(n_Trader)) {
                ++profiling.generated_trader_states;
            } else {
                ++profiling.generated_camel_states;
            }
            break;
        case _a_interest:
            ++profiling.generated_interest_states;
            break;
        case _a_merchant:
            ++profiling.generated_merchant_states;
            break;
        case _a_witch:
            ++profiling.generated_witch_states;
            break;
        case _a_disaster:
            ++profiling.generated_disaster_states;
            break;
    }
}

void GameSolver::append_state(
    FrontierAccumulator& target,
    SolverState state,
    const SolverAction& action,
    const uint32_t parent_trace,
    const int16_t parent_audit_progress
) {
    state.trace_id = append_trace(parent_trace, action);
    state.route_audit_progress = survival_audit
        ? survival_audit->advance_exact(parent_audit_progress, action)
        : RouteSurvivalAudit::NO_MATCH;
    if (survival_audit) {
        state.route_audit_quantity = action.action == _a_buy &&
            state.route_audit_progress != RouteSurvivalAudit::NO_MATCH
            ? RouteAuditQuantityStatus::Exact
            : RouteAuditQuantityStatus::NotApplicable;
        survival_audit->note_generated(
            state.route_audit_progress,
            state.route_audit_quantity
        );
    }
    target.insert(std::move(state));
    trim_generated_frontier(target);
    profiling.peak_candidate_states = std::max<uint64_t>(profiling.peak_candidate_states, target.size());
    count_generated_action(action);
}

uint32_t GameSolver::append_trace(const uint32_t parent_trace, const SolverAction& action) {
    TraceNode node;
    node.parent = parent_trace;
    node.action = action;
    if (parent_trace < traces.size()) {
        node.early_skeleton = traces[parent_trace].early_skeleton.extended(action);
    }
    traces.emplace_back(std::move(node));
    return static_cast<uint32_t>(traces.size() - 1);
}

uint32_t GameSolver::append_trace_actions(
    uint32_t parent_trace,
    const std::vector<SolverAction>& actions
) {
    for (const SolverAction& action : actions) {
        parent_trace = append_trace(parent_trace, action);
    }
    if (survival_audit) {
        survival_audit->note_materialized(rebuild_actions(parent_trace));
    }
    return parent_trace;
}

uint64_t GameSolver::skeleton_key(const SolverState& state) const noexcept {
    const EarlyActionSkeleton skeleton = early_skeleton(state);
    return skeleton.key(skeleton_depth) |
           ((static_cast<uint64_t>(state.player_world.merch) & 0x0fULL) << 40U) |
           ((static_cast<uint64_t>(state.player_world.witch) & 0x0fULL) << 44U) |
           ((static_cast<uint64_t>(state.player_world.disaster) & 0x0fULL) << 48U) |
           (static_cast<uint64_t>(state.has_flag(SSF_MERCHANT_HAPPENED)) << 52U) |
           (static_cast<uint64_t>(state.has_flag(SSF_WITCH_HAPPENED)) << 53U) |
           (static_cast<uint64_t>(state.has_flag(SSF_DISASTER_HAPPENED)) << 54U);
}

EarlyActionSkeleton GameSolver::early_skeleton(const SolverState& state) const noexcept {
    EarlyActionSkeleton skeleton;
    if (state.trace_id < traces.size()) {
        skeleton = traces[state.trace_id].early_skeleton;
    }
    if (state.deferred) {
        skeleton = state.deferred->extend_skeleton(skeleton);
    }
    return skeleton;
}

uint64_t GameSolver::continuation_structure_key(const SolverState& state) noexcept {
    const uint64_t trader = static_cast<uint64_t>(std::clamp<int>(state.trader, 0, 255));
    const uint64_t camel = static_cast<uint64_t>(std::clamp<int>(state.camel, 0, 255));
    const uint64_t interest = static_cast<uint64_t>(std::clamp<int>(state.interest_hundredths, 0, 511));
    const uint64_t merchant = static_cast<uint64_t>(state.player_world.merch) & 0x0fULL;
    const uint64_t witch = static_cast<uint64_t>(state.player_world.witch) & 0x0fULL;
    const bool carrying = state.spice > 0 || state.pottery > 0 || state.marble > 0 ||
                          state.silk > 0 || state.jewelry > 0;
    const bool open_purchase = state.deferred && state.deferred->has_open_purchase();
    return trader |
           (camel << 8U) |
           (interest << 16U) |
           (merchant << 25U) |
           (witch << 29U) |
           (static_cast<uint64_t>(carrying) << 33U) |
           (static_cast<uint64_t>(open_purchase) << 34U) |
           (static_cast<uint64_t>(state.has_flag(SSF_STATUE)) << 35U) |
           (static_cast<uint64_t>(state.has_flag(SSF_HAND_OF_MIDAS)) << 36U) |
           (static_cast<uint64_t>(state.has_flag(SSF_BOUGHT_LAST)) << 37U) |
           (static_cast<uint64_t>(state.has_flag(SSF_CARGO_BUY_LOCKED)) << 38U);
}

void GameSolver::append_open_purchase(
    FrontierAccumulator& target,
    DeferredPurchaseModel purchase,
    const SolverAction& generated_action,
    const int16_t parent_audit_progress
) {
    profiling.deferred_buy_seed_inputs += purchase.input_seed_count();
    profiling.deferred_buy_origins_kept += purchase.origin_count();
    const auto provenances = purchase.provenances();
    const uint32_t primary_trace = provenances.empty() ? 0 : provenances.front();
    auto deferred = std::make_shared<DeferredSearchState>(std::move(purchase));
    ++profiling.deferred_purchase_states;
    profiling.deferred_rows_total += deferred->row_count();
    profiling.deferred_max_rows = std::max<uint64_t>(profiling.deferred_max_rows, deferred->row_count());
    SolverState next_state = SolverState::from_player(deferred->representative(), primary_trace);
    next_state.deferred = std::move(deferred);
    next_state.route_audit_progress = survival_audit
        ? survival_audit->advance(parent_audit_progress, generated_action)
        : RouteSurvivalAudit::NO_MATCH;
    if (survival_audit) {
        next_state.route_audit_quantity = classify_route_audit_quantity(
            next_state.route_audit_progress,
            *next_state.deferred
        );
        survival_audit->note_generated(
            next_state.route_audit_progress,
            next_state.route_audit_quantity
        );
    }
    target.insert(std::move(next_state));
    trim_generated_frontier(target);
    profiling.peak_candidate_states = std::max<uint64_t>(profiling.peak_candidate_states, target.size());
    count_generated_action(generated_action);
}

void GameSolver::append_sold_transition(
    FrontierAccumulator& target,
    DeferredSoldModel sold,
    const SolverAction& action,
    const int16_t parent_audit_progress
) {
    const auto provenances = sold.provenances();
    const uint32_t primary_trace = provenances.empty() ? 0 : provenances.front();
    auto deferred = std::make_shared<DeferredSearchState>(std::move(sold));
    ++profiling.deferred_sold_states;
    profiling.deferred_rows_total += deferred->row_count();
    profiling.deferred_max_rows = std::max<uint64_t>(profiling.deferred_max_rows, deferred->row_count());
    SolverState next_state = SolverState::from_player(deferred->representative(), primary_trace);
    next_state.deferred = std::move(deferred);
    next_state.route_audit_progress = survival_audit
        ? survival_audit->advance(parent_audit_progress, action)
        : RouteSurvivalAudit::NO_MATCH;
    if (survival_audit) {
        next_state.route_audit_quantity = classify_route_audit_quantity(
            next_state.route_audit_progress,
            *next_state.deferred
        );
        survival_audit->note_generated(
            next_state.route_audit_progress,
            next_state.route_audit_quantity
        );
    }
    target.insert(std::move(next_state));
    trim_generated_frontier(target);
    profiling.peak_candidate_states = std::max<uint64_t>(profiling.peak_candidate_states, target.size());
    count_generated_action(action);
}

ContinuationMatrixRow GameSolver::continuation_matrix_row(const SolverState& state) {
    ContinuationMatrixRow row;
    if (state.deferred) {
        const auto cached = continuation_score_cache.find(state.deferred->ranking_identity());
        if (cached != continuation_score_cache.end()) {
            row = cached->second;
        } else {
            row.baseline_realistic = beam_score(state);
            row.baseline_optimistic = optimistic_final_bound(state);
            ++profiling.continuation_view_owners;
            for (const DeferredContinuationView& view : state.deferred->continuation_views(
                     handler,
                     deferred_player_action_reachable(state)
                 )) {
                ++profiling.continuation_views_evaluated;
                const SolverState candidate = SolverState::from_player(view.player, state.trace_id);
                const long double realistic = beam_score(candidate);
                const long double optimistic = optimistic_final_bound(candidate);
                size_t lane = 0;
                switch (view.kind) {
                    case DeferredContinuationKind::ImmediateSell:
                        lane = static_cast<size_t>(ContinuationLane::ImmediateSell);
                        break;
                    case DeferredContinuationKind::Trader:
                        lane = static_cast<size_t>(ContinuationLane::Trader);
                        break;
                    case DeferredContinuationKind::Camel:
                        lane = static_cast<size_t>(ContinuationLane::Camel);
                        break;
                    case DeferredContinuationKind::Interest:
                        lane = static_cast<size_t>(ContinuationLane::Interest);
                        break;
                    case DeferredContinuationKind::FoodOnlyBuy:
                        lane = static_cast<size_t>(ContinuationLane::FoodOnlyBuy);
                        break;
                }
                row.lanes[lane] = ContinuationCell{true, realistic, optimistic};
                if (realistic > row.baseline_realistic) {
                    ++profiling.continuation_view_score_wins;
                }
                if (optimistic > row.baseline_optimistic) {
                    ++profiling.continuation_view_bound_wins;
                }
            }
            continuation_score_cache.emplace(state.deferred->ranking_identity(), row);
        }
    } else {
        row.baseline_realistic = beam_score(state);
        row.baseline_optimistic = optimistic_final_bound(state);
    }
    row.gold = state.gold;
    row.food = state.food;
    row.stable_id = state.trace_id;
    return row;
}

void GameSolver::observe_route_audit_ranks(const std::vector<SolverState>& states) {
    if (!survival_audit || states.empty()) {
        return;
    }

    const int16_t progress = std::accumulate(
        states.begin(),
        states.end(),
        RouteSurvivalAudit::NO_MATCH,
        [](const int16_t best, const SolverState& state) {
            return std::max(best, state.route_audit_progress);
        }
    );
    if (progress == RouteSurvivalAudit::NO_MATCH) {
        return;
    }

    std::vector<ContinuationMatrixRow> rows;
    rows.reserve(states.size());
    for (const SolverState& state : states) {
        rows.emplace_back(continuation_matrix_row(state));
    }

    long double best_realistic = -std::numeric_limits<long double>::infinity();
    long double best_optimistic = -std::numeric_limits<long double>::infinity();
    std::array<long double, CONTINUATION_LANE_COUNT> best_lanes{};
    std::array<bool, CONTINUATION_LANE_COUNT> has_lane{};
    for (size_t index = 0; index < states.size(); ++index) {
        if (states[index].route_audit_progress != progress) {
            continue;
        }
        best_realistic = std::max(best_realistic, rows[index].best_realistic());
        best_optimistic = std::max(best_optimistic, rows[index].best_optimistic());
        for (size_t lane = 0; lane < CONTINUATION_LANE_COUNT; ++lane) {
            if (!rows[index].lanes[lane].valid) {
                continue;
            }
            if (!has_lane[lane] ||
                rows[index].lanes[lane].realistic > best_lanes[lane]) {
                has_lane[lane] = true;
                best_lanes[lane] = rows[index].lanes[lane].realistic;
            }
        }
    }

    size_t realistic_rank = 1;
    size_t optimistic_rank = 1;
    std::array<size_t, CONTINUATION_LANE_COUNT> lane_ranks{};
    for (size_t lane = 0; lane < CONTINUATION_LANE_COUNT; ++lane) {
        if (has_lane[lane]) {
            lane_ranks[lane] = 1;
        }
    }
    for (const ContinuationMatrixRow& row : rows) {
        if (row.best_realistic() > best_realistic) {
            ++realistic_rank;
        }
        if (row.best_optimistic() > best_optimistic) {
            ++optimistic_rank;
        }
        for (size_t lane = 0; lane < CONTINUATION_LANE_COUNT; ++lane) {
            if (has_lane[lane] && row.lanes[lane].valid &&
                row.lanes[lane].realistic > best_lanes[lane]) {
                ++lane_ranks[lane];
            }
        }
    }
    survival_audit->note_ranks(
        progress,
        realistic_rank,
        optimistic_rank,
        lane_ranks
    );
}

void GameSolver::reduce_to_beam_limit(std::vector<SolverState>& states) {
    const auto reduction_started = std::chrono::steady_clock::now();
    ExactFrontierReduction exact = FrontierReducer::reduce_exact(
        std::move(states),
        survival_audit ? &*survival_audit : nullptr
    );
    profiling.frontier_reduction_nanoseconds += static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - reduction_started
        ).count()
    );
    profiling.duplicate_prunes += exact.duplicate_prunes;
    profiling.dominance_prunes += exact.dominance_prunes;
    profiling.pre_beam_duplicate_prunes += exact.duplicate_prunes;
    profiling.pre_beam_dominance_prunes += exact.dominance_prunes;
    profiling.deferred_dominance_checks += exact.deferred_dominance_checks;
    profiling.deferred_partition_rejections += exact.deferred_partition_rejections;
    profiling.deferred_purchase_dominance_prunes +=
        exact.deferred_purchase_dominance_prunes;
    profiling.deferred_sold_dominance_prunes +=
        exact.deferred_sold_dominance_prunes;
    profiling.deferred_gold_function_comparisons +=
        exact.deferred_gold_function_comparisons;
    profiling.deferred_exact_spend_evaluations +=
        exact.deferred_exact_spend_evaluations;
    profiling.deferred_origin_coverage_checks +=
        exact.deferred_origin_coverage_checks;
    profiling.deferred_cross_shape_dominance_prunes +=
        exact.deferred_cross_shape_dominance_prunes;
    states = std::move(exact.states);

    if (array_limit > 0 && states.size() > static_cast<size_t>(array_limit)) {
        observe_route_audit_ranks(states);
        trim_to_beam_limit(states);
    }
    if (survival_audit) {
        for (const SolverState& state : states) {
            survival_audit->note_reduction(
                RouteAuditReductionStage::AfterBeam,
                state.route_audit_progress,
                state.route_audit_quantity
            );
        }
    }
}

std::vector<SolverState> GameSolver::finish_generated_frontier(
    FrontierAccumulator& accumulator
) {
    ExactFrontierReduction exact = accumulator.finish();
    profiling.duplicate_prunes += exact.duplicate_prunes;
    profiling.dominance_prunes += exact.dominance_prunes;
    profiling.pre_beam_duplicate_prunes += exact.duplicate_prunes;
    profiling.pre_beam_dominance_prunes += exact.dominance_prunes;
    profiling.deferred_dominance_checks += exact.deferred_dominance_checks;
    profiling.deferred_partition_rejections += exact.deferred_partition_rejections;
    profiling.deferred_purchase_dominance_prunes +=
        exact.deferred_purchase_dominance_prunes;
    profiling.deferred_sold_dominance_prunes +=
        exact.deferred_sold_dominance_prunes;
    profiling.deferred_gold_function_comparisons +=
        exact.deferred_gold_function_comparisons;
    profiling.deferred_exact_spend_evaluations +=
        exact.deferred_exact_spend_evaluations;
    profiling.deferred_origin_coverage_checks +=
        exact.deferred_origin_coverage_checks;
    profiling.deferred_cross_shape_dominance_prunes +=
        exact.deferred_cross_shape_dominance_prunes;
    return std::move(exact.states);
}

void GameSolver::trim_generated_frontier(FrontierAccumulator& accumulator) {
    if (proof_search_active || array_limit <= 0 ||
        accumulator.size() < static_cast<size_t>(array_limit) * 2ULL) {
        return;
    }
    std::vector<SolverState> states = finish_generated_frontier(accumulator);
    observe_route_audit_ranks(states);
    trim_to_beam_limit(states);
    FrontierAccumulator retained(
        survival_audit ? &*survival_audit : nullptr
    );
    for (SolverState& state : states) {
        retained.insert(std::move(state));
    }
    accumulator = std::move(retained);
}

void GameSolver::trim_to_beam_limit(std::vector<SolverState>& states) {
    if (array_limit <= 0 || states.size() <= static_cast<size_t>(array_limit)) {
        return;
    }

    const auto trim_started = std::chrono::steady_clock::now();
    const size_t keep = static_cast<size_t>(array_limit);
    continuation_score_cache.reserve(continuation_score_cache.size() + states.size());
    const auto scoring_started = std::chrono::steady_clock::now();
    for (const SolverState& state : states) {
        (void)continuation_matrix_row(state);
    }
    profiling.continuation_matrix_scoring_nanoseconds += static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - scoring_started
        ).count()
    );
    const auto realistic_score = [this](const SolverState& state) {
        if (state.deferred) {
            const auto found = continuation_score_cache.find(state.deferred->ranking_identity());
            if (found != continuation_score_cache.end()) {
                return found->second.best_realistic();
            }
        }
        return beam_score(state);
    };
    const auto realistic_better = [this, &realistic_score](const SolverState& lhs, const SolverState& rhs) {
        const long double lhs_score = realistic_score(lhs);
        const long double rhs_score = realistic_score(rhs);
        if (lhs_score != rhs_score) {
            return lhs_score > rhs_score;
        }
        if (lhs.gold != rhs.gold) {
            return lhs.gold > rhs.gold;
        }
        if (lhs.food != rhs.food) {
            return lhs.food > rhs.food;
        }
        return lhs.trace_id < rhs.trace_id;
    };
    std::vector<SolverState> selected;
    std::vector<SolverState> remaining;
    selected.reserve(keep);
    remaining.reserve(states.size());

    std::vector<uint8_t> reserved(states.size(), 0);
    const size_t refinement_keep = (keep * refinement_share_percent) / 100;
    if (refinement_keep > 0 && refinement_template.has_value()) {
        std::unordered_map<uint64_t, std::vector<size_t>> by_nearby_skeleton;
        by_nearby_skeleton.reserve(states.size());
        for (size_t index = 0; index < states.size(); ++index) {
            const EarlyActionSkeleton candidate = early_skeleton(states[index]);
            if (candidate.prefix_mismatches(*refinement_template, skeleton_depth) >
                refinement_maximum_mismatches) {
                continue;
            }
            by_nearby_skeleton[candidate.key(skeleton_depth)].push_back(index);
        }
        profiling.refinement_group_candidates += by_nearby_skeleton.size();
        profiling.refinement_max_groups = std::max<uint64_t>(
            profiling.refinement_max_groups,
            by_nearby_skeleton.size()
        );

        std::vector<std::vector<size_t>> groups;
        groups.reserve(by_nearby_skeleton.size());
        for (auto& [key, indices] : by_nearby_skeleton) {
            (void)key;
            std::sort(indices.begin(), indices.end(), [&](const size_t lhs, const size_t rhs) {
                return realistic_better(states[lhs], states[rhs]);
            });
            groups.emplace_back(std::move(indices));
        }
        std::sort(groups.begin(), groups.end(), [&](const auto& lhs, const auto& rhs) {
            return realistic_better(states[lhs.front()], states[rhs.front()]);
        });

        size_t selected_for_refinement = 0;
        for (size_t rank = 0; selected_for_refinement < refinement_keep; ++rank) {
            bool found_at_rank = false;
            for (const auto& group : groups) {
                if (rank >= group.size()) {
                    continue;
                }
                found_at_rank = true;
                reserved[group[rank]] = 1;
                ++selected_for_refinement;
                if (selected_for_refinement == refinement_keep) {
                    break;
                }
            }
            if (!found_at_rank) {
                break;
            }
        }
        profiling.refinement_reserved_states += selected_for_refinement;
    }

    const size_t refinement_reserved = static_cast<size_t>(
        std::count(reserved.begin(), reserved.end(), uint8_t{1})
    );
    const size_t skeleton_keep = std::min(
        (keep * skeleton_share_percent) / 100,
        keep - std::min(keep, refinement_reserved)
    );
    if (skeleton_keep > 0) {
        std::unordered_map<uint64_t, size_t> best_by_skeleton;
        best_by_skeleton.reserve(states.size());
        for (size_t index = 0; index < states.size(); ++index) {
            if (reserved[index]) {
                continue;
            }
            const uint64_t key = skeleton_key(states[index]);
            const auto found = best_by_skeleton.find(key);
            if (found == best_by_skeleton.end() ||
                realistic_better(states[index], states[found->second])) {
                best_by_skeleton[key] = index;
            }
        }
        profiling.skeleton_group_candidates += best_by_skeleton.size();
        profiling.skeleton_max_groups = std::max<uint64_t>(
            profiling.skeleton_max_groups,
            best_by_skeleton.size()
        );
        std::vector<size_t> winners;
        winners.reserve(best_by_skeleton.size());
        for (const auto& [key, index] : best_by_skeleton) {
            (void)key;
            winners.push_back(index);
        }
        std::sort(winners.begin(), winners.end(), [&](const size_t lhs, const size_t rhs) {
            return realistic_better(states[lhs], states[rhs]);
        });
        if (winners.size() > skeleton_keep) {
            winners.resize(skeleton_keep);
        }
        profiling.skeleton_reserved_states += winners.size();
        for (const size_t index : winners) {
            reserved[index] = 1;
        }
    }

    const size_t already_reserved = static_cast<size_t>(std::count(reserved.begin(), reserved.end(), uint8_t{1}));
    const size_t diversity_keep = std::min(
        (keep * diversity_share_percent) / 100,
        keep - std::min(keep, already_reserved)
    );
    if (diversity_keep > 0) {
        std::unordered_map<uint64_t, size_t> best_by_structure;
        best_by_structure.reserve(states.size());
        for (size_t index = 0; index < states.size(); ++index) {
            if (reserved[index]) {
                continue;
            }
            const uint64_t key = continuation_structure_key(states[index]);
            const auto found = best_by_structure.find(key);
            if (found == best_by_structure.end() ||
                realistic_better(states[index], states[found->second])) {
                best_by_structure[key] = index;
            }
        }
        std::vector<size_t> winners;
        winners.reserve(best_by_structure.size());
        for (const auto& [key, index] : best_by_structure) {
            (void)key;
            winners.push_back(index);
        }
        std::sort(winners.begin(), winners.end(), [&](const size_t lhs, const size_t rhs) {
            return realistic_better(states[lhs], states[rhs]);
        });
        if (winners.size() > diversity_keep) {
            winners.resize(diversity_keep);
        }
        for (const size_t index : winners) {
            reserved[index] = 1;
        }
    }

    for (size_t index = 0; index < states.size(); ++index) {
        if (reserved[index]) {
            selected.emplace_back(std::move(states[index]));
        } else {
            remaining.emplace_back(std::move(states[index]));
        }
    }

    const size_t ordinary_keep = keep - selected.size();
    std::vector<ContinuationMatrixRow> matrix_rows;
    matrix_rows.reserve(remaining.size());
    for (const SolverState& state : remaining) {
        matrix_rows.emplace_back(continuation_matrix_row(state));
    }
    const auto selection_started = std::chrono::steady_clock::now();
    const ContinuationMatrixSelection matrix_selection =
        ContinuationMatrixSelector::select(
            matrix_rows,
            ordinary_keep,
            continuation_lane_share_percent,
            optimistic_share_percent
        );
    profiling.continuation_matrix_selection_nanoseconds += static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - selection_started
        ).count()
    );
    profiling.continuation_matrix_lane_owners += matrix_selection.lane_owner_total;
    profiling.continuation_matrix_duplicate_ticket_skips +=
        matrix_selection.duplicate_ticket_skips;
    for (size_t lane = 0; lane < CONTINUATION_LANE_COUNT; ++lane) {
        profiling.continuation_matrix_lane_counts[lane] +=
            matrix_selection.lane_owner_counts[lane];
    }
    for (const size_t index : matrix_selection.owner_indices) {
        selected.emplace_back(std::move(remaining[index]));
    }

    profiling.beam_prunes += states.size() - selected.size();
    states = std::move(selected);
    profiling.beam_trim_nanoseconds += static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - trim_started
        ).count()
    );
}

bool GameSolver::buy_state_dominates(const Player& lhs, const Player& rhs) {
    return lhs.gold >= rhs.gold &&
           lhs.food >= rhs.food &&
           lhs.spice >= rhs.spice &&
           lhs.pottery >= rhs.pottery &&
           lhs.marble >= rhs.marble &&
           lhs.silk >= rhs.silk &&
           lhs.jewelry >= rhs.jewelry;
}

std::vector<SolverState> GameSolver::generate_buy_successors(const Player& current) {
    std::deque<Player> generated_players;
    const BuyEventStats buy_stats = Helpers::run_buy_event(
        const_cast<Player&>(current),
        generated_players,
        handler,
        merch_on,
        witch_on,
        purchase_profile
    );
    profiling.purchase_valuation_worlds += buy_stats.valuation_worlds;
    profiling.purchase_food_range_total += buy_stats.food_range_total;
    profiling.purchase_food_only_checks += buy_stats.food_only_checks;
    profiling.purchase_queries += buy_stats.best_buy_queries;
    profiling.purchase_portfolio_choices += buy_stats.portfolio_choices;
    profiling.purchase_unique_buys += buy_stats.unique_buys;
    profiling.purchase_duplicate_buys += buy_stats.duplicate_emits;
    profiling.purchase_dominance_prunes += buy_stats.dominance_prunes;
    profiling.max_purchase_food_range = std::max<uint64_t>(
        profiling.max_purchase_food_range,
        static_cast<uint64_t>(std::max(buy_stats.max_food_range, 0))
    );

    std::vector<Player> filtered_players;
    filtered_players.reserve(generated_players.size());
    while (!generated_players.empty()) {
        filtered_players.emplace_back(std::move(generated_players.front()));
        generated_players.pop_front();
    }

    std::vector<bool> dominated(filtered_players.size(), false);
    for (size_t i = 0; i < filtered_players.size(); ++i) {
        if (dominated[i]) {
            continue;
        }
        for (size_t j = 0; j < filtered_players.size(); ++j) {
            if (i == j || dominated[j]) {
                continue;
            }
            if (buy_state_dominates(filtered_players[i], filtered_players[j])) {
                dominated[j] = true;
            }
        }
    }

    std::vector<SolverState> results;
    results.reserve(filtered_players.size());
    for (size_t index = 0; index < filtered_players.size(); ++index) {
        if (!dominated[index]) {
            results.emplace_back(SolverState::from_player(filtered_players[index], 0));
        }
    }
    return results;
}

std::vector<SolverState> GameSolver::compress_layer(
    std::vector<SolverState> states,
    const int active_day_value,
    std::vector<SolverState>* preserved_states,
    std::vector<SolverState>* additional_preserved_states,
    const bool already_exact,
    const bool compact_trace_arena
) {
    const ScopedNanosecondTimer compression_timer(profiling.layer_compression_nanoseconds);
    states.erase(
        std::remove_if(
            states.begin(),
            states.end(),
            [active_day_value](const SolverState& state) {
                return active_day(state) != active_day_value;
            }
        ),
        states.end()
    );
    if (already_exact) {
        if (array_limit > 0 && states.size() > static_cast<size_t>(array_limit)) {
            observe_route_audit_ranks(states);
            trim_to_beam_limit(states);
        }
        if (survival_audit) {
            for (const SolverState& state : states) {
                survival_audit->note_reduction(
                    RouteAuditReductionStage::AfterBeam,
                    state.route_audit_progress,
                    state.route_audit_quantity
                );
            }
        }
    } else {
        reduce_to_beam_limit(states);
    }
    std::vector<SolverState> survivors = std::move(states);

    if (!compact_trace_arena) {
        return survivors;
    }

    std::unordered_set<uint32_t> needed_traces;
    const size_t preserved_count =
        (preserved_states == nullptr ? 0 : preserved_states->size()) +
        (additional_preserved_states == nullptr ? 0 : additional_preserved_states->size());
    needed_traces.reserve((survivors.size() + preserved_count) * 4);
    const auto collect_state_traces = [this, &needed_traces](const std::vector<SolverState>& source) {
        for (const auto& state : source) {
            uint32_t current = state.trace_id;
            while (current != 0 && current < traces.size() && needed_traces.insert(current).second) {
                current = traces[current].parent;
            }
            if (state.deferred) {
                for (const uint32_t provenance : state.deferred->provenances()) {
                    current = provenance;
                    while (current != 0 && current < traces.size() && needed_traces.insert(current).second) {
                        current = traces[current].parent;
                    }
                }
            }
        }
    };
    collect_state_traces(survivors);
    if (preserved_states != nullptr) {
        collect_state_traces(*preserved_states);
    }
    if (additional_preserved_states != nullptr) {
        collect_state_traces(*additional_preserved_states);
    }

    std::vector<TraceNode> compact_traces(1);
    compact_traces.reserve(needed_traces.size() + 1);
    std::unordered_map<uint32_t, uint32_t> remap;
    remap.reserve(needed_traces.size() + 1);
    remap[0] = 0;

    const auto remap_trace_chain = [&](const uint32_t trace_id) {
        std::vector<uint32_t> chain;
        uint32_t current = trace_id;
        while (current != 0 && current < traces.size() && remap.find(current) == remap.end()) {
            chain.push_back(current);
            current = traces[current].parent;
        }
        while (!chain.empty()) {
            const uint32_t old_id = chain.back();
            chain.pop_back();
            remap[old_id] = static_cast<uint32_t>(compact_traces.size());
            compact_traces.push_back(traces[old_id]);
            compact_traces.back().parent = remap[traces[old_id].parent];
        }
    };

    const auto build_state_remaps = [&remap_trace_chain](const std::vector<SolverState>& source) {
        for (const auto& state : source) {
            remap_trace_chain(state.trace_id);
            if (state.deferred) {
                for (const uint32_t provenance : state.deferred->provenances()) {
                    remap_trace_chain(provenance);
                }
            }
        }
    };
    build_state_remaps(survivors);
    if (preserved_states != nullptr) {
        build_state_remaps(*preserved_states);
    }
    if (additional_preserved_states != nullptr) {
        build_state_remaps(*additional_preserved_states);
    }

    const auto remap_state_traces = [&remap](SolverState& state) {
        const auto remapped = remap.find(state.trace_id);
        state.trace_id = remapped == remap.end() ? 0 : remapped->second;
        if (state.deferred) {
            state.deferred = std::make_shared<DeferredSearchState>(
                state.deferred->remap_provenances(remap)
            );
            const auto provenances = state.deferred->provenances();
            state.trace_id = provenances.empty() ? 0 : provenances.front();
        }
    };
    for (auto& state : survivors) {
        remap_state_traces(state);
    }
    if (preserved_states != nullptr) {
        for (auto& state : *preserved_states) {
            remap_state_traces(state);
        }
    }
    if (additional_preserved_states != nullptr) {
        for (auto& state : *additional_preserved_states) {
            remap_state_traces(state);
        }
    }
    traces = std::move(compact_traces);
    return survivors;
}

void GameSolver::expand_deferred_state(
    const SolverState& state,
    const int active_day_value,
    FrontierAccumulator& same_day,
    FrontierAccumulator& next_day
) {
    if (state.deferred->has_open_purchase()) {
        const ScopedNanosecondTimer expansion_timer(
            profiling.deferred_purchase_expansion_nanoseconds
        );
        expand_open_purchase(state, active_day_value, same_day, next_day);
    } else {
        const ScopedNanosecondTimer expansion_timer(
            profiling.deferred_sold_expansion_nanoseconds
        );
        expand_sold_ledger(state, active_day_value, same_day, next_day);
    }
}

void GameSolver::expand_open_purchase(
    const SolverState& state,
    const int active_day_value,
    FrontierAccumulator& same_day,
    FrontierAccumulator& next_day
) {
    ++profiling.processed_states;
    ++profiling.deferred_expansions;
    ++profiling.deferred_purchase_expansions;
    if (stop_requested) {
        return;
    }

    const DeferredPurchaseModel& purchase = state.deferred->purchase();
    std::optional<DeferredPurchaseModel> working = state.has_flag(SSF_DAY_START_SKIP)
        ? std::optional<DeferredPurchaseModel>(purchase.after_resume_same_day())
        : purchase.after_start_of_day();
    if (!working.has_value()) {
        return;
    }
    Player current = working->representative();

    if (!state.has_flag(SSF_DAY_START_SKIP)) {
        const EncounterOpportunity merchant_timing = EncounterTiming::merchant(current);
        bool generated_merchant_branch = false;
        if (merch_on && !current.merchant_happened && merchant_timing.due) {
            for (const auto option : ALL_MERCH_OPTIONS) {
                if (!merchant_option_allowed(option)) {
                    continue;
                }
                auto merchant = working->after_merchant(option);
                if (!merchant.has_value()) {
                    continue;
                }
                append_open_purchase(
                    same_day,
                    std::move(*merchant),
                    SolverAction{_a_merchant, buy_list(), static_cast<uint8_t>(option)},
                    state.route_audit_progress
                );
                ++profiling.current_day_requeues;
                generated_merchant_branch = true;
            }
            if (current.merchant_day > 0 && merchant_option_allowed(m_No_Merchant)) {
                auto declined = working->after_merchant(m_No_Merchant);
                if (declined.has_value()) {
                    append_open_purchase(
                        same_day,
                        std::move(*declined),
                        SolverAction{_a_merchant, buy_list(), static_cast<uint8_t>(m_No_Merchant)},
                        state.route_audit_progress
                    );
                    ++profiling.current_day_requeues;
                    generated_merchant_branch = true;
                }
            }
            if (generated_merchant_branch) {
                if (!merchant_timing.may_defer) {
                    enforce_memory_budget(same_day, next_day);
                    return;
                }
                ++profiling.merchant_deferral_continuations;
            }
        }

        const EncounterOpportunity witch_timing = EncounterTiming::witch(current);
        bool generated_witch_branch = false;
        if (witch_on && !current.witch_happened && witch_timing.due) {
            for (const auto option : ALL_WITCH_OPTIONS) {
                if (!witch_option_allowed(option)) {
                    continue;
                }
                DeferredPurchaseModel witch = working->after_witch(option);
                append_open_purchase(
                    same_day,
                    std::move(witch),
                    SolverAction{_a_witch, buy_list(), static_cast<uint8_t>(option)},
                    state.route_audit_progress
                );
                ++profiling.current_day_requeues;
                generated_witch_branch = true;
            }
            if (generated_witch_branch) {
                if (!witch_timing.may_defer) {
                    enforce_memory_budget(same_day, next_day);
                    return;
                }
                ++profiling.witch_deferral_continuations;
            }
        }

        if (!current.disaster_happened &&
            current.scheduled_disaster != d_No_Disaster &&
            current.day == current.disaster_day) {
            request_stop("deferred purchase reached unsupported disaster mode");
            return;
        }
    }

    if (current.day < current.max_day && !current.bought_last && Game::check_can_buy_items(current)) {
        const auto resolutions = working->resolve_food_only_for_next_buy();
        std::vector<DeferredPurchaseSeed> seeds;
        seeds.reserve(resolutions.size());
        for (const DeferredPurchaseResolution& resolution : resolutions) {
            seeds.push_back(DeferredPurchaseSeed{
                resolution.player.gold,
                resolution.player.food,
                append_trace_actions(resolution.parent_trace, resolution.actions),
            });
        }
        if (!seeds.empty()) {
            append_open_purchase(
                next_day,
                DeferredPurchaseModel::open(resolutions.front().player, seeds),
                SolverAction{_a_buy, buy_list(), 0},
                state.route_audit_progress
            );
        }
    }

    const auto record_open_finish = [this](const DeferredPurchaseModel& finished_purchase) {
        for (const DeferredPurchaseResolution& resolution : finished_purchase.resolve_finish(handler)) {
            const uint32_t trace_id = append_trace_actions(resolution.parent_trace, resolution.actions);
            record_finished(SolverState::from_player(resolution.player, trace_id));
        }
    };

    if (current.day == current.max_day) {
        const auto sold = working->resolve_sell(handler);
        if (!sold.empty()) {
            for (const DeferredPurchaseResolution& resolution : sold) {
                const uint32_t trace_id = append_trace_actions(resolution.parent_trace, resolution.actions);
                record_finished(SolverState::from_player(resolution.player, trace_id));
            }
        }
    } else if (auto sold = DeferredSoldModel::from_purchase(*working, handler); sold.has_value()) {
        append_sold_transition(
            next_day,
            std::move(*sold),
            SolverAction{_a_sell, buy_list(), 0},
            state.route_audit_progress
        );
    }

    if (auto trader = working->after_hire(n_Trader); trader.has_value()) {
        if (trader->representative().day == trader->representative().max_day) {
            record_open_finish(*trader);
        } else {
            append_open_purchase(
                next_day,
                std::move(*trader),
                SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Trader)},
                state.route_audit_progress
            );
        }
    }
    if (auto camel = working->after_hire(n_Camel); camel.has_value()) {
        if (camel->representative().day == camel->representative().max_day) {
            record_open_finish(*camel);
        } else {
            append_open_purchase(
                next_day,
                std::move(*camel),
                SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Camel)},
                state.route_audit_progress
            );
        }
    }

    if (auto interest = working->after_interest(); interest.has_value()) {
        const Player& interest_player = interest->representative();
        if (interest_player.day == interest_player.max_day &&
            !interest_player.quick_interest && !interest_player.day_start_skip) {
            record_open_finish(*interest);
        } else if (active_day(SolverState::from_player(interest_player)) == active_day_value) {
            append_open_purchase(
                same_day,
                std::move(*interest),
                SolverAction{_a_interest, buy_list(), 0},
                state.route_audit_progress
            );
            ++profiling.current_day_requeues;
        } else {
            append_open_purchase(
                next_day,
                std::move(*interest),
                SolverAction{_a_interest, buy_list(), 0},
                state.route_audit_progress
            );
        }
    }

    enforce_memory_budget(same_day, next_day);
}

void GameSolver::expand_sold_ledger(
    const SolverState& state,
    const int active_day_value,
    FrontierAccumulator& same_day,
    FrontierAccumulator& next_day
) {
    ++profiling.processed_states;
    ++profiling.deferred_expansions;
    ++profiling.deferred_sold_expansions;
    if (stop_requested) {
        return;
    }

    const DeferredSoldModel& stored = state.deferred->sold();
    std::optional<DeferredSoldModel> working = state.has_flag(SSF_DAY_START_SKIP)
        ? stored.after_resume_same_day()
        : stored.after_start_of_day();
    if (!working.has_value()) {
        return;
    }
    Player current = working->representative();

    if (!state.has_flag(SSF_DAY_START_SKIP)) {
        const EncounterOpportunity merchant_timing = EncounterTiming::merchant(current);
        bool generated_merchant_branch = false;
        if (merch_on && !current.merchant_happened &&
            merchant_timing.due) {
            for (const auto option : ALL_MERCH_OPTIONS) {
                if (!merchant_option_allowed(option)) {
                    continue;
                }
                auto merchant = working->after_merchant(option);
                if (!merchant.has_value()) {
                    continue;
                }
                append_sold_transition(
                    same_day,
                    std::move(*merchant),
                    SolverAction{_a_merchant, buy_list(), static_cast<uint8_t>(option)},
                    state.route_audit_progress
                );
                ++profiling.current_day_requeues;
                generated_merchant_branch = true;
            }
            if (current.merchant_day > 0 && merchant_option_allowed(m_No_Merchant)) {
                auto declined = working->after_merchant(m_No_Merchant);
                if (declined.has_value()) {
                    append_sold_transition(
                        same_day,
                        std::move(*declined),
                        SolverAction{_a_merchant, buy_list(), static_cast<uint8_t>(m_No_Merchant)},
                        state.route_audit_progress
                    );
                    ++profiling.current_day_requeues;
                    generated_merchant_branch = true;
                }
            }
            if (generated_merchant_branch) {
                if (!merchant_timing.may_defer) {
                    enforce_memory_budget(same_day, next_day);
                    return;
                }
                ++profiling.merchant_deferral_continuations;
            }
        }

        const EncounterOpportunity witch_timing = EncounterTiming::witch(current);
        bool generated_witch_branch = false;
        if (witch_on && !current.witch_happened && witch_timing.due) {
            for (const auto option : ALL_WITCH_OPTIONS) {
                if (!witch_option_allowed(option)) {
                    continue;
                }
                auto witch = working->after_witch(option);
                if (!witch.has_value()) {
                    continue;
                }
                append_sold_transition(
                    same_day,
                    std::move(*witch),
                    SolverAction{_a_witch, buy_list(), static_cast<uint8_t>(option)},
                    state.route_audit_progress
                );
                ++profiling.current_day_requeues;
                generated_witch_branch = true;
            }
            if (generated_witch_branch) {
                if (!witch_timing.may_defer) {
                    enforce_memory_budget(same_day, next_day);
                    return;
                }
                ++profiling.witch_deferral_continuations;
            }
        }

        if (!current.disaster_happened &&
            current.scheduled_disaster != d_No_Disaster &&
            current.day == current.disaster_day) {
            auto disaster = working->after_disaster(current.scheduled_disaster);
            if (disaster.has_value()) {
                append_sold_transition(
                    same_day,
                    std::move(*disaster),
                    SolverAction{
                        _a_disaster,
                        buy_list(),
                        static_cast<uint8_t>(current.scheduled_disaster),
                    },
                    state.route_audit_progress
                );
                ++profiling.current_day_requeues;
            }
            enforce_memory_budget(same_day, next_day);
            return;
        }
    }

    if (current.day < current.max_day && !current.bought_last &&
        Game::check_can_buy_items(current)) {
        const auto resolutions = working->materialize();
        std::vector<DeferredPurchaseSeed> seeds;
        seeds.reserve(resolutions.size());
        for (const DeferredPurchaseResolution& resolution : resolutions) {
            seeds.push_back(DeferredPurchaseSeed{
                resolution.player.gold,
                resolution.player.food,
                append_trace_actions(resolution.parent_trace, resolution.actions),
            });
        }
        if (!seeds.empty()) {
            append_open_purchase(
                next_day,
                DeferredPurchaseModel::open(resolutions.front().player, seeds),
                SolverAction{_a_buy, buy_list(), 0},
                state.route_audit_progress
            );
        }
    }

    const auto record_sold_finish = [this](DeferredSoldModel sold) {
        std::vector<DeferredPurchaseResolution> rows = sold.resolve_finish(
            MAX_FINISHED_TO_KEEP
        );
        for (const DeferredPurchaseResolution& row : rows) {
            const uint32_t trace_id = append_trace_actions(row.parent_trace, row.actions);
            record_finished(SolverState::from_player(row.player, trace_id));
        }
    };

    if (auto trader = working->after_hire(n_Trader); trader.has_value()) {
        if (trader->representative().day == trader->representative().max_day) {
            record_sold_finish(std::move(*trader));
        } else {
            append_sold_transition(
                next_day,
                std::move(*trader),
                SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Trader)},
                state.route_audit_progress
            );
        }
    }
    if (auto camel = working->after_hire(n_Camel); camel.has_value()) {
        if (camel->representative().day == camel->representative().max_day) {
            record_sold_finish(std::move(*camel));
        } else {
            append_sold_transition(
                next_day,
                std::move(*camel),
                SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Camel)},
                state.route_audit_progress
            );
        }
    }
    if (auto interest = working->after_interest(); interest.has_value()) {
        const Player& interested = interest->representative();
        if (interested.day == interested.max_day &&
            !interested.quick_interest && !interested.day_start_skip) {
            record_sold_finish(std::move(*interest));
        } else if (active_day(SolverState::from_player(interested)) == active_day_value) {
            append_sold_transition(
                same_day,
                std::move(*interest),
                SolverAction{_a_interest, buy_list(), 0},
                state.route_audit_progress
            );
            ++profiling.current_day_requeues;
        } else {
            append_sold_transition(
                next_day,
                std::move(*interest),
                SolverAction{_a_interest, buy_list(), 0},
                state.route_audit_progress
            );
        }
    }
    enforce_memory_budget(same_day, next_day);
}

void GameSolver::expand_state(const SolverState& state, const int active_day_value, FrontierAccumulator& same_day, FrontierAccumulator& next_day) {
    if (state.deferred) {
        expand_deferred_state(state, active_day_value, same_day, next_day);
        return;
    }
    const ScopedNanosecondTimer expansion_timer(profiling.concrete_expansion_nanoseconds);
    ++profiling.processed_states;
    if (stop_requested) {
        return;
    }
    Player current = state.to_player();
    if (!current.day_start_skip) {
        Game::start_of_day(current);
        if (current.food < 0) {
            return;
        }

        const EncounterOpportunity merchant_timing = EncounterTiming::merchant(current);
        bool generated_merchant_branch = false;
        if (merch_on && !current.merchant_happened &&
            merchant_timing.due) {
            for (const auto option : ALL_MERCH_OPTIONS) {
                if (!merchant_option_allowed(option)) {
                    continue;
                }
                if (!Game::check__afford_merchant(current, option)) {
                    continue;
                }
                Player merch = current;
                if (Game::event_merchant(merch, option)) {
                    SolverState next_state = SolverState::from_player(merch);
                    append_state(
                        same_day,
                        std::move(next_state),
                        SolverAction{_a_merchant, buy_list(), static_cast<uint8_t>(option)},
                        state.trace_id,
                        state.route_audit_progress
                    );
                    if (!enforce_memory_budget(same_day, next_day)) {
                        return;
                    }
                    ++profiling.current_day_requeues;
                    generated_merchant_branch = true;
                }
            }
            if (current.merchant_day > 0 && merchant_option_allowed(m_No_Merchant)) {
                Player declined = current;
                Game::event_merchant(declined, m_No_Merchant);
                append_state(
                    same_day,
                    SolverState::from_player(declined),
                    SolverAction{_a_merchant, buy_list(), static_cast<uint8_t>(m_No_Merchant)},
                    state.trace_id,
                    state.route_audit_progress
                );
            if (!enforce_memory_budget(same_day, next_day)) {
                    return;
                }
                ++profiling.current_day_requeues;
                generated_merchant_branch = true;
            }
            if (generated_merchant_branch) {
                if (!merchant_timing.may_defer) {
                    return;
                }
                ++profiling.merchant_deferral_continuations;
            }
        }

        const EncounterOpportunity witch_timing = EncounterTiming::witch(current);
        bool generated_witch_branch = false;
        if (witch_on && !current.witch_happened && witch_timing.due) {
            for (const auto option : ALL_WITCH_OPTIONS) {
                if (!witch_option_allowed(option)) {
                    continue;
                }
                Player witch = current;
                Game::event_witch(witch, option);
                SolverState next_state = SolverState::from_player(witch);
                append_state(
                    same_day,
                    std::move(next_state),
                    SolverAction{_a_witch, buy_list(), static_cast<uint8_t>(option)},
                    state.trace_id,
                    state.route_audit_progress
                );
            if (!enforce_memory_budget(same_day, next_day)) {
                    return;
                }
                ++profiling.current_day_requeues;
                generated_witch_branch = true;
            }
            if (generated_witch_branch) {
                if (!witch_timing.may_defer) {
                    return;
                }
                ++profiling.witch_deferral_continuations;
            }
        }

        if (!current.disaster_happened &&
            current.scheduled_disaster != d_No_Disaster &&
            current.day == current.disaster_day) {
            const disaster_options option = current.scheduled_disaster;
            Game::event_disaster(current, option);
            SolverState next_state = SolverState::from_player(current);
            append_state(
                same_day,
                std::move(next_state),
                SolverAction{_a_disaster, buy_list(), static_cast<uint8_t>(option)},
                state.trace_id,
                state.route_audit_progress
            );
            if (!enforce_memory_budget(same_day, next_day)) {
                return;
            }
            ++profiling.current_day_requeues;
            return;
        }
    } else {
        current.day_start_skip = false;
    }

    const int current_food_cost = current.food_consumption;

    if (current.day < current.max_day && !current.bought_last && Game::check_can_buy_items(current)) {
        const bool use_deferred_purchase =
            !current.traveler_mode && current.scheduled_disaster == d_No_Disaster;
        if (use_deferred_purchase) {
            append_open_purchase(
                next_day,
                DeferredPurchaseModel::open(
                    current,
                    {DeferredPurchaseSeed{current.gold, current.food, state.trace_id}}
                ),
                SolverAction{_a_buy, buy_list(), 0},
                state.route_audit_progress
            );
        } else {
            const size_t projected_buy_states = estimated_buy_branch_states(current);
            if (!enforce_memory_budget(same_day, next_day, projected_buy_states)) {
                return;
            }
            for (auto& buy_state : generate_buy_successors(current)) {
                const buy_list concrete_buy(
                    static_cast<uint8_t>(buy_state.food - current.food),
                    static_cast<uint8_t>(buy_state.spice - current.spice),
                    static_cast<uint8_t>(buy_state.pottery - current.pottery),
                    static_cast<uint8_t>(buy_state.marble - current.marble),
                    static_cast<uint8_t>(buy_state.silk - current.silk),
                    static_cast<uint8_t>(buy_state.jewelry - current.jewelry)
                );
                append_state(
                    next_day,
                    std::move(buy_state),
                    SolverAction{_a_buy, concrete_buy, 0},
                    state.trace_id,
                    state.route_audit_progress
                );
            if (!enforce_memory_budget(same_day, next_day)) {
                    return;
                }
            }
        }
    }

    if (current.day == current.max_day || current.food >= current_food_cost) {
        if (Game::check_has_items(current)) {
            Player sell = current;
            Game::event_sell_items(sell);
            SolverState sell_state = SolverState::from_player(sell);
            if (sell_state.day == sell_state.max_day) {
                sell_state.trace_id = append_trace(
                    state.trace_id,
                    SolverAction{_a_sell, buy_list(), 0}
                );
                record_finished(sell_state);
            if (!enforce_memory_budget(same_day, next_day)) {
                    return;
                }
            } else {
                append_state(
                    next_day,
                    std::move(sell_state),
                    SolverAction{_a_sell, buy_list(), 0},
                    state.trace_id,
                    state.route_audit_progress
                );
            if (!enforce_memory_budget(same_day, next_day)) {
                    return;
                }
            }
        }

        if (current.gold >= current.npc_shop[n_Trader][nd_Cost] &&
            current.day < current.max_day &&
            current.food >= current_food_cost + current.npc_shop[n_Trader][nd_Eat]) {
            Player trader = current;
            Game::event_buy_npc(trader, n_Trader);
            append_state(
                next_day,
                SolverState::from_player(trader),
                SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Trader)},
                state.trace_id,
                state.route_audit_progress
            );
            if (!enforce_memory_budget(same_day, next_day)) {
                return;
            }
        }

        if (current.gold >= current.npc_shop[n_Camel][nd_Cost] &&
            current.day < current.max_day &&
            current.food >= current_food_cost + current.npc_shop[n_Camel][nd_Eat]) {
            Player camel = current;
            Game::event_buy_npc(camel, n_Camel);
            append_state(
                next_day,
                SolverState::from_player(camel),
                SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Camel)},
                state.trace_id,
                state.route_audit_progress
            );
            if (!enforce_memory_budget(same_day, next_day)) {
                return;
            }
        }

        Player interest = current;
        Game::event_interest(interest);
        SolverState interest_state = SolverState::from_player(interest);
        if (interest_state.day == interest_state.max_day &&
            !interest_state.has_flag(SSF_QUICK_INTEREST) &&
            !interest_state.has_flag(SSF_DAY_START_SKIP)) {
            interest_state.trace_id = append_trace(
                state.trace_id,
                SolverAction{_a_interest, buy_list(), 0}
            );
            record_finished(interest_state);
            if (!enforce_memory_budget(same_day, next_day)) {
                return;
            }
        } else if (active_day(interest_state) == active_day_value) {
            append_state(
                same_day,
                std::move(interest_state),
                SolverAction{_a_interest, buy_list(), 0},
                state.trace_id,
                state.route_audit_progress
            );
            if (!enforce_memory_budget(same_day, next_day)) {
                return;
            }
            ++profiling.current_day_requeues;
        } else {
            append_state(
                next_day,
                std::move(interest_state),
                SolverAction{_a_interest, buy_list(), 0},
                state.trace_id,
                state.route_audit_progress
            );
            if (!enforce_memory_budget(same_day, next_day)) {
                return;
            }
        }
    }
}

bool GameSolver::should_start_hybrid_search(const std::vector<SolverState>& frontier) const {
    if (frontier.empty()) {
        return false;
    }
    if (hybrid_batch_size > 0 && frontier.size() <= hybrid_batch_size) {
        return false;
    }
    if (hybrid_split_day > 0 && hybrid_batch_size > 0) {
        return active_day(frontier.front()) >= hybrid_split_day;
    }
    if (hybrid_batch_size == 0 && skeleton_queue_day > 0 && frontier.size() > 1 &&
        active_day(frontier.front()) >= skeleton_queue_day) {
        return true;
    }
    if (memory_limit_bytes == 0 || array_limit <= 0) {
        return false;
    }
    if (hybrid_batch_size == 0) {
        const int final_encounter_day = frontier.front().has_flag(SSF_TRAVELER_MODE) ? 13 : 11;
        if (active_day(frontier.front()) <= final_encounter_day) {
            return false;
        }
    }
    if (frontier.size() * 2ULL < static_cast<size_t>(array_limit)) {
        return false;
    }

    const size_t candidate_limit = static_cast<size_t>(array_limit) * 2ULL;
    const uint64_t projected = estimated_memory_bytes(
        &frontier,
        nullptr,
        candidate_limit * 2ULL
    );
    const uint64_t working_limit = memory_limit_bytes - (memory_limit_bytes / 10ULL);
    const uint64_t actual = estimated_memory_bytes(&frontier, nullptr);
    const uint64_t resident = resident_memory_bytes();
    return projected >= working_limit * 3ULL / 4ULL ||
           actual >= working_limit * 3ULL / 5ULL ||
           resident >= working_limit * 3ULL / 5ULL;
}

bool GameSolver::run_layered_frontier(
    const bool main_thread,
    std::vector<SolverState>* waiting_frontier,
    const bool allow_hybrid_split,
    int& last_active_day
) {
    const std::vector<SolverState> no_same_day_candidates;
    while (!current_layer.empty() && !stop_requested) {
        continuation_score_cache.clear();
        if (allow_hybrid_split && should_start_hybrid_search(current_layer)) {
            return true;
        }
        const int current_active_day = active_day(current_layer.front());
        last_active_day = current_active_day;
        std::vector<SolverState> pending = std::move(current_layer);
        current_layer.clear();
        traces.shrink_to_fit();

        FrontierAccumulator next_day_candidates(
            survival_audit ? &*survival_audit : nullptr
        );
        while (!pending.empty() && !stop_requested) {
            pending_states_for_budget = &pending;
            waiting_states_for_budget = waiting_frontier;
            FrontierAccumulator same_day_candidates(
                survival_audit ? &*survival_audit : nullptr
            );
            if (!enforce_memory_budget(same_day_candidates, next_day_candidates)) {
                break;
            }
            std::sort(pending.begin(), pending.end(), [](const SolverState& lhs, const SolverState& rhs) {
                if (lhs.player_world.town != rhs.player_world.town) {
                    return lhs.player_world.town < rhs.player_world.town;
                }
                if (lhs.player_world.merch != rhs.player_world.merch) {
                    return lhs.player_world.merch < rhs.player_world.merch;
                }
                if (lhs.player_world.witch != rhs.player_world.witch) {
                    return lhs.player_world.witch < rhs.player_world.witch;
                }
                if (lhs.player_world.disaster != rhs.player_world.disaster) {
                    return lhs.player_world.disaster < rhs.player_world.disaster;
                }
                return lhs.trace_id < rhs.trace_id;
            });
            for (const auto& state : pending) {
                expand_state(state, current_active_day, same_day_candidates, next_day_candidates);
                if (stop_requested) {
                    break;
                }
            }
            if (stop_requested) {
                break;
            }
            std::vector<SolverState> exact_same_day = finish_generated_frontier(
                same_day_candidates
            );
            pending = compress_layer(
                std::move(exact_same_day),
                current_active_day,
                nullptr,
                nullptr,
                true,
                false
            );
            if (main_thread && (profiling.processed_states % static_cast<uint64_t>(std::max(print_counter, 1)) == 0 ||
                std::chrono::steady_clock::now() - last_status_at >= HEARTBEAT_INTERVAL)) {
                current_layer = pending;
                print_status(current_active_day, pending.size(), next_day_candidates.size());
                current_layer.clear();
                last_status_at = std::chrono::steady_clock::now();
            }
        }

        if (stop_requested || next_day_candidates.empty()) {
            break;
        }
        std::vector<SolverState> exact_next_day = finish_generated_frontier(
            next_day_candidates
        );
        const int next_active_day_value = active_day(exact_next_day.front());
        current_layer = compress_layer(
            std::move(exact_next_day),
            next_active_day_value,
            waiting_frontier,
            nullptr,
            true
        );
        pending_states_for_budget = nullptr;
        waiting_states_for_budget = waiting_frontier;
        if (!enforce_memory_budget(no_same_day_candidates, no_same_day_candidates)) {
            break;
        }
    }
    pending_states_for_budget = nullptr;
    waiting_states_for_budget = nullptr;
    return false;
}

bool GameSolver::enforce_memory_budget(
    const FrontierAccumulator& same_day_candidates,
    const FrontierAccumulator& next_day_candidates,
    const size_t projected_concrete_states
) {
    ++profiling.memory_budget_checks;
    if (time_limit_seconds > 0 &&
        std::chrono::steady_clock::now() - started_at >= std::chrono::seconds(time_limit_seconds)) {
        request_stop("internal time budget reached after " + std::to_string(time_limit_seconds) + " seconds");
        return false;
    }
    if (memory_limit_bytes == 0) {
        return true;
    }
    const uint64_t working_limit = memory_limit_bytes - (memory_limit_bytes / 10ULL);
    ++memory_checks_since_exact_scan;
    ++memory_checks_since_resident_sample;
    MemoryCheckCadence cadence = memory_check_cadence(
        std::max(last_memory_estimated_bytes, last_resident_bytes),
        working_limit
    );
    bool resident_sampled = false;
    if (last_resident_bytes == 0 ||
        memory_checks_since_resident_sample >= cadence.resident_sample_interval) {
        last_resident_bytes = resident_memory_bytes();
        memory_checks_since_resident_sample = 0;
        resident_sampled = true;
        ++profiling.memory_budget_resident_samples;
        profiling.peak_resident_bytes = std::max(
            profiling.peak_resident_bytes,
            last_resident_bytes
        );
    }
    const uint64_t cached_observed = std::max(
        last_memory_estimated_bytes,
        last_resident_bytes
    );
    cadence = memory_check_cadence(cached_observed, working_limit);
    const bool exact_scan_required =
        last_memory_estimated_bytes == 0 ||
        memory_checks_since_exact_scan >= cadence.exact_scan_interval ||
        projected_concrete_states > 0;
    if (!exact_scan_required) {
        return true;
    }

    const auto scan_started = std::chrono::steady_clock::now();
    const uint64_t estimated = estimated_memory_bytes(
        nullptr,
        nullptr,
        projected_concrete_states
    ) + same_day_candidates.memory_bytes() + next_day_candidates.memory_bytes();
    last_memory_estimated_bytes = estimated;
    memory_checks_since_exact_scan = 0;
    ++profiling.memory_budget_exact_scans;
    profiling.peak_estimated_bytes = std::max(profiling.peak_estimated_bytes, estimated);
    if (!resident_sampled) {
        last_resident_bytes = resident_memory_bytes();
        memory_checks_since_resident_sample = 0;
        ++profiling.memory_budget_resident_samples;
    }
    const uint64_t resident = last_resident_bytes;
    profiling.peak_resident_bytes = std::max(profiling.peak_resident_bytes, resident);
    const uint64_t observed = std::max(estimated, resident);
    profiling.memory_budget_scan_nanoseconds += static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - scan_started
        ).count()
    );
    if (observed <= working_limit) {
        return true;
    }

    std::ostringstream out;
    out << "memory budget reached"
        << " | est_mem_mb=" << (estimated / (1024ULL * 1024ULL))
        << " | rss_mb=" << (resident / (1024ULL * 1024ULL))
        << " | mem_limit_mb=" << (memory_limit_bytes / (1024ULL * 1024ULL))
        << " | pending=" << (pending_states_for_budget == nullptr ? 0 : pending_states_for_budget->size())
        << " | traces=" << traces.size()
        << " | same_day=" << same_day_candidates.size()
        << " | next_day=" << next_day_candidates.size();
    request_stop(out.str());
    return false;
}

void GameSolver::drain_hybrid_frontier(const bool main_thread, int& last_active_day) {
    std::vector<SolverState> waiting_frontier = std::move(current_layer);
    current_layer.clear();
    const int split_day = active_day(waiting_frontier.front());
    profiling.hybrid_frontier_states = waiting_frontier.size();
    profiling.hybrid_peak_waiting_states = waiting_frontier.size();
    profiling.hybrid_split_day = static_cast<uint64_t>(split_day);

    const size_t candidate_limit = static_cast<size_t>(array_limit) * 2ULL;
    profiling.hybrid_trigger_estimated_bytes = estimated_memory_bytes(
        &waiting_frontier,
        nullptr,
        candidate_limit * 2ULL
    );

    const bool explicit_batch_mode = hybrid_batch_size > 0;
    const bool skeleton_queue_mode = !explicit_batch_mode && skeleton_queue_day >= 0;
    const bool automatic_pressure_mode = !explicit_batch_mode && !skeleton_queue_mode;
    const HybridSkeletonOrder active_order = automatic_pressure_mode
        ? HybridSkeletonOrder::StructureInterleave
        : hybrid_skeleton_order;
    size_t active_batch_size = hybrid_batch_size;
    if (skeleton_queue_mode) {
        active_batch_size = static_cast<size_t>(array_limit);
    } else if (automatic_pressure_mode) {
        active_batch_size = std::max<size_t>(
            64,
            std::min<size_t>(5000, waiting_frontier.size() / 8U)
        );
    }
    profiling.hybrid_effective_batch_size = skeleton_queue_mode ? 0 : active_batch_size;

    const auto hybrid_group_key = [this, active_order](const SolverState& state) {
        return active_order == HybridSkeletonOrder::StructureInterleave
            ? continuation_structure_key(state)
            : skeleton_key(state);
    };

    if (skeleton_queue_mode) {
        std::unordered_set<uint64_t> split_groups;
        split_groups.reserve(waiting_frontier.size());
        for (const SolverState& state : waiting_frontier) {
            split_groups.insert(skeleton_key(state));
        }
        profiling.hybrid_split_skeleton_groups = split_groups.size();
    } else if (active_order != HybridSkeletonOrder::Score) {
        std::unordered_set<uint64_t> split_groups;
        split_groups.reserve(waiting_frontier.size());
        for (const SolverState& state : waiting_frontier) {
            split_groups.insert(hybrid_group_key(state));
        }
        profiling.hybrid_split_skeleton_groups = split_groups.size();
    }

    const auto score_better = [this](const SolverState& lhs, const SolverState& rhs) {
        const long double lhs_score = beam_score(lhs);
        const long double rhs_score = beam_score(rhs);
        if (lhs_score != rhs_score) {
            return lhs_score > rhs_score;
        }
        if (lhs.gold != rhs.gold) {
            return lhs.gold > rhs.gold;
        }
        return lhs.trace_id < rhs.trace_id;
    };
    const auto score_worse = [&score_better](const SolverState& lhs, const SolverState& rhs) {
        return score_better(rhs, lhs);
    };

    std::vector<size_t> group_remaining;
    std::vector<long double> group_diagnostic_ceilings;
    if (skeleton_queue_mode) {
        std::vector<SkeletonQueueCandidate> queue_candidates;
        queue_candidates.reserve(waiting_frontier.size());
        continuation_score_cache.reserve(
            continuation_score_cache.size() + waiting_frontier.size()
        );
        for (size_t index = 0; index < waiting_frontier.size(); ++index) {
            const SolverState& state = waiting_frontier[index];
            queue_candidates.push_back(SkeletonQueueCandidate{
                skeleton_key(state),
                continuation_matrix_row(state),
                index,
            });
        }
        const auto schedule_started = std::chrono::steady_clock::now();
        SkeletonQueueSchedule schedule = SkeletonWorkQueue::schedule(
            queue_candidates,
            continuation_lane_share_percent,
            optimistic_share_percent
        );
        profiling.skeleton_queue_schedule_nanoseconds += static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - schedule_started
            ).count()
        );
        std::vector<SolverState> source = std::move(waiting_frontier);
        waiting_frontier.clear();
        waiting_frontier.reserve(source.size());
        group_remaining.reserve(schedule.groups.size());
        group_diagnostic_ceilings.reserve(schedule.groups.size());
        // The drain pops from the back, so store weakest groups first and each
        // group's weakest member first. No state is shared across groups.
        for (auto group = schedule.groups.rbegin(); group != schedule.groups.rend(); ++group) {
            group_remaining.push_back(group->source_indices.size());
            group_diagnostic_ceilings.push_back(
                group->continuation.best_optimistic()
            );
            for (auto member = group->source_indices.rbegin();
                 member != group->source_indices.rend(); ++member) {
                waiting_frontier.emplace_back(std::move(source[*member]));
            }
        }
        profiling.skeleton_queue_groups = schedule.groups.size();
        profiling.skeleton_queue_states = waiting_frontier.size();
        profiling.skeleton_queue_split_day = static_cast<uint64_t>(split_day);
        profiling.skeleton_queue_matrix_rows += queue_candidates.size();
        profiling.skeleton_queue_duplicate_ticket_skips +=
            schedule.duplicate_ticket_skips;
        for (size_t ticket = 0; ticket < SKELETON_QUEUE_TICKET_COUNT; ++ticket) {
            profiling.skeleton_queue_head_ticket_counts[ticket] +=
                schedule.head_ticket_counts[ticket];
        }
    } else if (automatic_pressure_mode) {
        std::unordered_map<uint64_t, std::vector<SolverState>> grouped;
        grouped.reserve(profiling.hybrid_split_skeleton_groups);
        for (SolverState& state : waiting_frontier) {
            grouped[continuation_structure_key(state)].emplace_back(std::move(state));
        }
        std::vector<std::vector<SolverState>> groups;
        groups.reserve(grouped.size());
        for (auto& [key, states] : grouped) {
            (void)key;
            std::sort(states.begin(), states.end(), score_better);
            groups.emplace_back(std::move(states));
        }
        std::sort(groups.begin(), groups.end(), [&score_better](const auto& lhs, const auto& rhs) {
            return score_better(lhs.front(), rhs.front());
        });

        waiting_frontier.clear();
        waiting_frontier.reserve(profiling.hybrid_frontier_states);
        group_remaining.reserve(groups.size());
        // The drain pops from the back. Store the weakest structure first and
        // each structure's weakest state first, so one best-first structure is
        // completed before memory is spent on the next one.
        for (auto group = groups.rbegin(); group != groups.rend(); ++group) {
            group_remaining.push_back(group->size());
            for (auto state = group->rbegin(); state != group->rend(); ++state) {
                waiting_frontier.emplace_back(std::move(*state));
            }
        }
    } else if (active_order == HybridSkeletonOrder::Interleave ||
        active_order == HybridSkeletonOrder::StructureInterleave) {
        std::unordered_map<uint64_t, std::vector<SolverState>> grouped;
        grouped.reserve(profiling.hybrid_split_skeleton_groups);
        for (SolverState& state : waiting_frontier) {
            grouped[hybrid_group_key(state)].emplace_back(std::move(state));
        }
        std::vector<std::vector<SolverState>> groups;
        groups.reserve(grouped.size());
        for (auto& [key, states] : grouped) {
            (void)key;
            std::sort(states.begin(), states.end(), score_better);
            groups.emplace_back(std::move(states));
        }
        std::sort(groups.begin(), groups.end(), [&score_better](const auto& lhs, const auto& rhs) {
            return score_better(lhs.front(), rhs.front());
        });
        std::vector<SolverState> interleaved;
        interleaved.reserve(waiting_frontier.size());
        for (size_t rank = 0; interleaved.size() < waiting_frontier.size(); ++rank) {
            for (auto& group : groups) {
                if (rank < group.size()) {
                    interleaved.emplace_back(std::move(group[rank]));
                }
            }
        }
        std::reverse(interleaved.begin(), interleaved.end());
        waiting_frontier = std::move(interleaved);
    } else {
        std::sort(waiting_frontier.begin(), waiting_frontier.end(), [this, &score_worse, active_order](const SolverState& lhs, const SolverState& rhs) {
            if (active_order == HybridSkeletonOrder::Cohort) {
                const uint64_t lhs_skeleton = skeleton_key(lhs);
                const uint64_t rhs_skeleton = skeleton_key(rhs);
                if (lhs_skeleton != rhs_skeleton) {
                    return lhs_skeleton < rhs_skeleton;
                }
            }
            return score_worse(lhs, rhs);
        });
    }

    const int broad_array_limit = array_limit;
    if (!skeleton_queue_mode) {
        const size_t requested_suffix_limit = hybrid_suffix_limit == 0
            ? active_batch_size
            : hybrid_suffix_limit;
        array_limit = static_cast<int>(std::min<size_t>(
            static_cast<size_t>(broad_array_limit),
            requested_suffix_limit
        ));
    }
    try {
        while (!waiting_frontier.empty() && !stop_requested) {
            size_t take = std::min(active_batch_size, waiting_frontier.size());
            long double diagnostic_ceiling = 0.0L;
            if (skeleton_queue_mode) {
                take = group_remaining.back();
                diagnostic_ceiling = group_diagnostic_ceilings.back();
            } else if (automatic_pressure_mode) {
                take = std::min(take, group_remaining.back());
            }
            std::vector<SolverState> batch;
            batch.reserve(take);
            for (size_t index = 0; index < take; ++index) {
                batch.emplace_back(std::move(waiting_frontier.back()));
                waiting_frontier.pop_back();
            }
            if (skeleton_queue_mode || automatic_pressure_mode) {
                group_remaining.back() -= take;
                if (group_remaining.back() == 0) {
                    group_remaining.pop_back();
                    if (skeleton_queue_mode) {
                        group_diagnostic_ceilings.pop_back();
                    }
                }
            }
            ++profiling.hybrid_batches;
            profiling.hybrid_peak_waiting_states = std::max<uint64_t>(
                profiling.hybrid_peak_waiting_states,
                waiting_frontier.size()
            );
            current_layer = compress_layer(
                std::move(batch),
                split_day,
                &waiting_frontier
            );
            if (skeleton_queue_mode) {
                if (!finished.empty() &&
                    diagnostic_ceiling <= static_cast<long double>(best_points())) {
                    ++profiling.skeleton_queue_ceiling_below_current_best;
                }
                measuring_skeleton_queue_family = true;
                active_skeleton_queue_ceiling = diagnostic_ceiling;
                active_skeleton_queue_best_points = std::numeric_limits<int>::min();
            }
            (void)run_layered_frontier(main_thread, &waiting_frontier, false, last_active_day);
            if (skeleton_queue_mode) {
                if (active_skeleton_queue_best_points != std::numeric_limits<int>::min()) {
                    ++profiling.skeleton_queue_ceiling_checked_groups;
                    if (static_cast<long double>(active_skeleton_queue_best_points) >
                        active_skeleton_queue_ceiling) {
                        ++profiling.skeleton_queue_ceiling_violations;
                        profiling.skeleton_queue_ceiling_max_overrun = std::max<uint64_t>(
                            profiling.skeleton_queue_ceiling_max_overrun,
                            static_cast<uint64_t>(std::ceil(
                                static_cast<long double>(active_skeleton_queue_best_points) -
                                active_skeleton_queue_ceiling
                            ))
                        );
                    }
                }
                measuring_skeleton_queue_family = false;
                if (!stop_requested) {
                    ++profiling.skeleton_queue_completed_groups;
                }
            }
        }
    } catch (...) {
        measuring_skeleton_queue_family = false;
        array_limit = broad_array_limit;
        throw;
    }
    array_limit = broad_array_limit;
}

void GameSolver::find_solve_depth(const bool main_thread) {
    current_layer = compress_layer(std::move(current_layer), active_day(start_state));
    int last_active_day = current_layer.empty() ? active_day(start_state) : active_day(current_layer.front());

    const bool hybrid_requested = run_layered_frontier(main_thread, nullptr, true, last_active_day);
    if (hybrid_requested && !stop_requested) {
        drain_hybrid_frontier(main_thread, last_active_day);
    }

    if (main_thread) {
        print_status(current_layer.empty() ? last_active_day : active_day(current_layer.front()), 0, current_layer.size());
        if (stop_requested) {
            std::cout << "[!] Search stopped: " << stop_reason << std::endl;
        }
        print_solves();
    }
}

void GameSolver::find_solve_proof_depth(
    const size_t transposition_limit,
    const bool main_thread
) {
    if (transposition_limit == 0) {
        throw std::invalid_argument("Proof transposition limit must be greater than zero");
    }
    proof_search_active = true;
    struct ProofModeGuard {
        bool& active;
        ~ProofModeGuard() {
            active = false;
        }
    } proof_mode_guard{proof_search_active};

    std::vector<SolverState> stack = std::move(current_layer);
    current_layer.clear();
    std::unordered_set<SolverState, SolverStateHash> transpositions;
    std::deque<SolverState> transposition_order;
    transpositions.reserve(transposition_limit);
    const std::vector<SolverState> no_candidates;

    while (!stack.empty() && !stop_requested) {
        profiling.proof_peak_stack_states = std::max<uint64_t>(
            profiling.proof_peak_stack_states,
            stack.size()
        );
        SolverState state = std::move(stack.back());
        stack.pop_back();
        ++profiling.proof_states_popped;

        if (finished.size() >= MAX_FINISHED_TO_KEEP &&
            AdmissibleUpperBound::score_ceiling(state) <=
                static_cast<int64_t>(finished[MAX_FINISHED_TO_KEEP - 1].points)) {
            ++profiling.proof_bound_prunes;
            continue;
        }

        SolverState transposition_state = state;
        transposition_state.trace_id = 0;
        transposition_state.route_audit_progress = RouteSurvivalAudit::NO_MATCH;
        transposition_state.route_audit_quantity =
            RouteAuditQuantityStatus::NotApplicable;
        if (transpositions.find(transposition_state) != transpositions.end()) {
            ++profiling.proof_transposition_hits;
            continue;
        }
        if (transpositions.size() >= transposition_limit) {
            transpositions.erase(transposition_order.front());
            transposition_order.pop_front();
            ++profiling.proof_transposition_evictions;
        }
        transpositions.insert(transposition_state);
        transposition_order.emplace_back(std::move(transposition_state));
        profiling.proof_peak_transposition_states = std::max<uint64_t>(
            profiling.proof_peak_transposition_states,
            transpositions.size()
        );

        pending_states_for_budget = &stack;
        FrontierAccumulator same_day(
            survival_audit ? &*survival_audit : nullptr
        );
        FrontierAccumulator next_day(
            survival_audit ? &*survival_audit : nullptr
        );
        expand_state(state, active_day(state), same_day, next_day);
        if (stop_requested) {
            break;
        }

        std::vector<SolverState> children = finish_generated_frontier(same_day);
        std::vector<SolverState> later = finish_generated_frontier(next_day);
        children.reserve(children.size() + later.size());
        for (SolverState& child : later) {
            children.emplace_back(std::move(child));
        }
        std::sort(children.begin(), children.end(), [this](
            const SolverState& lhs,
            const SolverState& rhs
        ) {
            const long double lhs_score = beam_score(lhs);
            const long double rhs_score = beam_score(rhs);
            if (lhs_score != rhs_score) {
                return lhs_score < rhs_score;
            }
            return lhs.trace_id > rhs.trace_id;
        });
        for (SolverState& child : children) {
            stack.emplace_back(std::move(child));
        }

        if (!enforce_memory_budget(no_candidates, no_candidates)) {
            break;
        }
        if (main_thread &&
            std::chrono::steady_clock::now() - last_status_at >= HEARTBEAT_INTERVAL) {
            print_status(
                stack.empty() ? active_day(state) : active_day(stack.back()),
                stack.size(),
                0
            );
            last_status_at = std::chrono::steady_clock::now();
        }
    }

    pending_states_for_budget = nullptr;
    profiling.proof_search_completed = stack.empty() && !stop_requested;
    profiling.proof_top_three_certified =
        profiling.proof_search_completed && finished.size() >= MAX_FINISHED_TO_KEEP;
    if (main_thread) {
        print_status(
            stack.empty() ? active_day(start_state) : active_day(stack.back()),
            stack.size(),
            0
        );
        if (stop_requested) {
            std::cout << "[!] Proof search stopped: " << stop_reason << std::endl;
        } else if (profiling.proof_top_three_certified) {
            std::cout << "[*] Exact top three certified\n";
        } else {
            std::cout << "[*] Exact state space exhausted; fewer than three finishes exist\n";
        }
        print_solves();
    }
}

void GameSolver::find_solve_mapped(
    const std::filesystem::path& database_path,
    const size_t node_budget,
    const PersistentSearchOrder order,
    const bool main_thread
) {
    // A scenario key contains the exact root state plus every option that can
    // alter successor generation. Scheduling settings are intentionally not
    // included because they change traversal order, not the stored graph.
    mapped_search_order = order;
    std::vector<uint8_t> scenario_key = SolverStateCodec::semantic_key(start_state);
    const std::string configuration =
        std::string("mapped-rules-v1") +
        ";merch=" + (merch_on ? "1" : "0") +
        ";witch=" + (witch_on ? "1" : "0") +
        ";profile=" + std::to_string(static_cast<int>(purchase_profile)) +
        ";merchant-option=" + std::to_string(
            forced_merchant_option.has_value() ? static_cast<int>(*forced_merchant_option) : -1
        ) +
        ";witch-option=" + std::to_string(
            forced_witch_option.has_value() ? static_cast<int>(*forced_witch_option) : -1
        );
    scenario_key.insert(scenario_key.end(), configuration.begin(), configuration.end());
    const auto town = static_cast<town_options>(start_state.player_world.town);
    const auto town_name = to_conversion.find(town);
    const std::string label = town_name == to_conversion.end()
        ? "Town " + std::to_string(static_cast<int>(start_state.player_world.town))
        : town_name->second;

    PersistentSearchGraph graph(database_path, scenario_key, label, start_state);

    // Load previously found routes immediately. They are lower bounds even
    // while the root remains partial.
    for (FinishedSolve& route : graph.best_routes(MAX_FINISHED_TO_KEEP)) {
        finished.push_back(std::move(route));
    }
    sort_and_limit_finished();

    proof_search_active = true;
    struct MappedModeGuard {
        bool& exact_mode;
        std::vector<FinishedSolve>*& capture;
        ~MappedModeGuard() {
            exact_mode = false;
            capture = nullptr;
        }
    } mode_guard{proof_search_active, active_expansion_finishes};

    size_t expanded = 0;
    const auto session_can_continue = [this, node_budget, &expanded]() {
        if (stop_requested || (node_budget > 0 && expanded >= node_budget)) {
            return false;
        }
        if (time_limit_seconds > 0 &&
            std::chrono::steady_clock::now() - started_at >=
                std::chrono::seconds(time_limit_seconds)) {
            request_stop("persistent search session time budget reached");
            return false;
        }
        return true;
    };

    const auto expand_mapped_node = [this, &graph, &expanded](
        const int64_t node_id
    ) {
        std::optional<PersistentSearchNode> node = graph.load_open(node_id);
        if (!node.has_value()) {
            return true;
        }
        graph.localize_traces(*node, traces);

        // A disk-backed node is an exact parked subtree. Once three complete
        // routes establish a lower bound, commit an uncompetitive subtree as
        // solved without materializing its buy envelope in process memory.
        if (finished.size() >= MAX_FINISHED_TO_KEEP &&
            AdmissibleUpperBound::score_ceiling(node->state) <=
                static_cast<int64_t>(finished[MAX_FINISHED_TO_KEEP - 1].points)) {
            graph.commit_expansion(node->id, {}, {}, {}, traces);
            ++profiling.proof_bound_prunes;
            ++expanded;
            return true;
        }

        const size_t trace_count_before = traces.size();
        const std::vector<FinishedSolve> finished_before = finished;
        std::vector<FinishedSolve> terminal_routes;
        active_expansion_finishes = &terminal_routes;
        FrontierAccumulator same_day(
            survival_audit ? &*survival_audit : nullptr
        );
        FrontierAccumulator next_day(
            survival_audit ? &*survival_audit : nullptr
        );
        expand_state(node->state, active_day(node->state), same_day, next_day);
        active_expansion_finishes = nullptr;
        if (stop_requested) {
            // Expansion is atomic. Never mark a node expanded when a time or
            // memory stop interrupted successor generation.
            traces.resize(trace_count_before);
            finished = finished_before;
            return false;
        }

        std::vector<SolverState> children = finish_generated_frontier(same_day);
        std::vector<SolverState> later = finish_generated_frontier(next_day);
        children.reserve(children.size() + later.size());
        for (SolverState& child : later) children.emplace_back(std::move(child));
        std::vector<double> priorities;
        priorities.reserve(children.size());
        for (const SolverState& child : children) {
            priorities.push_back(static_cast<double>(beam_score(child)));
        }
        graph.commit_expansion(
            node->id, children, priorities, terminal_routes, traces
        );
        // Keep only score witnesses in memory. Their traces were just
        // globalized by commit_expansion and are reloaded from the graph at
        // the end of the session, but their scores immediately tighten the
        // exact bound for every parked subtree.
        for (const FinishedSolve& route : terminal_routes) {
            finished.push_back(FinishedSolve{
                route.points, {}, 0, FinishedSolve::Origin::Discovered
            });
        }
        sort_and_limit_finished();
        ++expanded;
        return true;
    };

    const auto report_progress = [this, &graph, main_thread]() {
        if (!main_thread ||
            std::chrono::steady_clock::now() - last_status_at < HEARTBEAT_INTERVAL) {
            return;
        }
        const PersistentSearchStats current = graph.stats();
        std::cout << "[*] Mapped exact search | expanded=" << current.expanded_this_run
                  << " | nodes=" << current.nodes
                  << " | open=" << current.open_nodes
                  << " | partial=" << current.partial_nodes
                  << " | solved=" << current.solved_nodes
                  << " | reused=" << current.reused_children_this_run
                  << " | db_mb=" << (current.database_bytes / (1024ULL * 1024ULL))
                  << " | rss_mb=" << (resident_memory_bytes() / (1024ULL * 1024ULL))
                  << std::endl;
        last_status_at = std::chrono::steady_clock::now();
    };

    if (order == PersistentSearchOrder::Depth) {
        while (session_can_continue()) {
            std::optional<PersistentSearchNode> node = graph.next_open();
            if (!node.has_value()) break;
            if (!expand_mapped_node(node->id)) break;
            report_progress();
        }
    } else {
        const size_t beam_width = static_cast<size_t>(array_limit);

        const auto retain_beam = [this, &graph, beam_width](
            std::vector<PersistentSearchNodeSummary> candidates
        ) {
            std::sort(candidates.begin(), candidates.end(), [](const auto& lhs, const auto& rhs) {
                if (lhs.priority != rhs.priority) return lhs.priority > rhs.priority;
                if (lhs.active_day != rhs.active_day) return lhs.active_day > rhs.active_day;
                return lhs.id < rhs.id;
            });
            candidates.erase(
                std::unique(candidates.begin(), candidates.end(), [](const auto& lhs, const auto& rhs) {
                    return lhs.id == rhs.id;
                }),
                candidates.end()
            );
            if (candidates.size() <= beam_width) return candidates;

            profiling.mapped_beam_candidate_visits += candidates.size();
            profiling.mapped_beam_parked_visits += candidates.size() - beam_width;
            continuation_score_cache.clear();
            std::vector<ContinuationMatrixRow> rows;
            rows.reserve(candidates.size());
            const auto scoring_started = std::chrono::steady_clock::now();
            for (const auto& candidate : candidates) {
                const std::optional<SolverState> state = graph.load_state(candidate.id);
                if (!state.has_value()) {
                    throw std::runtime_error("Missing mapped beam candidate");
                }
                rows.emplace_back(continuation_matrix_row(*state));
            }
            profiling.continuation_matrix_scoring_nanoseconds += static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - scoring_started
                ).count()
            );
            const auto selection_started = std::chrono::steady_clock::now();
            const ContinuationMatrixSelection selection = ContinuationMatrixSelector::select(
                rows,
                beam_width,
                continuation_lane_share_percent,
                optimistic_share_percent
            );
            profiling.continuation_matrix_selection_nanoseconds += static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - selection_started
                ).count()
            );
            profiling.continuation_matrix_lane_owners += selection.lane_owner_total;
            profiling.continuation_matrix_duplicate_ticket_skips +=
                selection.duplicate_ticket_skips;
            for (size_t lane = 0; lane < CONTINUATION_LANE_COUNT; ++lane) {
                profiling.continuation_matrix_lane_counts[lane] +=
                    selection.lane_owner_counts[lane];
            }
            std::vector<PersistentSearchNodeSummary> selected;
            selected.reserve(selection.owner_indices.size());
            for (const size_t index : selection.owner_indices) {
                selected.push_back(candidates[index]);
            }
            candidates = std::move(selected);
            return candidates;
        };

        while (session_can_continue()) {
            ++profiling.mapped_beam_sweeps;
            const size_t expanded_before_sweep = expanded;
            std::unordered_set<int64_t> traversed;
            std::vector<PersistentSearchNodeSummary> frontier{graph.root()};

            while (!frontier.empty() && session_can_continue()) {
                const int current_day = std::min_element(
                    frontier.begin(), frontier.end(), [](const auto& lhs, const auto& rhs) {
                        return lhs.active_day < rhs.active_day;
                    }
                )->active_day;
                std::vector<PersistentSearchNodeSummary> pending;
                std::vector<PersistentSearchNodeSummary> later;
                for (const auto& candidate : frontier) {
                    (candidate.active_day == current_day ? pending : later).push_back(candidate);
                }
                pending = retain_beam(std::move(pending));

                while (!pending.empty() && session_can_continue()) {
                    std::vector<PersistentSearchNodeSummary> same_day;
                    for (const PersistentSearchNodeSummary& candidate : pending) {
                        if (!session_can_continue()) break;
                        if (!traversed.insert(candidate.id).second) continue;
                        if (candidate.status == PersistentNodeStatus::Open &&
                            !expand_mapped_node(candidate.id)) {
                            break;
                        }
                        for (const auto& child : graph.unresolved_children(candidate.id)) {
                            (child.active_day == current_day ? same_day : later).push_back(child);
                        }
                        report_progress();
                    }
                    if (stop_requested) break;
                    pending = retain_beam(std::move(same_day));
                }
                if (stop_requested || !session_can_continue()) break;
                if (later.empty()) break;

                const int next_day = std::min_element(
                    later.begin(), later.end(), [](const auto& lhs, const auto& rhs) {
                        return lhs.active_day < rhs.active_day;
                    }
                )->active_day;
                std::vector<PersistentSearchNodeSummary> next_frontier;
                for (const auto& candidate : later) {
                    if (candidate.active_day == next_day) next_frontier.push_back(candidate);
                }
                frontier = retain_beam(std::move(next_frontier));
            }
            if (expanded == expanded_before_sweep) break;
        }
    }

    // Re-read through reverse trace links, rather than keeping duplicated paths
    // in the results table.
    finished.erase(
        std::remove_if(finished.begin(), finished.end(), [](const FinishedSolve& solve) {
            return solve.origin == FinishedSolve::Origin::Discovered;
        }),
        finished.end()
    );
    for (FinishedSolve& route : graph.best_routes(MAX_FINISHED_TO_KEEP)) {
        finished.push_back(std::move(route));
    }
    sort_and_limit_finished();

    const PersistentSearchStats final = graph.stats();
    profiling.mapped_expanded_this_run = final.expanded_this_run;
    profiling.mapped_reused_children_this_run = final.reused_children_this_run;
    profiling.mapped_nodes = final.nodes;
    profiling.mapped_open_nodes = final.open_nodes;
    profiling.mapped_partial_nodes = final.partial_nodes;
    profiling.mapped_solved_nodes = final.solved_nodes;
    profiling.mapped_database_bytes = final.database_bytes;
    profiling.proof_search_completed = final.root_solved;
    profiling.proof_top_three_certified =
        final.root_solved && final.terminal_routes >= MAX_FINISHED_TO_KEEP;
    if (main_thread) {
        std::cout << "[*] Mapped exact search saved | expanded=" << final.expanded_this_run
                  << " | nodes=" << final.nodes
                  << " | open=" << final.open_nodes
                  << " | partial=" << final.partial_nodes
                  << " | solved=" << final.solved_nodes
                  << " | reused=" << final.reused_children_this_run
                  << " | root=" << (final.root_solved ? "solved" : "partial")
                  << " | db_mb=" << (final.database_bytes / (1024ULL * 1024ULL))
                  << std::endl;
        if (stop_requested) {
            std::cout << "[!] Mapped session stopped: " << stop_reason << std::endl;
        }
        print_solves();
    }
}

void GameSolver::warm_purchase_trees() {
    handler.warmSupportedTrees();
}

int GameSolver::best_points() const {
    return finished.empty() ? std::numeric_limits<int>::min() : finished.front().points;
}

int GameSolver::best_discovered_points() const {
    int best = std::numeric_limits<int>::min();
    for (const FinishedSolve& solve : finished) {
        if (solve.origin == FinishedSolve::Origin::Discovered) {
            best = std::max(best, solve.points);
        }
    }
    return best;
}

int GameSolver::incumbent_points() const {
    for (const FinishedSolve& solve : finished) {
        if (solve.origin == FinishedSolve::Origin::Incumbent) {
            return solve.points;
        }
    }
    return std::numeric_limits<int>::min();
}

const std::vector<FinishedSolve>& GameSolver::best_solves() const {
    return finished;
}

const SolverProfiling& GameSolver::profiling_stats() const {
    return profiling;
}
