//
// Created by Jay on 1/23/2025.
//

#include "FenwickTree2D.h"

namespace {
constexpr uint32_t LOOKUP_TABLE_VERSION = 2;
}

FenwickTree2D::FenwickTree2D(const std::vector<buy_option>& elements) {
    for (const auto& element : elements) {
        max_cost = std::max(max_cost, element.cost);
        max_weight = std::max(max_weight, element.weight);
    }
    table.assign((static_cast<size_t>(max_cost) + 1) * (static_cast<size_t>(max_weight) + 1), LookupEntry{});

    for (const auto& [cost, weight, profit, buys] : elements) {
        if (cost <= max_cost && weight <= max_weight) {
            set_if_better(cost, weight, profit, buys);
        }
    }

    for (uint16_t cost = 0; cost <= max_cost; ++cost) {
        for (uint16_t weight = 0; weight <= max_weight; ++weight) {
            LookupEntry best = table[index(cost, weight)];
            if (cost > 0) {
                const LookupEntry& left = table[index(static_cast<uint16_t>(cost - 1), weight)];
                if (left.profit > best.profit) {
                    best = left;
                }
            }
            if (weight > 0) {
                const LookupEntry& up = table[index(cost, static_cast<uint16_t>(weight - 1))];
                if (up.profit > best.profit) {
                    best = up;
                }
            }
            table[index(cost, weight)] = best;
        }
    }
}

size_t FenwickTree2D::index(const uint16_t cost, const uint16_t weight) const {
    return static_cast<size_t>(cost) * (static_cast<size_t>(max_weight) + 1) + weight;
}

void FenwickTree2D::set_if_better(const uint16_t cost, const uint16_t weight, const uint32_t profit, const buy_list& buys) {
    LookupEntry& entry = table[index(cost, weight)];
    if (profit > entry.profit) {
        entry.profit = profit;
        entry.buys = buys;
    }
}

std::pair<uint32_t, buy_list> FenwickTree2D::query(const uint16_t& gold, const uint16_t& weight) {
    if (table.empty()) {
        return {0, buy_list()};
    }

    const uint16_t clamped_gold = std::min<uint16_t>(gold, max_cost);
    const uint16_t clamped_weight = std::min<uint16_t>(weight, max_weight);
    const LookupEntry& entry = table[index(clamped_gold, clamped_weight)];
    return {entry.profit, entry.buys};
}

void FenwickTree2D::serialize(const std::string& filename) const {
    std::filesystem::path savedTreesPath = std::filesystem::current_path() / "Saved_Trees";
    std::filesystem::create_directories(savedTreesPath);
    std::filesystem::path filepath = savedTreesPath / filename;
    std::ofstream out(filepath, std::ios::binary);
    if (!out) {
        throw std::ios_base::failure("Failed to open file for writing.");
    }

    out.write(reinterpret_cast<const char*>(&LOOKUP_TABLE_VERSION), sizeof(LOOKUP_TABLE_VERSION));
    out.write(reinterpret_cast<const char*>(&max_cost), sizeof(max_cost));
    out.write(reinterpret_cast<const char*>(&max_weight), sizeof(max_weight));
    size_t table_size = table.size();
    out.write(reinterpret_cast<const char*>(&table_size), sizeof(table_size));
    for (const auto& entry : table) {
        out.write(reinterpret_cast<const char*>(&entry.profit), sizeof(entry.profit));
        entry.buys.serialize(out);
    }
    out.close();
}

void FenwickTree2D::deserialize(const std::string& filename) {
    std::filesystem::path savedTreesPath = std::filesystem::current_path() / "Saved_Trees";
    std::filesystem::path filepath = savedTreesPath / filename;
    std::ifstream in(filepath, std::ios::binary);
    if (!in) {
        throw std::ios_base::failure("Failed to open file for reading.");
    }

    uint32_t version = 0;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != LOOKUP_TABLE_VERSION) {
        throw std::runtime_error("Unsupported purchase table version");
    }

    in.read(reinterpret_cast<char*>(&max_cost), sizeof(max_cost));
    in.read(reinterpret_cast<char*>(&max_weight), sizeof(max_weight));
    size_t table_size = 0;
    in.read(reinterpret_cast<char*>(&table_size), sizeof(table_size));
    table.resize(table_size);
    for (auto& entry : table) {
        in.read(reinterpret_cast<char*>(&entry.profit), sizeof(entry.profit));
        entry.buys.deserialize(in);
    }
    in.close();
}
