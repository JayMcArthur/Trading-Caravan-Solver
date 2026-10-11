//
// Created by Jay on 1/21/2025.
//

#include "BestPurchaseHandler.h"
#include "../game/Game.h"

#include <sstream>

namespace {
struct PurchaseItemSpec {
    uint8_t item_index;
    uint16_t cost;
    uint16_t weight;
    uint32_t value;
};

void increment_item(buy_list& buys, const uint8_t item_index) {
    switch (item_index) {
        case i_Spice:
            ++buys.spice;
            break;
        case i_Pottery:
            ++buys.pottery;
            break;
        case i_Marble:
            ++buys.marble;
            break;
        case i_Silk:
            ++buys.silk;
            break;
        case i_Jewelry:
            ++buys.jewelry;
            break;
        default:
            break;
    }
}

void generate_purchase_elements(
    const Player& buy_market,
    const Player& sell_market,
    const bool value_total_sale,
    std::vector<buy_option>& options
) {
    std::vector<PurchaseItemSpec> items;
    items.reserve(5);
    for (uint8_t item = i_Spice; item <= i_Jewelry; ++item) {
        const int cost = buy_market.item_shop[item][id_Buy];
        const int weight = buy_market.item_shop[item][id_Weight];
        const int value = value_total_sale
            ? sell_market.item_shop[item][id_Sell]
            : sell_market.item_shop[item][id_Sell] - buy_market.item_shop[item][id_Buy];
        if (value > 0 && cost > 0 && weight > 0) {
            items.push_back(PurchaseItemSpec{
                item,
                static_cast<uint16_t>(cost),
                static_cast<uint16_t>(weight),
                static_cast<uint32_t>(value),
            });
        }
    }
    if (items.empty()) {
        return;
    }

    const size_t stride = static_cast<size_t>(MAX_WEIGHT) + 1;
    const size_t total_states = (static_cast<size_t>(MAX_GOLD) + 1) * stride;
    std::vector<uint8_t> reachable(total_states, 0);
    std::vector<uint32_t> best_value(total_states, 0);
    std::vector<buy_list> best_buys(total_states);
    reachable[0] = 1;

    for (uint16_t cost = 0; cost <= MAX_GOLD; ++cost) {
        const size_t row_offset = static_cast<size_t>(cost) * stride;
        for (uint16_t weight = 0; weight <= MAX_WEIGHT; ++weight) {
            const size_t index = row_offset + weight;
            if (!reachable[index]) {
                continue;
            }
            for (const PurchaseItemSpec& item : items) {
                const uint32_t next_cost = static_cast<uint32_t>(cost) + item.cost;
                const uint32_t next_weight = static_cast<uint32_t>(weight) + item.weight;
                if (next_cost > MAX_GOLD || next_weight > MAX_WEIGHT) {
                    continue;
                }
                const size_t next_index = static_cast<size_t>(next_cost) * stride + next_weight;
                const uint32_t next_value = best_value[index] + item.value;
                if (!reachable[next_index] || next_value > best_value[next_index]) {
                    reachable[next_index] = 1;
                    best_value[next_index] = next_value;
                    best_buys[next_index] = best_buys[index];
                    increment_item(best_buys[next_index], item.item_index);
                }
            }
        }
    }

    options.reserve((MAX_GOLD + 1) * 8);
    for (uint16_t cost = 0; cost <= MAX_GOLD; ++cost) {
        const size_t row_offset = static_cast<size_t>(cost) * stride;
        for (uint16_t weight = 0; weight <= MAX_WEIGHT; ++weight) {
            const size_t index = row_offset + weight;
            if (reachable[index] && best_value[index] > 0) {
                options.emplace_back(cost, weight, best_value[index], best_buys[index]);
            }
        }
    }
}

bool same_buy(const buy_list& lhs, const buy_list& rhs) {
    return lhs.food == rhs.food && lhs.spice == rhs.spice && lhs.pottery == rhs.pottery &&
           lhs.marble == rhs.marble && lhs.silk == rhs.silk && lhs.jewelry == rhs.jewelry;
}
}

