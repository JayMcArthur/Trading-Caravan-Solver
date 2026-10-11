#include "DeferredGoldFunction.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <type_traits>

DeferredGoldFunction::DeferredGoldFunction(
    const int zero_spend_gold_value,
    const int maximum_original_spend
) : zero_spend_gold(zero_spend_gold_value), maximum_spend(maximum_original_spend) {}

DeferredGoldFunction DeferredGoldFunction::open(
    const int gold_before_buy,
    const int buy_day_income,
    const int maximum_original_spend
) {
    if (gold_before_buy < 0 || maximum_original_spend < 0 ||
        maximum_original_spend > gold_before_buy) {
        throw std::invalid_argument("Invalid deferred Buy gold range");
    }
    return DeferredGoldFunction(gold_before_buy + buy_day_income, maximum_original_spend);
}

int DeferredGoldFunction::evaluate(const int exact_original_spend) const {
    if (exact_original_spend < 0 || exact_original_spend > maximum_spend) {
        throw std::out_of_range("Deferred Buy spend is outside the feasible range");
    }
    return evaluate_unchecked(exact_original_spend);
}

int DeferredGoldFunction::maximum_original_spend() const noexcept {
    return maximum_spend;
}

size_t DeferredGoldFunction::operation_count() const noexcept {
    return operations.size();
}

size_t DeferredGoldFunction::memory_bytes() const noexcept {
    return sizeof(*this) + operations.capacity() * sizeof(Operation);
}

size_t DeferredGoldFunction::semantic_hash() const noexcept {
    size_t seed = static_cast<size_t>(zero_spend_gold);
    const auto combine = [&seed](const auto value) {
        using Value = std::decay_t<decltype(value)>;
        seed ^= std::hash<Value>{}(value) + 0x9e3779b97f4a7c15ULL +
                (seed << 6U) + (seed >> 2U);
    };
    combine(maximum_spend);
    for (const Operation& operation : operations) {
        combine(static_cast<uint8_t>(operation.type));
        combine(operation.value);
    }
    return seed;
}

bool DeferredGoldFunction::dominates(
    const DeferredGoldFunction& other,
    uint64_t* exact_spend_evaluations
) const noexcept {
    if (maximum_spend < other.maximum_spend) {
        return false;
    }

    // Identical monotone operation suffixes preserve an advantage at their
    // input, including every rounding step.
    if (operations == other.operations && zero_spend_gold >= other.zero_spend_gold) {
        return true;
    }

    for (int spend = 0; spend <= other.maximum_spend; ++spend) {
        if (exact_spend_evaluations != nullptr) {
            ++*exact_spend_evaluations;
        }
        if (evaluate_unchecked(spend) < other.evaluate_unchecked(spend)) {
            return false;
        }
    }
    return true;
}

void DeferredGoldFunction::add_gold(const int amount) {
    if (amount == 0) {
        return;
    }
    if (!operations.empty() && operations.back().type == OperationType::Add) {
        operations.back().value += amount;
        if (operations.back().value == 0) {
            operations.pop_back();
        }
        return;
    }
    operations.push_back(Operation{OperationType::Add, amount});
}

void DeferredGoldFunction::apply_interest(const int interest_hundredths) {
    if (interest_hundredths < 0) {
        throw std::invalid_argument("Deferred Interest must be nonnegative");
    }
    if (interest_hundredths == 100) {
        return;
    }
    operations.push_back(Operation{OperationType::Interest, interest_hundredths});
}

bool DeferredGoldFunction::spend_if_affordable(const int cost) {
    if (cost < 0) {
        throw std::invalid_argument("Deferred cost must be nonnegative");
    }
    if (evaluate_unchecked(0) < cost) {
        return false;
    }

    int low = 0;
    int high = maximum_spend;
    while (low < high) {
        const int middle = low + (high - low + 1) / 2;
        if (evaluate_unchecked(middle) >= cost) {
            low = middle;
        } else {
            high = middle - 1;
        }
    }

    maximum_spend = low;
    add_gold(-cost);
    return true;
}

bool DeferredGoldFunction::operator==(const DeferredGoldFunction& other) const noexcept {
    return zero_spend_gold == other.zero_spend_gold &&
           maximum_spend == other.maximum_spend &&
           operations == other.operations;
}

int DeferredGoldFunction::evaluate_unchecked(const int exact_original_spend) const noexcept {
    int gold = zero_spend_gold - exact_original_spend;
    for (const Operation& operation : operations) {
        if (operation.type == OperationType::Add) {
            gold += operation.value;
        } else {
            const double rate = static_cast<double>(operation.value) / 100.0;
            gold = static_cast<int>(std::round(rate * gold + 0.25));
        }
    }
    return gold;
}
