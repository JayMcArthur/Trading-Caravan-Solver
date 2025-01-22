//
// Created by Jay on 6/13/2023.
//

#include <cmath>
#include <bits/stdc++.h>
#include "Game.h"

class negative_gold_error : public std::exception {};
class negative_food_error : public std::exception {};


void Game::apply_town(Player &player, const town_options &town) {
    switch (town) {
        case _Normalia:  // 00 - No Effect
            break;
        case _Fang_Mo:  // 01 - Begin with 1 Pottery (weightless)
            player.fang_mo_pot += 1;
            break;
        case _L_Exquise:  // 02 - Traders don't need Food
            player.food_consumption -= (player.npc_shop[_Trader][_eat] * player.trader);
            player.npc_shop[_Trader][_eat] = 0;
            break;
        case _Mahaji:  // 03 - Spice sell +5
            player.item_shop[_Spice][_sell] += 5;
            break;
        case _Tocaccialli:  // 04 - Max Day +1
            player.max_day += 1;
            break;
        case _Bajaar:  // 05 - x2 Trader income
            player.npc_shop[_Trader][_effect] += 20;
            break;
        case _Doba_Lao:  // 06 - Interest Rate +20%
            player.interest_rate += 0.2;
            break;
        case _Huangoz:  // 07 - Marble Cost -20
            player.item_shop[_Marble][_buy] -= 20;
            break;
        case _Port_Willam:  // 08 - Carry Weight +15
            player.backpack += 15;
            break;
        case _Moonin:  // 09 - Call Merchant at anytime
            player.call_merchant = true;
            break;
        case _Cornecopea:  // 10 - Food is free
            player.item_shop[_Food][_buy] = 0;
            break;
        case _Eduming:  // 11 - Buying trader > Interest Rate +10%
            player.trader_interest = true;
            break;
        case _Octoyashi:  // 12 - Begin with +3 Food
            player.food += 3;
            break;
        case _Kifuai:  // 13 - Begin with +10 Gold
            player.gold += 10;
            break;
        case _Skjollird:  // 14 - Camel cost +5 and weight +10
            player.npc_shop[_Camel][_cost] += 5;
            player.npc_shop[_Camel][_effect] += 10;
            break;
        case _Petrinov:  // 15 - Interest rate doesn't decrease
            player.set_interest = true;
            break;
        case _Jilliqo:  // 16 - Daily Gold Income +10
            player.daily_income += 10;
            break;
        case _Gayawaku:  // 17 - Food is weightless
            player.item_shop[_Food][_weight] = 0;
            break;
        case _Vilparino:  // 18 - Strange Merchant 20% Sale!
            player.merch_discount = true;
            break;
        case _Sanctifan:  // 19 - One Free Collect Interest Action
            player.quick_interest = true;
            break;
        case _Mehaz:  // 20 - Camels don't need food
            player.food_consumption -= (player.npc_shop[_Camel][_eat] * player.camel);
            player.npc_shop[_Camel][_eat] = 0;
            break;
        case _Ipartus:  // 21 - Silk Cost -100
            player.item_shop[_Silk][_buy] -= 100;
            break;

    }
}

void Game::auto_game_loop(Player &player, const vector<ga_data> &event_list) {
    for (auto &event : event_list) {
        full_day(player, event);
    }
}

void Game::full_day(Player &player, const ga_data &event) {
    if (!player.day_start_skip) {
        start_of_day(player);
    } else {
        player.day_start_skip = false;
    }

    switch (event.action) {
        case _a_buy:
            event_buy_items(player, event.buy_data);
            break;
        case _a_sell:
            event_sell_items(player);
            break;
        case _a_npc:
            event_buy_npc(player, event.npc_data);
            break;
        case _a_interest:
            event_interest(player);
            break;
        case _a_merchant:
            event_merchant(player, event.merch_data);
            break;
        case _a_witch:
            event_witch(player, event.witch_data);
            break;
    }
}

void Game::start_of_day(Player &player) {
    if (player.day != 0) {
        player.food -= player.food_consumption;
        if (player.food < 0) {
            throw negative_food_error();
        }
    }
    player.day += 1;
}