BestPurchaseHandler::BestPurchaseHandler(const size_t exact_sale_cache_limit)
    : exactSaleCacheLimit(std::max<size_t>(exact_sale_cache_limit, 1)) {
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

BestPurchaseHandler::ExactSaleMarketView::ExactSaleMarketView(
    BestPurchaseHandler& owner_value,
    const uint32_t market_id_value,
    const size_t market_hash_value,
    const Player& buy_market_value,
    const Player& sell_market_value
) noexcept
    : owner(&owner_value),
      market_id(market_id_value),
      market_hash(market_hash_value),
      buy_market(&buy_market_value),
      sell_market(&sell_market_value) {}

std::shared_ptr<const std::vector<ExactPurchaseChoice>>
BestPurchaseHandler::ExactSaleMarketView::frontier(
    const uint16_t cost,
    const uint16_t weight
) const {
    return owner->getExactSaleFrontierPrepared(
        cost, weight, market_id, market_hash, *buy_market, *sell_market
    );
}

std::shared_ptr<const std::vector<ExactPurchaseChoice>>
BestPurchaseHandler::ExactSaleMarketView::bucket_frontier(
    const uint16_t cost,
    const uint16_t weight
) const {
    constexpr uint32_t COST_BUCKET_SIZE = 256;
    const uint32_t rounded =
        ((static_cast<uint32_t>(cost) + COST_BUCKET_SIZE - 1) / COST_BUCKET_SIZE) *
        COST_BUCKET_SIZE;
    const uint16_t bucket_cost = static_cast<uint16_t>(
        std::min<uint32_t>(rounded, MAX_GOLD)
    );
    return frontier(bucket_cost, weight);
}

BestPurchaseHandler::ExactSaleMarketView BestPurchaseHandler::prepareExactSaleMarket(
    const Player& buy_market,
    const Player& sell_market
) {
    const std::array<int, 15> signature = saleMarketSignature(buy_market, sell_market);
    const size_t market_hash = saleMarketSignatureHash(signature);
    const auto found = std::find(
        exactSaleMarketSignatures.begin(),
        exactSaleMarketSignatures.end(),
        signature
    );
    uint32_t market_id = 0;
    if (found == exactSaleMarketSignatures.end()) {
        market_id = static_cast<uint32_t>(exactSaleMarketSignatures.size());
        exactSaleMarketSignatures.push_back(signature);
    } else {
        market_id = static_cast<uint32_t>(
            std::distance(exactSaleMarketSignatures.begin(), found)
        );
    }
    return ExactSaleMarketView(
        *this, market_id, market_hash, buy_market, sell_market
    );
}

size_t BestPurchaseHandler::ExactSaleQueryKeyHash::operator()(const ExactSaleQueryKey& key) const noexcept {
    return key.cached_hash;
}

std::vector<PurchaseChoice> BestPurchaseHandler::getBestBuyPortfolio(
    const uint16_t cost,
    const uint16_t weight,
    const Player& player
) {
    (void)getBestBuy(cost, weight, player);
    const std::string key = convertWorldToKey(player.player_world);
    return loadedTrees.at(key).queryPortfolio(cost, weight);
}

std::shared_ptr<const std::vector<ExactPurchaseChoice>> BestPurchaseHandler::getExactSaleFrontier(
    const uint16_t cost,
    const uint16_t weight,
    const Player& buy_market,
    const Player& sell_market
) {
    return prepareExactSaleMarket(buy_market, sell_market).frontier(cost, weight);
}

std::shared_ptr<const std::vector<ExactPurchaseChoice>>
BestPurchaseHandler::getExactSaleFrontierPrepared(
    const uint16_t cost,
    const uint16_t weight,
    const uint32_t market_id,
    const size_t market_hash,
    const Player& buy_market,
    const Player& sell_market
) {
    const auto started = std::chrono::steady_clock::now();
    size_t query_hash = market_hash;
    query_hash ^= std::hash<uint16_t>{}(cost) + 0x9e3779b97f4a7c15ULL +
        (query_hash << 6U) + (query_hash >> 2U);
    query_hash ^= std::hash<uint16_t>{}(weight) + 0x9e3779b97f4a7c15ULL +
        (query_hash << 6U) + (query_hash >> 2U);
    const ExactSaleQueryKey query_key{market_id, cost, weight, query_hash};
    ++profiling_exact_sale_calls;
    ExactSaleHotEntry& hot = exactSaleHotCache[query_hash % exactSaleHotCache.size()];
    if (hot.choices && hot.key == query_key) {
        ++profiling_exact_sale_cache_hits;
        ++profiling_exact_sale_hot_cache_hits;
        profiling_exact_sale_time += std::chrono::steady_clock::now() - started;
        return hot.choices;
    }
    const auto cached = exactSaleCache.find(query_key);
    if (cached != exactSaleCache.end()) {
        ++profiling_exact_sale_cache_hits;
        hot = ExactSaleHotEntry{query_key, cached->second};
        profiling_exact_sale_time += std::chrono::steady_clock::now() - started;
        return cached->second;
    }
    const std::string key = saleMarketKey(buy_market, sell_market);
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
            buildSaleTree(buy_market, sell_market, key);
            saveTree(key);
            prebuiltKeys[key] = true;
        }
    }

    std::vector<ExactPurchaseChoice> choices;
    for (const PurchaseBreakpoint& point : loadedTrees.at(key).queryFrontier(cost, weight)) {
        if (point.value == 0) {
            continue;
        }
        choices.push_back(ExactPurchaseChoice{
            calculateSaleValue(sell_market, point.buys),
            calculateCost(buy_market, point.buys),
            calculateWeight(buy_market, point.buys),
            point.buys,
        });
    }

    for (uint8_t item = i_Spice; item <= i_Jewelry; ++item) {
        buy_list one;
        increment_item(one, item);
        const uint16_t actual_cost = calculateCost(buy_market, one);
        const uint16_t actual_weight = calculateWeight(buy_market, one);
        if (actual_cost <= cost && actual_weight <= weight &&
            std::none_of(choices.begin(), choices.end(), [&](const ExactPurchaseChoice& choice) {
                return same_buy(choice.buys, one);
            })) {
            choices.push_back(ExactPurchaseChoice{
                calculateSaleValue(sell_market, one),
                actual_cost,
                actual_weight,
                one,
            });
        }
    }

    std::vector<ExactPurchaseChoice> nondominated;
    nondominated.reserve(choices.size());
    for (size_t candidate_index = 0; candidate_index < choices.size(); ++candidate_index) {
        const ExactPurchaseChoice& candidate = choices[candidate_index];
        bool dominated = false;
        for (size_t other_index = 0; other_index < choices.size(); ++other_index) {
            if (candidate_index == other_index) {
                continue;
            }
            const ExactPurchaseChoice& other = choices[other_index];
            if (other.actual_cost <= candidate.actual_cost &&
                other.actual_weight <= candidate.actual_weight &&
                other.sale_value >= candidate.sale_value &&
                (other.actual_cost < candidate.actual_cost ||
                 other.actual_weight < candidate.actual_weight ||
                 other.sale_value > candidate.sale_value)) {
                dominated = true;
                break;
            }
        }
        if (!dominated) {
            nondominated.push_back(candidate);
        }
    }
    choices = std::move(nondominated);
    std::sort(choices.begin(), choices.end(), [](const ExactPurchaseChoice& lhs, const ExactPurchaseChoice& rhs) {
        return std::tie(lhs.actual_cost, lhs.actual_weight, lhs.sale_value) <
               std::tie(rhs.actual_cost, rhs.actual_weight, rhs.sale_value);
    });
    if (exactSaleCache.size() >= exactSaleCacheLimit) {
        exactSaleCache.clear();
        exactSaleHotCache = {};
    }
    auto immutable_choices = std::make_shared<const std::vector<ExactPurchaseChoice>>(
        std::move(choices)
    );
    const auto inserted = exactSaleCache.emplace(query_key, std::move(immutable_choices));
    ExactSaleHotEntry& inserted_hot = exactSaleHotCache[query_hash % exactSaleHotCache.size()];
    inserted_hot = ExactSaleHotEntry{query_key, inserted.first->second};
    profiling_exact_sale_time += std::chrono::steady_clock::now() - started;
    return inserted.first->second;
}

