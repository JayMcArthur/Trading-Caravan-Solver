#ifndef TRADING_CARAVAN_SOLVER_GAMESOLVER_H
#define TRADING_CARAVAN_SOLVER_GAMESOLVER_H

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "../consts/enums.h"
#include "../player/player.h"
#include "../purchaseHandler/BestPurchaseHandler.h"
#include "helpers.h"
#include "ContinuationMatrixSelector.h"
#include "EarlyActionSkeleton.h"
#include "RouteSurvivalAudit.h"

class FrontierAccumulator;
#include "SkeletonWorkQueue.h"
#include "SolverAction.h"

class DeferredPurchaseModel;
class DeferredSoldModel;
class DeferredSearchState;

struct TraceNode {
    uint32_t parent = 0;
    SolverAction action;
    EarlyActionSkeleton early_skeleton;
};

enum SolverStateFlags : uint16_t {
    SSF_SET_INTEREST = 1U << 0U,
    SSF_QUICK_INTEREST = 1U << 1U,
    SSF_STATUE = 1U << 2U,
    SSF_HAND_OF_MIDAS = 1U << 3U,
    SSF_MERCH_DISCOUNT = 1U << 4U,
    SSF_TRADER_INTEREST = 1U << 5U,
    SSF_MERCHANT_HAPPENED = 1U << 6U,
    SSF_CALL_MERCHANT = 1U << 7U,
    SSF_WITCH_HAPPENED = 1U << 8U,
    SSF_DAY_START_SKIP = 1U << 9U,
    SSF_BOUGHT_LAST = 1U << 10U,
    SSF_DISASTER_HAPPENED = 1U << 11U,
    SSF_TRAVELER_MODE = 1U << 12U,
    SSF_CARGO_BUY_LOCKED = 1U << 13U,
};

struct SolverState {
    int32_t gold = 0;
    uint16_t food = 0;
    uint16_t spice = 0;
    uint16_t pottery = 0;
    uint16_t marble = 0;
    uint16_t silk = 0;
    uint16_t jewelry = 0;
    int16_t trader = 0;
    int16_t camel = 0;
    int16_t food_consumption = 0;
    int16_t bonus_food_consumption = 0;
    int16_t backpack = 0;
    int16_t daily_income = 0;
    uint8_t day = 0;
    uint8_t max_day = 0;
    int16_t interest_hundredths = 0;
    uint8_t disaster_day = 0;
    uint8_t scheduled_disaster = d_No_Disaster;
    uint8_t merchant_day = 0;
    uint8_t witch_day = 0;
    world player_world;
    uint16_t flags = 0;
    std::array<int16_t, 18> item_shop{};
    std::array<int16_t, 6> npc_shop{};
    uint32_t trace_id = 0;
    std::shared_ptr<const DeferredSearchState> deferred;
    // Observational only: excluded from equality, hashing, ranking, and game
    // conversion so auditing cannot protect or otherwise alter a route.
    int16_t route_audit_progress = RouteSurvivalAudit::NO_MATCH;
    RouteAuditQuantityStatus route_audit_quantity =
        RouteAuditQuantityStatus::NotApplicable;

    [[nodiscard]] bool operator==(const SolverState& other) const noexcept;
    [[nodiscard]] Player to_player() const;
    static SolverState from_player(const Player& player, uint32_t trace_id = 0);
    [[nodiscard]] bool has_flag(uint16_t flag) const noexcept;
    void set_flag(uint16_t flag, bool value) noexcept;
};

struct SolverStateHash {
    [[nodiscard]] size_t operator()(const SolverState& state) const noexcept;
};

struct SolverContextKey {
    int16_t trader = 0;
    int16_t camel = 0;
    int16_t food_consumption = 0;
    int16_t bonus_food_consumption = 0;
    int16_t backpack = 0;
    int16_t daily_income = 0;
    uint8_t day = 0;
    uint8_t max_day = 0;
    int16_t interest_hundredths = 0;
    uint8_t disaster_day = 0;
    uint8_t scheduled_disaster = d_No_Disaster;
    uint8_t merchant_day = 0;
    uint8_t witch_day = 0;
    world player_world;
    uint16_t flags = 0;
    std::array<int16_t, 18> item_shop{};
    std::array<int16_t, 6> npc_shop{};

    [[nodiscard]] bool operator==(const SolverContextKey& other) const noexcept;
    static SolverContextKey from_state(const SolverState& state);
};

struct SolverContextKeyHash {
    [[nodiscard]] size_t operator()(const SolverContextKey& key) const noexcept;
};

