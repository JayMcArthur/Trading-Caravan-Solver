//
// Created by Jay on 2/4/2025.
//

#include "helpers.h"
#include "../game/Game.h"
#include <algorithm>
#include <unordered_set>

namespace {
uint64_t encode_buy(const buy_list& buy) {
    return static_cast<uint64_t>(buy.food) |
           (static_cast<uint64_t>(buy.spice) << 8U) |
           (static_cast<uint64_t>(buy.pottery) << 16U) |
           (static_cast<uint64_t>(buy.marble) << 24U) |
           (static_cast<uint64_t>(buy.silk) << 32U) |
           (static_cast<uint64_t>(buy.jewelry) << 40U);
}

bool is_empty_buy(const buy_list& buy) {
    return buy.food == 0 &&
           buy.spice == 0 &&
           buy.pottery == 0 &&
           buy.marble == 0 &&
           buy.silk == 0 &&
           buy.jewelry == 0;
}

std::vector<buy_list> find_buys_with_reserve(
    const Player& player,
    const Player& valuation,
    int food_needed,
    int reserve_gold,
    BestPurchaseHandler& handler,
    PurchaseProfile profile
);

std::vector<Player> purchase_valuations(
    const Player& current,
    const bool merchant_events_enabled,
    const bool witch_events_enabled,
    const PurchaseProfile profile
) {
    if (profile == PurchaseProfile::Greedy) {
        return {current};
    }
    std::vector<Player> merchant_worlds;
    merchant_worlds.reserve(2);
    merchant_worlds.push_back(current);

    const bool merchant_can_still_happen =
        merchant_events_enabled &&
        !current.merchant_happened &&
        (current.call_merchant || current.day < (current.merchant_day > 0
            ? current.merchant_day
            : (current.traveler_mode ? 9 : 7)));
    if (merchant_can_still_happen) {
        Player route_to_mahaji = current;
        route_to_mahaji.item_shop[i_Spice][id_Sell] += 20;
        route_to_mahaji.player_world.merch = m_Route_to_Mahaji;
        merchant_worlds.push_back(route_to_mahaji);
    }

    const bool witch_can_still_happen =
        witch_events_enabled &&
        !current.witch_happened &&
        current.day < (current.witch_day > 0
            ? current.witch_day
            : (current.traveler_mode ? 13 : 11));
    if (!witch_can_still_happen) {
        return merchant_worlds;
    }

    std::vector<Player> valuations;
    valuations.reserve(merchant_worlds.size() * 3);
    for (const Player& merchant_world : merchant_worlds) {
        valuations.push_back(merchant_world);

        Player midas = merchant_world;
        midas.item_shop[i_Marble][id_Sell] = 0;
        midas.player_world.witch = w_Midas_was_a_Trader;
        valuations.push_back(midas);

        Player time_is_money = merchant_world;
        time_is_money.item_shop[i_Jewelry][id_Sell] -= 500;
        time_is_money.player_world.witch = w_Time_is_Money;
        valuations.push_back(time_is_money);
    }
    return valuations;
}

int guaranteed_next_turn_income(const Player& player) {
    return player.daily_income + player.trader * player.npc_shop[n_Trader][nd_Effect];
}

bool reserve_portfolio_enabled(const Player& player, const PurchaseProfile profile) {
    return profile == PurchaseProfile::Wide ||
           (profile == PurchaseProfile::Strategic && player.set_interest);
}

std::vector<PurchaseChoice> purchase_choices(
    const Player& player,
    const Player& valuation,
    const int gold_limit,
    const int weight_limit,
    BestPurchaseHandler& handler,
    const PurchaseProfile profile
) {
    const uint16_t gold = static_cast<uint16_t>(std::max(gold_limit, 0));
    const uint16_t weight = static_cast<uint16_t>(std::max(weight_limit, 0));
    if (reserve_portfolio_enabled(player, profile)) {
        return handler.getBestBuyPortfolio(gold, weight, valuation);
    }
    const auto best = handler.getBestBuy(gold, weight, valuation);
    return {PurchaseChoice{best.first, gold, best.second}};
}

std::vector<int> reserve_shortfalls(const Player& player, const int gold_after_food) {
    std::vector<int> shortfalls;
    shortfalls.reserve(8);
    shortfalls.push_back(0);

    const int guaranteed_income = guaranteed_next_turn_income(player);
    const int merchant_tier1 = player.merch_discount ? 20 : 25;
    const int merchant_tier2 = player.merch_discount ? 40 : 50;
    const int merchant_tier3 = player.merch_discount ? 80 : 100;
    const int trader_cost = player.npc_shop[n_Trader][nd_Cost];
    const int camel_cost = player.npc_shop[n_Camel][nd_Cost];

    const int costs[] = {
        trader_cost,
        camel_cost,
        merchant_tier1,
        merchant_tier2,
        merchant_tier3,
        trader_cost + camel_cost,
        trader_cost + trader_cost,
        camel_cost + camel_cost
    };

    for (const int cost : costs) {
        const int shortfall = std::max(0, cost - guaranteed_income);
        if (shortfall <= gold_after_food) {
            shortfalls.push_back(shortfall);
        }
    }

    std::sort(shortfalls.begin(), shortfalls.end());
    shortfalls.erase(std::unique(shortfalls.begin(), shortfalls.end()), shortfalls.end());
    return shortfalls;
}

uint16_t calculate_nonfood_cost(const Player& player, const buy_list& buy) {
    return static_cast<uint16_t>(
        buy.spice * player.item_shop[i_Spice][id_Buy] +
        buy.pottery * player.item_shop[i_Pottery][id_Buy] +
        buy.marble * player.item_shop[i_Marble][id_Buy] +
        buy.silk * player.item_shop[i_Silk][id_Buy] +
        buy.jewelry * player.item_shop[i_Jewelry][id_Buy]
    );
}

uint16_t calculate_nonfood_weight(const Player& player, const buy_list& buy) {
    return static_cast<uint16_t>(
        buy.spice * player.item_shop[i_Spice][id_Weight] +
        buy.pottery * player.item_shop[i_Pottery][id_Weight] +
        buy.marble * player.item_shop[i_Marble][id_Weight] +
        buy.silk * player.item_shop[i_Silk][id_Weight] +
        buy.jewelry * player.item_shop[i_Jewelry][id_Weight]
    );
}

bool buy_result_dominates(const Player& lhs, const Player& rhs) {
    return lhs.gold >= rhs.gold &&
           lhs.food >= rhs.food &&
           lhs.spice >= rhs.spice &&
           lhs.pottery >= rhs.pottery &&
           lhs.marble >= rhs.marble &&
           lhs.silk >= rhs.silk &&
           lhs.jewelry >= rhs.jewelry;
}

std::vector<Player> compress_buy_results(std::vector<Player> candidates) {
    std::sort(candidates.begin(), candidates.end(), [](const Player& lhs, const Player& rhs) {
        if (lhs.gold != rhs.gold) {
            return lhs.gold > rhs.gold;
        }
        if (lhs.food != rhs.food) {
            return lhs.food > rhs.food;
        }
        const int lhs_items = lhs.spice + lhs.pottery + lhs.marble + lhs.silk + lhs.jewelry;
        const int rhs_items = rhs.spice + rhs.pottery + rhs.marble + rhs.silk + rhs.jewelry;
        return lhs_items > rhs_items;
    });

    std::vector<Player> survivors;
    survivors.reserve(candidates.size());
    for (const Player& candidate : candidates) {
        bool dominated = false;
        for (const Player& kept : survivors) {
            if (buy_result_dominates(kept, candidate)) {
                dominated = true;
                break;
            }
        }
        if (!dominated) {
            survivors.emplace_back(candidate);
        }
    }
    return survivors;
}
}

