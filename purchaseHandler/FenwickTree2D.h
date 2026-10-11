//
// Created by Jay on 1/23/2025.
//

#ifndef TRADING_CARAVAN_SOLVER_FENWICKTREE2D_H
#define TRADING_CARAVAN_SOLVER_FENWICKTREE2D_H

#include <vector>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "../consts/enums.h"

struct PurchaseChoice {
    uint32_t profit = 0;
    uint16_t minimum_budget = 0;
    buy_list buys = buy_list();
};

struct PurchaseBreakpoint {
    uint32_t value = 0;
    uint16_t exact_cost = 0;
    buy_list buys = buy_list();
};

class FenwickTree2D {
public:
    FenwickTree2D() = default;
    explicit FenwickTree2D(const std::vector<buy_option>& elements);
    std::pair<uint32_t, buy_list> query(const uint16_t& gold, const uint16_t& weight) const;
    std::vector<PurchaseChoice> queryPortfolio(uint16_t gold, uint16_t weight) const;
    std::vector<PurchaseBreakpoint> queryFrontier(uint16_t gold, uint16_t weight) const;
    void serialize(const std::string& filename) const;
    void deserialize(const std::string& filename);
    [[nodiscard]] size_t memoryBytes() const noexcept;
private:
    struct LookupEntry {
        uint32_t profit = 0;
        buy_list buys = buy_list();
    };

    struct Breakpoint {
        uint32_t profit = 0;
        buy_list buys = buy_list();
        uint16_t cost = 0;
    };

    uint16_t max_cost = 0;
    uint16_t max_weight = 0;
    std::vector<std::vector<Breakpoint>> rows;
};


#endif //TRADING_CARAVAN_SOLVER_FENWICKTREE2D_H
