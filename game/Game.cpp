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
        case t_Normalia:  // 00 - No Effect
            break;
        case t_Fang_Mo:  // 01 - Begin with 1 Pottery (weightless)
            player.fang_mo_pot += 1;
            player.player_world.town = t_Fang_Mo;
            break;
        case t_L_Exquise:  // 02 - Traders don't need Food
            player.food_consumption -= (player.npc_shop[n_Trader][nd_Eat] * player.trader);
            player.npc_shop[n_Trader][nd_Eat] = 0;
            player.player_world.town = t_L_Exquise;
            break;
        case t_Mahaji:  // 03 - Spice sell +5
            player.item_shop[i_Spice][id_Sell] += 5;
            player.player_world.town = t_Mahaji;
            break;
        case t_Tocaccialli:  // 04 - Max Day +1
            player.max_day += 1;
            player.player_world.town = t_Tocaccialli;
            break;
        case t_Bajaar:  // 05 - x2 Trader income
            player.npc_shop[n_Trader][nd_Effect] += 20;
            player.player_world.town = t_Bajaar;
            break;
        case t_Doba_Lao:  // 06 - Interest Rate +20%
            player.interest_rate += 0.2;
            player.player_world.town = t_Doba_Lao;
            break;
        case t_Huangoz:  // 07 - Marble Cost -20
            player.item_shop[i_Marble][id_Buy] -= 20;
            player.player_world.town = t_Huangoz;
            break;
        case t_Port_Willam:  // 08 - Carry Weight +15
            player.backpack += 15;
            player.player_world.town = t_Port_Willam;
            break;
        case t_Moonin:  // 09 - Call Merchant at anytime
            player.call_merchant = true;
            player.player_world.town = t_Moonin;
            break;
        case t_Cornecopea:  // 10 - Food is free
            player.item_shop[i_Food][id_Buy] = 0;
            player.player_world.town = t_Cornecopea;
            break;
        case t_Eduming:  // 11 - Buying trader > Interest Rate +10%
            player.trader_interest = true;
            player.player_world.town = t_Eduming;
            break;
        case t_Octoyashi:  // 12 - Begin with +3 Food
            player.food += 3;
            player.player_world.town = t_Octoyashi;
            break;
        case t_Kifuai:  // 13 - Begin with +10 Gold
            player.gold += 10;
            player.player_world.town = t_Kifuai;
            break;
        case t_Skjollird:  // 14 - Camel cost +5 and weight +10
            player.npc_shop[n_Camel][nd_Cost] += 5;
            player.npc_shop[n_Camel][nd_Effect] += 10;
            player.player_world.town = t_Skjollird;
            break;
        case t_Petrinov:  // 15 - Interest rate doesn't decrease
            player.set_interest = true;
            player.player_world.town = t_Petrinov;
            break;
        case t_Jilliqo:  // 16 - Daily Gold Income +10
            player.daily_income += 10;
            player.player_world.town = t_Jilliqo;
            break;
        case t_Gayawaku:  // 17 - Food is weightless
            player.item_shop[i_Food][id_Weight] = 0;
            player.player_world.town = t_Gayawaku;
            break;
        case t_Vilparino:  // 18 - Strange Merchant 20% Sale!
            player.merch_discount = true;
            player.player_world.town = t_Vilparino;
            break;
        case t_Sanctifan:  // 19 - One Free Collect Interest Action
            player.quick_interest = true;
            player.player_world.town = t_Sanctifan;
            break;
        case t_Mehaz:  // 20 - Camels don't need food
            player.food_consumption -= (player.npc_shop[n_Camel][nd_Eat] * player.camel);
            player.npc_shop[n_Camel][nd_Eat] = 0;
            player.player_world.town = t_Mehaz;
            break;
        case t_Ipartus:  // 21 - Silk Cost -100
            player.item_shop[i_Silk][id_Buy] -= 100;
            player.player_world.town = t_Ipartus;
            break;

    }
}

void Game::auto_game_loop(Player &player, const std::vector<ga_data> &event_list) {
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
        /*
        if (player.food < 0) {
            for (const auto & action : player.actions) {
                if (&action != &player.actions.front()) [[likely]] {
                    std::cout << ", "<<  action ;
                } else [[unlikely]]{
                    std::cout <<  action;
                }
            }
            throw negative_food_error();
        }
        */
    }
    player.day += 1;
}

void Game::end_of_day(Player &player, const std::string &action) {
    std::string type;
    player.gold += player.daily_income + player.trader * player.npc_shop[n_Trader][nd_Effect];
    player.actions.emplace_back(std::to_string(player.day) + ": " + action);
    if (player.day == player.max_day) {
        type = "Final";
    } else {
        type = "Day End";
    }
    player.points = check_points(player, type);
}

