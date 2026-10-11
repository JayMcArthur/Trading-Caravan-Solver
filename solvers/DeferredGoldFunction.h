#ifndef TRADING_CARAVAN_SOLVER_DEFERREDGOLDFUNCTION_H
#define TRADING_CARAVAN_SOLVER_DEFERREDGOLDFUNCTION_H

#include <cstddef>
#include <cstdint>
#include <vector>

class SolverStateCodecAccess;

class DeferredGoldFunction {
public:
    [[nodiscard]] static DeferredGoldFunction open(
        int gold_before_buy,
        int buy_day_income,
        int maximum_original_spend
    );

    [[nodiscard]] int evaluate(int exact_original_spend) const;
    [[nodiscard]] int maximum_original_spend() const noexcept;
    [[nodiscard]] size_t operation_count() const noexcept;
    [[nodiscard]] size_t memory_bytes() const noexcept;
    [[nodiscard]] size_t semantic_hash() const noexcept;
    [[nodiscard]] bool dominates(
        const DeferredGoldFunction& other,
        uint64_t* exact_spend_evaluations = nullptr
    ) const noexcept;

    void add_gold(int amount);
    void apply_interest(int interest_hundredths);
    [[nodiscard]] bool spend_if_affordable(int cost);

    [[nodiscard]] bool operator==(const DeferredGoldFunction& other) const noexcept;

private:
    friend class SolverStateCodecAccess;
    enum class OperationType : uint8_t {
        Add,
        Interest,
    };

    struct Operation {
        OperationType type = OperationType::Add;
        int value = 0;

        [[nodiscard]] bool operator==(const Operation& other) const noexcept {
            return type == other.type && value == other.value;
        }
    };

    int zero_spend_gold = 0;
    int maximum_spend = 0;
    std::vector<Operation> operations;

    DeferredGoldFunction(int zero_spend_gold_value, int maximum_original_spend);
    [[nodiscard]] int evaluate_unchecked(int exact_original_spend) const noexcept;
};

#endif // TRADING_CARAVAN_SOLVER_DEFERREDGOLDFUNCTION_H
