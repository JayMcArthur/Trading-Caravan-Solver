//
// Created by Jay on 6/14/2023.
//

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "GameSolver.h"
#include "helpers.h"

namespace {
template <typename T>
void hash_combine(size_t& seed, const T& value) {
    seed ^= std::hash<T>{}(value) + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
}
}

void TraceNode::serialize(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&parent), sizeof(parent));
    size_t length = action.size();
    out.write(reinterpret_cast<const char*>(&length), sizeof(length));
    out.write(action.data(), static_cast<std::streamsize>(length));
}

void TraceNode::deserialize(std::istream& in) {
    in.read(reinterpret_cast<char*>(&parent), sizeof(parent));
    size_t length = 0;
    in.read(reinterpret_cast<char*>(&length), sizeof(length));
    action.resize(length);
    in.read(action.data(), static_cast<std::streamsize>(length));
}

void SolverStateKey::serialize(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&gold), sizeof(gold));
    out.write(reinterpret_cast<const char*>(&food), sizeof(food));
    out.write(reinterpret_cast<const char*>(&spice), sizeof(spice));
    out.write(reinterpret_cast<const char*>(&pottery), sizeof(pottery));
    out.write(reinterpret_cast<const char*>(&marble), sizeof(marble));
    out.write(reinterpret_cast<const char*>(&silk), sizeof(silk));
    out.write(reinterpret_cast<const char*>(&jewelry), sizeof(jewelry));
    out.write(reinterpret_cast<const char*>(&fang_mo_pot), sizeof(fang_mo_pot));
    out.write(reinterpret_cast<const char*>(&trader), sizeof(trader));
    out.write(reinterpret_cast<const char*>(&camel), sizeof(camel));
    out.write(reinterpret_cast<const char*>(&food_consumption), sizeof(food_consumption));
    out.write(reinterpret_cast<const char*>(&backpack), sizeof(backpack));
    out.write(reinterpret_cast<const char*>(&daily_income), sizeof(daily_income));
    out.write(reinterpret_cast<const char*>(&day), sizeof(day));
    out.write(reinterpret_cast<const char*>(&max_day), sizeof(max_day));
    out.write(reinterpret_cast<const char*>(&interest_rate), sizeof(interest_rate));
    out.write(reinterpret_cast<const char*>(&player_world), sizeof(player_world));
    out.write(reinterpret_cast<const char*>(&set_interest), sizeof(set_interest));
    out.write(reinterpret_cast<const char*>(&quick_interest), sizeof(quick_interest));
    out.write(reinterpret_cast<const char*>(&statue), sizeof(statue));
    out.write(reinterpret_cast<const char*>(&hand_of_midas), sizeof(hand_of_midas));
    out.write(reinterpret_cast<const char*>(&merch_discount), sizeof(merch_discount));
    out.write(reinterpret_cast<const char*>(&trader_interest), sizeof(trader_interest));
    out.write(reinterpret_cast<const char*>(&merchant_happened), sizeof(merchant_happened));
    out.write(reinterpret_cast<const char*>(&call_merchant), sizeof(call_merchant));
    out.write(reinterpret_cast<const char*>(&witch_happened), sizeof(witch_happened));
    out.write(reinterpret_cast<const char*>(&day_start_skip), sizeof(day_start_skip));
    out.write(reinterpret_cast<const char*>(&bought_last), sizeof(bought_last));
    out.write(reinterpret_cast<const char*>(item_shop.data()), static_cast<std::streamsize>(item_shop.size() * sizeof(int)));
    out.write(reinterpret_cast<const char*>(npc_shop.data()), static_cast<std::streamsize>(npc_shop.size() * sizeof(int)));
}

