#include "RoutePolisher.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_set>
#include <utility>

#include "../game/Game.h"
#include "../purchaseHandler/BestPurchaseHandler.h"

namespace {
struct Candidate {
    std::vector<ga_data> route;
    int score = 0;
    std::string key;
};

std::string route_key(const std::vector<ga_data>& route) {
    std::string key;
    key.reserve(route.size() * 8);
    for (const ga_data& action : route) {
        key.push_back(static_cast<char>(action.action));
        switch (action.action) {
            case _a_buy:
                key.push_back(static_cast<char>(action.buy_data.food));
                key.push_back(static_cast<char>(action.buy_data.spice));
                key.push_back(static_cast<char>(action.buy_data.pottery));
                key.push_back(static_cast<char>(action.buy_data.marble));
                key.push_back(static_cast<char>(action.buy_data.silk));
                key.push_back(static_cast<char>(action.buy_data.jewelry));
                break;
            case _a_npc:
                key.push_back(static_cast<char>(action.npc_data));
                break;
            case _a_merchant:
                key.push_back(static_cast<char>(action.merch_data));
                break;
            case _a_witch:
                key.push_back(static_cast<char>(action.witch_data));
                break;
            case _a_disaster:
                key.push_back(static_cast<char>(action.disaster_data));
                break;
            case _a_sell:
            case _a_interest:
            case _a_wait:
                break;
        }
        key.push_back(static_cast<char>(0xff));
    }
    return key;
}

bool candidate_better(const Candidate& lhs, const Candidate& rhs) {
    if (lhs.score != rhs.score) {
        return lhs.score > rhs.score;
    }
    return lhs.key < rhs.key;
}

std::string strategy_key(const std::vector<ga_data>& route) {
    std::string key;
    key.reserve(route.size() * 2);
    for (const ga_data& action : route) {
        key.push_back(static_cast<char>(action.action));
        switch (action.action) {
            case _a_buy:
                key.push_back(action.buy_data.spice || action.buy_data.pottery ||
                    action.buy_data.marble || action.buy_data.silk || action.buy_data.jewelry);
                break;
            case _a_npc:
                key.push_back(static_cast<char>(action.npc_data));
                break;
            case _a_merchant:
                key.push_back(static_cast<char>(action.merch_data));
                break;
            case _a_witch:
                key.push_back(static_cast<char>(action.witch_data));
                break;
            case _a_disaster:
                key.push_back(static_cast<char>(action.disaster_data));
                break;
            case _a_sell:
            case _a_interest:
            case _a_wait:
                key.push_back(0);
                break;
        }
    }
    return key;
}

void retain_diverse_candidates(std::vector<Candidate>& candidates, const size_t width) {
    if (candidates.size() <= width) {
        return;
    }
    const size_t score_slots = std::max<size_t>(1, width / 2);
    std::unordered_set<std::string> strategies;
    std::vector<Candidate> retained;
    retained.reserve(width);
    std::vector<size_t> skipped;
    for (size_t index = 0; index < candidates.size(); ++index) {
        const bool distinct = strategies.insert(strategy_key(candidates[index].route)).second;
        if (index < score_slots || distinct) {
            retained.emplace_back(std::move(candidates[index]));
            if (retained.size() == width) {
                break;
            }
        } else {
            skipped.push_back(index);
        }
    }
    for (const size_t index : skipped) {
        if (retained.size() == width) {
            break;
        }
        retained.emplace_back(std::move(candidates[index]));
    }
    std::sort(retained.begin(), retained.end(), candidate_better);
    candidates = std::move(retained);
}

bool same_action(const ga_data& lhs, const ga_data& rhs) {
    if (lhs.action != rhs.action) {
        return false;
    }
    switch (lhs.action) {
        case _a_buy:
            return lhs.buy_data.food == rhs.buy_data.food &&
                   lhs.buy_data.spice == rhs.buy_data.spice &&
                   lhs.buy_data.pottery == rhs.buy_data.pottery &&
                   lhs.buy_data.marble == rhs.buy_data.marble &&
                   lhs.buy_data.silk == rhs.buy_data.silk &&
                   lhs.buy_data.jewelry == rhs.buy_data.jewelry;
        case _a_npc:
            return lhs.npc_data == rhs.npc_data;
        case _a_merchant:
            return lhs.merch_data == rhs.merch_data;
        case _a_witch:
            return lhs.witch_data == rhs.witch_data;
        case _a_disaster:
            return lhs.disaster_data == rhs.disaster_data;
        case _a_sell:
        case _a_interest:
        case _a_wait:
            return true;
    }
    return false;
}

std::array<uint8_t*, 6> buy_fields(buy_list& buy) {
    return {&buy.food, &buy.spice, &buy.pottery, &buy.marble, &buy.silk, &buy.jewelry};
}

bool retime_food(const town_options town, std::vector<ga_data>& route) {
    for (ga_data& action : route) {
        if (action.action == _a_buy) {
            action.buy_data.food = 0;
        }
    }
    Player player;
    Game::apply_town(player, town);
    size_t last_buy = route.size();
    const auto supply = [&](const int minimum_food) {
        const int shortage = minimum_food - player.food;
        if (shortage <= 0) {
            return true;
        }
        if (last_buy == route.size() || route[last_buy].buy_data.food + shortage > 255) {
            return false;
        }
        route[last_buy].buy_data.food += shortage;
        player.food += shortage;
        return true;
    };

    for (size_t index = 0; index < route.size(); ++index) {
        if (!player.day_start_skip) {
            Game::start_of_day(player);
        } else {
            player.day_start_skip = false;
        }
        if (!supply(0)) {
            return false;
        }
        const ga_data& action = route[index];
        if (action.action == _a_npc) {
            if (!supply(player.food_consumption + player.npc_shop[action.npc_data][nd_Eat])) {
                return false;
            }
        } else if ((action.action == _a_sell || action.action == _a_interest) &&
                   player.day != player.max_day && !supply(player.food_consumption)) {
            return false;
        }
        if (action.action == _a_buy) {
            last_buy = index;
        }

        // Only derive the food schedule here. Real affordability, capacity,
        // encounter timing and completion are checked by strict replay below.
        player.gold = 1000000;
        player.day_start_skip = true; // The day's food was consumed above.
        Game::full_day(player, action);
    }
    return true;
}

bool optimize_cargo_before_sale(
    const town_options town,
    std::vector<ga_data>& route,
    BestPurchaseHandler& handler
) {
    Player player;
    Game::apply_town(player, town);
    for (size_t index = 0; index < route.size(); ++index) {
        if (!player.day_start_skip) {
            Game::start_of_day(player);
        } else {
            player.day_start_skip = false;
        }
        if (player.food < 0) {
            return false;
        }
        ga_data& action = route[index];
        if (action.action == _a_sell && !Game::check_has_items(player)) {
            return false;
        }
        if (action.action == _a_npc &&
            (player.gold < player.npc_shop[action.npc_data][nd_Cost] ||
             player.food < player.food_consumption + player.npc_shop[action.npc_data][nd_Eat])) {
            return false;
        }
        if (action.action == _a_buy) {
            if (player.day == player.max_day || player.bought_last || !Game::check_can_buy_items(player)) {
                return false;
            }
            size_t next = index + 1;
            while (next < route.size() &&
                   (route[next].action == _a_merchant || route[next].action == _a_witch ||
                    route[next].action == _a_npc)) {
                ++next;
            }
            if (next < route.size() && route[next].action == _a_sell) {
                const int food = action.buy_data.food;
                Player sale_market = player;
                sale_market.gold = 1000000;
                sale_market.day_start_skip = true;
                Game::full_day(sale_market, ga_data(_a_buy, food, 0, 0, 0, 0, 0));
                int net_cost = 0;
                int required_cash = 0;
                for (size_t event = index + 1; event < next; ++event) {
                    sale_market.gold = 1000000;
                    const ga_data& intervening = route[event];
                    const int hire_cost = intervening.action == _a_npc ?
                        sale_market.npc_shop[intervening.npc_data][nd_Cost] : 0;
                    Game::full_day(sale_market, route[event]);
                    const int delta = 1000000 - sale_market.gold;
                    // Hires must be paid before that day's income arrives.
                    const int upfront_cost = intervening.action == _a_npc ? hire_cost : delta;
                    required_cash = std::max(required_cash, net_cost + upfront_cost);
                    net_cost += delta;
                }
                const int buy_income = player.daily_income +
                    player.trader * player.npc_shop[n_Trader][nd_Effect];
                const int reserve = std::max(0, required_cash - buy_income);
                const int budget = player.gold - food * player.item_shop[i_Food][id_Buy] - reserve;
                const int capacity = Game::check_weight_left(player) -
                    food * player.item_shop[i_Food][id_Weight];
                if (budget < 0 || capacity < 0 ||
                    budget > std::numeric_limits<uint16_t>::max() ||
                    capacity > std::numeric_limits<uint16_t>::max()) {
                    return false;
                }
                const auto choices = handler.getExactSaleFrontier(
                    static_cast<uint16_t>(budget), static_cast<uint16_t>(capacity),
                    player, sale_market
                );
                buy_list cargo;
                int best_profit = 0;
                for (const ExactPurchaseChoice& choice : *choices) {
                    const int profit = static_cast<int>(choice.sale_value) - choice.actual_cost;
                    if (profit > best_profit) {
                        best_profit = profit;
                        cargo = choice.buys;
                    }
                }
                cargo.food = static_cast<uint8_t>(food);
                action.buy_data = cargo;
            }
            const auto fields = buy_fields(action.buy_data);
            int cost = 0;
            for (size_t item = 0; item < fields.size(); ++item) {
                cost += *fields[item] * player.item_shop[item][id_Buy];
            }
            if (cost > player.gold) {
                return false;
            }
        }
        player.day_start_skip = true;
        Game::full_day(player, action);
    }
    return true;
}

template <typename Emit>
void emit_duration_change(const std::vector<ga_data>& route, const int extra_days, Emit& emit) {
    if (extra_days >= 0) {
        std::vector<ga_data> next = route;
        next.insert(next.end(), extra_days, ga_data(_a_interest));
        emit(std::move(next));
        return;
    }
    for (size_t index = 0; index < route.size(); ++index) {
        if (route[index].action == _a_merchant || route[index].action == _a_witch ||
            route[index].action == _a_disaster) {
            continue;
        }
        std::vector<ga_data> next = route;
        next.erase(next.begin() + index);
        emit_duration_change(next, extra_days + 1, emit);
    }
}

int witch_extra_days(const witch_options option) {
    return option == w_Time_is_Money ? 1 : option == w_Animal_Lover ? -1 : 0;
}

template <typename Emit>
void generate_neighbors(const std::vector<ga_data>& source, Emit&& emit) {
    constexpr std::array<int, 8> DELTAS{-8, -4, -2, -1, 1, 2, 4, 8};

    std::vector<size_t> buy_indices;
    for (size_t action_index = 0; action_index < source.size(); ++action_index) {
        const ga_data& action = source[action_index];
        if (action.action == _a_buy || action.action == _a_npc || action.action == _a_interest) {
            std::vector<ga_data> next = source;
            next[action_index] = ga_data(_a_sell);
            emit(std::move(next));
        }
        if (action.action == _a_buy) {
            buy_indices.push_back(action_index);
            if (action.buy_data.spice == 0 && action.buy_data.pottery == 0 &&
                action.buy_data.marble == 0 && action.buy_data.silk == 0 && action.buy_data.jewelry == 0) {
                for (const ga_data replacement : {
                    ga_data(_a_npc, n_Trader), ga_data(_a_npc, n_Camel), ga_data(_a_interest)
                }) {
                    std::vector<ga_data> next = source;
                    next[action_index] = replacement;
                    emit(std::move(next));
                }
            }
            for (size_t field = 0; field < 6; ++field) {
                for (const int delta : DELTAS) {
                    std::vector<ga_data> next = source;
                    auto fields = buy_fields(next[action_index].buy_data);
                    const int value = static_cast<int>(*fields[field]) + delta;
                    if (value < 0 || value > 255) {
                        continue;
                    }
                    *fields[field] = static_cast<uint8_t>(value);
                    emit(std::move(next));
                }
            }

            for (size_t from = 1; from < 6; ++from) {
                for (size_t to = 1; to < 6; ++to) {
                    if (from == to) {
                        continue;
                    }
                    for (const int amount : {1, 2}) {
                        std::vector<ga_data> next = source;
                        auto fields = buy_fields(next[action_index].buy_data);
                        if (*fields[from] < amount || *fields[to] > 255 - amount) {
                            continue;
                        }
                        *fields[from] = static_cast<uint8_t>(*fields[from] - amount);
                        *fields[to] = static_cast<uint8_t>(*fields[to] + amount);
                        emit(std::move(next));
                    }
                }
            }
        } else if (action.action == _a_npc) {
            std::vector<ga_data> next = source;
            next[action_index].npc_data = action.npc_data == n_Trader ? n_Camel : n_Trader;
            emit(std::move(next));
            next = source;
            next[action_index] = ga_data(_a_interest);
            emit(std::move(next));
            next = source;
            next[action_index] = ga_data(_a_buy, 0, 0, 0, 0, 0, 0);
            emit(std::move(next));
        } else if (action.action == _a_interest) {
            std::vector<ga_data> trader = source;
            trader[action_index] = ga_data(_a_npc, n_Trader);
            emit(std::move(trader));
            std::vector<ga_data> camel = source;
            camel[action_index] = ga_data(_a_npc, n_Camel);
            emit(std::move(camel));
            std::vector<ga_data> food = source;
            food[action_index] = ga_data(_a_buy, 0, 0, 0, 0, 0, 0);
            emit(std::move(food));
        } else if (action.action == _a_sell) {
            for (const ga_data replacement : {
                ga_data(_a_interest), ga_data(_a_npc, n_Trader),
                ga_data(_a_npc, n_Camel), ga_data(_a_buy, 0, 0, 0, 0, 0, 0)
            }) {
                std::vector<ga_data> next = source;
                next[action_index] = replacement;
                emit(std::move(next));
            }
        } else if (action.action == _a_merchant) {
            for (const merch_options option : ALL_MERCH_OPTIONS) {
                if (option == action.merch_data) {
                    continue;
                }
                std::vector<ga_data> next = source;
                next[action_index].merch_data = option;
                emit(next);
                const int extra_days = (option == m_Magic_Cleppsydra) -
                    (action.merch_data == m_Magic_Cleppsydra);
                if (extra_days != 0) {
                    emit_duration_change(next, extra_days, emit);
                }
            }
        } else if (action.action == _a_witch) {
            for (const witch_options option : ALL_WITCH_OPTIONS) {
                if (option == action.witch_data) {
                    continue;
                }
                std::vector<ga_data> next = source;
                next[action_index].witch_data = option;
                emit(next);
                const int extra_days = witch_extra_days(option) - witch_extra_days(action.witch_data);
                if (extra_days != 0) {
                    emit_duration_change(next, extra_days, emit);
                }
            }
        }
    }

    // Create both trading actions together; an isolated empty Sell or an
    // unsold cargo Buy cannot cross the cargo-lock rule on its own.
    for (size_t left = 0; left < source.size(); ++left) {
        const auto replaceable = [](const ga_data& action) {
            return action.action == _a_npc || action.action == _a_interest ||
                (action.action == _a_buy && action.buy_data.spice == 0 &&
                 action.buy_data.pottery == 0 && action.buy_data.marble == 0 &&
                 action.buy_data.silk == 0 && action.buy_data.jewelry == 0);
        };
        if (!replaceable(source[left])) {
            continue;
        }
        for (size_t right = left + 1; right < source.size(); ++right) {
            if (source[right].action == _a_merchant || source[right].action == _a_witch) {
                continue;
            }
            if (replaceable(source[right])) {
                std::vector<ga_data> next = source;
                next[left] = ga_data(_a_buy, 0, 0, 0, 0, 0, 0);
                next[right] = ga_data(_a_sell);
                emit(std::move(next));
            }
            if (source[right].action != _a_npc) {
                break;
            }
        }
    }

    for (size_t left = 0; left + 1 < source.size(); ++left) {
        if (same_action(source[left], source[left + 1])) {
            continue;
        }
        std::vector<ga_data> next = source;
        std::swap(next[left], next[left + 1]);
        emit(std::move(next));
    }

    // Long swaps let the polisher test a substantially different action order
    // without requiring every lower-scoring intermediate adjacent swap to
    // survive the beam. Exact replay still decides whether the result is legal.
    for (size_t left = 0; left < source.size(); ++left) {
        for (size_t right = left + 2; right < source.size(); ++right) {
            if (same_action(source[left], source[right])) {
                continue;
            }
            std::vector<ga_data> next = source;
            std::swap(next[left], next[right]);
            emit(std::move(next));
        }
    }

    for (size_t from_pos = 0; from_pos < buy_indices.size(); ++from_pos) {
        for (size_t to_pos = 0; to_pos < buy_indices.size(); ++to_pos) {
            if (from_pos == to_pos) {
                continue;
            }
            for (const int amount : {1, 2, 4, 8}) {
                std::vector<ga_data> next = source;
                buy_list& from_buy = next[buy_indices[from_pos]].buy_data;
                buy_list& to_buy = next[buy_indices[to_pos]].buy_data;
                if (from_buy.food < amount || to_buy.food > 255 - amount) {
                    continue;
                }
                from_buy.food = static_cast<uint8_t>(from_buy.food - amount);
                to_buy.food = static_cast<uint8_t>(to_buy.food + amount);
                emit(std::move(next));
            }
        }
    }

    // Moving one action preserves the relative order of the others. This can
    // cross order changes that no single swap makes legal, even with new food.
    for (size_t from = 0; from < source.size(); ++from) {
        for (size_t to = 0; to < source.size(); ++to) {
            if (from == to) {
                continue;
            }
            std::vector<ga_data> next = source;
            const ga_data moved = next[from];
            next.erase(next.begin() + from);
            next.insert(next.begin() + to, moved);
            emit(std::move(next));
        }
    }
}
}

