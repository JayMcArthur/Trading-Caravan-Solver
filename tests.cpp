#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include "game/Game.h"
#include "purchaseHandler/FenwickTree2D.h"
#include "purchaseHandler/BestPurchaseHandler.h"
#include "solvers/helpers.h"

namespace {
std::vector<buy_option> build_small_options(const Player& player, const int gold_limit, const int weight_limit) {
    std::vector<buy_option> options;
    uint32_t best = 0;
    for (int spice = 0; spice <= gold_limit / player.item_shop[i_Spice][id_Buy]; ++spice) {
        for (int pottery = 0; pottery <= gold_limit / player.item_shop[i_Pottery][id_Buy]; ++pottery) {
            for (int marble = 0; marble <= gold_limit / player.item_shop[i_Marble][id_Buy]; ++marble) {
                for (int silk = 0; silk <= gold_limit / player.item_shop[i_Silk][id_Buy]; ++silk) {
                    for (int jewelry = 0; jewelry <= gold_limit / player.item_shop[i_Jewelry][id_Buy]; ++jewelry) {
                        const int cost =
                            spice * player.item_shop[i_Spice][id_Buy] +
                            pottery * player.item_shop[i_Pottery][id_Buy] +
                            marble * player.item_shop[i_Marble][id_Buy] +
                            silk * player.item_shop[i_Silk][id_Buy] +
                            jewelry * player.item_shop[i_Jewelry][id_Buy];
                        const int weight =
                            spice * player.item_shop[i_Spice][id_Weight] +
                            pottery * player.item_shop[i_Pottery][id_Weight] +
                            marble * player.item_shop[i_Marble][id_Weight] +
                            silk * player.item_shop[i_Silk][id_Weight] +
                            jewelry * player.item_shop[i_Jewelry][id_Weight];

                        if (cost > gold_limit || weight > weight_limit) {
                            continue;
                        }

                        const uint32_t profit =
                            spice * std::max(player.item_shop[i_Spice][id_Sell] - player.item_shop[i_Spice][id_Buy], 0) +
                            pottery * std::max(player.item_shop[i_Pottery][id_Sell] - player.item_shop[i_Pottery][id_Buy], 0) +
                            marble * std::max(player.item_shop[i_Marble][id_Sell] - player.item_shop[i_Marble][id_Buy], 0) +
                            silk * std::max(player.item_shop[i_Silk][id_Sell] - player.item_shop[i_Silk][id_Buy], 0) +
                            jewelry * std::max(player.item_shop[i_Jewelry][id_Sell] - player.item_shop[i_Jewelry][id_Buy], 0);
                        best = std::max(best, profit);
                        options.emplace_back(
                            static_cast<uint16_t>(cost),
                            static_cast<uint16_t>(weight),
                            profit,
                            buy_list(0, static_cast<uint8_t>(spice), static_cast<uint8_t>(pottery), static_cast<uint8_t>(marble), static_cast<uint8_t>(silk), static_cast<uint8_t>(jewelry))
                        );
                    }
                }
            }
        }
    }
    (void)best;
    return options;
}

uint32_t brute_force_profit(const Player& player, const int gold_limit, const int weight_limit) {
    uint32_t best = 0;
    for (const auto& option : build_small_options(player, gold_limit, weight_limit)) {
        if (option.cost <= gold_limit && option.weight <= weight_limit) {
            best = std::max(best, option.profit);
        }
    }
    return best;
}

void test_sturdy_saddle() {
    Player player;
    player.gold = 100;
    const int original_capacity = player.npc_shop[n_Camel][nd_Effect];
    const bool purchased = Game::event_merchant(player, m_Sturdy_Saddle);
    assert(purchased);
    assert(player.npc_shop[n_Camel][nd_Effect] == original_capacity + 20);
}

void test_animal_lover() {
    Player player;
    const int original_camels = player.camel;
    const int original_food = player.food_consumption;
    Game::event_witch(player, w_Animal_Lover);
    assert(player.camel == original_camels + 2);
    assert(player.food_consumption == original_food + 2);
    assert(player.max_day == 14);
}

void test_interest_rounding() {
    Player player;
    player.gold = 1;
    player.trader = 0;
    player.npc_shop[n_Trader][nd_Effect] = 0;
    player.interest_rate = 2.8;
    Game::event_interest(player);
    assert(player.gold == 3);
}

void test_fenwick_matches_bruteforce() {
    Player player;
    const int gold_limit = 40;
    const int weight_limit = 20;
    FenwickTree2D tree(build_small_options(player, gold_limit, weight_limit));
    const auto [profit, buy_data] = tree.query(gold_limit, weight_limit);
    (void)buy_data;
    const uint32_t brute_profit = brute_force_profit(player, gold_limit, weight_limit);
    assert(profit == brute_profit);
}

void test_fenwick_serialize_round_trip() {
    Player player;
    const int gold_limit = 40;
    const int weight_limit = 20;
    const std::string key = "unit_test_tree.bin";
    FenwickTree2D original(build_small_options(player, gold_limit, weight_limit));
    original.serialize(key);

    FenwickTree2D loaded;
    loaded.deserialize(key);
    const auto original_result = original.query(gold_limit, weight_limit);
    const auto loaded_result = loaded.query(gold_limit, weight_limit);
    assert(original_result.first == loaded_result.first);
    assert(original_result.second.food == loaded_result.second.food);
    assert(original_result.second.spice == loaded_result.second.spice);
    assert(original_result.second.pottery == loaded_result.second.pottery);
    assert(original_result.second.marble == loaded_result.second.marble);
    assert(original_result.second.silk == loaded_result.second.silk);
    assert(original_result.second.jewelry == loaded_result.second.jewelry);
}

void test_run_buy_event_with_zero_food_consumption() {
    Player player;
    Game::event_witch(player, w_Oasis_of_Sanctifan);
    player.day_start_skip = false;
    player.gold = 100;

    BestPurchaseHandler handler;
    std::deque<Player> generated;
    const BuyEventStats stats = Helpers::run_buy_event(player, generated, handler);

    assert(player.food_consumption == 0);
    assert(stats.best_buy_queries >= 1);
    assert(!generated.empty());
}
}

int main() {
    test_sturdy_saddle();
    test_animal_lover();
    test_interest_rounding();
    test_fenwick_matches_bruteforce();
    test_fenwick_serialize_round_trip();
    test_run_buy_event_with_zero_food_consumption();
    std::cout << "All tests passed\n";
    return 0;
}