void SolverStateKey::deserialize(std::istream& in) {
    in.read(reinterpret_cast<char*>(&gold), sizeof(gold));
    in.read(reinterpret_cast<char*>(&food), sizeof(food));
    in.read(reinterpret_cast<char*>(&spice), sizeof(spice));
    in.read(reinterpret_cast<char*>(&pottery), sizeof(pottery));
    in.read(reinterpret_cast<char*>(&marble), sizeof(marble));
    in.read(reinterpret_cast<char*>(&silk), sizeof(silk));
    in.read(reinterpret_cast<char*>(&jewelry), sizeof(jewelry));
    in.read(reinterpret_cast<char*>(&fang_mo_pot), sizeof(fang_mo_pot));
    in.read(reinterpret_cast<char*>(&trader), sizeof(trader));
    in.read(reinterpret_cast<char*>(&camel), sizeof(camel));
    in.read(reinterpret_cast<char*>(&food_consumption), sizeof(food_consumption));
    in.read(reinterpret_cast<char*>(&backpack), sizeof(backpack));
    in.read(reinterpret_cast<char*>(&daily_income), sizeof(daily_income));
    in.read(reinterpret_cast<char*>(&day), sizeof(day));
    in.read(reinterpret_cast<char*>(&max_day), sizeof(max_day));
    in.read(reinterpret_cast<char*>(&interest_rate), sizeof(interest_rate));
    in.read(reinterpret_cast<char*>(&player_world), sizeof(player_world));
    in.read(reinterpret_cast<char*>(&set_interest), sizeof(set_interest));
    in.read(reinterpret_cast<char*>(&quick_interest), sizeof(quick_interest));
    in.read(reinterpret_cast<char*>(&statue), sizeof(statue));
    in.read(reinterpret_cast<char*>(&hand_of_midas), sizeof(hand_of_midas));
    in.read(reinterpret_cast<char*>(&merch_discount), sizeof(merch_discount));
    in.read(reinterpret_cast<char*>(&trader_interest), sizeof(trader_interest));
    in.read(reinterpret_cast<char*>(&merchant_happened), sizeof(merchant_happened));
    in.read(reinterpret_cast<char*>(&call_merchant), sizeof(call_merchant));
    in.read(reinterpret_cast<char*>(&witch_happened), sizeof(witch_happened));
    in.read(reinterpret_cast<char*>(&day_start_skip), sizeof(day_start_skip));
    in.read(reinterpret_cast<char*>(&bought_last), sizeof(bought_last));
    in.read(reinterpret_cast<char*>(item_shop.data()), static_cast<std::streamsize>(item_shop.size() * sizeof(int)));
    in.read(reinterpret_cast<char*>(npc_shop.data()), static_cast<std::streamsize>(npc_shop.size() * sizeof(int)));
}

bool SolverStateKey::operator==(const SolverStateKey& other) const noexcept {
    return gold == other.gold &&
           food == other.food &&
           spice == other.spice &&
           pottery == other.pottery &&
           marble == other.marble &&
           silk == other.silk &&
           jewelry == other.jewelry &&
           fang_mo_pot == other.fang_mo_pot &&
           trader == other.trader &&
           camel == other.camel &&
           food_consumption == other.food_consumption &&
           backpack == other.backpack &&
           daily_income == other.daily_income &&
           day == other.day &&
           max_day == other.max_day &&
           interest_rate == other.interest_rate &&
           player_world.town == other.player_world.town &&
           player_world.merch == other.player_world.merch &&
           player_world.witch == other.player_world.witch &&
           set_interest == other.set_interest &&
           quick_interest == other.quick_interest &&
           statue == other.statue &&
           hand_of_midas == other.hand_of_midas &&
           merch_discount == other.merch_discount &&
           trader_interest == other.trader_interest &&
           merchant_happened == other.merchant_happened &&
           call_merchant == other.call_merchant &&
           witch_happened == other.witch_happened &&
           day_start_skip == other.day_start_skip &&
           bought_last == other.bought_last &&
           item_shop == other.item_shop &&
           npc_shop == other.npc_shop;
}

SolverStateKey SolverStateKey::from_player(const Player& player) {
    SolverStateKey key;
    key.gold = player.gold;
    key.food = player.food;
    key.spice = player.spice;
    key.pottery = player.pottery;
    key.marble = player.marble;
    key.silk = player.silk;
    key.jewelry = player.jewelry;
    key.fang_mo_pot = player.fang_mo_pot;
    key.trader = player.trader;
    key.camel = player.camel;
    key.food_consumption = player.food_consumption;
    key.backpack = player.backpack;
    key.daily_income = player.daily_income;
    key.day = player.day;
    key.max_day = player.max_day;
    key.interest_rate = player.interest_rate;
    key.player_world = player.player_world;
    key.set_interest = player.set_interest;
    key.quick_interest = player.quick_interest;
    key.statue = player.statue;
    key.hand_of_midas = player.hand_of_midas;
    key.merch_discount = player.merch_discount;
    key.trader_interest = player.trader_interest;
    key.merchant_happened = player.merchant_happened;
    key.call_merchant = player.call_merchant;
    key.witch_happened = player.witch_happened;
    key.day_start_skip = player.day_start_skip;
    key.bought_last = player.bought_last;
    std::memcpy(key.item_shop.data(), player.item_shop, sizeof(player.item_shop));
    std::memcpy(key.npc_shop.data(), player.npc_shop, sizeof(player.npc_shop));
    return key;
}

