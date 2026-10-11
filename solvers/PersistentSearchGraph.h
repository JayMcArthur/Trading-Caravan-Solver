#ifndef TRADING_CARAVAN_SOLVER_PERSISTENTSEARCHGRAPH_H
#define TRADING_CARAVAN_SOLVER_PERSISTENTSEARCHGRAPH_H

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "GameSolver.h"

struct PersistentSearchNode {
    int64_t id = 0;
    SolverState state;
};

enum class PersistentNodeStatus : uint8_t {
    Open,
    Partial,
    Solved,
};

struct PersistentSearchNodeSummary {
    int64_t id = 0;
    PersistentNodeStatus status = PersistentNodeStatus::Open;
    int active_day = 0;
    double priority = 0.0;
};

struct PersistentSearchStats {
    uint64_t nodes = 0;
    uint64_t open_nodes = 0;
    uint64_t partial_nodes = 0;
    uint64_t solved_nodes = 0;
    uint64_t edges = 0;
    uint64_t terminal_routes = 0;
    uint64_t trace_links = 0;
    uint64_t expanded_this_run = 0;
    uint64_t reused_children_this_run = 0;
    bool root_solved = false;
    std::optional<int> root_best_score;
    uint64_t database_bytes = 0;
};

// Disk-backed exact graph. node_index is deliberately compact and hot;
// semantic keys, snapshots, edges, traces, and paths live in cold tables.
class PersistentSearchGraph {
public:
    PersistentSearchGraph(
        const std::filesystem::path& database_path,
        const std::vector<uint8_t>& scenario_key,
        const std::string& scenario_label,
        const SolverState& root
    );
    ~PersistentSearchGraph();
    PersistentSearchGraph(PersistentSearchGraph&&) noexcept;
    PersistentSearchGraph& operator=(PersistentSearchGraph&&) noexcept;
    PersistentSearchGraph(const PersistentSearchGraph&) = delete;
    PersistentSearchGraph& operator=(const PersistentSearchGraph&) = delete;

    [[nodiscard]] std::optional<PersistentSearchNode> next_open() const;
    [[nodiscard]] PersistentSearchNodeSummary root() const;
    [[nodiscard]] std::vector<PersistentSearchNodeSummary> unresolved_children(
        int64_t node_id
    ) const;
    [[nodiscard]] std::optional<SolverState> load_state(int64_t node_id) const;
    [[nodiscard]] std::optional<PersistentSearchNode> load_open(int64_t node_id) const;
    // Load only this node's trace ancestors and deferred provenances into a
    // compact process-local arena, remapping the state to those local IDs.
    void localize_traces(
        PersistentSearchNode& node,
        std::vector<TraceNode>& traces
    );
    void commit_expansion(
        int64_t node_id,
        const std::vector<SolverState>& children,
        const std::vector<double>& priorities,
        const std::vector<FinishedSolve>& terminal_routes,
        const std::vector<TraceNode>& traces
    );
    [[nodiscard]] std::vector<FinishedSolve> best_routes(size_t limit) const;
    [[nodiscard]] PersistentSearchStats stats() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif // TRADING_CARAVAN_SOLVER_PERSISTENTSEARCHGRAPH_H
