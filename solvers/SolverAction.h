#ifndef TRADING_CARAVAN_SOLVER_SOLVERACTION_H
#define TRADING_CARAVAN_SOLVER_SOLVERACTION_H

#include <cstdint>

#include "../consts/enums.h"

struct SolverAction {
    game_actions action = _a_sell;
    buy_list buy = buy_list();
    uint8_t option = 0;
};

#endif // TRADING_CARAVAN_SOLVER_SOLVERACTION_H
