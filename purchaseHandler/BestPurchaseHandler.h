//
// Created by Jay on 1/21/2025.
//

#ifndef TRADING_CARAVAN_SOLVER_BESTPURCHASEHANDLER_H
#define TRADING_CARAVAN_SOLVER_BESTPURCHASEHANDLER_H

#include <fstream>
#include <chrono>
#include <filesystem>
#include <cmath>
#include <iostream>
#include <limits>
#include <unordered_map>
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
    struct ProfilingKeySummary {
        std::string key;
        uint64_t calls = 0;
        double total_ms = 0.0;
        double query_ms = 0.0;
        double ensure_ms = 0.0;
    };

    struct ProfilingSummary {
        uint64_t calls = 0;
        uint64_t cache_misses = 0;
        double total_ms = 0.0;
        double query_ms = 0.0;
        double ensure_ms = 0.0;
        ProfilingKeySummary slowest_total_key;
        ProfilingKeySummary slowest_query_key;
    };

    BestPurchaseHandler();
    std::pair<uint32_t, buy_list> getBestBuy(uint16_t cost, uint16_t weight, const Player& player);
    void prebuildSupportedTrees();
    void warmSupportedTrees();
    bool verifySupportedTrees(uint16_t max_gold, uint16_t max_weight, std::ostream& out);
    [[nodiscard]] ProfilingSummary getProfilingSummary() const;

private:
    struct ProfilingKeyData {
        uint64_t calls = 0;
        std::chrono::nanoseconds total_time{0};
        std::chrono::nanoseconds query_time{0};
        std::chrono::nanoseconds ensure_time{0};
    };

    std::unordered_map<std::string, FenwickTree2D> loadedTrees;
    std::unordered_map<std::string, bool> prebuiltKeys;
    uint64_t profiling_calls = 0;
    uint64_t profiling_cache_misses = 0;
    std::chrono::nanoseconds profiling_total_time{0};
    std::chrono::nanoseconds profiling_query_time{0};
    std::chrono::nanoseconds profiling_ensure_time{0};
    std::unordered_map<std::string, ProfilingKeyData> profiling_per_key;
    void loadTree(const std::string& key);
    void saveTree(const std::string& key);
    void buildTree(const Player& player, const std::string& key);
    static std::vector<Player> buildSupportedWorldPlayers();
    static std::pair<uint32_t, buy_list> bruteForceBestBuy(const Player& player, uint16_t cost, uint16_t weight);
    static uint32_t calculateProfit(const Player& player, const buy_list& buy);
    static uint16_t calculateCost(const Player& player, const buy_list& buy);
    static uint16_t calculateWeight(const Player& player, const buy_list& buy);

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
        struct ItemSpec {
            uint8_t item_index;
            uint16_t cost;
            uint16_t weight;
            uint32_t profit;
        };

        std::vector<ItemSpec> profitable_items;
        profitable_items.reserve(5);
        for (uint8_t item = i_Spice; item <= i_Jewelry; ++item) {
            const int profit = player.item_shop[item][id_Sell] - player.item_shop[item][id_Buy];
            const int cost = player.item_shop[item][id_Buy];
            const int weight = player.item_shop[item][id_Weight];
            if (profit > 0 && cost > 0 && weight > 0) {
                profitable_items.push_back(ItemSpec{
                    item,
                    static_cast<uint16_t>(cost),
                    static_cast<uint16_t>(weight),
                    static_cast<uint32_t>(profit)
                });
            }
        }

        if (profitable_items.empty()) {
            return;
        }

        const size_t stride = static_cast<size_t>(MAX_WEIGHT) + 1;
        const size_t total_states = (static_cast<size_t>(MAX_GOLD) + 1) * stride;
        std::vector<uint8_t> reachable(total_states, 0);
        std::vector<uint32_t> best_profit(total_states, 0);
        std::vector<buy_list> best_buys(total_states);

        reachable[0] = 1;

        for (uint16_t cost = 0; cost <= MAX_GOLD; ++cost) {
            const size_t row_offset = static_cast<size_t>(cost) * stride;
            for (uint16_t weight = 0; weight <= MAX_WEIGHT; ++weight) {
                const size_t index = row_offset + weight;
                if (!reachable[index]) {
                    continue;
                }

                for (const auto& item : profitable_items) {
                    const uint32_t next_cost = static_cast<uint32_t>(cost) + item.cost;
                    const uint32_t next_weight = static_cast<uint32_t>(weight) + item.weight;
                    if (next_cost > MAX_GOLD || next_weight > MAX_WEIGHT) {
                        continue;
                    }

                    const size_t next_index = static_cast<size_t>(next_cost) * stride + next_weight;
                    const uint32_t next_profit = best_profit[index] + item.profit;
                    if (!reachable[next_index] || next_profit > best_profit[next_index]) {
                        reachable[next_index] = 1;
                        best_profit[next_index] = next_profit;
                        best_buys[next_index] = best_buys[index];
                        switch (item.item_index) {
                            case i_Spice:
                                ++best_buys[next_index].spice;
                                break;
                            case i_Pottery:
                                ++best_buys[next_index].pottery;
                                break;
                            case i_Marble:
                                ++best_buys[next_index].marble;
                                break;
                            case i_Silk:
                                ++best_buys[next_index].silk;
                                break;
                            case i_Jewelry:
                                ++best_buys[next_index].jewelry;
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        }

        options.reserve((MAX_GOLD + 1) * 8);
        for (uint16_t cost = 0; cost <= MAX_GOLD; ++cost) {
            const size_t row_offset = static_cast<size_t>(cost) * stride;
            for (uint16_t weight = 0; weight <= MAX_WEIGHT; ++weight) {
                const size_t index = row_offset + weight;
                if (reachable[index] && best_profit[index] > 0) {
                    options.emplace_back(cost, weight, best_profit[index], best_buys[index]);
                }
            }
        }
    }

};

#endif //TRADING_CARAVAN_SOLVER_BESTPURCHASEHANDLER_H