const char* purchase_profile_name(const PurchaseProfile profile) {
    switch (profile) {
        case PurchaseProfile::Greedy:
            return "greedy";
        case PurchaseProfile::Scenario:
            return "scenario";
        case PurchaseProfile::Strategic:
            return "strategic";
        case PurchaseProfile::Wide:
            return "wide";
    }
    return "unknown";
}

void Helpers::run_merchant_event(Player& current, std::deque<Player>& list) {
    for (auto event : ALL_MERCH_OPTIONS) {
        const int can_afford = Game::check__afford_merchant(current, event);
        if (can_afford) {
            list.emplace_back(current);
            Player& merch = list.back();
            Game::event_merchant(merch, event);
        }
    }
}

void Helpers::run_witch_event(Player& current, std::deque<Player>& list) {
    for (auto event : ALL_WITCH_OPTIONS) {
        list.emplace_back(current);
        Player& witch = list.back();
        Game::event_witch(witch, event);
    }
}

BuyEventStats Helpers::run_buy_event(
    Player& current,
    std::deque<Player>& list,
    BestPurchaseHandler& handler,
    const bool merchant_events_enabled,
    const bool witch_events_enabled,
    const PurchaseProfile profile
) {
    BuyEventStats stats;
    const int current_food_cost = current.food_consumption;
    const int days_left = current.max_day - current.day;
    const int food_multi = days_left > 1 ? 2 : 1; // Would buy for today and tomorrow or no point
    const int max_weight = current.item_shop[i_Food][id_Weight] == 0 ? 100 : Game::check_weight_left(current) / current.item_shop[i_Food][id_Weight];
    const int max_afford = current.item_shop[i_Food][id_Buy] == 0 ? 100 : current.gold / current.item_shop[i_Food][id_Buy];
    const int max_eat = std::max(current.npc_shop[n_Trader][nd_Eat], current.npc_shop[n_Camel][nd_Eat]);
    const int min_eat = std::min(current.npc_shop[n_Trader][nd_Eat], current.npc_shop[n_Camel][nd_Eat]);
    const int future_days = days_left - 1;
    const int max_want = (current_food_cost * days_left) + (max_eat * (future_days * (future_days + 1)) / 2); // Max could need (Today + tomorrow+2 + Day3+4 + etc)

    const int max_food = std::min({max_weight, max_afford, max_want});
    int needed_food = std::max(0, (food_multi * current_food_cost) - current.food);
    if (max_food >= needed_food) {
        stats.food_range_total = static_cast<uint64_t>(max_food - needed_food + 1);
        stats.max_food_range = max_food - needed_food + 1;
        stats.max_food_min = needed_food;
        stats.max_food_max = max_food;
        stats.widest_range_day = current.day;
    }
    const auto valuations = purchase_valuations(current, merchant_events_enabled, witch_events_enabled, profile);
    stats.valuation_worlds = valuations.size();
    std::unordered_set<uint64_t> emitted_buys;
    emitted_buys.reserve(static_cast<size_t>((max_food - needed_food + 1) * 2) * valuations.size());
    std::vector<Player> candidate_results;
    candidate_results.reserve(static_cast<size_t>(std::max(0, max_food - needed_food + 1)) * 3U * valuations.size());
    const int base_weight_left = Game::check_weight_left(current);
    const int food_buy_cost = current.item_shop[i_Food][id_Buy];
    const int food_buy_weight = current.item_shop[i_Food][id_Weight];
    const auto all_reserve_levels = reserve_shortfalls(current, current.gold);

    for (const Player& valuation : valuations) {
        for (const int reserve_gold : all_reserve_levels) {
            int food_to_try = needed_food;
            while (food_to_try <= max_food) {
                const int gold_after_food = current.gold - (food_buy_cost * food_to_try);
                if (reserve_gold > gold_after_food) {
                    break;
                }

                ++stats.best_buy_queries;
                std::vector<buy_list> best_buys = reserve_gold == 0
                    ? find_buys(current, valuation, food_to_try, false, handler, profile)
                    : find_buys_with_reserve(current, valuation, food_to_try, reserve_gold, handler, profile);
                stats.portfolio_choices += best_buys.size();

                for (const buy_list& best_buy : best_buys) {
                    if (is_empty_buy(best_buy)) {
                        continue;
                    }
                    if (emitted_buys.insert(encode_buy(best_buy)).second) {
                        candidate_results.emplace_back(current);
                        Game::event_buy_items(candidate_results.back(), best_buy);
                    } else {
                        ++stats.duplicate_emits;
                    }
                }

                const buy_list& best_buy = best_buys.front();

                const uint16_t nonfood_cost = calculate_nonfood_cost(current, best_buy);
                const uint16_t nonfood_weight = calculate_nonfood_weight(current, best_buy);
                if (nonfood_cost == 0 && nonfood_weight == 0) {
                    ++food_to_try;
                    continue;
                }

                int next_food = food_to_try + 1;
                int max_same_food = max_food;
                if (food_buy_cost > 0) {
                    max_same_food = std::min(max_same_food, (current.gold - reserve_gold - static_cast<int>(nonfood_cost)) / food_buy_cost);
                }
                if (food_buy_weight > 0) {
                    max_same_food = std::min(max_same_food, (base_weight_left - static_cast<int>(nonfood_weight)) / food_buy_weight);
                }
                if (max_same_food >= food_to_try) {
                    next_food = max_same_food + 1;
                }
                food_to_try = std::max(next_food, food_to_try + 1);
            }
        }
    }

    for (int food_only = needed_food; food_only <= max_food; ++food_only) {
        ++stats.food_only_checks;
        if (check__valid_only_food(food_only + current.food, max_eat, min_eat, current_food_cost)) {
            buy_list food_only_buy = find_buys(current, current, food_only, true, handler, profile).front();
            if (is_empty_buy(food_only_buy)) {
                continue;
            }
            if (emitted_buys.insert(encode_buy(food_only_buy)).second) {
                candidate_results.emplace_back(current);
                Game::event_buy_items(candidate_results.back(), food_only_buy);
            } else {
                ++stats.duplicate_emits;
            }
        }
    }

    stats.unique_buys = candidate_results.size();
    const auto compressed_results = compress_buy_results(std::move(candidate_results));
    stats.dominance_prunes = stats.unique_buys - compressed_results.size();
    stats.emitted_states = compressed_results.size();
    for (const Player& candidate : compressed_results) {
        list.emplace_back(candidate);
    }
    return stats;
}

