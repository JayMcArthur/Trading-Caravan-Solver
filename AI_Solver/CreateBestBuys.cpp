//
// Created by Jay on 1/21/2025.
//

#include "CreateBestBuys.h"


FenwickTree2D(const std::vector<buy_option>& elements) {
    for (const auto& [cost, weight, _, __] : elements) {
        costs.push_back(cost);
        weights.push_back(weight);
    }
    // Remove duplicates and sort
    std::sort(costs.begin(), costs.end());
    costs.erase(std::unique(costs.begin(), costs.end()), costs.end());
    std::sort(weights.begin(), weights.end());
    weights.erase(std::unique(weights.begin(), weights.end()), weights.end());

    // Map normalized indices
    for (int i = 0; i < costs.size(); ++i) x_to_idx[costs[i]] = i + 1;
    for (int i = 0; i < weights.size(); ++i) y_to_idx[weights[i]] = i + 1;

    max_x = costs.size();
    max_y = weights.size();

    tree.resize(max_x + 1, std::vector<std::pair<int, std::vector<int>>>(max_y + 1, {INT_MIN, {}}));

    // Populate the Fenwick Tree
    for (const auto& [cost, weight, profit, buys] : elements) {
        update(cost, weight, profit, buys);
    }
}

uint16_t FenwickTree2D::normalize(const std::vector<uint16_t>& sorted_list, uint16_t value) {
    auto it = std::upper_bound(sorted_list.begin(), sorted_list.end(), value);
    return it == sorted_list.begin() ? 0 : (it - sorted_list.begin());
}

void FenwickTree2D::update(int cost, int weight, int profit, const buy_list& buys) {
    int xi = x_to_idx[cost];
    while (xi <= max_x) {
        int yi = y_to_idx[weight];
        while (yi <= max_y) {
            if (profit > tree[xi][yi].first) {
                tree[xi][yi] = {profit, buys};
            }
            yi += yi & -yi;
        }
        xi += xi & -xi;
    }
}

std::pair<uint32_t, buy_list> FenwickTree2D::query(uint16_t gold, uint16_t weight) {
    uint16_t norm_x = normalize(costs, gold);
    uint16_t norm_y = normalize(weights, weight);

    int max_value = INT_MIN;
    std::vector<int> best_buys;
    int xi = norm_x;
    while (xi > 0) {
        int yi = norm_y;
        while (yi > 0) {
            if (tree[xi][yi].first > max_value) {
                max_value = tree[xi][yi].first;
                best_buys = tree[xi][yi].second;
            }
            yi -= yi & -yi;
        }
        xi -= xi & -xi;
    }
    return {max_value, best_buys};
}