void Game::end_of_day(Player &player, const std::string &action) {
    string type;
    player.gold += player.daily_income + player.trader * player.npc_shop[_Trader][_effect];
    player.actions.emplace_back(std::to_string(player.day) + ": " + action);
    if (player.day == player.max_day) {
        type = "Final";
    } else {
        type = "Day End";
    }
    player.points = check_points(player, type);
}

// TODO -- This needs to be double checked if it is optimal
void Game::find_buy(Player &player, const int &food_needed, const bool &only_food) {
    // Food, Spice, Pottery, Marble, Silk, Jewelry
    int what_to_buy[6] = {food_needed, 0, 0, 0, 0, 0};
    int gold_left = player.gold - (player.item_shop[_Food][_buy] * food_needed);
    int weight_left = check_weight_left(player) - (player.item_shop[_Food][_weight] * food_needed);

    if (not only_food) {
        // Sort reference based on value
        // TODO - this doesn't consider if you will buy something that changes item shop
        tuple<double, double, items> item_pair[6] = {
                {0.0, _Food, 0},
                {check_item_roi(player,_Spice), check__item_efficiency(player, _Spice), _Spice},
                {check_item_roi(player,_Pottery), check__item_efficiency(player, _Pottery), _Pottery},
                {check_item_roi(player,_Marble), check__item_efficiency(player, _Marble), _Marble},
                {check_item_roi(player,_Silk), check__item_efficiency(player, _Silk), _Silk},
                {check_item_roi(player,_Jewelry), check__item_efficiency(player, _Jewelry), _Jewelry}
        };

        // ID BUY Sel WW (ROI/RE) > ROI/W, RE/W
        // I1 050 075 50 (1.5/25) > 0.0300, 0.5
        // I2 100 130 75 (1.3/30) > 0.0173, 0.4
        // 140: 50, 50 > 50g
        // 140: 50, 75 > 55g [x]
        // 150: 50, 50, 50 > 75g [x]
        // 150: 75, 75 > 60g

        // II BUY Sel WW (ROI/RE) > ROI/W, RE/W
        // I1 100 120 15 (1.2/20) > 0.08, 1.25
        // I2 100 150 25 (1.5/50) > 0.06, 2
        // 55: 25, 25 > 50g
        // 55: 25, 15, 15 > 65g [x]


        // Sort by ROI, then by space efficiency
        sort(item_pair, item_pair + 6, [](const auto& a, const auto& b) {
            if (std::get<0>(a) != std::get<0>(b)) {
                return std::get<0>(a) > std::get<0>(b);  // Sort by ROI first
            }
            return std::get<1>(a) > std::get<1>(b);  // If ROI is the same, sort by space efficiency
        });

        for (auto &i : item_pair) {
            item = std::get<2>(i);
            if (item == _Food) {
                continue;
            }
            while (weight_left >= player.item_shop[item][_weight] && gold_left >= player.item_shop[item][_buy]) {
                weight_left -= player.item_shop[item][_weight];
                gold_left -= player.item_shop[item][_buy];
                what_to_buy[item] += 1;
            }
        }
        // Handle edge cases where space is left but no more items fit efficiently
        for (auto &i : item_pair) {
            roi = std::get<1>(i);
            item = std::get<2>(i);

            // Consider lower ROI items if they fill remaining space efficiently
            if (gold_left >= player.item_shop[item][_buy] && weight_left >= player.item_shop[item][_weight]) {
                while (weight_left >= player.item_shop[item][_weight] && gold_left >= player.item_shop[item][_buy]) {
                    weight_left -= player.item_shop[item][_weight];
                    gold_left -= player.item_shop[item][_buy];
                    what_to_buy[item] += 1;
                }
            }
        }
    }

    event_buy_items(player, what_to_buy);
}

