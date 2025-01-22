//
// Created by Jay on 6/13/2023.
//

#ifndef TRADING_CARAVAN_SOLVER_GAME_H
#define TRADING_CARAVAN_SOLVER_GAME_H

using namespace std;

#include <string>
#include "enums.h"
#include "player.h"

class Game {
public:
    static void apply_town(Player &player, const town_options &town);
    static void auto_game_loop(Player &player, const vector<ga_data> &event_list);
    static void full_day(Player &player, const ga_data &event);
    static void start_of_day(Player &player);
    static void find_buy(Player &player, const int &food_needed, const bool &only_food=false);

    static void event_buy_items(Player &player, const int (&to_buy)[6]);
    static void event_sell_items(Player &player);
    static void event_buy_npc(Player &player, const npcs &selection);
    static void event_interest(Player &player);
    static bool event_merchant(Player &player, const merch_options &selection);
    static void event_witch(Player &player, const witch_options &selection);
    static int check_has_items(const Player &player);
    static int check_points(const Player &player, const string &type);
    static int check_weight_left(const Player &player);
    static double check_item_worth(const Player &player, const items &item);


private:
    static void end_of_day(Player &player, const string &action);
};


#endif //TRADING_CARAVAN_SOLVER_GAME_H
