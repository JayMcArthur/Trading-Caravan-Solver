//
// Created by Jay on 1/21/2025.
//

#ifndef TRADING_CARAVAN_SOLVER_BESTPURCHASEHANDLER_H
#define TRADING_CARAVAN_SOLVER_BESTPURCHASEHANDLER_H

#include <fstream>
#include <filesystem>
#include <cmath>
#include <iostream>
#include "../consts/enums.h"
#include "../player/player.h"
#include "FenwickTree2D.h"

const int MAX_WEIGHT = 360; // This is from internal tests but could be a little higher
const int MAX_GOLD = 50000;

const town_options VALID_TOWNS[] = {
        t_Normalia,    // Default
        t_Mahaji,      // Spice Sell +5
        t_Huangoz,     // Marble Buy -20
        t_Cornecopea,  // Food is free
        t_Gayawaku,    // Food is weightless
        t_Ipartus      // Silk Buy -100
};
const merch_options VALID_MERCHANTS[] = {
        m_No_Merchant,    // Merch has not happened or can not afford
        m_Route_to_Mahaji // Spice Sell +20
};
const witch_options VALID_WITCHES[] = {
        w_No_Witch,           // Witch has not happened
        w_Vertue_of_Patience, // Jewelry Buy +100
        w_Midas_was_a_Trader, // Marble Sell -300
        w_Time_is_Money,      // Jewelry Sell -500
        w_Oasis_of_Sanctifan, // Silk Buy +200
        w_The_Stonecutter     // Marble Weight -15 and Buy +60
};

template <typename T, size_t N>
bool isValidOption(const uint8_t& value, const T (&validOptions)[N]) {
    return std::any_of(std::begin(validOptions), std::end(validOptions), [&value](const auto& option) {
        return value == option;
    });
}

class BestPurchaseHandler {
public:
    BestPurchaseHandler();
    std::pair<uint32_t, buy_list> getBestBuy(uint16_t cost, uint16_t weight, const Player& player);

private:
    std::unordered_map<std::string, FenwickTree2D> loadedTrees;
    std::unordered_map<std::string, bool> prebuiltKeys;
    void loadTree(const std::string& key);
    void saveTree(const std::string& key);
    void buildTree(const Player& player, const std::string& key);

    static std::string convertWorldToKey(const world& world_data) {
        std::string converted;
        int town = world_data.town;
        int merch = world_data.merch;
        int witch = world_data.witch;
        if (!isValidOption(town, VALID_TOWNS)) {
            town = t_Normalia;
        }
        if (!isValidOption(merch, VALID_MERCHANTS)) {
            merch = m_No_Merchant;
        }
        if (!isValidOption(witch, VALID_WITCHES)) {
            witch = w_No_Witch;
        }

        return "tree_" + std::to_string(town) + "_" + std::to_string(merch) + "_" + std::to_string(witch) + ".bin";
    }
    static void generateElementsForWorld(const Player& player, std::vector<buy_option>& options) {
        int max_spice = std::ceil(static_cast<double>(MAX_WEIGHT) / player.item_shop[i_Spice][id_Weight]);
        int max_pottery = std::ceil(static_cast<double>(MAX_WEIGHT) / player.item_shop[i_Pottery][id_Weight]);
        int max_marble = std::ceil(static_cast<double>(MAX_WEIGHT) / player.item_shop[i_Marble][id_Weight]);
        int max_silk = std::ceil(static_cast<double>(MAX_WEIGHT) / player.item_shop[i_Silk][id_Weight]);
        int max_jewelry = std::ceil(static_cast<double>(MAX_WEIGHT) / player.item_shop[i_Jewelry][id_Weight]);

        std::unordered_map<std::string, size_t> found;

        // 1 spice, 8 pottery, 1 marble

        for (int s = 0; s < max_spice; ++s) {
            for (int p = 0; p < max_pottery; ++p) {
                for (int m = 0; m < max_marble; ++m) {
                    for (int i = 0; i < max_silk; ++i) {
                        for (int j = 0; j < max_jewelry; ++j) {
                            // Calculate profit, weight, and cost
                            uint32_t profit = std::max(player.item_shop[i_Spice][id_Sell] - player.item_shop[i_Spice][id_Buy], 0) * s +
                                    std::max(player.item_shop[i_Pottery][id_Sell] - player.item_shop[i_Pottery][id_Buy], 0) * p +
                                    std::max(player.item_shop[i_Marble][id_Sell] - player.item_shop[i_Marble][id_Buy], 0) * m +
                                    std::max(player.item_shop[i_Silk][id_Sell] - player.item_shop[i_Silk][id_Buy], 0) * i +
                                    std::max(player.item_shop[i_Jewelry][id_Sell] - player.item_shop[i_Jewelry][id_Buy], 0) * j;
                            int weight = player.item_shop[i_Spice][id_Weight] * s +
                                         player.item_shop[i_Pottery][id_Weight] * p +
                                         player.item_shop[i_Marble][id_Weight] * m +
                                         player.item_shop[i_Silk][id_Weight] * i +
                                         player.item_shop[i_Jewelry][id_Weight] * j;
                            int cost = player.item_shop[i_Spice][id_Buy] * s +
                                       player.item_shop[i_Pottery][id_Buy] * p +
                                       player.item_shop[i_Marble][id_Buy] * m +
                                       player.item_shop[i_Silk][id_Buy] * i +
                                       player.item_shop[i_Jewelry][id_Buy] * j;

                            // Create a unique index using cost and weight
                            std::string index = std::to_string(cost) + "_" +  std::to_string(weight);

                            // Skip invalid combinations
                            if ((MAX_GOLD < cost && MAX_WEIGHT < weight) || profit == 0) {
                                continue;
                            }

                            // Check if index is new or update the result if profit is higher
                            auto it = found.find(index);
                            if (it == found.end()) {
                                // New combination, add to found and found_list
                                found[index] = options.size();
                                options.emplace_back(cost, weight, profit, buy_list(0, s, p, m, i, j));
                            } else {
                                size_t idx = it->second;
                                if (options[idx].profit < profit) {
                                    options[idx].profit = profit;
                                    options[idx].buy_data = buy_list(0, s, p, m, i, j);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

};

#endif //TRADING_CARAVAN_SOLVER_BESTPURCHASEHANDLER_H
