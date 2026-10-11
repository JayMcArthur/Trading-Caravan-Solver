#ifndef TRADING_CARAVAN_SOLVER_VALIDATEDROUTEREPLAY_H
#define TRADING_CARAVAN_SOLVER_VALIDATEDROUTEREPLAY_H

#include <cstddef>
#include <string>
#include <vector>

#include "../consts/enums.h"
#include "../player/player.h"

struct RouteReplayResult {
    bool legal = false;
    int score = 0;
    size_t failed_action = 0;
    std::string reason;
    Player final_player;
    std::vector<std::string> action_log;
};

class ValidatedRouteReplay {
public:
    [[nodiscard]] static RouteReplayResult run(
        town_options town,
        const std::vector<ga_data>& route
    );

};

#endif // TRADING_CARAVAN_SOLVER_VALIDATEDROUTEREPLAY_H
