#include "../purchaseHandler/FenwickTree2D.h"
#include "../runtime/ProcessSafety.h"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <tuple>

namespace {
bool equal(const buy_list& a, const buy_list& b) {
    return std::tie(a.food, a.spice, a.pottery, a.marble, a.silk, a.jewelry) ==
           std::tie(b.food, b.spice, b.pottery, b.marble, b.silk, b.jewelry);
}

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (!install_process_safety_limits(768, 150)) return 2;
    try {
        require(argc == 3, "Usage: ConvertPurchaseCache DENSE_INPUT NEW_SPARSE_OUTPUT");
        const auto input = std::filesystem::absolute(argv[1]);
        const auto output = std::filesystem::absolute(argv[2]);
        require(!std::filesystem::exists(output), "Output already exists; retained");
        std::ifstream header(input, std::ios::binary);
        uint32_t version = 0;
        uint16_t max_gold = 0, max_weight = 0;
        uint64_t entries = 0;
        header.read(reinterpret_cast<char*>(&version), 4);
        header.read(reinterpret_cast<char*>(&max_gold), 2);
        header.read(reinterpret_cast<char*>(&max_weight), 2);
        header.read(reinterpret_cast<char*>(&entries), 8);
        const uint64_t expected = (uint64_t(max_gold) + 1) * (uint64_t(max_weight) + 1);
        require(bool(header) && version == 2 && entries == expected &&
                max_gold < UINT16_MAX && max_weight < UINT16_MAX &&
                std::filesystem::file_size(input) == 16 + expected * 10,
                "Invalid or unsupported dense cache");

        FenwickTree2D original, converted;
        original.deserialize(input.string());
        original.serialize(output.string());
        converted.deserialize(output.string());
        for (uint32_t weight = 0; weight <= max_weight; ++weight) {
            const auto before = original.queryFrontier(max_gold, weight);
            const auto after = converted.queryFrontier(max_gold, weight);
            require(before.size() == after.size(), "Frontier length differs");
            for (size_t i = 0; i < before.size(); ++i) {
                require(before[i].value == after[i].value && before[i].exact_cost == after[i].exact_cost &&
                        equal(before[i].buys, after[i].buys), "Frontier differs");
            }
            for (uint32_t gold = 0; gold <= max_gold; ++gold) {
                const auto a = original.query(gold, weight);
                const auto b = converted.query(gold, weight);
                require(a.first == b.first && equal(a.second, b.second), "Query differs");
            }
            for (uint16_t gold : std::array<uint16_t, 4>{0, uint16_t(max_gold / 2), max_gold, UINT16_MAX}) {
                const auto a = original.queryPortfolio(gold, weight);
                const auto b = converted.queryPortfolio(gold, weight);
                require(a.size() == b.size(), "Portfolio length differs");
                for (size_t i = 0; i < a.size(); ++i) {
                    require(a[i].profit == b[i].profit && a[i].minimum_budget == b[i].minimum_budget &&
                            equal(a[i].buys, b[i].buys), "Portfolio differs");
                }
            }
        }
        const auto a = original.query(UINT16_MAX, UINT16_MAX);
        const auto b = converted.query(UINT16_MAX, UINT16_MAX);
        require(a.first == b.first && equal(a.second, b.second), "Clamped query differs");
        std::cout << "{\"cells_verified\":" << expected << ",\"rows_verified\":"
                  << uint32_t(max_weight) + 1 << "}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
