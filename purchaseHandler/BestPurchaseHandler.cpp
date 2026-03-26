//
// Created by Jay on 1/21/2025.
//

#include "BestPurchaseHandler.h"
#include "../game/Game.h"

BestPurchaseHandler::BestPurchaseHandler() {
    std::filesystem::path savedTreesPath = std::filesystem::current_path() / "Saved_Trees";
    std::filesystem::create_directories(savedTreesPath);
    for (const auto& entry : std::filesystem::directory_iterator(savedTreesPath)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".bin")  {
                // std::cout << filename << std::endl;
                prebuiltKeys[filename] = true;
            }
        }
    }
}

std::pair<uint32_t, buy_list> BestPurchaseHandler::getBestBuy(uint16_t cost, uint16_t weight, const Player& player) {
    const auto total_started = std::chrono::steady_clock::now();
    std::string key = convertWorldToKey(player.player_world);
    auto& key_profile = profiling_per_key[key];
    ++profiling_calls;
    ++key_profile.calls;

    const auto ensure_started = std::chrono::steady_clock::now();
    if (loadedTrees.find(key) == loadedTrees.end()) {
        ++profiling_cache_misses;
        bool loaded = false;
        if (prebuiltKeys.find(key) != prebuiltKeys.end()) {
            try {
                loadTree(key);
                loaded = true;
            } catch (const std::exception&) {
                prebuiltKeys.erase(key);
            }
        }
        if (!loaded) {
            buildTree(player, key);
            saveTree(key);
            prebuiltKeys[key] = true;
        }
    }
    const auto ensure_elapsed = std::chrono::steady_clock::now() - ensure_started;
    profiling_ensure_time += ensure_elapsed;
    key_profile.ensure_time += ensure_elapsed;

    const auto query_started = std::chrono::steady_clock::now();
    const auto result = loadedTrees[key].query(cost, weight);
    const auto query_elapsed = std::chrono::steady_clock::now() - query_started;
    profiling_query_time += query_elapsed;
    key_profile.query_time += query_elapsed;

    const auto total_elapsed = std::chrono::steady_clock::now() - total_started;
    profiling_total_time += total_elapsed;
    key_profile.total_time += total_elapsed;
    return result;
}

void BestPurchaseHandler::prebuildSupportedTrees() {
    for (const Player& player : buildSupportedWorldPlayers()) {
        const std::string key = convertWorldToKey(player.player_world);
        if (loadedTrees.find(key) == loadedTrees.end()) {
            bool loaded = false;
            if (prebuiltKeys.find(key) != prebuiltKeys.end()) {
                try {
                    loadTree(key);
                    loaded = true;
                } catch (const std::exception&) {
                    prebuiltKeys.erase(key);
                }
            }
            if (!loaded) {
                buildTree(player, key);
                saveTree(key);
                prebuiltKeys[key] = true;
            }
        }
    }
}

void BestPurchaseHandler::warmSupportedTrees() {
    for (const Player& player : buildSupportedWorldPlayers()) {
        const std::string key = convertWorldToKey(player.player_world);
        if (loadedTrees.find(key) != loadedTrees.end()) {
            std::cout << "[*] Warm tree already loaded: " << key << '\n';
            continue;
        }

        std::cout << "[*] Warming tree: " << key << '\n';
        bool loaded = false;
        if (prebuiltKeys.find(key) != prebuiltKeys.end()) {
            try {
                loadTree(key);
                loaded = true;
                std::cout << "[*] Loaded tree from disk: " << key << '\n';
            } catch (const std::exception&) {
                prebuiltKeys.erase(key);
            }
        }

        if (!loaded) {
            std::cout << "[*] Building missing warm tree: " << key << '\n';
            buildTree(player, key);
            saveTree(key);
            prebuiltKeys[key] = true;
            std::cout << "[*] Built and saved warm tree: " << key << '\n';
        }
    }
}