RoutePolishResult RoutePolisher::polish(
    const town_options town,
    const std::vector<std::vector<ga_data>>& seeds,
    const RoutePolishSettings& settings
) {
    RoutePolishResult result;
    if (settings.beam_width == 0 || settings.maximum_rounds == 0 ||
        settings.maximum_evaluations == 0) {
        result.replay.reason = "route-polish limits must be greater than zero";
        return result;
    }

    std::unordered_set<std::string> visited;
    visited.reserve(std::min<size_t>(settings.maximum_evaluations, 500000));
    std::vector<Candidate> frontier;
    for (const auto& seed : seeds) {
        if (result.evaluated_routes >= settings.maximum_evaluations) {
            break;
        }
        RouteReplayResult replay = ValidatedRouteReplay::run(town, seed);
        Candidate candidate{seed, replay.score, route_key(seed)};
        ++result.evaluated_routes;
        if (!visited.insert(candidate.key).second || !replay.legal) {
            if (result.replay.reason.empty() && !replay.legal) {
                result.replay = std::move(replay);
            }
            continue;
        }
        ++result.legal_routes;
        frontier.emplace_back(std::move(candidate));
    }
    if (frontier.empty()) {
        return result;
    }
    Candidate best = frontier.front();
    for (const Candidate& candidate : frontier) {
        if (candidate.score > best.score) {
            best = candidate;
        }
    }
    std::sort(frontier.begin(), frontier.end(), candidate_better);
    if (frontier.size() > settings.beam_width) {
        frontier.resize(settings.beam_width);
    }
    BestPurchaseHandler cargo_handler(256);
    for (size_t round = 0; round < settings.maximum_rounds &&
         result.evaluated_routes < settings.maximum_evaluations; ++round) {
        std::vector<Candidate> next_frontier;
        const size_t expected = std::min<size_t>(
            settings.maximum_evaluations - result.evaluated_routes,
            settings.beam_width * 512
        );
        next_frontier.reserve(expected);

        const auto consider = [&](const std::vector<ga_data>& route) {
            if (result.evaluated_routes >= settings.maximum_evaluations) {
                return;
            }
            std::string key = route_key(route);
            if (!visited.insert(key).second) {
                return;
            }
            RouteReplayResult replay = ValidatedRouteReplay::run(town, route);
            ++result.evaluated_routes;
            if (!replay.legal) {
                return;
            }
            ++result.legal_routes;
            next_frontier.push_back(Candidate{
                route, replay.score, std::move(key)
            });
        };
        for (const Candidate& parent : frontier) {
            generate_neighbors(parent.route, [&](std::vector<ga_data> route) {
                consider(route);
                if (result.evaluated_routes < settings.maximum_evaluations && retime_food(town, route)) {
                    consider(route);
                    if (result.evaluated_routes < settings.maximum_evaluations &&
                        optimize_cargo_before_sale(town, route, cargo_handler)) {
                        consider(route);
                    }
                }
            });
            if (result.evaluated_routes >= settings.maximum_evaluations) {
                break;
            }
        }

        if (next_frontier.empty()) {
            break;
        }
        std::sort(next_frontier.begin(), next_frontier.end(), candidate_better);
        if (next_frontier.front().score > best.score) {
            best = next_frontier.front();
        }
        // Cargo quantity variants should not displace every other strategy.
        retain_diverse_candidates(next_frontier, settings.beam_width);
        frontier = std::move(next_frontier);
        result.completed_rounds = round + 1;
    }

    result.route = std::move(best.route);
    result.replay = ValidatedRouteReplay::run(town, result.route);
    return result;
}