size_t SolverStateKeyHash::operator()(const SolverStateKey& key) const noexcept {
    size_t seed = 0;
    hash_combine(seed, key.gold);
    hash_combine(seed, key.food);
    hash_combine(seed, key.spice);
    hash_combine(seed, key.pottery);
    hash_combine(seed, key.marble);
    hash_combine(seed, key.silk);
    hash_combine(seed, key.jewelry);
    hash_combine(seed, key.fang_mo_pot);
    hash_combine(seed, key.trader);
    hash_combine(seed, key.camel);
    hash_combine(seed, key.food_consumption);
    hash_combine(seed, key.backpack);
    hash_combine(seed, key.daily_income);
    hash_combine(seed, key.day);
    hash_combine(seed, key.max_day);
    hash_combine(seed, key.interest_rate);
    hash_combine(seed, key.player_world.town);
    hash_combine(seed, key.player_world.merch);
    hash_combine(seed, key.player_world.witch);
    hash_combine(seed, key.set_interest);
    hash_combine(seed, key.quick_interest);
    hash_combine(seed, key.statue);
    hash_combine(seed, key.hand_of_midas);
    hash_combine(seed, key.merch_discount);
    hash_combine(seed, key.trader_interest);
    hash_combine(seed, key.merchant_happened);
    hash_combine(seed, key.call_merchant);
    hash_combine(seed, key.witch_happened);
    hash_combine(seed, key.day_start_skip);
    hash_combine(seed, key.bought_last);
    for (int value : key.item_shop) {
        hash_combine(seed, value);
    }
    for (int value : key.npc_shop) {
        hash_combine(seed, value);
    }
    return seed;
}

GameSolver::GameSolver(
    const town_options town,
    const bool _merch_on,
    const bool _witch_on,
    const int _array_limit,
    const int _print_counter,
    const std::vector<ga_data>& que,
    std::string checkpoint_file,
    const uint64_t _memory_limit_mb
) {
    array_limit = _array_limit;
    print_counter = _print_counter;
    merch_on = _merch_on;
    witch_on = _witch_on;
    if (_memory_limit_mb > 0) {
        memory_limit_bytes = _memory_limit_mb * 1024ULL * 1024ULL;
    } else {
        const uint64_t available_memory = detect_available_memory_bytes();
        if (available_memory > 0) {
            memory_limit_bytes = (available_memory * 3ULL) / 4ULL;
        }
    }
    checkpoint_path = checkpoint_file.empty() ? default_checkpoint_path(town, _merch_on, _witch_on) : std::filesystem::path(std::move(checkpoint_file));
    started_at = std::chrono::steady_clock::now();
    last_status_at = started_at;
    last_checkpoint_at = 0;
    seen_states.reserve(static_cast<size_t>(std::max(array_limit, 1)));
    to_process.clear();
    traces.reserve(static_cast<size_t>(std::max(array_limit / 4, 1024)));
    finished.reserve(MAX_FINISHED_TO_KEEP);

    if (!load_checkpoint()) {
        traces.push_back(TraceNode{});
        Game::apply_town(start_player, town);
        if (!que.empty()) {
            Game::auto_game_loop(start_player, que);
        }
        enqueue_state(start_player);
    }
}

uint64_t GameSolver::detect_available_memory_bytes() {
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo) {
        return 0;
    }

    std::string label;
    uint64_t value_kb = 0;
    std::string unit;
    while (meminfo >> label >> value_kb >> unit) {
        if (label == "MemAvailable:") {
            return value_kb * 1024ULL;
        }
    }
    return 0;
}

std::filesystem::path GameSolver::default_checkpoint_path(const town_options town, const bool merch, const bool witch) {
    std::filesystem::path dir = std::filesystem::current_path() / "checkpoints";
    std::filesystem::create_directories(dir);
    return dir / (to_conversion.at(town) + "_" + std::to_string(static_cast<int>(merch)) + "_" + std::to_string(static_cast<int>(witch)) + ".bin");
}

void GameSolver::warm_purchase_trees() {
    handler.warmSupportedTrees();
    started_at = std::chrono::steady_clock::now();
    last_status_at = started_at;
    std::cout << "[*] Purchase tree warmup complete" << std::endl;
}

