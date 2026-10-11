//
// Created by Jay on 1/23/2025.
//

#include "FenwickTree2D.h"

#include <limits>

namespace {
constexpr uint32_t DENSE_LOOKUP_TABLE_VERSION = 2;
constexpr uint32_t LOOKUP_TABLE_VERSION = 3;
}

FenwickTree2D::FenwickTree2D(const std::vector<buy_option>& elements) {
    for (const auto& element : elements) {
        max_cost = std::max(max_cost, element.cost);
        max_weight = std::max(max_weight, element.weight);
    }
    const size_t stride = static_cast<size_t>(max_weight) + 1;
    std::vector<LookupEntry> dense((static_cast<size_t>(max_cost) + 1) * stride);
    const auto index = [stride](const uint16_t cost, const uint16_t weight) {
        return static_cast<size_t>(cost) * stride + weight;
    };

    for (const auto& [cost, weight, profit, buys] : elements) {
        if (cost <= max_cost && weight <= max_weight) {
            LookupEntry& entry = dense[index(cost, weight)];
            if (profit > entry.profit) {
                entry.profit = profit;
                entry.buys = buys;
            }
        }
    }

    for (uint16_t cost = 0; cost <= max_cost; ++cost) {
        for (uint16_t weight = 0; weight <= max_weight; ++weight) {
            LookupEntry best = dense[index(cost, weight)];
            if (cost > 0) {
                const LookupEntry& left = dense[index(static_cast<uint16_t>(cost - 1), weight)];
                if (left.profit > best.profit) {
                    best = left;
                }
            }
            if (weight > 0) {
                const LookupEntry& up = dense[index(cost, static_cast<uint16_t>(weight - 1))];
                if (up.profit > best.profit) {
                    best = up;
                }
            }
            dense[index(cost, weight)] = best;
        }
    }

    rows.resize(static_cast<size_t>(max_weight) + 1);
    for (uint16_t weight = 0; weight <= max_weight; ++weight) {
        auto& row = rows[weight];
        uint32_t last_profit = std::numeric_limits<uint32_t>::max();
        for (uint16_t cost = 0; cost <= max_cost; ++cost) {
            const LookupEntry& entry = dense[index(cost, weight)];
            if (entry.profit != last_profit) {
                row.push_back(Breakpoint{entry.profit, entry.buys, cost});
                last_profit = entry.profit;
            }
        }
    }
}

std::pair<uint32_t, buy_list> FenwickTree2D::query(const uint16_t& gold, const uint16_t& weight) const {
    if (rows.empty()) {
        return {0, buy_list()};
    }

    const uint16_t clamped_gold = std::min<uint16_t>(gold, max_cost);
    const uint16_t clamped_weight = std::min<uint16_t>(weight, max_weight);
    const auto& row = rows[clamped_weight];
    const auto after = std::upper_bound(row.begin(), row.end(), clamped_gold, [](const uint16_t cost, const Breakpoint& point) {
        return cost < point.cost;
    });
    const Breakpoint& point = *(after - 1);
    return {point.profit, point.buys};
}

std::vector<PurchaseChoice> FenwickTree2D::queryPortfolio(const uint16_t gold, const uint16_t weight) const {
    std::vector<PurchaseChoice> choices;
    if (rows.empty()) {
        return choices;
    }

    const uint16_t clamped_gold = std::min<uint16_t>(gold, max_cost);
    const uint16_t clamped_weight = std::min<uint16_t>(weight, max_weight);
    const auto& row = rows[clamped_weight];
    const auto after = std::upper_bound(row.begin(), row.end(), clamped_gold, [](const uint16_t cost, const Breakpoint& point) {
        return cost < point.cost;
    });
    const Breakpoint& maximum = *(after - 1);

    const auto same_buys = [](const buy_list& lhs, const buy_list& rhs) {
        return lhs.food == rhs.food &&
               lhs.spice == rhs.spice &&
               lhs.pottery == rhs.pottery &&
               lhs.marble == rhs.marble &&
               lhs.silk == rhs.silk &&
               lhs.jewelry == rhs.jewelry;
    };
    const auto append = [&choices, &same_buys](const Breakpoint& point) {
        if (std::none_of(choices.begin(), choices.end(), [&point, &same_buys](const PurchaseChoice& choice) {
            return same_buys(choice.buys, point.buys);
        })) {
            choices.push_back(PurchaseChoice{point.profit, point.cost, point.buys});
        }
    };

    append(maximum);
    if (maximum.profit == 0) {
        return choices;
    }

    for (const auto [numerator, denominator] : {std::pair<uint32_t, uint32_t>{9, 10}, {3, 4}}) {
        const uint32_t target = (maximum.profit * numerator + denominator - 1) / denominator;
        const auto point = std::lower_bound(row.begin(), after, target, [](const Breakpoint& candidate, const uint32_t wanted_profit) {
            return candidate.profit < wanted_profit;
        });
        if (point != after) {
            append(*point);
        }
    }
    return choices;
}