struct SolverProfiling {
    uint64_t processed_states = 0;
    uint64_t generated_states = 0;
    uint64_t generated_buy_states = 0;
    uint64_t generated_wait_states = 0;
    uint64_t generated_sell_states = 0;
    uint64_t generated_trader_states = 0;
    uint64_t generated_camel_states = 0;
    uint64_t generated_interest_states = 0;
    uint64_t generated_merchant_states = 0;
    uint64_t generated_witch_states = 0;
    uint64_t generated_disaster_states = 0;
    uint64_t merchant_deferral_continuations = 0;
    uint64_t witch_deferral_continuations = 0;
    uint64_t purchase_valuation_worlds = 0;
    uint64_t purchase_food_range_total = 0;
    uint64_t purchase_food_only_checks = 0;
    uint64_t purchase_queries = 0;
    uint64_t purchase_portfolio_choices = 0;
    uint64_t purchase_unique_buys = 0;
    uint64_t purchase_duplicate_buys = 0;
    uint64_t purchase_dominance_prunes = 0;
    uint64_t max_purchase_food_range = 0;
    uint64_t deferred_purchase_states = 0;
    uint64_t deferred_sold_states = 0;
    uint64_t deferred_buy_seed_inputs = 0;
    uint64_t deferred_buy_origins_kept = 0;
    uint64_t deferred_rows_total = 0;
    uint64_t deferred_max_rows = 0;
    uint64_t deferred_expansions = 0;
    uint64_t deferred_purchase_expansions = 0;
    uint64_t deferred_sold_expansions = 0;
    uint64_t continuation_view_owners = 0;
    uint64_t continuation_views_evaluated = 0;
    uint64_t continuation_view_score_wins = 0;
    uint64_t continuation_view_bound_wins = 0;
    uint64_t continuation_matrix_lane_owners = 0;
    uint64_t continuation_matrix_duplicate_ticket_skips = 0;
    std::array<uint64_t, CONTINUATION_LANE_COUNT> continuation_matrix_lane_counts{};
    uint64_t continuation_matrix_scoring_nanoseconds = 0;
    uint64_t continuation_matrix_selection_nanoseconds = 0;
    uint64_t beam_trim_nanoseconds = 0;
    uint64_t deferred_purchase_expansion_nanoseconds = 0;
    uint64_t deferred_sold_expansion_nanoseconds = 0;
    uint64_t concrete_expansion_nanoseconds = 0;
    uint64_t frontier_reduction_nanoseconds = 0;
    uint64_t layer_compression_nanoseconds = 0;
    uint64_t duplicate_prunes = 0;
    uint64_t dominance_prunes = 0;
    uint64_t pre_beam_duplicate_prunes = 0;
    uint64_t pre_beam_dominance_prunes = 0;
    uint64_t deferred_dominance_checks = 0;
    uint64_t deferred_partition_rejections = 0;
    uint64_t deferred_purchase_dominance_prunes = 0;
    uint64_t deferred_sold_dominance_prunes = 0;
    uint64_t deferred_gold_function_comparisons = 0;
    uint64_t deferred_exact_spend_evaluations = 0;
    uint64_t deferred_origin_coverage_checks = 0;
    uint64_t deferred_cross_shape_dominance_prunes = 0;
    uint64_t current_day_requeues = 0;
    uint64_t finished_states = 0;
    uint64_t peak_candidate_states = 0;
    uint64_t beam_prunes = 0;
    uint64_t peak_resident_bytes = 0;
    uint64_t peak_estimated_bytes = 0;
    uint64_t memory_budget_checks = 0;
    uint64_t memory_budget_exact_scans = 0;
    uint64_t memory_budget_resident_samples = 0;
    uint64_t memory_budget_scan_nanoseconds = 0;
    uint64_t hybrid_batches = 0;
    uint64_t hybrid_frontier_states = 0;
    uint64_t hybrid_peak_waiting_states = 0;
    uint64_t hybrid_split_day = 0;
    uint64_t hybrid_trigger_estimated_bytes = 0;
    uint64_t hybrid_split_skeleton_groups = 0;
    uint64_t hybrid_effective_batch_size = 0;
    uint64_t skeleton_queue_groups = 0;
    uint64_t skeleton_queue_states = 0;
    uint64_t skeleton_queue_completed_groups = 0;
    uint64_t skeleton_queue_split_day = 0;
    uint64_t skeleton_queue_matrix_rows = 0;
    uint64_t skeleton_queue_duplicate_ticket_skips = 0;
    std::array<uint64_t, SKELETON_QUEUE_TICKET_COUNT> skeleton_queue_head_ticket_counts{};
    uint64_t skeleton_queue_ceiling_checked_groups = 0;
    uint64_t skeleton_queue_ceiling_violations = 0;
    uint64_t skeleton_queue_ceiling_max_overrun = 0;
    uint64_t skeleton_queue_ceiling_below_current_best = 0;
    uint64_t skeleton_queue_schedule_nanoseconds = 0;
    uint64_t skeleton_group_candidates = 0;
    uint64_t skeleton_reserved_states = 0;
    uint64_t skeleton_max_groups = 0;
    uint64_t refinement_group_candidates = 0;
    uint64_t refinement_reserved_states = 0;
    uint64_t refinement_max_groups = 0;
    uint64_t proof_states_popped = 0;
    uint64_t proof_transposition_hits = 0;
    uint64_t proof_transposition_evictions = 0;
    uint64_t proof_bound_prunes = 0;
    uint64_t proof_peak_stack_states = 0;
    uint64_t proof_peak_transposition_states = 0;
    uint64_t mapped_expanded_this_run = 0;
    uint64_t mapped_reused_children_this_run = 0;
    uint64_t mapped_nodes = 0;
    uint64_t mapped_open_nodes = 0;
    uint64_t mapped_partial_nodes = 0;
    uint64_t mapped_solved_nodes = 0;
    uint64_t mapped_database_bytes = 0;
    uint64_t mapped_beam_sweeps = 0;
    uint64_t mapped_beam_candidate_visits = 0;
    uint64_t mapped_beam_parked_visits = 0;
    bool proof_search_completed = false;
    bool proof_top_three_certified = false;
};