uint64_t GameSolver::estimated_memory_bytes() const {
    uint64_t total = 0;
    total += static_cast<uint64_t>(to_process.size()) * sizeof(Player);
    total += static_cast<uint64_t>(seen_states.size()) * sizeof(SolverStateKey);
    total += static_cast<uint64_t>(finished.size()) * sizeof(Player);
    total += static_cast<uint64_t>(traces.size()) * sizeof(TraceNode);
    for (const auto& trace : traces) {
        total += static_cast<uint64_t>(trace.action.capacity());
    }
    return total;
}

bool GameSolver::would_exceed_limits(const size_t additional_states) const {
    if (array_limit > 0 && seen_states.size() + additional_states > static_cast<size_t>(array_limit)) {
        return true;
    }
    if (memory_limit_bytes == 0) {
        return false;
    }

    const uint64_t projected_memory =
        estimated_memory_bytes() +
        (static_cast<uint64_t>(additional_states) * sizeof(Player)) +
        (static_cast<uint64_t>(additional_states) * sizeof(SolverStateKey)) +
        (static_cast<uint64_t>(additional_states) * sizeof(TraceNode));
    return projected_memory > memory_limit_bytes;
}

void GameSolver::request_stop(std::string reason) {
    if (stop_requested) {
        return;
    }
    stop_requested = true;
    stop_reason = std::move(reason);
}

bool GameSolver::enqueue_state(const Player& player) {
    ++profiling.enqueue_calls;

    if (would_exceed_limits(1)) {
        std::ostringstream reason;
        reason << "Resource limit reached before enqueue"
               << " | processed=" << processed
               << " | frontier=" << to_process.size()
               << " | unique=" << seen_states.size();
        if (array_limit > 0) {
            reason << " | array_limit=" << array_limit;
        }
        if (memory_limit_bytes > 0) {
            reason << " | est_mem_mb=" << (estimated_memory_bytes() / (1024ULL * 1024ULL))
                   << " | mem_limit_mb=" << (memory_limit_bytes / (1024ULL * 1024ULL));
        }
        request_stop(reason.str());
        return false;
    }

    const auto key_started = std::chrono::steady_clock::now();
    SolverStateKey key = SolverStateKey::from_player(player);
    profiling.state_key_time += std::chrono::steady_clock::now() - key_started;

    const auto insert_started = std::chrono::steady_clock::now();
    auto [_, inserted] = seen_states.insert(key);
    profiling.seen_insert_time += std::chrono::steady_clock::now() - insert_started;
    if (!inserted) {
        ++profiling.enqueue_duplicates;
        return false;
    }

    const auto push_started = std::chrono::steady_clock::now();
    Player copy = player;
    traces.push_back(TraceNode{copy.trace_id, copy.last_action});
    copy.trace_id = static_cast<uint32_t>(traces.size() - 1);
    copy.last_action.clear();
    to_process.emplace_back(std::move(copy));
    profiling.frontier_push_time += std::chrono::steady_clock::now() - push_started;
    return true;
}

void GameSolver::record_finished(const Player& player) {
    ++profiling.finished_records;
    finished.emplace_back(player);
    std::sort(finished.begin(), finished.end(), [](const Player& lhs, const Player& rhs) {
        return lhs.points > rhs.points;
    });
    if (finished.size() > MAX_FINISHED_TO_KEEP) {
        finished.resize(MAX_FINISHED_TO_KEEP);
    }
}