std::vector<PurchaseBreakpoint> FenwickTree2D::queryFrontier(
    const uint16_t gold,
    const uint16_t weight
) const {
    std::vector<PurchaseBreakpoint> frontier;
    if (rows.empty()) {
        return frontier;
    }

    const uint16_t clamped_gold = std::min<uint16_t>(gold, max_cost);
    const uint16_t clamped_weight = std::min<uint16_t>(weight, max_weight);
    const auto& row = rows[clamped_weight];
    const auto after = std::upper_bound(row.begin(), row.end(), clamped_gold, [](const uint16_t cost, const Breakpoint& point) {
        return cost < point.cost;
    });
    frontier.reserve(static_cast<size_t>(std::distance(row.begin(), after)));
    for (auto point = row.begin(); point != after; ++point) {
        frontier.push_back(PurchaseBreakpoint{point->profit, point->cost, point->buys});
    }
    return frontier;
}

size_t FenwickTree2D::memoryBytes() const noexcept {
    size_t bytes = rows.capacity() * sizeof(std::vector<Breakpoint>);
    for (const auto& row : rows) {
        bytes += row.capacity() * sizeof(Breakpoint);
    }
    return bytes;
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
    for (const auto& row : rows) {
        const uint32_t row_size = static_cast<uint32_t>(row.size());
        out.write(reinterpret_cast<const char*>(&row_size), sizeof(row_size));
        for (const auto& point : row) {
            out.write(reinterpret_cast<const char*>(&point.cost), sizeof(point.cost));
            out.write(reinterpret_cast<const char*>(&point.profit), sizeof(point.profit));
            point.buys.serialize(out);
        }
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
    if (version != LOOKUP_TABLE_VERSION && version != DENSE_LOOKUP_TABLE_VERSION) {
        throw std::runtime_error("Unsupported purchase table version");
    }

    in.read(reinterpret_cast<char*>(&max_cost), sizeof(max_cost));
    in.read(reinterpret_cast<char*>(&max_weight), sizeof(max_weight));
    rows.clear();
    rows.resize(static_cast<size_t>(max_weight) + 1);

    if (version == LOOKUP_TABLE_VERSION) {
        for (auto& row : rows) {
            uint32_t row_size = 0;
            in.read(reinterpret_cast<char*>(&row_size), sizeof(row_size));
            row.resize(row_size);
            for (auto& point : row) {
                in.read(reinterpret_cast<char*>(&point.cost), sizeof(point.cost));
                in.read(reinterpret_cast<char*>(&point.profit), sizeof(point.profit));
                point.buys.deserialize(in);
            }
        }
    } else {
        size_t table_size = 0;
        in.read(reinterpret_cast<char*>(&table_size), sizeof(table_size));
        const size_t expected_size = (static_cast<size_t>(max_cost) + 1) * (static_cast<size_t>(max_weight) + 1);
        if (table_size != expected_size) {
            throw std::runtime_error("Corrupt dense purchase table size");
        }
        std::vector<uint32_t> last_profit(rows.size(), std::numeric_limits<uint32_t>::max());
        for (uint16_t cost = 0; cost <= max_cost; ++cost) {
            for (uint16_t weight = 0; weight <= max_weight; ++weight) {
                LookupEntry entry;
                in.read(reinterpret_cast<char*>(&entry.profit), sizeof(entry.profit));
                entry.buys.deserialize(in);
                if (entry.profit != last_profit[weight]) {
                    rows[weight].push_back(Breakpoint{entry.profit, entry.buys, cost});
                    last_profit[weight] = entry.profit;
                }
            }
        }
    }
    if (!in) {
        throw std::runtime_error("Truncated purchase table");
    }
    for (const auto& row : rows) {
        if (row.empty() || row.front().cost != 0) {
            throw std::runtime_error("Corrupt purchase table row");
        }
    }
}
