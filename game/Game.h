//
// Created by Jay on 6/13/2023.
//

#ifndef TRADING_CARAVAN_SOLVER_GAME_H
#define TRADING_CARAVAN_SOLVER_GAME_H

#include <string>
#include "../consts/enums.h"
#include "../player/player.h"

class Game {
public:
    static void apply_town(Player &player, const town_options &town);
    static void auto_game_loop(Player &player, const std::vector<ga_data> &event_list);
    static void full_day(Player &player, const ga_data &event);
    static void start_of_day(Player &player);
    static void event_buy_items(Player &player, const buy_list &to_buy);
    static void event_sell_items(Player &player);
    static void event_buy_npc(Player &player, const npcs &selection);
    static void event_interest(Player &player);
    static bool event_merchant(Player &player, const merch_options &selection);
    static void event_witch(Player &player, const witch_options &selection);
    static int check_has_items(const Player &player);
    static int check_points(const Player &player, const std::string &type);
    static int check_weight_left(const Player &player);
    static bool check__afford_merchant(Player &player, const merch_options &selection);

private:
    static void end_of_day(Player &player, const std::string &action);

};


#endif //TRADING_CARAVAN_SOLVER_GAME_H
