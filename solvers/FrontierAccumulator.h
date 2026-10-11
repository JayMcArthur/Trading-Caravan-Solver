#ifndef TRADING_CARAVAN_SOLVER_FRONTIERACCUMULATOR_H
#define TRADING_CARAVAN_SOLVER_FRONTIERACCUMULATOR_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "GameSolver.h"

class RouteSurvivalAudit;

struct ExactFrontierReduction {
    std::vector<SolverState> states;
    size_t duplicate_prunes = 0;
    size_t dominance_prunes = 0;
    size_t deferred_dominance_checks = 0;
    uint64_t deferred_partition_rejections = 0;
    size_t deferred_purchase_dominance_prunes = 0;
    size_t deferred_sold_dominance_prunes = 0;
    uint64_t deferred_gold_function_comparisons = 0;
    uint64_t deferred_exact_spend_evaluations = 0;
    uint64_t deferred_origin_coverage_checks = 0;
    uint64_t deferred_cross_shape_dominance_prunes = 0;
};

class FrontierAccumulator {
public:
    explicit FrontierAccumulator(RouteSurvivalAudit* audit = nullptr);
    ~FrontierAccumulator();
    FrontierAccumulator(FrontierAccumulator&&) noexcept;
    FrontierAccumulator& operator=(FrontierAccumulator&&) noexcept;
    FrontierAccumulator(const FrontierAccumulator&) = delete;
    FrontierAccumulator& operator=(const FrontierAccumulator&) = delete;

    void insert(SolverState state);
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] uint64_t memory_bytes() const;
    [[nodiscard]] ExactFrontierReduction finish();

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif // TRADING_CARAVAN_SOLVER_FRONTIERACCUMULATOR_H