void Game::event_buy_items(Player &player, const buy_list &to_buy) {
    player.gold -= to_buy.food * player.item_shop[i_Food][id_Buy] +
                   to_buy.spice * player.item_shop[i_Spice][id_Buy] +
                   to_buy.pottery * player.item_shop[i_Pottery][id_Buy] +
                   to_buy.marble * player.item_shop[i_Marble][id_Buy] +
                   to_buy.silk * player.item_shop[i_Silk][id_Buy] +
                   to_buy.jewelry * player.item_shop[i_Jewelry][id_Buy];
    if (player.gold < 0){
        throw negative_gold_error();
    }
    player.food += to_buy.food;
    player.spice += to_buy.spice;
    player.pottery += to_buy.pottery;
    player.marble += to_buy.marble;
    player.silk += to_buy.silk;
    player.jewelry += to_buy.jewelry;
    player.bought_last = true;

    std::stringstream action;
    action << "Buy [" << std::to_string(to_buy.food) << ", " << std::to_string(to_buy.spice) << ", " << std::to_string(to_buy.pottery) << ", ";
    action << std::to_string(to_buy.marble) << ", " << std::to_string(to_buy.silk) << ", " << std::to_string(to_buy.jewelry) << "]";
    end_of_day(player, action.str());
}

void Game::event_sell_items(Player &player) {
    player.gold += player.spice * player.item_shop[i_Spice][id_Sell] +
            (player.pottery + player.fang_mo_pot) * player.item_shop[i_Pottery][id_Sell] +
             player.marble * player.item_shop[i_Marble][id_Sell] +
             player.silk * player.item_shop[i_Silk][id_Sell] +
             player.jewelry * player.item_shop[i_Jewelry][id_Sell] +
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
    if (selection == n_Trader && player.gold >= player.npc_shop[n_Trader][nd_Cost]) {
        player.gold -= player.npc_shop[n_Trader][nd_Cost];
        player.trader += 1;
        player.food_consumption += player.npc_shop[n_Trader][nd_Eat];
        player.interest_rate += player.trader_interest ? 0.1 : 0;
        player.bought_last = false;
        end_of_day(player, "Trader");
    }
    if (selection == n_Camel && player.gold >= player.npc_shop[n_Camel][nd_Cost]) {
        player.gold -= player.npc_shop[n_Camel][nd_Cost];
        player.camel += 1;
        player.food_consumption += player.npc_shop[n_Camel][nd_Eat];
        player.bought_last = false;
        end_of_day(player, "Camel");
    }
}

