//
// Created by Jay on 1/21/2025.
//

#ifndef TRADING_CARAVAN_SOLVER_CREATEBESTBUYS_H
#define TRADING_CARAVAN_SOLVER_CREATEBESTBUYS_H

using namespace std;

#include <vector>
#include <tuple>
#include <map>
#include <algorithm>
#include <iostream>
#include <climits>
#include "enums.h"

struct buy_option {
    uint16_t gold;
    uint16_t weight;
    uint32_t profit;
    buy_list buy_data;
};

class CreateBestBuys {
    // Has a list of Fenwick Trees
    // has a list of keys
    // I pass it a Cost, Weight, and World
    // It throws world into converter. checks to see if it is prebuilt, if not checks to see if it is saved, if not the I built it and save it
    // I then pas the cost and weight to the fenwick tree and get the best buy
    // I can use Binary Serialization
    // some memory types can be made smaller (2^16 for weight and gold?)
    // buys can be a struct with 2^8 for each buy
};



class FenwickTree2D {
private:
    std::vector<uint16_t> costs, weights;
    std::map<uint16_t, uint16_t> x_to_idx, y_to_idx;
    uint16_t max_x, max_y;
    std::vector<std::vector<std::pair<uint32_t, buy_list>>> tree;
    uint16_t normalize(const std::vector<uint16_t>& sorted_list, uint16_t value);

public:
    FenwickTree2D(const std::vector<buy_option>& elements);
    void update(uint16_t cost, uint16_t weight, uint32_t profit, const buy_list& buys);
    std::pair<uint32_t, buy_list> query(uint16_t gold, uint16_t weight) const;
    void serialize(const std::string& filename) const;
    static FenwickTree2D deserialize(const std::string& filename);
};

/*
int main() {
    // Input data
    std::vector<std::tuple<int, int, int, std::vector<int>>> elements = {
        {1, 1, 5, {1}},
        {3, 3, 7, {2}},
        {4, 2, 8, {3}},
        {5, 1, 9, {4}},
    };

    // Initialize Fenwick Tree with elements
    FenwickTree2D fenwick(elements);

    // Query Fenwick Tree
    std::vector<std::pair<int, int>> queries = {{3, 5}, {5, 2}, {1, 1}};
    for (const auto& [gold, weight] : queries) {
        auto [max_profit, best_buys] = fenwick.query(gold, weight);
        std::cout << "Max profit: " << max_profit << ", Best buys: ";
        for (int buy : best_buys) {
            std::cout << buy << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
*/

#endif //TRADING_CARAVAN_SOLVER_CREATEBESTBUYS_H