void Helpers::run_sell_event(Player& current, std::deque<Player>& list, std::set<Player>& final) {
    if (current.day != current.max_day) {
        list.emplace_back(current);
        Player& sell = list.back();
        Game::event_sell_items(sell);
    } else {
        Player sell = current;
        list.emplace_back(current);
        Game::event_sell_items(sell);
        final.insert(sell);
    }
}

void Helpers::run_trader_event(Player& current, std::deque<Player>& list) {
    list.emplace_back(current);
    Player& trader = list.back();
    Game::event_buy_npc(trader, n_Trader);

}

void Helpers::run_camel_event(Player& current, std::deque<Player>& list) {
    list.emplace_back(current);
    Player& camel = list.back();
    Game::event_buy_npc(camel, n_Camel);
}

void Helpers::run_interest_event(Player& current, std::deque<Player>& list, std::set<Player>& final) {
    if (current.day != current.max_day) {
        list.emplace_back(current);
        Player& interest = list.back();
        Game::event_interest(interest);
    } else {
        Player interest = current;
        Game::event_interest(interest);
        final.insert(interest);
    }
}

std::vector<buy_list> Helpers::find_buys(
    const Player& player,
    const Player& valuation,
    const int& food_needed,
    const bool& only_food,
    BestPurchaseHandler& handler,
    const PurchaseProfile profile
) {
    std::vector<buy_list> buys;
    int gold_left = player.gold - (player.item_shop[i_Food][id_Buy] * food_needed);
    int weight_left = Game::check_weight_left(player) - (player.item_shop[i_Food][id_Weight] * food_needed);

    if (not only_food) {
        for (const PurchaseChoice& choice : purchase_choices(player, valuation, gold_left, weight_left, handler, profile)) {
            buy_list what_to_buy = choice.buys;
            what_to_buy.food = static_cast<uint8_t>(food_needed);
            buys.push_back(what_to_buy);
        }
    } else {
        buys.emplace_back(food_needed, 0, 0, 0, 0, 0);
    }
    return buys;
}

