#ifndef TRADING_CARAVAN_SOLVER_ADMISSIBLEUPPERBOUND_H
#define TRADING_CARAVAN_SOLVER_ADMISSIBLEUPPERBOUND_H

#include <cstdint>

struct SolverState;

// A proof-search bound, deliberately separate from beam-ranking heuristics.
// The returned value is a ceiling on Game::check_points, whose public result
// type is int. It is deliberately a relaxation, not a ranking heuristic: it
// may overestimate heavily, but must never fall below a legal final score.
class AdmissibleUpperBound {
public:
    [[nodiscard]] static int64_t score_ceiling(const SolverState& state) noexcept;
};

#endif // TRADING_CARAVAN_SOLVER_ADMISSIBLEUPPERBOUND_H
