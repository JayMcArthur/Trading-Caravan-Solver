//
// Created by Jay on 2/4/2025.
//

#include "helpers.h"
#include "../game/Game.h"
#include <algorithm>

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

void Helpers::run_buy_event(Player& current, std::deque<Player>& list, BestPurchaseHandler& handler) {
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

    // TODO -- Find some way to cut this search space
    for (; needed_food <= max_food; ++needed_food) {
        list.emplace_back(current);
        Player& to_buy_everything = list.back();
        find_buy(to_buy_everything, needed_food, false, handler);
        if (check__valid_only_food(needed_food + current.food, max_eat, min_eat, current_food_cost)) {
            list.emplace_back(current);
            Player& to_buy_food = list.back();
            find_buy(to_buy_food, needed_food, true, handler);
        }
    }
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

void Helpers::find_buy(Player &player, const int &food_needed, const bool &only_food, BestPurchaseHandler& handler) {
    buy_list what_to_buy = buy_list(food_needed, 0, 0, 0, 0, 0);
    int gold_left = player.gold - (player.item_shop[i_Food][id_Buy] * food_needed);
    int weight_left = Game::check_weight_left(player) - (player.item_shop[i_Food][id_Weight] * food_needed);

    if (not only_food) {
        std::pair<uint32_t, buy_list> profit;
        profit = handler.getBestBuy(gold_left, weight_left, player);
        what_to_buy = profit.second;
        what_to_buy.food = food_needed;
    }

    Game::event_buy_items(player, what_to_buy);
}

bool Helpers::check__valid_only_food(int want_food, const int& max_eat, const int& min_eat, const int& daily_food) {
    // TODO - This + Run Buy event need to be remade
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