struct MemoryCheckCadence {
    uint64_t resident_sample_interval = 64;
    uint64_t exact_scan_interval = 4096;
};

[[nodiscard]] MemoryCheckCadence memory_check_cadence(
    uint64_t observed_bytes,
    uint64_t working_limit_bytes
) noexcept;

struct FinishedSolve {
    enum class Origin : uint8_t {
        Discovered,
        Incumbent,
    };

    int points = 0;
    std::vector<SolverAction> actions;
    // Persistent mapped search stores this compact reverse link instead of a
    // duplicate full action path. Zero is also the root trace sentinel.
    uint32_t trace_id = 0;
    Origin origin = Origin::Discovered;
};

enum class HybridSkeletonOrder : uint8_t {
    Score,
    Cohort,
    Interleave,
    StructureInterleave,
};

enum class PersistentSearchOrder : uint8_t {
    Depth,
    Beam,
};

class GameSolver {
public:
    explicit GameSolver(
        const Player& start_player,
        bool _merch_on = true,
        bool _witch_on = true,
        int _array_limit = 1000000,
        int _print_counter = 1000,
        std::string checkpoint_file = "",
        uint64_t _memory_limit_mb = 0,
        PurchaseProfile _purchase_profile = PurchaseProfile::Strategic,
        uint8_t _optimistic_share_percent = 25,
        bool _sell_action_aware_beam = false,
        uint8_t _diversity_share_percent = 0
    );

    explicit GameSolver(
        town_options town = t_Normalia,
        bool _merch_on = true,
        bool _witch_on = true,
        int _array_limit = 1000000,
        int _print_counter = 1000,
        const std::vector<ga_data>& que = {},
        std::string checkpoint_file = "",
        uint64_t _memory_limit_mb = 0,
        PurchaseProfile _purchase_profile = PurchaseProfile::Strategic,
        uint8_t _optimistic_share_percent = 25,
        bool _sell_action_aware_beam = false,
        uint8_t _diversity_share_percent = 0,
        int _merchant_day = 0,
        int _witch_day = 0
    );

    void find_solve_depth(bool main_thread = true);
    void find_solve_proof_depth(size_t transposition_limit, bool main_thread = true);
    void find_solve_mapped(
        const std::filesystem::path& database_path,
        size_t node_budget = 0,
        PersistentSearchOrder order = PersistentSearchOrder::Depth,
        bool main_thread = true
    );
    void warm_purchase_trees();
    [[nodiscard]] int best_points() const;
    [[nodiscard]] int best_discovered_points() const;
    [[nodiscard]] int incumbent_points() const;
    [[nodiscard]] const std::vector<FinishedSolve>& best_solves() const;
    [[nodiscard]] const SolverProfiling& profiling_stats() const;
    void print_benchmark_json(std::ostream& out, double campaign_elapsed_seconds = -1.0) const;
    void seed_incumbent(int points, std::vector<SolverAction> actions);
    void set_time_limit_seconds(uint64_t seconds) noexcept;
    void set_forced_merchant_option(std::optional<merch_options> option) noexcept;
    void set_forced_witch_option(std::optional<witch_options> option) noexcept;
    void set_hybrid_batch_size(size_t states) noexcept;
    void set_hybrid_suffix_limit(size_t states) noexcept;
    void set_hybrid_split_day(int day) noexcept;
    void set_hybrid_skeleton_order(HybridSkeletonOrder order) noexcept;
    void set_skeleton_share_percent(uint8_t percent) noexcept;
    void set_skeleton_depth(uint8_t actions) noexcept;
    void set_continuation_lane_share_percent(uint8_t percent) noexcept;
    void set_skeleton_queue_day(int day) noexcept;
    void set_refinement_template(
        const std::vector<SolverAction>& actions,
        uint8_t share_percent = 50,
        uint8_t maximum_mismatches = 1
    );
    void set_route_survival_audit(std::vector<SolverAction> route);
    [[nodiscard]] const RouteSurvivalAudit* route_survival_audit() const noexcept;

private:
    static constexpr size_t MAX_FINISHED_TO_KEEP = 3;
    static constexpr auto HEARTBEAT_INTERVAL = std::chrono::seconds(5);