void Game::event_buy_items(Player &player, const int (&to_buy)[6]) {
    player.gold -= to_buy[_Food] * player.item_shop[_Food][_buy] +
                   to_buy[_Spice] * player.item_shop[_Spice][_buy] +
                   to_buy[_Pottery] * player.item_shop[_Pottery][_buy] +
                   to_buy[_Marble] * player.item_shop[_Marble][_buy] +
                   to_buy[_Silk] * player.item_shop[_Silk][_buy] +
                   to_buy[_Jewelry] * player.item_shop[_Jewelry][_buy];
    if (player.gold < 0){
        //string gold_value = to_string(player.gold);
        throw negative_gold_error();
    }
    player.food += to_buy[_Food];
    player.spice += to_buy[_Spice];
    player.pottery += to_buy[_Pottery];
    player.marble += to_buy[_Marble];
    player.silk += to_buy[_Silk];
    player.jewelry += to_buy[_Jewelry];
    player.bought_last = true;

    stringstream action;
    action << "Buy [" << to_buy[_Food] << ", " << to_buy[_Spice] << ", " << to_buy[_Pottery] << ", " << to_buy[_Marble];
    action << ", " << to_buy[_Silk] << ", " << to_buy[_Jewelry] << "]";
    end_of_day(player, action.str());
}

void Game::event_sell_items(Player &player) {
    player.gold += player.spice * player.item_shop[_Spice][_sell] +
            (player.pottery + player.fang_mo_pot) * player.item_shop[_Pottery][_sell] +
             player.marble * player.item_shop[_Marble][_sell] +
             player.silk * player.item_shop[_Silk][_sell] +
             player.jewelry * player.item_shop[_Jewelry][_sell] +
            (player.hand_of_midas ? 100 : 0);
    player.spice = 0;
    player.pottery = 0;
    player.fang_mo_pot = 0;
    player.marble = 0;
    player.silk = 0;
    player.jewelry = 0;
    player.bought_last = false;

    end_of_day(player, "Sell");
}

void Game::event_buy_npc(Player &player, const npcs &selection) {
    if (selection == _Trader && player.gold >= player.npc_shop[_Trader][_cost]) {
        player.gold -= player.npc_shop[_Trader][_cost];
        player.trader += 1;
        player.food_consumption += player.npc_shop[_Trader][_eat];
        player.interest_rate += player.trader_interest ? 0.1 : 0;
        player.bought_last = false;
        end_of_day(player, "Trader");
    }
    if (selection == _Camel && player.gold >= player.npc_shop[_Camel][_cost]) {
        player.gold -= player.npc_shop[_Camel][_cost];
        player.camel += 1;
        player.food_consumption += player.npc_shop[_Camel][_eat];
        player.bought_last = false;
        end_of_day(player, "Camel");
    }
}

void Game::event_interest(Player &player) {
    // TODO -- double check that rounding always matches
    player.gold = (int)round(player.interest_rate * (player.gold + 0.25));
    if (not (player.set_interest || player.quick_interest)) {
        player.interest_rate = max(player.interest_rate - 0.1, 0.1);
    }

    if (not player.quick_interest) {
        player.bought_last = false;
        end_of_day(player, "Interest");
    } else {
        player.day_start_skip = true;
        player.quick_interest = false;
        player.actions.emplace_back(std::to_string(player.day) + ": Quick Interest");
        player.points = check_points(player, "Quick Action");
    }

}

