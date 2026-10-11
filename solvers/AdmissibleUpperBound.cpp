#include "AdmissibleUpperBound.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

#include "GameSolver.h"

namespace {

constexpr int64_t SCORE_CAP = std::numeric_limits<int>::max();

int64_t capped_add(const int64_t lhs, const int64_t rhs) noexcept {
    if (lhs >= SCORE_CAP || rhs >= SCORE_CAP || lhs > SCORE_CAP - rhs) {
        return SCORE_CAP;
    }
    return lhs + rhs;
}

int64_t capped_multiply(const int64_t lhs, const int64_t rhs) noexcept {
    if (lhs == 0 || rhs == 0) {
        return 0;
    }
    if (lhs >= SCORE_CAP || rhs >= SCORE_CAP || lhs > SCORE_CAP / rhs) {
        return SCORE_CAP;
    }
    return lhs * rhs;
}

int64_t ceil_divide(const int64_t numerator, const int64_t denominator) noexcept {
    return (numerator + denominator - 1) / denominator;
}

int64_t cargo_quantity(const SolverState& state, const int item) noexcept {
    switch (item) {
        case i_Spice:
            return state.spice;
        case i_Pottery:
            return state.pottery;
        case i_Marble:
            return state.marble;
        case i_Silk:
            return state.silk;
        case i_Jewelry:
            return state.jewelry;
        default:
            return 0;
    }
}

} // namespace

int64_t AdmissibleUpperBound::score_ceiling(const SolverState& state) noexcept {
    // Deferred states retain a representative with the maximum concrete gold;
    // their market and all non-food fields are shared. This relaxation ignores
    // food feasibility, so that representative also bounds every envelope row.

    const int active_day = state.day + (state.has_flag(SSF_DAY_START_SKIP) ? 0 : 1);
    const int base_days = std::max(0, static_cast<int>(state.max_day) - active_day + 1);
    const bool merchant_available = !state.has_flag(SSF_MERCHANT_HAPPENED);
    const bool witch_available = !state.has_flag(SSF_WITCH_HAPPENED);
    const bool seer_available = !state.has_flag(SSF_DISASTER_HAPPENED) &&
        state.scheduled_disaster == d_Seer;
    // Magic Clepsydra and Time is Money can each add one day. Treat an
    // unspent encounter as free and available even if its window has closed.
    const int relaxed_days = base_days + (merchant_available ? 1 : 0) +
        (witch_available ? 1 : 0) + (seer_available ? 1 : 0) +
        (state.has_flag(SSF_QUICK_INTEREST) ? 1 : 0);

    std::array<int64_t, 6> best_sell{};
    int64_t liquid_wealth = state.gold;
    for (int item = i_Spice; item <= i_Jewelry; ++item) {
        const int64_t buy = state.item_shop[item * 3 + id_Buy];
        // Route to Mahaji is included while the Merchant encounter remains
        // unspent, regardless of whether its legal window has closed.
        best_sell[item] = std::max<int64_t>(
            buy,
            static_cast<int64_t>(state.item_shop[item * 3 + id_Sell]) +
                (item == i_Spice && merchant_available ? 20 : 0)
        );
        liquid_wealth = capped_add(
            liquid_wealth,
            capped_multiply(cargo_quantity(state, item), best_sell[item])
        );
    }
    if (state.has_flag(SSF_HAND_OF_MIDAS) || merchant_available) {
        liquid_wealth = capped_add(liquid_wealth, 100);
    }

    // This relaxed capacity combines every favorable carrying bonus and gives
    // away one camel per future action. A non-positive cargo weight permits
    // an unbounded quantity under this relaxation, so safely disable pruning.
    const int64_t future_crew = relaxed_days + (merchant_available ? 1 : 0) +
        (witch_available ? 2 : 0);
    const int64_t capacity = std::max<int64_t>(0, state.backpack) +
        (merchant_available ? 20 : 0) + (witch_available ? 20 : 0) +
        capped_multiply(
            std::max<int64_t>(0, state.camel) + future_crew,
            std::max<int64_t>(0, state.npc_shop[n_Camel * 3 + nd_Effect]) +
                (merchant_available ? 20 : 0) + (witch_available ? 10 : 0)
        );
    int64_t best_trade_profit = 0;
    for (int item = i_Spice; item <= i_Jewelry; ++item) {
        const int64_t weight = state.item_shop[item * 3 + id_Weight] -
            (item == i_Marble && witch_available ? 15 : 0);
        if (weight <= 0) {
            return SCORE_CAP;
        }
        const int64_t unit_profit = std::max<int64_t>(
            0,
            best_sell[item] - state.item_shop[item * 3 + id_Buy]
        );
        best_trade_profit = std::max(
            best_trade_profit,
            capped_multiply(ceil_divide(capacity, weight), unit_profit)
        );
    }
    if (state.has_flag(SSF_HAND_OF_MIDAS) || merchant_available) {
        best_trade_profit = capped_add(best_trade_profit, 100);
    }

    // Give every current and future trader Midas's income bonus and make all
    // of them available before the next action. This ignores food, prices,
    // affordability, and the one-event restriction, making it an upper bound.
    const int64_t total_traders = std::max<int64_t>(0, state.trader) + future_crew;
    const int64_t trader_income = std::max<int64_t>(
        0,
        static_cast<int64_t>(state.npc_shop[n_Trader * 3 + nd_Effect]) +
            (witch_available ? 150 : 0)
    );
    const int64_t passive_income = capped_add(
        std::max<int64_t>(0, state.daily_income),
        capped_multiply(total_traders, trader_income)
    );

    // Blue Treasure and every possible Eduming hire are applied before the
    // first relaxed Interest action and the rate never decays.
    const long double interest_rate = std::max<long double>(
        1.0L,
        static_cast<long double>(state.interest_hundredths) / 100.0L
    ) + (merchant_available ? 0.2L : 0.0L) + (state.has_flag(SSF_TRADER_INTEREST) ?
        static_cast<long double>(future_crew) * 0.1L : 0.0L);

    std::vector<int64_t> wealth(static_cast<size_t>(relaxed_days) + 1, 0);
    wealth[0] = liquid_wealth;
    for (int day = 0; day < relaxed_days; ++day) {
        const int64_t after_interest = std::min<int64_t>(
            SCORE_CAP,
            static_cast<int64_t>(std::ceil(
                static_cast<long double>(wealth[day]) * interest_rate + 0.25L
            ))
        );
        wealth[day + 1] = std::max(
            wealth[day + 1],
            capped_add(after_interest, passive_income)
        );
        if (day + 2 <= relaxed_days) {
            wealth[day + 2] = std::max(
                wealth[day + 2],
                capped_add(
                    capped_add(wealth[day], capped_multiply(2, passive_income)),
                    best_trade_profit
                )
            );
        }
    }

    const int64_t total_crew = std::max<int64_t>(0, state.trader) +
        std::max<int64_t>(0, state.camel) + future_crew;
    const int64_t crew_value = capped_add(
        capped_multiply(std::max<int64_t>(0, state.trader),
                         std::max<int64_t>(0, state.npc_shop[n_Trader * 3 + nd_Cost])),
        capped_multiply(std::max<int64_t>(0, state.camel) + future_crew,
                         std::max<int64_t>(0, state.npc_shop[n_Camel * 3 + nd_Cost]))
    );
    // A free Wooden Statuette is assumed even when the merchant is unavailable.
    return capped_add(
        capped_add(wealth.back(), crew_value),
        capped_multiply(total_crew, 500)
    );
}
