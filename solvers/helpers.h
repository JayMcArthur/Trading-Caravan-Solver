//
// Created by Jay on 2/4/2025.
//

#ifndef TRADING_CARAVAN_SOLVER_HELPERS_H
#define TRADING_CARAVAN_SOLVER_HELPERS_H

#include "../player/player.h"
#include "../purchaseHandler/BestPurchaseHandler.h"
#include <deque>
#include <set>

enum class PurchaseProfile : uint8_t {
    Greedy,
    Scenario,
    Strategic,
    Wide,
};

[[nodiscard]] const char* purchase_profile_name(PurchaseProfile profile);

struct BuyEventStats {
    uint64_t food_range_total = 0;
    uint64_t best_buy_queries = 0;
    uint64_t food_only_checks = 0;
    uint64_t emitted_states = 0;
    uint64_t duplicate_emits = 0;
    uint64_t valuation_worlds = 0;
    uint64_t unique_buys = 0;
    uint64_t dominance_prunes = 0;
    uint64_t portfolio_choices = 0;
    int max_food_range = 0;
    int max_food_min = 0;
    int max_food_max = 0;
    int widest_range_day = 0;
};

class Helpers {
public:
    static void run_merchant_event(Player& current, std::deque<Player>& list);
    static void run_witch_event(Player& current, std::deque<Player>& list);
    static BuyEventStats run_buy_event(
        Player& current,
        std::deque<Player>& list,
        BestPurchaseHandler& handler,
        bool merchant_events_enabled,
        bool witch_events_enabled,
        PurchaseProfile profile = PurchaseProfile::Strategic
    );
    static void run_sell_event(Player& current, std::deque<Player>& list, std::set<Player>& final);
    static void run_trader_event(Player& current, std::deque<Player>& list);
    static void run_camel_event(Player& current, std::deque<Player>& list);
    static void run_interest_event(Player& current, std::deque<Player>& list, std::set<Player>& final);
    static bool day_in_array(const int (&list)[3], const int &value);
private:
    static std::vector<buy_list> find_buys(
        const Player& player,
        const Player& valuation,
        const int& food_needed,
        const bool& only_food,
        BestPurchaseHandler& handler,
        PurchaseProfile profile
    );
    static bool check__valid_only_food(int want_food, const int& max_eat, const int& min_eat, const int& daily_food);
};


#endif //TRADING_CARAVAN_SOLVER_HELPERS_H
