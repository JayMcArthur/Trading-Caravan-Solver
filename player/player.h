//
// Created by Jay on 6/13/2023.
//

#ifndef TRADING_CARAVAN_SOLVER_PLAYER_H
#define TRADING_CARAVAN_SOLVER_PLAYER_H

#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
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
    std::vector<int> available_actions = {};
    int points = 0;
    uint32_t trace_id = 0;
    std::string last_action = {};
    world player_world;

    bool operator < (const Player& player) const {
        return (points < player.points);
    }

    bool operator > (const Player& player) const {
        return (points > player.points);
    }

    void serialize(std::ostream& out) const {
        out.write(reinterpret_cast<const char*>(&gold), sizeof(gold));
        out.write(reinterpret_cast<const char*>(&food), sizeof(food));
        out.write(reinterpret_cast<const char*>(&spice), sizeof(spice));
        out.write(reinterpret_cast<const char*>(&pottery), sizeof(pottery));
        out.write(reinterpret_cast<const char*>(&marble), sizeof(marble));
        out.write(reinterpret_cast<const char*>(&silk), sizeof(silk));
        out.write(reinterpret_cast<const char*>(&jewelry), sizeof(jewelry));
        out.write(reinterpret_cast<const char*>(&fang_mo_pot), sizeof(fang_mo_pot));
        out.write(reinterpret_cast<const char*>(&trader), sizeof(trader));
        out.write(reinterpret_cast<const char*>(&camel), sizeof(camel));
        out.write(reinterpret_cast<const char*>(&food_consumption), sizeof(food_consumption));
        out.write(reinterpret_cast<const char*>(item_shop), sizeof(item_shop));
        out.write(reinterpret_cast<const char*>(npc_shop), sizeof(npc_shop));
        out.write(reinterpret_cast<const char*>(&backpack), sizeof(backpack));
        out.write(reinterpret_cast<const char*>(&daily_income), sizeof(daily_income));
        out.write(reinterpret_cast<const char*>(&interest_rate), sizeof(interest_rate));
        out.write(reinterpret_cast<const char*>(&set_interest), sizeof(set_interest));
        out.write(reinterpret_cast<const char*>(&quick_interest), sizeof(quick_interest));
        out.write(reinterpret_cast<const char*>(&statue), sizeof(statue));
        out.write(reinterpret_cast<const char*>(&hand_of_midas), sizeof(hand_of_midas));
        out.write(reinterpret_cast<const char*>(&merch_discount), sizeof(merch_discount));
        out.write(reinterpret_cast<const char*>(&trader_interest), sizeof(trader_interest));
        out.write(reinterpret_cast<const char*>(&day), sizeof(day));
        out.write(reinterpret_cast<const char*>(&max_day), sizeof(max_day));
        out.write(reinterpret_cast<const char*>(&merchant_happened), sizeof(merchant_happened));
        out.write(reinterpret_cast<const char*>(&call_merchant), sizeof(call_merchant));
        out.write(reinterpret_cast<const char*>(&witch_happened), sizeof(witch_happened));
        out.write(reinterpret_cast<const char*>(&day_start_skip), sizeof(day_start_skip));
        out.write(reinterpret_cast<const char*>(&bought_last), sizeof(bought_last));
        out.write(reinterpret_cast<const char*>(&points), sizeof(points));
        out.write(reinterpret_cast<const char*>(&trace_id), sizeof(trace_id));
        out.write(reinterpret_cast<const char*>(&player_world), sizeof(player_world));

        size_t available_size = available_actions.size();
        out.write(reinterpret_cast<const char*>(&available_size), sizeof(available_size));
        for (int action : available_actions) {
            out.write(reinterpret_cast<const char*>(&action), sizeof(action));
        }

        size_t last_action_size = last_action.size();
        out.write(reinterpret_cast<const char*>(&last_action_size), sizeof(last_action_size));
        out.write(last_action.data(), static_cast<std::streamsize>(last_action_size));
    }

    void deserialize(std::istream& in) {
        in.read(reinterpret_cast<char*>(&gold), sizeof(gold));
        in.read(reinterpret_cast<char*>(&food), sizeof(food));
        in.read(reinterpret_cast<char*>(&spice), sizeof(spice));
        in.read(reinterpret_cast<char*>(&pottery), sizeof(pottery));
        in.read(reinterpret_cast<char*>(&marble), sizeof(marble));
        in.read(reinterpret_cast<char*>(&silk), sizeof(silk));
        in.read(reinterpret_cast<char*>(&jewelry), sizeof(jewelry));
        in.read(reinterpret_cast<char*>(&fang_mo_pot), sizeof(fang_mo_pot));
        in.read(reinterpret_cast<char*>(&trader), sizeof(trader));
        in.read(reinterpret_cast<char*>(&camel), sizeof(camel));
        in.read(reinterpret_cast<char*>(&food_consumption), sizeof(food_consumption));
        in.read(reinterpret_cast<char*>(item_shop), sizeof(item_shop));
        in.read(reinterpret_cast<char*>(npc_shop), sizeof(npc_shop));
        in.read(reinterpret_cast<char*>(&backpack), sizeof(backpack));
        in.read(reinterpret_cast<char*>(&daily_income), sizeof(daily_income));
        in.read(reinterpret_cast<char*>(&interest_rate), sizeof(interest_rate));
        in.read(reinterpret_cast<char*>(&set_interest), sizeof(set_interest));
        in.read(reinterpret_cast<char*>(&quick_interest), sizeof(quick_interest));
        in.read(reinterpret_cast<char*>(&statue), sizeof(statue));
        in.read(reinterpret_cast<char*>(&hand_of_midas), sizeof(hand_of_midas));
        in.read(reinterpret_cast<char*>(&merch_discount), sizeof(merch_discount));
        in.read(reinterpret_cast<char*>(&trader_interest), sizeof(trader_interest));
        in.read(reinterpret_cast<char*>(&day), sizeof(day));
        in.read(reinterpret_cast<char*>(&max_day), sizeof(max_day));
        in.read(reinterpret_cast<char*>(&merchant_happened), sizeof(merchant_happened));
        in.read(reinterpret_cast<char*>(&call_merchant), sizeof(call_merchant));
        in.read(reinterpret_cast<char*>(&witch_happened), sizeof(witch_happened));
        in.read(reinterpret_cast<char*>(&day_start_skip), sizeof(day_start_skip));
        in.read(reinterpret_cast<char*>(&bought_last), sizeof(bought_last));
        in.read(reinterpret_cast<char*>(&points), sizeof(points));
        in.read(reinterpret_cast<char*>(&trace_id), sizeof(trace_id));
        in.read(reinterpret_cast<char*>(&player_world), sizeof(player_world));

        size_t available_size = 0;
        in.read(reinterpret_cast<char*>(&available_size), sizeof(available_size));
        available_actions.clear();
        available_actions.reserve(available_size);
        for (size_t i = 0; i < available_size; ++i) {
            int action = 0;
            in.read(reinterpret_cast<char*>(&action), sizeof(action));
            available_actions.emplace_back(action);
        }

        size_t last_action_size = 0;
        in.read(reinterpret_cast<char*>(&last_action_size), sizeof(last_action_size));
        last_action.resize(last_action_size);
        in.read(last_action.data(), static_cast<std::streamsize>(last_action_size));
    }
};


#endif //TRADING_CARAVAN_SOLVER_PLAYER_H