std::shared_ptr<const std::vector<ExactPurchaseChoice>> BestPurchaseHandler::getExactSaleBucketFrontier(
    const uint16_t cost,
    const uint16_t weight,
    const Player& buy_market,
    const Player& sell_market
) {
    // The returned choices are ordered by exact cost. Deferred purchase
    // resolution filters this small budget superset against the true gold
    // function, so nearby food budgets can share one immutable cache row.
    return prepareExactSaleMarket(buy_market, sell_market).bucket_frontier(cost, weight);
}

uint64_t BestPurchaseHandler::loadedTreeMemoryBytes() const noexcept {
    uint64_t bytes = 0;
    for (const auto& [key, tree] : loadedTrees) {
        (void)key;
        bytes += tree.memoryBytes();
    }
    bytes += exactSaleCache.bucket_count() * sizeof(void*);
    for (const auto& [key, choices] : exactSaleCache) {
        bytes += sizeof(key);
        bytes += sizeof(choices);
        if (choices) {
            bytes += sizeof(*choices) + choices->capacity() * sizeof(ExactPurchaseChoice);
        }
    }
    bytes += exactSaleMarketSignatures.capacity() * sizeof(std::array<int, 15>);
    return bytes;
}

void BestPurchaseHandler::prepareCacheSlot(const std::string& key) {
    if (loadedTrees.find(key) != loadedTrees.end()) {
        return;
    }
    if (loadedTrees.size() >= MAX_LOADED_TREES) {
        loadedTrees.clear();
    }
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
    prepareCacheSlot(key);
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
    prepareCacheSlot(key);
    std::vector<buy_option> elements;
    generateElementsForWorld(player, elements);
    loadedTrees[key] = FenwickTree2D(elements);
    std::cout << "[*] Build Finished" << std::endl;
}