namespace {
std::vector<buy_list> find_buys_with_reserve(
    const Player& player,
    const Player& valuation,
    const int food_needed,
    const int reserve_gold,
    BestPurchaseHandler& handler,
    const PurchaseProfile profile
) {
    std::vector<buy_list> buys;
    const int gold_left = player.gold - (player.item_shop[i_Food][id_Buy] * food_needed);
    const int weight_left = Game::check_weight_left(player) - (player.item_shop[i_Food][id_Weight] * food_needed);
    const int usable_gold = std::max(0, gold_left - reserve_gold);

    for (const PurchaseChoice& choice : purchase_choices(player, valuation, usable_gold, weight_left, handler, profile)) {
        buy_list what_to_buy = choice.buys;
        what_to_buy.food = static_cast<uint8_t>(food_needed);
        buys.push_back(what_to_buy);
    }
    return buys;
}
}

bool Helpers::check__valid_only_food(int want_food, const int& max_eat, const int& min_eat, const int& daily_food) {
    if (daily_food <= 0) {
        return want_food == 0;
    }

    int count = 0;
    while (want_food >= 2*daily_food) {
        want_food -= daily_food;
        ++count;
    }
    const bool exact_food = want_food == daily_food;
    bool max_eating = false;
    if (max_eat != 0) {
        max_eating = want_food / max_eat <= count and want_food % max_eat == 0;
        max_eating = max_eating or ((want_food - daily_food) / max_eat <= count+1 and (want_food - daily_food) % max_eat == 0);
    }
    bool min_eating = false;
    if (min_eat != 0 and min_eat != max_eat) {
        min_eating = want_food / min_eat <= count and want_food % min_eat == 0;
        min_eating = min_eating or ((want_food - daily_food) / min_eat <= count+1 and (want_food - daily_food) % min_eat == 0);
    }

    if (exact_food or max_eating or min_eating) {
        return true;
    }
    return false;
};

bool Helpers::day_in_array(const int (&list)[3], const int &value) {
    if (value == list[0] || value == list[1] || value == list[2]) {
        return true;
    }
    return false;
};
