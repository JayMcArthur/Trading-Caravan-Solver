//
// Created by Jay on 6/13/2023.
//

#ifndef TRADING_CARAVAN_SOLVER_PLAYER_H
#define TRADING_CARAVAN_SOLVER_PLAYER_H

#include <string>
#include <vector>
#include "../consts/enums.h"


struct Player {
    // Inventory
    int gold = 30;
    int food = 5;
    int spice = 0;
    int pottery = 0;
    int marble = 0;
    int silk = 0;
    int jewelry = 0;
    int fang_mo_pot = 0;
    // Crew
    int trader = 1;
    int camel = 1;
    int food_consumption = 2;
    // Stores
    int item_shop[6][3] = {
        // Buy, Sell, Weight
        {3, 2, 2},
        {5, 10, 4},
        {20, 33, 5},
        {110, 300, 20},
        {530, 1150, 8},
        {900, 2500, 12}
    };
    int npc_shop[2][3] = {
        // Buy, Earn/Bag, Eat
        {25, 20, 1},
        {30, 30, 1}
    };
    // Effects
    int backpack = 0;
    int daily_income = 0;
    double interest_rate = 1.50;
    bool set_interest = false;
    bool quick_interest = false;
    bool statue = false;
    bool hand_of_midas = false;
    bool merch_discount = false;
    bool trader_interest = false;
    // Day Info
    int day = 0;
    int max_day = 15;
    bool merchant_happened = false;
    bool call_merchant = false;
    bool witch_happened = false;
    bool day_start_skip = false;
    bool bought_last = false;
    std::vector<std::string> actions = {}; // 15 Days, 1 Day from Town, Merch, & Witch
    std::vector<int> available_actions = {};
    int points = 0;
    world player_world;

    bool operator < (const Player& player) const {
        return (points < player.points);
    }

    bool operator > (const Player& player) const {
        return (points > player.points);
    }
};


#endif //TRADING_CARAVAN_SOLVER_PLAYER_H
