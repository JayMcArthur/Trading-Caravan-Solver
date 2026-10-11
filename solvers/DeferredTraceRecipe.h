#ifndef TRADING_CARAVAN_SOLVER_DEFERREDTRACERECIPE_H
#define TRADING_CARAVAN_SOLVER_DEFERREDTRACERECIPE_H

#include <cstddef>
#include <memory>
#include <unordered_set>
#include <vector>

#include "SolverAction.h"

class DeferredTraceRecipe {
public:
    void append(const SolverAction& action);

    [[nodiscard]] std::vector<SolverAction> resolve(
        const buy_list& concrete_buy,
        const SolverAction* final_action = nullptr
    ) const;

    [[nodiscard]] std::vector<SolverAction> actions_after_buy() const;
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] size_t memory_bytes() const noexcept;
    [[nodiscard]] size_t dynamic_memory_bytes(
        std::unordered_set<const void*>& counted_shared
    ) const noexcept;
    [[nodiscard]] bool operator==(const DeferredTraceRecipe& other) const noexcept;

private:
    struct Node {
        SolverAction action;
        std::shared_ptr<const Node> previous;
        size_t depth = 0;
    };

    std::shared_ptr<const Node> tail;
};

#endif // TRADING_CARAVAN_SOLVER_DEFERREDTRACERECIPE_H
