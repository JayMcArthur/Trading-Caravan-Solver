//
// Created by Jay on 2/4/2025.
//

#ifndef TRADING_CARAVAN_SOLVER_HELPERS_H
#define TRADING_CARAVAN_SOLVER_HELPERS_H

#include "../player/player.h"
#include "../purchaseHandler/BestPurchaseHandler.h"
#include <deque>
#include <set>

struct BuyEventStats {
    uint64_t food_range_total = 0;
    uint64_t best_buy_queries = 0;
    uint64_t food_only_checks = 0;
    uint64_t emitted_states = 0;
    uint64_t duplicate_emits = 0;
    int max_food_range = 0;
    int max_food_min = 0;
    int max_food_max = 0;
    int widest_range_day = 0;
};

class Helpers {
public:
    static void run_merchant_event(Player& current, std::deque<Player>& list);
    static void run_witch_event(Player& current, std::deque<Player>& list);
    static BuyEventStats run_buy_event(Player& current, std::deque<Player>& list, BestPurchaseHandler& handler);
    static void run_sell_event(Player& current, std::deque<Player>& list, std::set<Player>& final);
    static void run_trader_event(Player& current, std::deque<Player>& list);
    static void run_camel_event(Player& current, std::deque<Player>& list);
    static void run_interest_event(Player& current, std::deque<Player>& list, std::set<Player>& final);
    static bool day_in_array(const int (&list)[3], const int &value);
private:
    static buy_list find_buy(const Player& player, const int& food_needed, const bool& only_food, BestPurchaseHandler& handler);
    static bool check__valid_only_food(int want_food, const int& max_eat, const int& min_eat, const int& daily_food);
};


#endif //TRADING_CARAVAN_SOLVER_HELPERS_H
