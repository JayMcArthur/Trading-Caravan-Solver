#include "DeferredPurchaseOracle.h"

#include <algorithm>
#include <cassert>
#include <optional>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "../game/Game.h"
#include "../solvers/DeferredGoldFunction.h"

namespace {
struct Market {
    int food_price = 1;
    int food_weight = 1;
    int cargo_price = 3;
    int cargo_weight = 2;
    int cargo_sell = 5;
};

struct World {
    int income = 2;
    int food_rate = 1;
    int capacity = 8;
    int interest_hundredths = 150;
};

struct BuyChoice {
    int food = 0;
    int cargo = 0;
};

struct ConcreteState {
    int gold = 0;
    int food = 0;
    int cargo = 0;
    std::vector<BuyChoice> history;
};

struct OriginRow {
    DeferredGoldFunction gold = DeferredGoldFunction::open(0, 0, 0);
    int food_before_buy = 0;
    std::vector<BuyChoice> history;
};

struct SymbolicMarker {
    Market market;
    int capacity_at_buy = 0;
    int cumulative_food_used = 0;
    int maximum_prefix_food_used = 0;
    std::vector<OriginRow> origins;
};

struct HarnessState {
    Market market;
    World world;
    std::vector<ConcreteState> explicit_states;
    std::optional<SymbolicMarker> symbolic;
    std::vector<ConcreteState> symbolic_sold;
    bool bought_last = true;
    bool sold = false;
};

enum class Action {
    NoCost,
    HireTrader,
    HireCamel,
    Interest,
    NextBuy,
    Sell,
};

int game_interest(const int gold, const int hundredths) {
    Player player;
    player.gold = gold;
    player.trader = 0;
    player.daily_income = 0;
    player.interest_rate = static_cast<double>(hundredths) / 100.0;
    Game::event_interest(player);
    return player.gold;
}

std::vector<BuyChoice> enumerate_buys(
    const int gold,
    const int existing_food,
    const int capacity,
    const Market& market
) {
    std::vector<BuyChoice> choices;
    for (int food = 0; food <= capacity; ++food) {
        for (int cargo = 0; cargo <= capacity; ++cargo) {
            const int cost = food * market.food_price + cargo * market.cargo_price;
            const int weight = (existing_food + food) * market.food_weight +
                               cargo * market.cargo_weight;
            if (cost <= gold && weight <= capacity) {
                choices.push_back(BuyChoice{food, cargo});
            }
        }
    }
    return choices;
}

std::vector<ConcreteState> physical_frontier(std::vector<ConcreteState> states) {
    std::vector<ConcreteState> frontier;
    for (ConcreteState& candidate : states) {
        bool dominated = false;
        for (const ConcreteState& other : states) {
            if (&candidate == &other || candidate.cargo != other.cargo) {
                continue;
            }
            if (other.gold >= candidate.gold && other.food >= candidate.food &&
                (other.gold > candidate.gold || other.food > candidate.food)) {
                dominated = true;
                break;
            }
        }
        if (!dominated) {
            const auto duplicate = std::find_if(frontier.begin(), frontier.end(), [&](const ConcreteState& existing) {
                return existing.gold == candidate.gold && existing.food == candidate.food &&
                       existing.cargo == candidate.cargo;
            });
            if (duplicate == frontier.end()) {
                frontier.push_back(candidate);
            }
        }
    }
    std::sort(frontier.begin(), frontier.end(), [](const ConcreteState& lhs, const ConcreteState& rhs) {
        return std::tie(lhs.gold, lhs.food, lhs.cargo) < std::tie(rhs.gold, rhs.food, rhs.cargo);
    });
    return frontier;
}

SymbolicMarker open_marker(
    const std::vector<ConcreteState>& origins,
    const Market& market,
    const World& world
) {
    SymbolicMarker marker;
    marker.market = market;
    marker.capacity_at_buy = world.capacity;
    for (const ConcreteState& origin : origins) {
        marker.origins.push_back(OriginRow{
            DeferredGoldFunction::open(origin.gold, world.income, origin.gold),
            origin.food,
            origin.history,
        });
    }
    return marker;
}

std::vector<ConcreteState> explicit_buy(
    const std::vector<ConcreteState>& origins,
    const Market& market,
    const World& world
) {
    std::vector<ConcreteState> bought;
    for (const ConcreteState& origin : origins) {
        for (const BuyChoice choice : enumerate_buys(origin.gold, origin.food, world.capacity, market)) {
            const int spend = choice.food * market.food_price + choice.cargo * market.cargo_price;
            ConcreteState result{
                origin.gold - spend + world.income,
                origin.food + choice.food,
                choice.cargo,
                origin.history,
            };
            result.history.push_back(choice);
            bought.push_back(std::move(result));
        }
    }
    return physical_frontier(std::move(bought));
}

std::vector<ConcreteState> materialize(const SymbolicMarker& marker) {
    std::vector<ConcreteState> states;
    for (const OriginRow& origin : marker.origins) {
        for (const BuyChoice choice : enumerate_buys(
            origin.gold.maximum_original_spend(),
            origin.food_before_buy,
            marker.capacity_at_buy,
            marker.market
        )) {
            const int spend = choice.food * marker.market.food_price +
                              choice.cargo * marker.market.cargo_price;
            const int food = origin.food_before_buy + choice.food - marker.cumulative_food_used;
            if (food < 0 || spend > origin.gold.maximum_original_spend()) {
                continue;
            }
            ConcreteState state{
                origin.gold.evaluate(spend),
                food,
                choice.cargo,
                origin.history,
            };
            state.history.push_back(choice);
            states.push_back(std::move(state));
        }
    }
    return physical_frontier(std::move(states));
}

void assert_same_frontier(const HarnessState& state) {
    std::vector<ConcreteState> explicit_frontier = physical_frontier(state.explicit_states);
    std::vector<ConcreteState> symbolic_frontier = state.sold
        ? physical_frontier(state.symbolic_sold)
        : materialize(*state.symbolic);
    assert(explicit_frontier.size() == symbolic_frontier.size());
    for (size_t index = 0; index < explicit_frontier.size(); ++index) {
        assert(explicit_frontier[index].gold == symbolic_frontier[index].gold);
        assert(explicit_frontier[index].food == symbolic_frontier[index].food);
        assert(explicit_frontier[index].cargo == symbolic_frontier[index].cargo);
    }
}

bool origin_has_feasible_buy(const OriginRow& origin, const SymbolicMarker& marker) {
    for (const BuyChoice choice : enumerate_buys(
        origin.gold.maximum_original_spend(),
        origin.food_before_buy,
        marker.capacity_at_buy,
        marker.market
    )) {
        const int spend = choice.food * marker.market.food_price +
                          choice.cargo * marker.market.cargo_price;
        const int remaining_food = origin.food_before_buy + choice.food - marker.cumulative_food_used;
        if (spend <= origin.gold.maximum_original_spend() && remaining_food >= 0) {
            return true;
        }
    }
    return false;
}

void start_day(HarnessState& state) {
    for (ConcreteState& concrete : state.explicit_states) {
        concrete.food -= state.world.food_rate;
    }
    state.explicit_states.erase(
        std::remove_if(state.explicit_states.begin(), state.explicit_states.end(), [](const ConcreteState& concrete) {
            return concrete.food < 0;
        }),
        state.explicit_states.end()
    );

    SymbolicMarker& marker = *state.symbolic;
    marker.cumulative_food_used += state.world.food_rate;
    marker.maximum_prefix_food_used = std::max(
        marker.maximum_prefix_food_used,
        marker.cumulative_food_used
    );
    marker.origins.erase(
        std::remove_if(marker.origins.begin(), marker.origins.end(), [&](const OriginRow& origin) {
            return !origin_has_feasible_buy(origin, marker);
        }),
        marker.origins.end()
    );
}

void add_income(HarnessState& state) {
    for (ConcreteState& concrete : state.explicit_states) {
        concrete.gold += state.world.income;
    }
    for (OriginRow& origin : state.symbolic->origins) {
        origin.gold.add_gold(state.world.income);
    }
}

void apply_cost(HarnessState& state, const int cost) {
    state.explicit_states.erase(
        std::remove_if(state.explicit_states.begin(), state.explicit_states.end(), [&](const ConcreteState& concrete) {
            return concrete.gold < cost;
        }),
        state.explicit_states.end()
    );
    for (ConcreteState& concrete : state.explicit_states) {
        concrete.gold -= cost;
    }

    SymbolicMarker& marker = *state.symbolic;
    std::vector<OriginRow> affordable_origins;
    affordable_origins.reserve(marker.origins.size());
    for (OriginRow origin : marker.origins) {
        if (origin.gold.spend_if_affordable(cost) && origin_has_feasible_buy(origin, marker)) {
            affordable_origins.push_back(std::move(origin));
        }
    }
    marker.origins = std::move(affordable_origins);
}

std::optional<HarnessState> advance(const HarnessState& before, const Action action) {
    if (before.sold || !before.symbolic.has_value()) {
        return std::nullopt;
    }
    if (action == Action::NextBuy && before.bought_last) {
        return std::nullopt;
    }
    const std::vector<ConcreteState> current_symbolic = materialize(*before.symbolic);
    if (action == Action::Sell && std::none_of(current_symbolic.begin(), current_symbolic.end(), [](const ConcreteState& state) {
        return state.cargo > 0;
    })) {
        return std::nullopt;
    }

    HarnessState state = before;
    start_day(state);
    if (state.explicit_states.empty() || state.symbolic->origins.empty()) {
        assert_same_frontier(state);
        return state;
    }

    switch (action) {
        case Action::NoCost:
            add_income(state);
            state.bought_last = false;
            break;
        case Action::HireTrader:
            apply_cost(state, 3);
            state.world.food_rate += 1;
            state.world.income += 1;
            add_income(state);
            state.bought_last = false;
            break;
        case Action::HireCamel:
            apply_cost(state, 4);
            state.world.food_rate += 1;
            state.world.capacity += 3;
            add_income(state);
            state.bought_last = false;
            break;
        case Action::Interest:
            for (ConcreteState& concrete : state.explicit_states) {
                concrete.gold = game_interest(concrete.gold, state.world.interest_hundredths);
            }
            for (OriginRow& origin : state.symbolic->origins) {
                origin.gold.apply_interest(state.world.interest_hundredths);
            }
            add_income(state);
            state.bought_last = false;
            break;
        case Action::NextBuy: {
            std::vector<ConcreteState> explicit_origins;
            for (ConcreteState concrete : state.explicit_states) {
                if (concrete.cargo == 0) {
                    explicit_origins.push_back(std::move(concrete));
                }
            }
            explicit_origins = physical_frontier(std::move(explicit_origins));

            std::vector<ConcreteState> symbolic_origins;
            for (ConcreteState concrete : materialize(*state.symbolic)) {
                if (concrete.cargo == 0) {
                    symbolic_origins.push_back(std::move(concrete));
                }
            }
            symbolic_origins = physical_frontier(std::move(symbolic_origins));

            state.explicit_states = explicit_buy(explicit_origins, state.market, state.world);
            state.symbolic = open_marker(symbolic_origins, state.market, state.world);
            state.bought_last = true;
            break;
        }
        case Action::Sell: {
            std::vector<ConcreteState> explicit_sold;
            for (ConcreteState concrete : state.explicit_states) {
                if (concrete.cargo > 0) {
                    concrete.gold += concrete.cargo * state.market.cargo_sell + state.world.income;
                    concrete.cargo = 0;
                    explicit_sold.push_back(std::move(concrete));
                }
            }
            std::vector<ConcreteState> symbolic_sold;
            for (ConcreteState concrete : materialize(*state.symbolic)) {
                if (concrete.cargo > 0) {
                    concrete.gold += concrete.cargo * state.market.cargo_sell + state.world.income;
                    concrete.cargo = 0;
                    symbolic_sold.push_back(std::move(concrete));
                }
            }
            state.explicit_states = physical_frontier(std::move(explicit_sold));
            state.symbolic_sold = physical_frontier(std::move(symbolic_sold));
            state.symbolic.reset();
            state.sold = true;
            state.bought_last = false;
            break;
        }
    }

    assert_same_frontier(state);
    return state;
}

HarnessState initial_state(const Market market, const World world, const int gold, const int food) {
    const ConcreteState origin{gold, food, 0, {}};
    HarnessState state;
    state.market = market;
    state.world = world;
    state.explicit_states = explicit_buy({origin}, market, world);
    state.symbolic = open_marker({origin}, market, world);
    assert_same_frontier(state);
    return state;
}

void walk(const HarnessState& state, const int depth, size_t& checked) {
    ++checked;
    if (depth == 0 || state.sold || state.explicit_states.empty()) {
        return;
    }
    for (const Action action : {
        Action::NoCost,
        Action::HireTrader,
        Action::HireCamel,
        Action::Interest,
        Action::NextBuy,
        Action::Sell,
    }) {
        const auto next = advance(state, action);
        if (next.has_value()) {
            walk(*next, depth - 1, checked);
        }
    }
}
}

void run_deferred_purchase_oracle_tests() {
    size_t checked = 0;
    for (const int gold : {8, 13, 21}) {
        for (const int interest_hundredths : {0, 55, 135, 280}) {
            for (const int capacity : {6, 9}) {
                Market market;
                market.food_price = gold % 3 + 1;
                market.cargo_price = gold % 4 + 2;
                market.cargo_sell = market.cargo_price + 3;
                World world;
                world.capacity = capacity;
                world.interest_hundredths = interest_hundredths;
                walk(initial_state(market, world, gold, 1), 5, checked);
            }
        }
    }
    assert(checked > 1000);
}
