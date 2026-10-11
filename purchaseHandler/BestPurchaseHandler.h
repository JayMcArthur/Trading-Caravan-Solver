//
// Created by Jay on 1/21/2025.
//

#ifndef TRADING_CARAVAN_SOLVER_BESTPURCHASEHANDLER_H
#define TRADING_CARAVAN_SOLVER_BESTPURCHASEHANDLER_H

#include <array>
#include <cstdint>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>
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
const disaster_options VALID_PURCHASE_DISASTERS[] = {
        d_No_Disaster,
        d_Marblecrisis,
        d_Silkpenury,
        d_Jewelrycrisis,
        d_Silkcrisis
};

template <typename T, size_t N>
bool isValidOption(const T value, const T (&validOptions)[N]) {
    return std::any_of(std::begin(validOptions), std::end(validOptions), [&value](const auto& option) {
        return value == option;
    });
}

struct ExactPurchaseChoice {
    uint32_t sale_value = 0;
    uint16_t actual_cost = 0;
    uint16_t actual_weight = 0;
    buy_list buys = buy_list();
};

class BestPurchaseHandler {
    static constexpr size_t MAX_EXACT_SALE_CACHE_ENTRIES = 4096;

public:
    // Non-owning query context; keep it within the lifetime of both market players.
    class ExactSaleMarketView {
    public:
        [[nodiscard]] std::shared_ptr<const std::vector<ExactPurchaseChoice>> frontier(
            uint16_t cost,
            uint16_t weight
        ) const;
        [[nodiscard]] std::shared_ptr<const std::vector<ExactPurchaseChoice>> bucket_frontier(
            uint16_t cost,
            uint16_t weight
        ) const;

    private:
        friend class BestPurchaseHandler;
        ExactSaleMarketView(
            BestPurchaseHandler& owner,
            uint32_t market_id,
            size_t market_hash,
            const Player& buy_market,
            const Player& sell_market
        ) noexcept;

        BestPurchaseHandler* owner = nullptr;
        uint32_t market_id = 0;
        size_t market_hash = 0;
        const Player* buy_market = nullptr;
        const Player* sell_market = nullptr;
    };

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
        uint64_t exact_sale_calls = 0;
        uint64_t exact_sale_cache_hits = 0;
        uint64_t exact_sale_hot_cache_hits = 0;
        uint64_t exact_sale_cache_entries = 0;
        double total_ms = 0.0;
        double query_ms = 0.0;
        double ensure_ms = 0.0;
        double exact_sale_ms = 0.0;
        ProfilingKeySummary slowest_total_key;
        ProfilingKeySummary slowest_query_key;
    };

    explicit BestPurchaseHandler(size_t exact_sale_cache_limit = MAX_EXACT_SALE_CACHE_ENTRIES);
    [[nodiscard]] ExactSaleMarketView prepareExactSaleMarket(
        const Player& buy_market,
        const Player& sell_market
    );
    std::pair<uint32_t, buy_list> getBestBuy(uint16_t cost, uint16_t weight, const Player& player);
    std::vector<PurchaseChoice> getBestBuyPortfolio(uint16_t cost, uint16_t weight, const Player& player);
    [[nodiscard]] std::shared_ptr<const std::vector<ExactPurchaseChoice>> getExactSaleFrontier(
        uint16_t cost,
        uint16_t weight,
        const Player& buy_market,
        const Player& sell_market
    );
    [[nodiscard]] std::shared_ptr<const std::vector<ExactPurchaseChoice>> getExactSaleBucketFrontier(
        uint16_t cost,
        uint16_t weight,
        const Player& buy_market,
        const Player& sell_market
    );
    void prebuildSupportedTrees();
    void warmSupportedTrees();
    bool verifySupportedTrees(uint16_t max_gold, uint16_t max_weight, std::ostream& out);
    [[nodiscard]] ProfilingSummary getProfilingSummary() const;
    [[nodiscard]] uint64_t loadedTreeMemoryBytes() const noexcept;
    [[nodiscard]] static std::string cacheKeyForWorld(const world& world_data) {
        return convertWorldToKey(world_data);
    }