void GameSolver::print_profile_summary() const {
    const auto format_ms = [](const std::chrono::nanoseconds duration) {
        return std::chrono::duration<double, std::milli>(duration).count();
    };
    const auto avg_ns = [](const std::chrono::nanoseconds duration, const uint64_t count) {
        return count == 0 ? 0.0 : static_cast<double>(duration.count()) / static_cast<double>(count);
    };

    std::cout << "Profile | enqueue_calls=" << profiling.enqueue_calls
              << " | duplicates=" << profiling.enqueue_duplicates
              << " | finished=" << profiling.finished_records
              << " | est_mem_mb=" << (estimated_memory_bytes() / (1024ULL * 1024ULL))
              << " | state_key_ms=" << std::fixed << std::setprecision(1) << format_ms(profiling.state_key_time)
              << " | seen_insert_ms=" << format_ms(profiling.seen_insert_time)
              << " | frontier_push_ms=" << format_ms(profiling.frontier_push_time)
              << " | progress_state_ms=" << format_ms(profiling.progress_state_time)
              << " | checkpoint_ms=" << format_ms(profiling.checkpoint_time)
              << " | status_print_ms=" << format_ms(profiling.status_print_time)
              << " | solve_print_ms=" << format_ms(profiling.solve_print_time)
              << " | avg_state_key_ns=" << std::setprecision(0) << avg_ns(profiling.state_key_time, profiling.enqueue_calls)
              << " | avg_insert_ns=" << avg_ns(profiling.seen_insert_time, profiling.enqueue_calls)
              << " | avg_push_ns=" << avg_ns(profiling.frontier_push_time, profiling.enqueue_calls)
              << std::endl;
    std::cout << "Branches | merchant_ms=" << std::fixed << std::setprecision(1) << format_ms(profiling.merchant_branch_time)
              << " (" << profiling.merchant_branch_calls << ")"
              << " | witch_ms=" << format_ms(profiling.witch_branch_time)
              << " (" << profiling.witch_branch_calls << ")"
              << " | buy_ms=" << format_ms(profiling.buy_branch_time)
              << " (" << profiling.buy_branch_calls << ")"
              << " | sell_ms=" << format_ms(profiling.sell_branch_time)
              << " (" << profiling.sell_branch_calls << ")"
              << " | trader_ms=" << format_ms(profiling.trader_branch_time)
              << " (" << profiling.trader_branch_calls << ")"
              << " | camel_ms=" << format_ms(profiling.camel_branch_time)
              << " (" << profiling.camel_branch_calls << ")"
              << " | interest_ms=" << format_ms(profiling.interest_branch_time)
              << " (" << profiling.interest_branch_calls << ")"
              << std::endl;
    std::cout << "BuyStats | avg_food_range="
              << std::fixed << std::setprecision(1)
              << (profiling.buy_branch_calls == 0 ? 0.0 : static_cast<double>(profiling.buy_food_range_total) / static_cast<double>(profiling.buy_branch_calls))
              << " | best_buy_queries=" << profiling.buy_best_buy_queries
              << " | food_only_checks=" << profiling.buy_food_only_checks
              << " | emitted_states=" << profiling.buy_emitted_states
              << " | duplicate_emits=" << profiling.buy_duplicate_emits
              << " | max_food_range=" << profiling.buy_max_food_range
              << " | widest_day=" << profiling.buy_widest_range_day
              << " | widest_min=" << profiling.buy_max_food_min
              << " | widest_max=" << profiling.buy_max_food_max
              << std::endl;

    const auto handler_profile = handler.getProfilingSummary();
    std::cout << "BuyQuery | calls=" << handler_profile.calls
              << " | cache_misses=" << handler_profile.cache_misses
              << " | total_ms=" << std::fixed << std::setprecision(1) << handler_profile.total_ms
              << " | ensure_ms=" << handler_profile.ensure_ms
              << " | query_ms=" << handler_profile.query_ms;
    if (!handler_profile.slowest_total_key.key.empty()) {
        std::cout << " | slow_total_key=" << handler_profile.slowest_total_key.key
                  << " (" << handler_profile.slowest_total_key.total_ms << " ms"
                  << ", calls=" << handler_profile.slowest_total_key.calls << ")";
    }
    if (!handler_profile.slowest_query_key.key.empty()) {
        std::cout << " | slow_query_key=" << handler_profile.slowest_query_key.key
                  << " (" << handler_profile.slowest_query_key.query_ms << " ms"
                  << ", calls=" << handler_profile.slowest_query_key.calls << ")";
    }
    std::cout << std::endl;
}

void GameSolver::print_solves(const int amount) const {
    if (amount != 1) {
        std::cout << "Printing Solves\n";
    }

    const int limit = std::min(amount, static_cast<int>(finished.size()));
    for (int index = 0; index < limit; ++index) {
        const Player& solve = finished[index];
        const auto actions = rebuild_actions(solve.trace_id);
        std::cout << "Place " << (index + 1) << ": " << solve.points << "\n";
        for (size_t action_index = 0; action_index < actions.size(); ++action_index) {
            if (action_index != 0) {
                std::cout << ", ";
            }
            std::cout << actions[action_index];
        }
        std::cout << "\n\n" << std::flush;
    }
}

std::vector<std::string> GameSolver::rebuild_actions(const uint32_t trace_id) const {
    std::vector<std::string> actions;
    uint32_t current = trace_id;
    while (current != 0 && current < traces.size()) {
        const TraceNode& node = traces[current];
        if (!node.action.empty()) {
            actions.emplace_back(node.action);
        }
        current = node.parent;
    }
    std::reverse(actions.begin(), actions.end());
    return actions;
}