    SolverState start_state;
    BestPurchaseHandler handler;
    bool merch_on = true;
    bool witch_on = true;
    PurchaseProfile purchase_profile = PurchaseProfile::Strategic;
    uint8_t optimistic_share_percent = 25;
    uint8_t diversity_share_percent = 0;
    bool sell_action_aware_beam = false;
    uint8_t skeleton_share_percent = 0;
    uint8_t skeleton_depth = 8;
    uint8_t continuation_lane_share_percent = 5;
    int skeleton_queue_day = 0;
    uint8_t refinement_share_percent = 0;
    uint8_t refinement_maximum_mismatches = 1;
    std::optional<EarlyActionSkeleton> refinement_template;
    std::optional<RouteSurvivalAudit> survival_audit;
    std::unordered_map<uint64_t, ContinuationMatrixRow> continuation_score_cache;
    int array_limit = 0;
    int print_counter = 0;
    uint64_t memory_limit_bytes = 0;
    uint64_t time_limit_seconds = 0;
    size_t hybrid_batch_size = 0;
    size_t hybrid_suffix_limit = 0;
    int hybrid_split_day = 0;
    HybridSkeletonOrder hybrid_skeleton_order = HybridSkeletonOrder::Score;
    std::optional<merch_options> forced_merchant_option;
    std::optional<witch_options> forced_witch_option;
    size_t prefix_actions = 0;
    const int merch_days[3] = {5, 6, 7};
    const int witch_days[3] = {9, 10, 11};
    const int traveler_merch_days[3] = {7, 8, 9};
    const int traveler_witch_days[3] = {11, 12, 13};
    std::filesystem::path checkpoint_path;
    std::vector<SolverState> current_layer;
    std::vector<TraceNode> traces;
    std::vector<FinishedSolve> finished;
    SolverProfiling profiling;
    std::chrono::steady_clock::time_point started_at;
    std::chrono::steady_clock::time_point last_status_at;
    bool stop_requested = false;
    bool proof_search_active = false;
    PersistentSearchOrder mapped_search_order = PersistentSearchOrder::Depth;
    std::string stop_reason;
    bool measuring_skeleton_queue_family = false;
    std::vector<FinishedSolve>* active_expansion_finishes = nullptr;
    long double active_skeleton_queue_ceiling = 0.0L;
    int active_skeleton_queue_best_points = std::numeric_limits<int>::min();
    const std::vector<SolverState>* pending_states_for_budget = nullptr;
    const std::vector<SolverState>* waiting_states_for_budget = nullptr;
    uint64_t last_resident_bytes = 0;
    uint64_t last_memory_estimated_bytes = 0;
    uint64_t memory_checks_since_exact_scan = 0;
    uint64_t memory_checks_since_resident_sample = 0;