private:
    struct ExactSaleQueryKey {
        uint32_t market_id = 0;
        uint16_t cost = 0;
        uint16_t weight = 0;
        size_t cached_hash = 0;

        [[nodiscard]] bool operator==(const ExactSaleQueryKey& other) const noexcept {
            return market_id == other.market_id && cost == other.cost && weight == other.weight;
        }
    };

    struct ExactSaleQueryKeyHash {
        [[nodiscard]] size_t operator()(const ExactSaleQueryKey& key) const noexcept;
    };

    struct ExactSaleHotEntry {
        ExactSaleQueryKey key;
        std::shared_ptr<const std::vector<ExactPurchaseChoice>> choices;
    };

    struct ProfilingKeyData {
        uint64_t calls = 0;
        std::chrono::nanoseconds total_time{0};
        std::chrono::nanoseconds query_time{0};
        std::chrono::nanoseconds ensure_time{0};
    };

    std::unordered_map<std::string, FenwickTree2D> loadedTrees;
    std::unordered_map<std::string, bool> prebuiltKeys;
    std::unordered_map<
        ExactSaleQueryKey,
        std::shared_ptr<const std::vector<ExactPurchaseChoice>>,
        ExactSaleQueryKeyHash
    > exactSaleCache;
    std::vector<std::array<int, 15>> exactSaleMarketSignatures;
    // Deferred ledgers revisit a small cycle of exact queries millions of
    // times. Shared immutable rows keep an in-progress resolution valid even
    // if bounded-cache eviction removes the map's ownership in the meantime.
    std::array<ExactSaleHotEntry, 256> exactSaleHotCache{};
    size_t exactSaleCacheLimit = MAX_EXACT_SALE_CACHE_ENTRIES;
    uint64_t profiling_calls = 0;
    uint64_t profiling_cache_misses = 0;
    std::chrono::nanoseconds profiling_total_time{0};
    std::chrono::nanoseconds profiling_query_time{0};
    std::chrono::nanoseconds profiling_ensure_time{0};
    uint64_t profiling_exact_sale_calls = 0;
    uint64_t profiling_exact_sale_cache_hits = 0;
    uint64_t profiling_exact_sale_hot_cache_hits = 0;
    std::chrono::nanoseconds profiling_exact_sale_time{0};
    std::unordered_map<std::string, ProfilingKeyData> profiling_per_key;
    static constexpr size_t MAX_LOADED_TREES = 12;
    void prepareCacheSlot(const std::string& key);
    void loadTree(const std::string& key);
    void saveTree(const std::string& key);
    void buildTree(const Player& player, const std::string& key);
    void buildSaleTree(const Player& buy_market, const Player& sell_market, const std::string& key);
    static std::vector<Player> buildSupportedWorldPlayers();
    static std::pair<uint32_t, buy_list> bruteForceBestBuy(const Player& player, uint16_t cost, uint16_t weight);
    static uint32_t calculateProfit(const Player& player, const buy_list& buy);
    static uint16_t calculateCost(const Player& player, const buy_list& buy);
    static uint16_t calculateWeight(const Player& player, const buy_list& buy);
    static uint32_t calculateSaleValue(const Player& player, const buy_list& buy);
    [[nodiscard]] std::shared_ptr<const std::vector<ExactPurchaseChoice>>
    getExactSaleFrontierPrepared(
        uint16_t cost,
        uint16_t weight,
        uint32_t market_id,
        size_t market_hash,
        const Player& buy_market,
        const Player& sell_market
    );
    static std::array<int, 15> saleMarketSignature(
        const Player& buy_market,
        const Player& sell_market
    );
    static size_t saleMarketSignatureHash(const std::array<int, 15>& signature) noexcept;
    static std::string saleMarketKey(const Player& buy_market, const Player& sell_market);
    static void generateElementsForWorld(const Player& player, std::vector<buy_option>& options);
    static void generateSaleElements(
        const Player& buy_market,
        const Player& sell_market,
        std::vector<buy_option>& options
    );

    static std::string convertWorldToKey(const world& world_data) {
        std::string converted;
        town_options town = static_cast<town_options>(world_data.town);
        merch_options merch = static_cast<merch_options>(world_data.merch);
        witch_options witch = static_cast<witch_options>(world_data.witch);
        disaster_options disaster = static_cast<disaster_options>(world_data.disaster);
        if (!isValidOption(town, VALID_TOWNS)) {
            town = t_Normalia;
        }
        if (!isValidOption(merch, VALID_MERCHANTS)) {
            merch = m_No_Merchant;
        }
        if (!isValidOption(witch, VALID_WITCHES)) {
            witch = w_No_Witch;
        }
        if (!isValidOption(disaster, VALID_PURCHASE_DISASTERS)) {
            disaster = d_No_Disaster;
        }

        std::string key = "tree_" + std::to_string(town) + "_" + std::to_string(merch) + "_" + std::to_string(witch);
        if (disaster != d_No_Disaster) {
            key += "_d" + std::to_string(disaster);
        }
        return key + ".bin";
    }
};

#endif //TRADING_CARAVAN_SOLVER_BESTPURCHASEHANDLER_H
