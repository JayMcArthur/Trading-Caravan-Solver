//
// Created by Jay on 1/23/2025.
//

#include "FenwickTree2D.h"

FenwickTree2D::FenwickTree2D(const std::vector<buy_option>& elements) {
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
    for (long unsigned int i = 0; i < costs.size(); ++i) x_to_idx[costs[i]] = i + 1;
    for (long unsigned int i = 0; i < weights.size(); ++i) y_to_idx[weights[i]] = i + 1;

    max_x = costs.size();
    max_y = weights.size();

    tree.resize(max_x + 1, std::vector<std::pair<uint32_t, buy_list>>(max_y + 1, {0, buy_list()}));

    // Populate the Fenwick Tree
    for (const auto& [cost, weight, profit, buys] : elements) {
        update(cost, weight, profit, buys);
    }
}

uint16_t FenwickTree2D::normalize(const std::vector<uint16_t>& sorted_list, uint16_t value) {
    auto it = std::upper_bound(sorted_list.begin(), sorted_list.end(), value);
    return it == sorted_list.begin() ? 0 : (it - sorted_list.begin());
}

void FenwickTree2D::update(const uint16_t& cost, const uint16_t& weight, const uint32_t& profit, const buy_list& buys) {
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

std::pair<uint32_t, buy_list> FenwickTree2D::query(const uint16_t& gold, const uint16_t& weight) {
    if (costs.empty() || weights.empty()) {
        return {0, buy_list()}; // Return default values if the tree is empty
    }

    uint16_t norm_x = normalize(costs, gold); // 900
    uint16_t norm_y = normalize(weights, weight); // 54

    uint32_t max_value = 0;
    buy_list best_buys = buy_list();
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

void FenwickTree2D::serialize(const std::string& filename) const {
    std::filesystem::path savedTreesPath = std::filesystem::current_path().parent_path() / "Saved_Trees";
    std::filesystem::path filepath = savedTreesPath / filename;
    std::ofstream out(filepath, std::ios::binary);
    // std::cout << "Start Serialize" << std::endl;
    // std::cout << "File: " << filepath << std::endl;
    if (!out) {
        throw std::ios_base::failure("Failed to open file for writing.");
    }

    unsigned long int costs_size = costs.size();
    unsigned long int weights_size = weights.size();
    // std::cout << "Cost Size: " << costs_size << ", Weight Size: " << weights_size << std::endl;
    out.write(reinterpret_cast<const char*>(&costs_size), sizeof(costs_size));
    out.write(reinterpret_cast<const char*>(&weights_size), sizeof(weights_size));
    out.write(reinterpret_cast<const char*>(costs.data()), costs_size * sizeof(uint16_t));
    out.write(reinterpret_cast<const char*>(weights.data()), weights_size * sizeof(uint16_t));

    for (const auto& row : tree) {
        for (const auto& cell : row) {
            out.write(reinterpret_cast<const char*>(&cell.first), sizeof(uint32_t));
            cell.second.serialize(out);
        }
    }

    // std::cout << "End Serialize" << std::endl;
    out.close();
}

void FenwickTree2D::deserialize(const std::string& filename) {
    std::filesystem::path savedTreesPath = std::filesystem::current_path().parent_path() / "Saved_Trees";
    std::filesystem::path filepath = savedTreesPath / filename;
    // std::cout << "Start Deserialize" << std::endl;
    // std::cout << "File: " << filepath << std::endl;
    std::ifstream in(filepath, std::ios::binary);
    if (!in) {
        throw std::ios_base::failure("Failed to open file for reading.");
    }

    unsigned long int costs_size, weights_size;
    in.read(reinterpret_cast<char*>(&costs_size), sizeof(costs_size));
    in.read(reinterpret_cast<char*>(&weights_size), sizeof(weights_size));
    // std::cout << "Cost Size: " << costs_size << ", Weight Size: " << weights_size << std::endl;

    costs.resize(costs_size);
    weights.resize(weights_size);
    in.read(reinterpret_cast<char*>(costs.data()), costs_size * sizeof(uint16_t));
    in.read(reinterpret_cast<char*>(weights.data()), weights_size * sizeof(uint16_t));

    x_to_idx.clear();
    y_to_idx.clear();
    for (long unsigned int i = 0; i < costs.size(); ++i) x_to_idx[costs[i]] = i + 1;
    for (long unsigned int i = 0; i < weights.size(); ++i) y_to_idx[weights[i]] = i + 1;

    max_x = costs.size();
    max_y = weights.size();
    // std::cout << "max_x: " << max_x << ", max_y: " << max_y << std::endl;

    // Resize the Fenwick tree and read its data
    tree.clear();
    tree.resize(max_x + 1, std::vector<std::pair<uint32_t, buy_list>>(max_y + 1, {0, buy_list()}));

    for (size_t x = 0; x <= max_x; ++x) {
        for (size_t y = 0; y <= max_y; ++y) {
            uint32_t profit;
            in.read(reinterpret_cast<char*>(&profit), sizeof(profit));
            buy_list buys;
            buys.deserialize(in);
            tree[x][y] = {profit, buys};
        }
    }

    in.close();
}