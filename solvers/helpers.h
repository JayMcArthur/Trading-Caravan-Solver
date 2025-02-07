//
// Created by Jay on 2/4/2025.
//

#ifndef TRADING_CARAVAN_SOLVER_HELPERS_H
#define TRADING_CARAVAN_SOLVER_HELPERS_H

#include "../player/player.h"
#include "../purchaseHandler/BestPurchaseHandler.h"
#include <deque>
#include <set>


class Helpers {
public:
    static void run_merchant_event(Player& current, std::deque<Player>& list);
    static void run_witch_event(Player& current, std::deque<Player>& list);
    static void run_buy_event(Player& current, std::deque<Player>& list, BestPurchaseHandler& handler);
    static void run_sell_event(Player& current, std::deque<Player>& list, std::set<Player>& final);
    static void run_trader_event(Player& current, std::deque<Player>& list);
    static void run_camel_event(Player& current, std::deque<Player>& list);
    static void run_interest_event(Player& current, std::deque<Player>& list, std::set<Player>& final);
    static bool day_in_array(const int (&list)[3], const int &value);
private:
    static void find_buy(Player &player, const int &food_needed, const bool &only_food, BestPurchaseHandler& handler);
    static bool check__valid_only_food(int want_food, const int& max_eat, const int& min_eat, const int& daily_food);
};


#endif //TRADING_CARAVAN_SOLVER_HELPERS_H