void Game::event_interest(Player &player) {
    // TODO -- double check that rounding always matches
    player.gold = (int)round(player.interest_rate * (player.gold + 0.25));
    if (not (player.set_interest || player.quick_interest)) {
        player.interest_rate = std::max(player.interest_rate - 0.1, 0.1);
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
        case m_No_Merchant:
            break;
        case m_Cornucopia: // +2 food every turn
            if (player.gold >= (player.merch_discount ? 20 : 25)) {
                player.gold -= (player.merch_discount ? 20 : 25);
                player.food_consumption = std::max(player.food_consumption - 2, 0);
                player.actions.emplace_back("Merch - " + mo_conversion.at(m_Cornucopia));
                player.merchant_happened = true;
                player.day_start_skip = true;
                player.player_world.merch = m_Cornucopia;
                return true;
            }
            break;
        case m_Route_to_Mahaji: // Spice is worth $20 more
            if (player.gold >= (player.merch_discount ? 20 : 25)) {
                player.gold -= (player.merch_discount ? 20 : 25);
                player.item_shop[i_Spice][id_Sell] += 20;
                player.actions.emplace_back("Merch - " + mo_conversion.at(m_Route_to_Mahaji));
                player.merchant_happened = true;
                player.day_start_skip = true;
                player.player_world.merch = m_Route_to_Mahaji;
                return true;
            }
            break;
        case m_Wooden_Statuette: // +1 Statue (aka +500 per Trader and Camel)
            if (player.gold >= (player.merch_discount ? 20 : 25)) {
                player.gold -= (player.merch_discount ? 20 : 25);
                player.statue = true;
                player.actions.emplace_back("Merch - " + mo_conversion.at(m_Wooden_Statuette));
                player.merchant_happened = true;
                player.day_start_skip = true;
                player.player_world.merch = m_Wooden_Statuette;
                return true;
            }
            break;
        case m_Canvas_Bag: // # +20 storage
            if (player.gold >= (player.merch_discount ? 40 : 50)) {
                player.gold -= (player.merch_discount ? 40 : 50);
                player.backpack += 20;
                player.actions.emplace_back("Merch - " + mo_conversion.at(m_Canvas_Bag));
                player.merchant_happened = true;
                player.day_start_skip = true;
                player.player_world.merch = m_Canvas_Bag;
                return true;
            }
            break;
        case m_Leaders_Necklace: // +1 Trader
            if (player.gold >= (player.merch_discount ? 40 : 50)) {
                player.gold -= (player.merch_discount ? 40 : 50);
                player.trader += 1;
                player.food_consumption += player.npc_shop[n_Trader][nd_Eat];
                player.interest_rate += player.trader_interest ? 0.1 : 0; // TODO Should this happen?
                player.actions.emplace_back("Merch - " + mo_conversion.at(m_Leaders_Necklace));
                player.merchant_happened = true;
                player.day_start_skip = true;
                player.player_world.merch = m_Leaders_Necklace;
                return true;
            }
            break;
        case m_Hand_of_Midas: // 100g when you sell everything
            if (player.gold >= (player.merch_discount ? 40 : 50)) {
                player.gold -= (player.merch_discount ? 40 : 50);
                player.hand_of_midas = true;
                player.actions.emplace_back("Merch - " + mo_conversion.at(m_Hand_of_Midas));
                player.merchant_happened = true;
                player.day_start_skip = true;
                player.player_world.merch = m_Hand_of_Midas;
                return true;
            }
            break;
        case m_Sturdy_Saddle: // Camels carry an extra 20
            if (player.gold >= (player.merch_discount ? 80 : 100)) {
                player.gold -= (player.merch_discount ? 80 : 100);
                player.npc_shop[n_Camel][id_Weight] += 20;
                player.actions.emplace_back("Merch - " + mo_conversion.at(m_Sturdy_Saddle));
                player.merchant_happened = true;
                player.day_start_skip = true;
                player.player_world.merch = m_Sturdy_Saddle;
                return true;
            }
            break;
        case m_Magic_Cleppsydra: // 1 more day to trade
            if (player.gold >= (player.merch_discount ? 80 : 100)) {
                player.gold -= (player.merch_discount ? 80 : 100);
                player.max_day += 1;
                player.actions.emplace_back("Merch - " + mo_conversion.at(m_Magic_Cleppsydra));
                player.merchant_happened = true;
                player.day_start_skip = true;
                player.player_world.merch = m_Magic_Cleppsydra;
                return true;
            }
            break;
        case m_Blue_Treasure: // An extra 20% from saving
            if (player.gold >= (player.merch_discount ? 80 : 100)) {
                player.gold -= (player.merch_discount ? 80 : 100);
                player.interest_rate += 0.2;
                player.actions.emplace_back("Merch - " + mo_conversion.at(m_Blue_Treasure));
                player.merchant_happened = true;
                player.day_start_skip = true;
                player.player_world.merch = m_Blue_Treasure;
                return true;
            }
            break;
    }
    return false;
}

