#include "FrontierReducer.h"

#include <utility>

ExactFrontierReduction FrontierReducer::reduce_exact(
    std::vector<SolverState> states,
    RouteSurvivalAudit* audit
) {
    FrontierAccumulator accumulator(audit);
    for (SolverState& state : states) {
        accumulator.insert(std::move(state));
    }
    return accumulator.finish();
}