void GameSolver::maybe_checkpoint(const bool force) {
    if (!force && processed - last_checkpoint_at < CHECKPOINT_INTERVAL) {
        return;
    }
    std::cout << "[*] Saving checkpoint at processed=" << processed
              << " frontier=" << to_process.size()
              << " unique=" << seen_states.size() << std::endl;
    const auto checkpoint_started = std::chrono::steady_clock::now();
    save_checkpoint();
    profiling.checkpoint_time += std::chrono::steady_clock::now() - checkpoint_started;
    last_checkpoint_at = processed;
    std::cout << "[*] Checkpoint saved: " << checkpoint_path << std::endl;
}

void GameSolver::save_checkpoint() const {
    std::filesystem::create_directories(checkpoint_path.parent_path());
    const std::filesystem::path temp_path = checkpoint_path.string() + ".tmp";
    std::ofstream out(temp_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::ios_base::failure("Failed to open checkpoint for writing");
    }

    out.write(reinterpret_cast<const char*>(&CHECKPOINT_VERSION), sizeof(CHECKPOINT_VERSION));
    out.write(reinterpret_cast<const char*>(&merch_on), sizeof(merch_on));
    out.write(reinterpret_cast<const char*>(&witch_on), sizeof(witch_on));
    out.write(reinterpret_cast<const char*>(&array_limit), sizeof(array_limit));
    out.write(reinterpret_cast<const char*>(&print_counter), sizeof(print_counter));
    out.write(reinterpret_cast<const char*>(&processed), sizeof(processed));
    start_player.serialize(out);

    size_t frontier_size = to_process.size();
    out.write(reinterpret_cast<const char*>(&frontier_size), sizeof(frontier_size));
    for (const auto& player : to_process) {
        player.serialize(out);
    }

    size_t seen_size = seen_states.size();
    out.write(reinterpret_cast<const char*>(&seen_size), sizeof(seen_size));
    for (const auto& key : seen_states) {
        key.serialize(out);
    }

    size_t trace_size = traces.size();
    out.write(reinterpret_cast<const char*>(&trace_size), sizeof(trace_size));
    for (const auto& trace : traces) {
        trace.serialize(out);
    }

    size_t finished_size = finished.size();
    out.write(reinterpret_cast<const char*>(&finished_size), sizeof(finished_size));
    for (const auto& player : finished) {
        player.serialize(out);
    }
    out.close();

    if (std::filesystem::exists(checkpoint_path)) {
        std::filesystem::remove(checkpoint_path);
    }
    std::filesystem::rename(temp_path, checkpoint_path);
}

bool GameSolver::load_checkpoint() {
    if (checkpoint_path.empty() || !std::filesystem::exists(checkpoint_path)) {
        return false;
    }

    std::ifstream in(checkpoint_path, std::ios::binary);
    if (!in) {
        return false;
    }

    uint32_t version = 0;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != CHECKPOINT_VERSION) {
        return false;
    }

    in.read(reinterpret_cast<char*>(&merch_on), sizeof(merch_on));
    in.read(reinterpret_cast<char*>(&witch_on), sizeof(witch_on));
    in.read(reinterpret_cast<char*>(&array_limit), sizeof(array_limit));
    in.read(reinterpret_cast<char*>(&print_counter), sizeof(print_counter));
    in.read(reinterpret_cast<char*>(&processed), sizeof(processed));
    start_player.deserialize(in);

    size_t frontier_size = 0;
    in.read(reinterpret_cast<char*>(&frontier_size), sizeof(frontier_size));
    to_process.clear();
    for (size_t i = 0; i < frontier_size; ++i) {
        Player player;
        player.deserialize(in);
        to_process.emplace_back(std::move(player));
    }

    size_t seen_size = 0;
    in.read(reinterpret_cast<char*>(&seen_size), sizeof(seen_size));
    seen_states.clear();
    seen_states.reserve(seen_size);
    for (size_t i = 0; i < seen_size; ++i) {
        SolverStateKey key;
        key.deserialize(in);
        seen_states.insert(std::move(key));
    }

    size_t trace_size = 0;
    in.read(reinterpret_cast<char*>(&trace_size), sizeof(trace_size));
    traces.clear();
    traces.resize(trace_size);
    for (size_t i = 0; i < trace_size; ++i) {
        traces[i].deserialize(in);
    }

    size_t finished_size = 0;
    in.read(reinterpret_cast<char*>(&finished_size), sizeof(finished_size));
    finished.clear();
    finished.reserve(finished_size);
    for (size_t i = 0; i < finished_size; ++i) {
        Player player;
        player.deserialize(in);
        finished.emplace_back(std::move(player));
    }

    if (!in.good() && !in.eof()) {
        return false;
    }

    std::cout << "[*] Resumed from checkpoint: " << checkpoint_path << std::endl;
    return true;
}