void Game::event_witch(Player &player, const witch_options &selection) {
    switch (selection) {
        case w_No_Witch:
            break;
        case w_Presents_Gift: // +20 weight, -20% Saving
            player.backpack += 20;
            player.interest_rate = std::max(player.interest_rate - 0.2, 0.1) ;
            player.actions.emplace_back("Witch - " + wo_conversion.at(w_Presents_Gift));
            player.witch_happened = true;
            player.day_start_skip = true;
            player.player_world.witch = w_Presents_Gift;
            break;
        case w_Vertue_of_Patience: // Saving doesn't decrease, +100 Jewelry cost
            player.set_interest = true;
            player.item_shop[i_Jewelry][id_Buy] += 100;
            player.actions.emplace_back("Witch - " + wo_conversion.at(w_Vertue_of_Patience));
            player.witch_happened = true;
            player.day_start_skip = true;
            player.player_world.witch = w_Vertue_of_Patience;
            break;
        case w_Midas_was_a_Trader: // +150 Trader earn, -300 Marble sell
            player.npc_shop[n_Trader][nd_Effect] += 150;
            player.item_shop[i_Marble][id_Sell] *= 0;
            player.actions.emplace_back("Witch - " + wo_conversion.at(w_Midas_was_a_Trader));
            player.witch_happened = true;
            player.day_start_skip = true;
            player.player_world.witch = w_Midas_was_a_Trader;
            break;
        case w_Camelization: // +10 Camel weight, +1 Camel food cost
            player.npc_shop[n_Camel][nd_Effect] += 10;
            player.food_consumption += player.npc_shop[n_Camel][nd_Eat] * player.camel;
            player.npc_shop[n_Camel][nd_Eat] *= 2;
            player.actions.emplace_back("Witch - " + wo_conversion.at(w_Camelization));
            player.witch_happened = true;
            player.day_start_skip = true;
            player.player_world.witch = w_Camelization;
            break;
        case w_Time_is_Money: // +1 Day, - 500 Jewelry sell
            player.max_day += 1;
            player.item_shop[i_Jewelry][id_Sell] -= 500;
            player.actions.emplace_back("Witch - " + wo_conversion.at(w_Time_is_Money));
            player.witch_happened = true;
            player.day_start_skip = true;
            player.player_world.witch = w_Time_is_Money;
            break;
        case w_Animal_Lover: // +2 Camels, -1 Day
            // TODO - Do these Camel need to eat? Check with different towns
            player.camel += 2;
            player.food_consumption += player.npc_shop[n_Camel][nd_Eat] * player.camel * 2;
            player.max_day -= 1;
            player.actions.emplace_back("Witch - " + wo_conversion.at(w_Animal_Lover));
            player.witch_happened = true;
            player.day_start_skip = true;
            player.player_world.witch = w_Animal_Lover;
            break;
        case w_Oasis_of_Sanctifan: // Crew don't eat, +200 Silk buy
            player.npc_shop[n_Trader][nd_Eat] = 0;
            player.npc_shop[n_Camel][nd_Eat] = 0;
            player.food_consumption = 0;
            player.item_shop[i_Silk][id_Buy] += 200;
            player.actions.emplace_back("Witch - " + wo_conversion.at(w_Oasis_of_Sanctifan));
            player.witch_happened = true;
            player.day_start_skip = true;
            player.player_world.witch = w_Oasis_of_Sanctifan;
            break;
        case w_The_Stonecutter: // -15 Marble weight, +60 Marble cost
            player.item_shop[i_Marble][id_Weight] -= 15;
            player.item_shop[i_Marble][id_Buy] += 60;
            player.actions.emplace_back("Witch - " + wo_conversion.at(w_The_Stonecutter));
            player.witch_happened = true;
            player.day_start_skip = true;
            player.player_world.witch = w_The_Stonecutter;
            break;
    }
}

bool Game::check__afford_merchant(Player &player, const merch_options &selection) {
    switch (selection) {
        case m_No_Merchant:
            break;
        case m_Cornucopia: // +2 food every turn
        case m_Route_to_Mahaji: // Spice is worth $20 more
        case m_Wooden_Statuette: // +1 Statue (aka +500 per Trader and Camel)
            if (player.gold >= (player.merch_discount ? 20 : 25)) {
                return true;
            }
            break;
        case m_Canvas_Bag: // # +20 storage
        case m_Leaders_Necklace: // +1 Trader
        case m_Hand_of_Midas: // 100g when you sell everything
            if (player.gold >= (player.merch_discount ? 40 : 50)) {
                return true;
            }
            break;
        case m_Sturdy_Saddle: // Camels carry an extra 20
        case m_Magic_Cleppsydra: // 1 more day to trade
        case m_Blue_Treasure: // An extra 20% from saving
            if (player.gold >= (player.merch_discount ? 80 : 100)) {
                return true;
            }
            break;
    }
    return false;
}

int Game::check_has_items(const Player &player) {
    return (player.spice + player.pottery + player.marble + player.silk + player.jewelry + player.fang_mo_pot) > 0;
}

int Game::check_points(const Player &player, const std::string &type) {
    // TODO this needs to be better
    // Camels are not worth enough so it discards them
    // Statue is worth too much
    return (player.gold +
            (player.spice * player.item_shop[i_Spice][id_Buy]) +
            ((player.pottery + player.fang_mo_pot) * player.item_shop[i_Pottery][id_Buy]) +
            (player.marble * player.item_shop[i_Marble][id_Buy]) +
            (player.silk * player.item_shop[i_Silk][id_Buy]) +
            (player.jewelry * player.item_shop[i_Jewelry][id_Buy]) +
            (player.trader * player.npc_shop[n_Trader][nd_Cost]) +
            (player.camel * player.npc_shop[n_Camel][nd_Cost]) +
            (player.statue ? (player.trader + player.camel) * (type == "Final" ? 500 : 25) : 0)
    );
}

int Game::check_weight_left(const Player &player) {
    return ((player.food * -player.item_shop[i_Food][id_Weight]) +
            (player.spice * -player.item_shop[i_Spice][id_Weight]) +
            (player.pottery * -player.item_shop[i_Pottery][id_Weight]) +
            (player.marble * -player.item_shop[i_Marble][id_Weight]) +
            (player.silk * -player.item_shop[i_Silk][id_Weight]) +
            (player.jewelry * -player.item_shop[i_Jewelry][id_Weight]) +
            (player.camel * player.npc_shop[n_Camel][nd_Effect]) +
            player.backpack
    );
}