    void initialize(const Player& start_player, uint64_t memory_limit_mb_override);
    [[nodiscard]] static uint64_t detect_available_memory_bytes();
    [[nodiscard]] static uint64_t resident_memory_bytes();
    [[nodiscard]] static int active_day(const SolverState& state);
    [[nodiscard]] static std::string format_action(const SolverAction& action);
    [[nodiscard]] std::vector<SolverAction> rebuild_actions(uint32_t trace_id) const;
    [[nodiscard]] uint64_t estimated_memory_bytes() const;
    [[nodiscard]] uint64_t estimated_memory_bytes(
        const std::vector<SolverState>* same_day_candidates,
        const std::vector<SolverState>* next_day_candidates,
        size_t projected_concrete_states = 0
    ) const;
    [[nodiscard]] long double optimistic_final_bound(const SolverState& state) const;
    [[nodiscard]] long double optimistic_trade_profit_for_day(const SolverState& state, long double available_gold, const std::array<int, 6>& sell_prices) const;
    [[nodiscard]] size_t estimated_buy_branch_states(const Player& current) const;
    [[nodiscard]] long double beam_score(const SolverState& state) const;
    [[nodiscard]] bool can_still_reach_merchant_bonus(const SolverState& state) const;
    [[nodiscard]] bool merchant_option_allowed(merch_options option) const noexcept;
    [[nodiscard]] bool witch_option_allowed(witch_options option) const noexcept;
    [[nodiscard]] bool deferred_player_action_reachable(const SolverState& state) const noexcept;
    [[nodiscard]] RouteAuditQuantityStatus classify_route_audit_quantity(
        int16_t progress,
        const DeferredSearchState& deferred
    );
    [[nodiscard]] std::array<int, 6> best_future_sell_prices(const SolverState& state) const;
    void request_stop(const std::string& reason);
    bool enforce_memory_budget(
        const std::vector<SolverState>& same_day_candidates,
        const std::vector<SolverState>& next_day_candidates,
        size_t projected_concrete_states = 0
    );
    bool enforce_memory_budget(
        const FrontierAccumulator& same_day_candidates,
        const FrontierAccumulator& next_day_candidates,
        size_t projected_concrete_states = 0
    );
    void print_status(int active_day_value, size_t pending_same_day, size_t next_day_candidates) const;
    void print_solves() const;
    void sort_and_limit_finished();
    void record_finished(const SolverState& state);
    void append_state(
        FrontierAccumulator& target,
        SolverState state,
        const SolverAction& action,
        uint32_t parent_trace,
        int16_t parent_audit_progress
    );
    [[nodiscard]] uint32_t append_trace(uint32_t parent_trace, const SolverAction& action);
    [[nodiscard]] uint32_t append_trace_actions(uint32_t parent_trace, const std::vector<SolverAction>& actions);
    [[nodiscard]] uint64_t skeleton_key(const SolverState& state) const noexcept;
    [[nodiscard]] EarlyActionSkeleton early_skeleton(const SolverState& state) const noexcept;
    [[nodiscard]] static uint64_t continuation_structure_key(const SolverState& state) noexcept;
    void append_open_purchase(
        FrontierAccumulator& target,
        DeferredPurchaseModel purchase,
        const SolverAction& generated_action,
        int16_t parent_audit_progress
    );
    void append_sold_transition(
        FrontierAccumulator& target,
        DeferredSoldModel sold,
        const SolverAction& action,
        int16_t parent_audit_progress
    );
    void count_generated_action(const SolverAction& action);
    [[nodiscard]] ContinuationMatrixRow continuation_matrix_row(const SolverState& state);
    void reduce_to_beam_limit(std::vector<SolverState>& states);
    [[nodiscard]] std::vector<SolverState> finish_generated_frontier(
        FrontierAccumulator& accumulator
    );
    void trim_generated_frontier(FrontierAccumulator& accumulator);
    void observe_route_audit_ranks(const std::vector<SolverState>& states);
    void trim_to_beam_limit(std::vector<SolverState>& states);
    std::vector<SolverState> compress_layer(
        std::vector<SolverState> states,
        int active_day_value,
        std::vector<SolverState>* preserved_states = nullptr,
        std::vector<SolverState>* additional_preserved_states = nullptr,
        bool already_exact = false,
        bool compact_trace_arena = true
    );
    [[nodiscard]] bool should_start_hybrid_search(const std::vector<SolverState>& frontier) const;
    [[nodiscard]] bool run_layered_frontier(
        bool main_thread,
        std::vector<SolverState>* waiting_frontier,
        bool allow_hybrid_split,
        int& last_active_day
    );
    void drain_hybrid_frontier(bool main_thread, int& last_active_day);
    void expand_state(const SolverState& state, int active_day_value, FrontierAccumulator& same_day, FrontierAccumulator& next_day);
    void expand_deferred_state(const SolverState& state, int active_day_value, FrontierAccumulator& same_day, FrontierAccumulator& next_day);
    void expand_open_purchase(const SolverState& state, int active_day_value, FrontierAccumulator& same_day, FrontierAccumulator& next_day);
    void expand_sold_ledger(const SolverState& state, int active_day_value, FrontierAccumulator& same_day, FrontierAccumulator& next_day);
    std::vector<SolverState> generate_buy_successors(const Player& current);
    static bool buy_state_dominates(const Player& lhs, const Player& rhs);
};

#endif //TRADING_CARAVAN_SOLVER_GAMESOLVER_H