bool BestPurchaseHandler::verifySupportedTrees(const uint16_t max_gold, const uint16_t max_weight, std::ostream& out) {
    bool all_ok = true;
    for (const Player& player : buildSupportedWorldPlayers()) {
        const std::string key = convertWorldToKey(player.player_world);
        if (loadedTrees.find(key) == loadedTrees.end()) {
            bool loaded = false;
            if (prebuiltKeys.find(key) != prebuiltKeys.end()) {
                try {
                    loadTree(key);
                    loaded = true;
                } catch (const std::exception&) {
                    prebuiltKeys.erase(key);
                }
            }
            if (!loaded) {
                buildTree(player, key);
                saveTree(key);
                prebuiltKeys[key] = true;
            }
        }

        out << "[*] Verifying " << key << " up to gold=" << max_gold << " weight=" << max_weight << '\n';
        for (uint16_t gold = 0; gold <= max_gold; ++gold) {
            for (uint16_t weight = 0; weight <= max_weight; ++weight) {
                const auto fenwick = loadedTrees[key].query(gold, weight);
                const auto brute = bruteForceBestBuy(player, gold, weight);
                const uint16_t fenwick_cost = calculateCost(player, fenwick.second);
                const uint16_t fenwick_weight = calculateWeight(player, fenwick.second);
                const uint32_t fenwick_profit = calculateProfit(player, fenwick.second);
                if (fenwick_cost > gold || fenwick_weight > weight || fenwick_profit != fenwick.first || fenwick.first != brute.first) {
                    out << "[!] Mismatch for " << key
                        << " gold=" << gold
                        << " weight=" << weight
                        << " fenwick_profit=" << fenwick.first
                        << " brute_profit=" << brute.first
                        << " fenwick_cost=" << fenwick_cost
                        << " fenwick_weight=" << fenwick_weight
                        << " realized_profit=" << fenwick_profit << '\n';
                    all_ok = false;
                    return false;
                }
            }
        }
    }
    return all_ok;
}

void BestPurchaseHandler::loadTree(const std::string& key) {
    loadedTrees[key] = FenwickTree2D();
    try {
        loadedTrees[key].deserialize(key);
    } catch (const std::exception& e) {
        throw std::runtime_error("Deserialization failed for tree: " + key + ", Error: " + e.what());
    }
}

void BestPurchaseHandler::saveTree(const std::string& key) {
    loadedTrees[key].serialize(key);
}

void BestPurchaseHandler::buildTree(const Player& player, const std::string& key) {
    std::cout << "[*] Building Tree " << key << std::endl;
    std::vector<buy_option> elements;
    generateElementsForWorld(player, elements);
    loadedTrees[key] = FenwickTree2D(elements);
    std::cout << "[*] Build Finished" << std::endl;
}

std::vector<Player> BestPurchaseHandler::buildSupportedWorldPlayers() {
    std::vector<Player> players;
    players.reserve(std::size(VALID_TOWNS) * std::size(VALID_MERCHANTS) * std::size(VALID_WITCHES));

    for (const auto town : VALID_TOWNS) {
        Player base_player;
        Game::apply_town(base_player, town);

        for (const auto merch : VALID_MERCHANTS) {
            Player merch_player = base_player;
            if (merch != m_No_Merchant) {
                merch_player.gold = std::max<int>(merch_player.gold, 200);
                if (!Game::event_merchant(merch_player, merch)) {
                    continue;
                }
                merch_player.day_start_skip = false;
            }

            for (const auto witch : VALID_WITCHES) {
                Player world_player = merch_player;
                if (witch != w_No_Witch) {
                    Game::event_witch(world_player, witch);
                    world_player.day_start_skip = false;
                }
                players.emplace_back(std::move(world_player));
            }
        }
    }
    return players;
}

std::pair<uint32_t, buy_list> BestPurchaseHandler::bruteForceBestBuy(const Player& player, const uint16_t cost_limit, const uint16_t weight_limit) {
    uint32_t best_profit = 0;
    buy_list best_buy;

    const int max_spice = player.item_shop[i_Spice][id_Buy] == 0 ? weight_limit : std::min<int>(cost_limit / player.item_shop[i_Spice][id_Buy], weight_limit / std::max(player.item_shop[i_Spice][id_Weight], 1));
    const int max_pottery = player.item_shop[i_Pottery][id_Buy] == 0 ? weight_limit : std::min<int>(cost_limit / player.item_shop[i_Pottery][id_Buy], weight_limit / std::max(player.item_shop[i_Pottery][id_Weight], 1));
    const int max_marble = player.item_shop[i_Marble][id_Buy] == 0 ? weight_limit : std::min<int>(cost_limit / player.item_shop[i_Marble][id_Buy], weight_limit / std::max(player.item_shop[i_Marble][id_Weight], 1));
    const int max_silk = player.item_shop[i_Silk][id_Buy] == 0 ? weight_limit : std::min<int>(cost_limit / player.item_shop[i_Silk][id_Buy], weight_limit / std::max(player.item_shop[i_Silk][id_Weight], 1));
    const int max_jewelry = player.item_shop[i_Jewelry][id_Buy] == 0 ? weight_limit : std::min<int>(cost_limit / player.item_shop[i_Jewelry][id_Buy], weight_limit / std::max(player.item_shop[i_Jewelry][id_Weight], 1));

    for (int spice = 0; spice <= max_spice; ++spice) {
        for (int pottery = 0; pottery <= max_pottery; ++pottery) {
            for (int marble = 0; marble <= max_marble; ++marble) {
                for (int silk = 0; silk <= max_silk; ++silk) {
                    for (int jewelry = 0; jewelry <= max_jewelry; ++jewelry) {
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

                        if (cost > cost_limit || weight > weight_limit) {
                            continue;
                        }

                        const uint32_t profit =
                            spice * std::max(player.item_shop[i_Spice][id_Sell] - player.item_shop[i_Spice][id_Buy], 0) +
                            pottery * std::max(player.item_shop[i_Pottery][id_Sell] - player.item_shop[i_Pottery][id_Buy], 0) +
                            marble * std::max(player.item_shop[i_Marble][id_Sell] - player.item_shop[i_Marble][id_Buy], 0) +
                            silk * std::max(player.item_shop[i_Silk][id_Sell] - player.item_shop[i_Silk][id_Buy], 0) +
                            jewelry * std::max(player.item_shop[i_Jewelry][id_Sell] - player.item_shop[i_Jewelry][id_Buy], 0);

                        if (profit > best_profit) {
                            best_profit = profit;
                            best_buy = buy_list(0, spice, pottery, marble, silk, jewelry);
                        }
                    }
                }
            }
        }
    }

    return {best_profit, best_buy};
}

