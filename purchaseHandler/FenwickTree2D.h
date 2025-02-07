//
// Created by Jay on 1/23/2025.
//

#ifndef TRADING_CARAVAN_SOLVER_FENWICKTREE2D_H
#define TRADING_CARAVAN_SOLVER_FENWICKTREE2D_H

#include <map>
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
    void update(const uint16_t& cost, const uint16_t& weight, const uint32_t& profit, const buy_list& buys);
    std::pair<uint32_t, buy_list> query(const uint16_t& gold, const uint16_t& weight);
    void serialize(const std::string& filename) const;
    void deserialize(const std::string& filename);
private:
    std::vector<uint16_t> costs, weights;
    std::map<uint16_t, uint16_t> x_to_idx, y_to_idx;
    uint16_t max_x = 0;
    uint16_t max_y = 0;
    std::vector<std::vector<std::pair<uint32_t, buy_list>>> tree;
    static uint16_t normalize(const std::vector<uint16_t>& sorted_list, uint16_t value);
};


#endif //TRADING_CARAVAN_SOLVER_FENWICKTREE2D_H
