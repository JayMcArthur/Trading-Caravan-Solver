#include "EncounterTiming.h"

#include <algorithm>
#include <iterator>

namespace {
constexpr int CLASSICAL_MERCHANT_DAYS[] = {5, 6, 7};
constexpr int CLASSICAL_WITCH_DAYS[] = {9, 10, 11};
constexpr int TRAVELER_MERCHANT_DAYS[] = {7, 8, 9};
constexpr int TRAVELER_WITCH_DAYS[] = {11, 12, 13};

template <size_t Size>
bool contains(const int (&days)[Size], const int day) noexcept {
    return std::find(std::begin(days), std::end(days), day) != std::end(days);
}

template <size_t Size>
bool has_later_day(const int (&days)[Size], const int day, const int max_day) noexcept {
    return std::any_of(std::begin(days), std::end(days), [day, max_day](const int candidate) {
        return day < candidate && candidate <= max_day;
    });
}
}

EncounterOpportunity EncounterTiming::merchant(const Player& player) noexcept {
    if (player.merchant_day > 0) {
        return {player.day == player.merchant_day, false};
    }
    const bool calendar_due = player.traveler_mode
        ? contains(TRAVELER_MERCHANT_DAYS, player.day)
        : contains(CLASSICAL_MERCHANT_DAYS, player.day);
    const bool due = player.call_merchant || calendar_due;
    const bool may_defer = due && (player.call_merchant
        ? player.day < player.max_day
        : (player.traveler_mode
            ? has_later_day(TRAVELER_MERCHANT_DAYS, player.day, player.max_day)
            : has_later_day(CLASSICAL_MERCHANT_DAYS, player.day, player.max_day)));
    return {due, may_defer};
}

EncounterOpportunity EncounterTiming::witch(const Player& player) noexcept {
    if (player.witch_day > 0) {
        return {player.day == player.witch_day, false};
    }
    const bool calendar_due = player.traveler_mode
        ? contains(TRAVELER_WITCH_DAYS, player.day)
        : contains(CLASSICAL_WITCH_DAYS, player.day);
    const bool may_defer = calendar_due && (player.traveler_mode
        ? has_later_day(TRAVELER_WITCH_DAYS, player.day, player.max_day)
        : has_later_day(CLASSICAL_WITCH_DAYS, player.day, player.max_day));
    return {calendar_due, may_defer};
}
