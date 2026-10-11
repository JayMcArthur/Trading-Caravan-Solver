#ifndef TRADING_CARAVAN_SOLVER_FRONTIERREDUCER_H
#define TRADING_CARAVAN_SOLVER_FRONTIERREDUCER_H

#include "FrontierAccumulator.h"

class FrontierReducer {
public:
    // Removes only future-equivalent or provably dominated states. Beam
    // selection remains a separate, explicitly lossy step in GameSolver.
    [[nodiscard]] static ExactFrontierReduction reduce_exact(
        std::vector<SolverState> states,
        RouteSurvivalAudit* audit = nullptr
    );
};

#endif // TRADING_CARAVAN_SOLVER_FRONTIERREDUCER_H