void GameSolver::progress_state(Player current) {
    const auto progress_started = std::chrono::steady_clock::now();
    const Player debug_state = current;
    if (!current.day_start_skip) {
        Game::start_of_day(current);
        if (current.food < 0) {
            profiling.progress_state_time += std::chrono::steady_clock::now() - progress_started;
            return;
        }

        if (merch_on && !current.merchant_happened && (current.call_merchant || Helpers::day_in_array(merch_days, current.day))) {
            const auto branch_started = std::chrono::steady_clock::now();
            std::deque<Player> generated;
            Helpers::run_merchant_event(current, generated);
            while (!generated.empty()) {
                enqueue_state(generated.back());
                generated.pop_back();
            }
            profiling.merchant_branch_time += std::chrono::steady_clock::now() - branch_started;
            ++profiling.merchant_branch_calls;
        }

        if (witch_on && !current.witch_happened && Helpers::day_in_array(witch_days, current.day)) {
            const auto branch_started = std::chrono::steady_clock::now();
            std::deque<Player> generated;
            Helpers::run_witch_event(current, generated);
            while (!generated.empty()) {
                enqueue_state(generated.back());
                generated.pop_back();
            }
            profiling.witch_branch_time += std::chrono::steady_clock::now() - branch_started;
            ++profiling.witch_branch_calls;
        }
    } else {
        current.day_start_skip = false;
    }

    const int current_food_cost = current.food_consumption;

    if (current.day < current.max_day && !current.bought_last) {
        const auto branch_started = std::chrono::steady_clock::now();
        std::deque<Player> generated;
        const BuyEventStats buy_stats = Helpers::run_buy_event(current, generated, handler);
        while (!generated.empty()) {
            enqueue_state(generated.back());
            generated.pop_back();
        }
        profiling.buy_food_range_total += buy_stats.food_range_total;
        profiling.buy_best_buy_queries += buy_stats.best_buy_queries;
        profiling.buy_food_only_checks += buy_stats.food_only_checks;
        profiling.buy_emitted_states += buy_stats.emitted_states;
        profiling.buy_duplicate_emits += buy_stats.duplicate_emits;
        if (buy_stats.max_food_range > profiling.buy_max_food_range) {
            profiling.buy_max_food_range = buy_stats.max_food_range;
            profiling.buy_max_food_min = buy_stats.max_food_min;
            profiling.buy_max_food_max = buy_stats.max_food_max;
            profiling.buy_widest_range_day = buy_stats.widest_range_day;
        }
        profiling.buy_branch_time += std::chrono::steady_clock::now() - branch_started;
        ++profiling.buy_branch_calls;
    }

    if (current.day == current.max_day || current.food >= current_food_cost) {
        if (Game::check_has_items(current)) {
            const auto branch_started = std::chrono::steady_clock::now();
            if (current.day != current.max_day) {
                Player sell = current;
                Game::event_sell_items(sell);
                enqueue_state(sell);
            } else {
                Player sell = current;
                Game::event_sell_items(sell);
                record_finished(sell);
            }
            profiling.sell_branch_time += std::chrono::steady_clock::now() - branch_started;
            ++profiling.sell_branch_calls;
        }

        if (current.gold >= current.npc_shop[n_Trader][nd_Cost] &&
            current.day < current.max_day &&
            current.food >= current_food_cost + current.npc_shop[n_Trader][nd_Eat]) {
            const auto branch_started = std::chrono::steady_clock::now();
            Player trader = current;
            Game::event_buy_npc(trader, n_Trader);
            enqueue_state(trader);
            profiling.trader_branch_time += std::chrono::steady_clock::now() - branch_started;
            ++profiling.trader_branch_calls;
        }

        if (current.gold >= current.npc_shop[n_Camel][nd_Cost] &&
            current.day < current.max_day &&
            current.food >= current_food_cost + current.npc_shop[n_Camel][nd_Eat]) {
            const auto branch_started = std::chrono::steady_clock::now();
            Player camel = current;
            Game::event_buy_npc(camel, n_Camel);
            enqueue_state(camel);
            profiling.camel_branch_time += std::chrono::steady_clock::now() - branch_started;
            ++profiling.camel_branch_calls;
        }

        const auto branch_started = std::chrono::steady_clock::now();
        Player interest = current;
        Game::event_interest(interest);
        if (current.day < current.max_day) {
            enqueue_state(interest);
        } else {
            record_finished(interest);
        }
        profiling.interest_branch_time += std::chrono::steady_clock::now() - branch_started;
        ++profiling.interest_branch_calls;
    }
    const auto progress_elapsed = std::chrono::steady_clock::now() - progress_started;
    profiling.progress_state_time += progress_elapsed;
    if (progress_elapsed >= SLOW_STATE_THRESHOLD) {
        const auto ms = std::chrono::duration<double, std::milli>(progress_elapsed).count();
        std::cout << "[!] Slow state | ms=" << std::fixed << std::setprecision(1) << ms
                  << " | day=" << debug_state.day
                  << " | max_day=" << debug_state.max_day
                  << " | gold=" << debug_state.gold
                  << " | food=" << debug_state.food
                  << " | traders=" << debug_state.trader
                  << " | camels=" << debug_state.camel
                  << " | bought_last=" << debug_state.bought_last
                  << " | merch=" << static_cast<int>(debug_state.player_world.merch)
                  << " | witch=" << static_cast<int>(debug_state.player_world.witch)
                  << " | interest=" << debug_state.interest_rate
                  << std::endl;
    }
}