uint32_t BestPurchaseHandler::calculateProfit(const Player& player, const buy_list& buy) {
    return
        buy.spice * std::max(player.item_shop[i_Spice][id_Sell] - player.item_shop[i_Spice][id_Buy], 0) +
        buy.pottery * std::max(player.item_shop[i_Pottery][id_Sell] - player.item_shop[i_Pottery][id_Buy], 0) +
        buy.marble * std::max(player.item_shop[i_Marble][id_Sell] - player.item_shop[i_Marble][id_Buy], 0) +
        buy.silk * std::max(player.item_shop[i_Silk][id_Sell] - player.item_shop[i_Silk][id_Buy], 0) +
        buy.jewelry * std::max(player.item_shop[i_Jewelry][id_Sell] - player.item_shop[i_Jewelry][id_Buy], 0);
}

uint16_t BestPurchaseHandler::calculateCost(const Player& player, const buy_list& buy) {
    return static_cast<uint16_t>(
        buy.spice * player.item_shop[i_Spice][id_Buy] +
        buy.pottery * player.item_shop[i_Pottery][id_Buy] +
        buy.marble * player.item_shop[i_Marble][id_Buy] +
        buy.silk * player.item_shop[i_Silk][id_Buy] +
        buy.jewelry * player.item_shop[i_Jewelry][id_Buy]
    );
}

uint16_t BestPurchaseHandler::calculateWeight(const Player& player, const buy_list& buy) {
    return static_cast<uint16_t>(
        buy.spice * player.item_shop[i_Spice][id_Weight] +
        buy.pottery * player.item_shop[i_Pottery][id_Weight] +
        buy.marble * player.item_shop[i_Marble][id_Weight] +
        buy.silk * player.item_shop[i_Silk][id_Weight] +
        buy.jewelry * player.item_shop[i_Jewelry][id_Weight]
    );
}

BestPurchaseHandler::ProfilingSummary BestPurchaseHandler::getProfilingSummary() const {
    const auto to_ms = [](const std::chrono::nanoseconds duration) {
        return std::chrono::duration<double, std::milli>(duration).count();
    };

    ProfilingSummary summary;
    summary.calls = profiling_calls;
    summary.cache_misses = profiling_cache_misses;
    summary.total_ms = to_ms(profiling_total_time);
    summary.query_ms = to_ms(profiling_query_time);
    summary.ensure_ms = to_ms(profiling_ensure_time);

    double slowest_total_ms = -1.0;
    double slowest_query_ms = -1.0;
    for (const auto& [key, data] : profiling_per_key) {
        const double total_ms = to_ms(data.total_time);
        const double query_ms = to_ms(data.query_time);
        const double ensure_ms = to_ms(data.ensure_time);
        if (total_ms > slowest_total_ms) {
            slowest_total_ms = total_ms;
            summary.slowest_total_key = ProfilingKeySummary{key, data.calls, total_ms, query_ms, ensure_ms};
        }
        if (query_ms > slowest_query_ms) {
            slowest_query_ms = query_ms;
            summary.slowest_query_key = ProfilingKeySummary{key, data.calls, total_ms, query_ms, ensure_ms};
        }
    }

    return summary;
}