bool Game::event_merchant(Player &player, const merch_options &selection) {
    switch (selection) {
        case _Cornucopia: // +2 food every turn
            if (player.gold >= (player.merch_discount ? 20 : 25)) {
                player.gold -= (player.merch_discount ? 20 : 25);
                player.food_consumption = max(player.food_consumption - 2, 0);
                player.actions.emplace_back("Merch - " + mo_conversion.at(_Cornucopia));
                player.merchant_happened = true;
                player.day_start_skip = true;
                return true;
            }
            break;
        case _Route_to_Mahaji: // Spice is worth $20 more
            if (player.gold >= (player.merch_discount ? 20 : 25)) {
                player.gold -= (player.merch_discount ? 20 : 25);
                player.item_shop[_Spice][_sell] += 20;
                player.actions.emplace_back("Merch - " + mo_conversion.at(_Route_to_Mahaji));
                player.merchant_happened = true;
                player.day_start_skip = true;
                return true;
            }
            break;
        case _Wooden_Statuette: // +1 Statue (aka +500 per Trader and Camel)
            if (player.gold >= (player.merch_discount ? 20 : 25)) {
                player.gold -= (player.merch_discount ? 20 : 25);
                player.statue = true;
                player.actions.emplace_back("Merch - " + mo_conversion.at(_Wooden_Statuette));
                player.merchant_happened = true;
                player.day_start_skip = true;
                return true;
            }
            break;
        case _Canvas_Bag: // # +20 storage
            if (player.gold >= (player.merch_discount ? 40 : 50)) {
                player.gold -= (player.merch_discount ? 40 : 50);
                player.backpack += 20;
                player.actions.emplace_back("Merch - " + mo_conversion.at(_Canvas_Bag));
                player.merchant_happened = true;
                player.day_start_skip = true;
                return true;
            }
            break;
        case _Leaders_Necklace: // +1 Trader
            if (player.gold >= (player.merch_discount ? 40 : 50)) {
                player.gold -= (player.merch_discount ? 40 : 50);
                player.trader += 1;
                player.food_consumption += player.npc_shop[_Trader][_eat];
                player.interest_rate += player.trader_interest ? 0.1 : 0; // TODO Should this happen?
                player.actions.emplace_back("Merch - " + mo_conversion.at(_Leaders_Necklace));
                player.merchant_happened = true;
                player.day_start_skip = true;
                return true;
            }
            break;
        case _Hand_of_Midas: // 100g when you sell everything
            if (player.gold >= (player.merch_discount ? 40 : 50)) {
                player.gold -= (player.merch_discount ? 40 : 50);
                player.hand_of_midas = true;
                player.actions.emplace_back("Merch - " + mo_conversion.at(_Hand_of_Midas));
                player.merchant_happened = true;
                player.day_start_skip = true;
                return true;
            }
            break;
        case _Sturdy_Saddle: // Camels carry an extra 20
            if (player.gold >= (player.merch_discount ? 80 : 100)) {
                player.gold -= (player.merch_discount ? 80 : 100);
                player.npc_shop[_Camel][_weight] += 20;
                player.actions.emplace_back("Merch - " + mo_conversion.at(_Sturdy_Saddle));
                player.merchant_happened = true;
                player.day_start_skip = true;
                return true;
            }
            break;
        case _Magic_Cleppsydra: // 1 more day to trade
            if (player.gold >= (player.merch_discount ? 80 : 100)) {
                player.gold -= (player.merch_discount ? 80 : 100);
                player.max_day += 1;
                player.actions.emplace_back("Merch - " + mo_conversion.at(_Magic_Cleppsydra));
                player.merchant_happened = true;
                player.day_start_skip = true;
                return true;
            }
            break;
        case _Blue_Treasure: // An extra 20% from saving
            if (player.gold >= (player.merch_discount ? 80 : 100)) {
                player.gold -= (player.merch_discount ? 80 : 100);
                player.interest_rate += 0.2;
                player.actions.emplace_back("Merch - " + mo_conversion.at(_Blue_Treasure));
                player.merchant_happened = true;
                player.day_start_skip = true;
                return true;
            }
            break;
    }
    return false;
}