void BestPurchaseHandler::buildSaleTree(
    const Player& buy_market,
    const Player& sell_market,
    const std::string& key
) {
    std::cout << "[*] Building exact sale tree " << key << std::endl;
    prepareCacheSlot(key);
    std::vector<buy_option> elements;
    generateSaleElements(buy_market, sell_market, elements);
    loadedTrees[key] = FenwickTree2D(elements);
    std::cout << "[*] Exact sale tree build finished" << std::endl;
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

uint32_t BestPurchaseHandler::calculateSaleValue(const Player& player, const buy_list& buy) {
    return static_cast<uint32_t>(
        buy.spice * std::max(player.item_shop[i_Spice][id_Sell], 0) +
        buy.pottery * std::max(player.item_shop[i_Pottery][id_Sell], 0) +
        buy.marble * std::max(player.item_shop[i_Marble][id_Sell], 0) +
        buy.silk * std::max(player.item_shop[i_Silk][id_Sell], 0) +
        buy.jewelry * std::max(player.item_shop[i_Jewelry][id_Sell], 0)
    );
}

std::string BestPurchaseHandler::saleMarketKey(const Player& buy_market, const Player& sell_market) {
    std::ostringstream key;
    key << "tree_sale_v1";
    for (int item = i_Spice; item <= i_Jewelry; ++item) {
        key << "_" << buy_market.item_shop[item][id_Buy]
            << "-" << buy_market.item_shop[item][id_Weight]
            << "-" << sell_market.item_shop[item][id_Sell];
    }
    key << ".bin";
    return key.str();
}

std::array<int, 15> BestPurchaseHandler::saleMarketSignature(
    const Player& buy_market,
    const Player& sell_market
) {
    std::array<int, 15> signature{};
    size_t index = 0;
    for (int item = i_Spice; item <= i_Jewelry; ++item) {
        signature[index++] = buy_market.item_shop[item][id_Buy];
        signature[index++] = buy_market.item_shop[item][id_Weight];
        signature[index++] = sell_market.item_shop[item][id_Sell];
    }
    return signature;
}

size_t BestPurchaseHandler::saleMarketSignatureHash(
    const std::array<int, 15>& signature
) noexcept {
    size_t seed = 0;
    for (const int value : signature) {
        seed ^= std::hash<int>{}(value) + 0x9e3779b97f4a7c15ULL +
            (seed << 6U) + (seed >> 2U);
    }
    return seed;
}

void BestPurchaseHandler::generateElementsForWorld(
    const Player& player,
    std::vector<buy_option>& options
) {
    generate_purchase_elements(player, player, false, options);
}

void BestPurchaseHandler::generateSaleElements(
    const Player& buy_market,
    const Player& sell_market,
    std::vector<buy_option>& options
) {
    generate_purchase_elements(buy_market, sell_market, true, options);
}

BestPurchaseHandler::ProfilingSummary BestPurchaseHandler::getProfilingSummary() const {
    const auto to_ms = [](const std::chrono::nanoseconds duration) {
        return std::chrono::duration<double, std::milli>(duration).count();
    };

    ProfilingSummary summary;
    summary.calls = profiling_calls;
    summary.cache_misses = profiling_cache_misses;
    summary.exact_sale_calls = profiling_exact_sale_calls;
    summary.exact_sale_cache_hits = profiling_exact_sale_cache_hits;
    summary.exact_sale_hot_cache_hits = profiling_exact_sale_hot_cache_hits;
    summary.exact_sale_cache_entries = exactSaleCache.size();
    summary.total_ms = to_ms(profiling_total_time);
    summary.query_ms = to_ms(profiling_query_time);
    summary.ensure_ms = to_ms(profiling_ensure_time);
    summary.exact_sale_ms = to_ms(profiling_exact_sale_time);

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