void GameSolver::find_solve_threads(const int amount) {
    std::cout << "[*] Threaded solver is temporarily routed to single-thread execution while exact dedup/checkpointing is active.\n";
    (void)amount;
    find_solve_depth(true);
}

void GameSolver::find_solve_depth(const bool main_thread) {
    int round_num = 0;
    while (!to_process.empty() && !stop_requested) {
        Player current = std::move(to_process.back());
        to_process.pop_back();
        progress_state(std::move(current));
        ++processed;

        if (main_thread && processed % static_cast<uint64_t>(print_counter) == 0) {
            ++round_num;
            const auto status_started = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started_at).count();
            const double rate = elapsed > 0.0 ? static_cast<double>(processed) / elapsed : 0.0;
            std::cout << "Processing Round " << round_num << " | processed=" << processed
                      << " | frontier=" << to_process.size()
                      << " | unique=" << seen_states.size()
                      << " | dupes=" << profiling.enqueue_duplicates
                      << " | est_mem_mb=" << (estimated_memory_bytes() / (1024ULL * 1024ULL))
                      << " | states/sec=" << std::fixed << std::setprecision(1) << rate << std::endl;
            print_profile_summary();
            profiling.status_print_time += std::chrono::steady_clock::now() - status_started;

            if (!finished.empty() && finished.front().points > last_reported_best_points) {
                const auto solve_started = std::chrono::steady_clock::now();
                last_reported_best_points = finished.front().points;
                print_solves(1);
                profiling.solve_print_time += std::chrono::steady_clock::now() - solve_started;
            }
            last_status_at = std::chrono::steady_clock::now();
        } else if (main_thread && std::chrono::steady_clock::now() - last_status_at >= HEARTBEAT_INTERVAL) {
            const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started_at).count();
            const double rate = elapsed > 0.0 ? static_cast<double>(processed) / elapsed : 0.0;
            std::cout << "[*] Heartbeat | processed=" << processed
                      << " | frontier=" << to_process.size()
                      << " | unique=" << seen_states.size()
                      << " | finished=" << finished.size()
                      << " | est_mem_mb=" << (estimated_memory_bytes() / (1024ULL * 1024ULL))
                      << " | states/sec=" << std::fixed << std::setprecision(1) << rate << std::endl;
            last_status_at = std::chrono::steady_clock::now();
        }

        maybe_checkpoint(false);
    }

    if (stop_requested) {
        std::cout << "[!] Search stopped early: " << stop_reason << std::endl;
    } else {
        std::cout << "[*] Search frontier exhausted at processed=" << processed << std::endl;
    }
    maybe_checkpoint(true);
    if (main_thread) {
        print_profile_summary();
        print_solves(static_cast<int>(MAX_FINISHED_TO_KEEP));
    }
}

void GameSolver::find_solve_breath(const bool continuous) {
    (void)continuous;
    find_solve_depth(true);
}
