//
// Created by Jay on 6/14/2023.
//

#ifndef TRADING_CARAVAN_SOLVER_GAMESOLVER_H
#define TRADING_CARAVAN_SOLVER_GAMESOLVER_H

#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <iosfwd>
#include <limits>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>
#include "../game/Game.h"
#include "../purchaseHandler/BestPurchaseHandler.h"
#include "../consts/enums.h"

struct SolverStateKey {
    int gold = 0;
    int food = 0;
    int spice = 0;
    int pottery = 0;
    int marble = 0;
    int silk = 0;
    int jewelry = 0;
    int fang_mo_pot = 0;
    int trader = 0;
    int camel = 0;
    int food_consumption = 0;
    int backpack = 0;
    int daily_income = 0;
    int day = 0;
    int max_day = 0;
    double interest_rate = 0.0;
    world player_world;
    bool set_interest = false;
    bool quick_interest = false;
    bool statue = false;
    bool hand_of_midas = false;
    bool merch_discount = false;
    bool trader_interest = false;
    bool merchant_happened = false;
    bool call_merchant = false;
    bool witch_happened = false;
    bool day_start_skip = false;
    bool bought_last = false;
    std::array<int, 18> item_shop{};
    std::array<int, 6> npc_shop{};

    [[nodiscard]] bool operator==(const SolverStateKey& other) const noexcept;
    void serialize(std::ostream& out) const;
    void deserialize(std::istream& in);
    static SolverStateKey from_player(const Player& player);
};

struct SolverStateKeyHash {
    [[nodiscard]] size_t operator()(const SolverStateKey& key) const noexcept;
};

struct TraceNode {
    uint32_t parent = 0;
    std::string action;

    void serialize(std::ostream& out) const;
    void deserialize(std::istream& in);
};

struct SolverProfiling {
    uint64_t enqueue_calls = 0;
    uint64_t enqueue_duplicates = 0;
    uint64_t finished_records = 0;
    uint64_t merchant_branch_calls = 0;
    uint64_t witch_branch_calls = 0;
    uint64_t buy_branch_calls = 0;
    uint64_t buy_food_range_total = 0;
    uint64_t buy_best_buy_queries = 0;
    uint64_t buy_food_only_checks = 0;
    uint64_t buy_emitted_states = 0;
    uint64_t buy_duplicate_emits = 0;
    uint64_t sell_branch_calls = 0;
    uint64_t trader_branch_calls = 0;
    uint64_t camel_branch_calls = 0;
    uint64_t interest_branch_calls = 0;
    std::chrono::nanoseconds state_key_time{0};
    std::chrono::nanoseconds seen_insert_time{0};
    std::chrono::nanoseconds frontier_push_time{0};
    std::chrono::nanoseconds progress_state_time{0};
    std::chrono::nanoseconds checkpoint_time{0};
    std::chrono::nanoseconds status_print_time{0};
    std::chrono::nanoseconds solve_print_time{0};
    std::chrono::nanoseconds merchant_branch_time{0};
    std::chrono::nanoseconds witch_branch_time{0};
    std::chrono::nanoseconds buy_branch_time{0};
    std::chrono::nanoseconds sell_branch_time{0};
    std::chrono::nanoseconds trader_branch_time{0};
    std::chrono::nanoseconds camel_branch_time{0};
    std::chrono::nanoseconds interest_branch_time{0};
    int buy_max_food_range = 0;
    int buy_max_food_min = 0;
    int buy_max_food_max = 0;
    int buy_widest_range_day = 0;
};

class GameSolver {
public:
    explicit GameSolver(
        town_options town = t_Normalia,
        bool _merch_on = true,
        bool _witch_on = true,
        int _array_limit = 1000000,
        int _print_counter = 1000,
        const std::vector<ga_data>& que = {},
        std::string checkpoint_file = "",
        uint64_t _memory_limit_mb = 0
    );

    void find_solve_breath(bool continuous = true);
    void find_solve_threads(int amount);
    void find_solve_depth(bool main_thread = true);
    void warm_purchase_trees();

private:
    static constexpr uint32_t CHECKPOINT_VERSION = 2;
    static constexpr size_t MAX_FINISHED_TO_KEEP = 10;
    static constexpr uint64_t CHECKPOINT_INTERVAL = 10000000;
    static constexpr auto HEARTBEAT_INTERVAL = std::chrono::seconds(5);
    static constexpr auto SLOW_STATE_THRESHOLD = std::chrono::milliseconds(500);

    Player start_player;
    BestPurchaseHandler handler;
    std::deque<Player> to_process = {};
    std::unordered_set<SolverStateKey, SolverStateKeyHash> seen_states;
    std::vector<Player> finished;
    std::vector<TraceNode> traces;
    uint64_t processed = 0;
    int array_limit = 0;
    int print_counter = 0;
    bool merch_on = true;
    const int merch_days[3] = {5, 6, 7};
    bool witch_on = true;
    const int witch_days[3] = {9, 10, 11};
    std::vector<std::thread> threads = {};
    std::filesystem::path checkpoint_path;
    std::chrono::steady_clock::time_point started_at;
    std::chrono::steady_clock::time_point last_status_at;
    uint64_t last_checkpoint_at = 0;
    int last_reported_best_points = std::numeric_limits<int>::min();
    uint64_t memory_limit_bytes = 0;
    bool stop_requested = false;
    std::string stop_reason;
    SolverProfiling profiling;

    void progress_state(Player current);
    void print_solves(int amount) const;
    void print_profile_summary() const;
    void record_finished(const Player& player);
    bool enqueue_state(const Player& player);
    [[nodiscard]] std::vector<std::string> rebuild_actions(uint32_t trace_id) const;
    void maybe_checkpoint(bool force = false);
    void save_checkpoint() const;
    bool load_checkpoint();
    static std::filesystem::path default_checkpoint_path(town_options town, bool merch, bool witch);
    static uint64_t detect_available_memory_bytes();
    [[nodiscard]] uint64_t estimated_memory_bytes() const;
    [[nodiscard]] bool would_exceed_limits(size_t additional_states = 0) const;
    void request_stop(std::string reason);
};

#endif //TRADING_CARAVAN_SOLVER_GAMESOLVER_H
