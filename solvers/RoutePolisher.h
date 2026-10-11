#ifndef TRADING_CARAVAN_SOLVER_ROUTEPOLISHER_H
#define TRADING_CARAVAN_SOLVER_ROUTEPOLISHER_H

#include <cstddef>
#include <vector>

#include "ValidatedRouteReplay.h"

struct RoutePolishSettings {
    size_t beam_width = 64;
    size_t maximum_rounds = 10;
    size_t maximum_evaluations = 250000;
};

struct RoutePolishResult {
    std::vector<ga_data> route;
    RouteReplayResult replay;
    size_t evaluated_routes = 0;
    size_t legal_routes = 0;
    size_t completed_rounds = 0;
};

class RoutePolisher {
public:
    [[nodiscard]] static RoutePolishResult polish(
        town_options town,
        const std::vector<std::vector<ga_data>>& seeds,
        const RoutePolishSettings& settings = {}
    );
};

#endif // TRADING_CARAVAN_SOLVER_ROUTEPOLISHER_H
