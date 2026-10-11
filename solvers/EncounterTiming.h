#ifndef TRADING_CARAVAN_SOLVER_ENCOUNTERTIMING_H
#define TRADING_CARAVAN_SOLVER_ENCOUNTERTIMING_H

#include "../player/player.h"

struct EncounterOpportunity {
    bool due = false;
    bool may_defer = false;
};

class EncounterTiming {
public:
    [[nodiscard]] static EncounterOpportunity merchant(const Player& player) noexcept;
    [[nodiscard]] static EncounterOpportunity witch(const Player& player) noexcept;
};

#endif // TRADING_CARAVAN_SOLVER_ENCOUNTERTIMING_H