void Game::event_witch(Player &player, const witch_options &selection) {
    switch (selection) {
        case _Presents_Gift: // +20 weight, -20% Saving
            player.backpack += 20;
            player.interest_rate = max(player.interest_rate - 0.2, 0.1) ;
            player.actions.emplace_back("Witch - " + wo_conversion.at(_Presents_Gift));
            player.witch_happened = true;
            player.day_start_skip = true;
            break;
        case _Vertue_of_Patience: // Saving doesn't decrease, +100 Jewelry cost
            player.set_interest = true;
            player.item_shop[_Jewelry][_buy] += 100;
            player.actions.emplace_back("Witch - " + wo_conversion.at(_Vertue_of_Patience));
            player.witch_happened = true;
            player.day_start_skip = true;
            break;
        case _Midas_was_a_Trader: // +150 Trader earn, -300 Marble sell
            player.npc_shop[_Trader][_effect] += 150;
            player.item_shop[_Marble][_sell] *= 0;
            player.actions.emplace_back("Witch - " + wo_conversion.at(_Midas_was_a_Trader));
            player.witch_happened = true;
            player.day_start_skip = true;
            break;
        case _Camelization: // +10 Camel weight, +1 Camel food cost
            player.npc_shop[_Camel][_effect] += 10;
            player.food_consumption += player.npc_shop[_Camel][_eat] * player.camel;
            player.npc_shop[_Camel][_eat] *= 2;
            player.actions.emplace_back("Witch - " + wo_conversion.at(_Camelization));
            player.witch_happened = true;
            player.day_start_skip = true;
            break;
        case _Time_is_Money: // +1 Day, - 500 Jewelry sell
            player.max_day += 1;
            player.item_shop[_Jewelry][_sell] -= 500;
            player.actions.emplace_back("Witch - " + wo_conversion.at(_Time_is_Money));
            player.witch_happened = true;
            player.day_start_skip = true;
            break;
        case _Animal_Lover: // +2 Camels, -1 Day
            // TODO - Do these Camel need to eat? Check with different towns
            player.camel += 2;
            player.food_consumption += player.npc_shop[_Camel][_eat] * player.camel * 2;
            player.max_day -= 1;
            player.actions.emplace_back("Witch - " + wo_conversion.at(_Animal_Lover));
            player.witch_happened = true;
            player.day_start_skip = true;
            break;
        case _Oasis_of_Sanctifan: // Crew don't eat, +200 Silk buy
            player.npc_shop[_Trader][_eat] = 0;
            player.npc_shop[_Camel][_eat] = 0;
            player.food_consumption = 0;
            player.item_shop[_Silk][_buy] += 200;
            player.actions.emplace_back("Witch - " + wo_conversion.at(_Oasis_of_Sanctifan));
            player.witch_happened = true;
            player.day_start_skip = true;
            break;
        case _The_Stonecutter: // -15 Marble weight, +60 Marble cost
            player.item_shop[_Marble][_weight] -= 15;
            player.item_shop[_Marble][_buy] += 60;
            player.actions.emplace_back("Witch - " + wo_conversion.at(_The_Stonecutter));
            player.witch_happened = true;
            player.day_start_skip = true;
            break;
    }
}

int Game::check_has_items(const Player &player) {
    return (player.spice + player.pottery + player.marble + player.silk + player.jewelry + player.fang_mo_pot) > 0;
}

int Game::check_points(const Player &player, const std::string &type) {
    // TODO this needs to be better
    // Camels are not worth enough so it discards them
    // Statue is worth too much
    return (player.gold +
            (player.spice * player.item_shop[_Spice][_buy]) +
            ((player.pottery + player.fang_mo_pot) * player.item_shop[_Pottery][_buy]) +
            (player.marble * player.item_shop[_Marble][_buy]) +
            (player.silk * player.item_shop[_Silk][_buy]) +
            (player.jewelry * player.item_shop[_Jewelry][_buy]) +
            (player.trader * player.npc_shop[_Trader][_cost]) +
            (player.camel * player.npc_shop[_Camel][_cost]) +
            (player.statue ? (player.trader + player.camel) * (type == "Final" ? 500 : 25) : 0)
    );
}

int Game::check_weight_left(const Player &player) {
    return ((player.food * -player.item_shop[_Food][_weight]) +
            (player.spice * -player.item_shop[_Spice][_weight]) +
            (player.pottery * -player.item_shop[_Pottery][_weight]) +
            (player.marble * -player.item_shop[_Marble][_weight]) +
            (player.silk * -player.item_shop[_Silk][_weight]) +
            (player.jewelry * -player.item_shop[_Jewelry][_weight]) +
            (player.camel * player.npc_shop[_Camel][_effect]) +
            player.backpack
    );
}

double Game::check__item_efficiency(const Player &player, const items &item) {
    int weight = max(player.item_shop[item][_weight], 1);
    return double check_item_roi(player, item) / weight;
}

double Game::check__item_roi(const Player &player, const items &item) {
    int sell = player.item_shop[item][_sell];
    int buy = player.item_shop[item][_buy];
    return (sell - buy)/buy;
}

