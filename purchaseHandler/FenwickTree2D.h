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

class FenwickTree2D {
public:
    FenwickTree2D() = default;
    explicit FenwickTree2D(const std::vector<buy_option>& elements);
    std::pair<uint32_t, buy_list> query(const uint16_t& gold, const uint16_t& weight);
    void serialize(const std::string& filename) const;
    void deserialize(const std::string& filename);
private:
    struct LookupEntry {
        uint32_t profit = 0;
        buy_list buys = buy_list();
    };

    uint16_t max_cost = 0;
    uint16_t max_weight = 0;
    std::vector<LookupEntry> table;

    [[nodiscard]] size_t index(uint16_t cost, uint16_t weight) const;
    void set_if_better(uint16_t cost, uint16_t weight, uint32_t profit, const buy_list& buys);
};


#endif //TRADING_CARAVAN_SOLVER_FENWICKTREE2D_H
