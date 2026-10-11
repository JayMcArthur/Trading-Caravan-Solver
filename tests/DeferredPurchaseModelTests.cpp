#include "DeferredPurchaseModelTests.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <limits>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "../game/Game.h"
#include "../solvers/DeferredPurchaseModel.h"
#include "../solvers/DeferredSearchState.h"
#include "../solvers/DeferredSoldModel.h"

namespace {
struct ExplicitBuy {
    Player player;
    buy_list buy;
};

Player small_market_player() {
    Player player;
    player.day = 1;
    player.max_day = 15;
    player.gold = 60;
    player.food = 1;
    player.trader = 0;
    player.camel = 0;
    player.backpack = 15;
    player.daily_income = 2;
    player.food_consumption = 1;
    player.interest_rate = 0.55;
    player.item_shop[i_Food][id_Buy] = 2;
    player.item_shop[i_Food][id_Weight] = 1;
    for (int item = i_Spice; item <= i_Jewelry; ++item) {
        player.item_shop[item][id_Buy] = 1000;
        player.item_shop[item][id_Weight] = 1000;
        player.item_shop[item][id_Sell] = 0;
    }
    player.item_shop[i_Spice][id_Buy] = 3;
    player.item_shop[i_Spice][id_Weight] = 2;
    player.item_shop[i_Spice][id_Sell] = 7;
    player.item_shop[i_Pottery][id_Buy] = 5;
    player.item_shop[i_Pottery][id_Weight] = 3;
    player.item_shop[i_Pottery][id_Sell] = 11;
    return player;
}

std::vector<ExplicitBuy> explicit_buys(const Player& before) {
    std::vector<ExplicitBuy> rows;
    const int free_weight = Game::check_weight_left(before);
    for (int food = 0; food <= 15; ++food) {
        for (int spice = 0; spice <= 7; ++spice) {
            for (int pottery = 0; pottery <= 5; ++pottery) {
                const int cost = food * before.item_shop[i_Food][id_Buy] +
                                 spice * before.item_shop[i_Spice][id_Buy] +
                                 pottery * before.item_shop[i_Pottery][id_Buy];
                const int weight = food * before.item_shop[i_Food][id_Weight] +
                                   spice * before.item_shop[i_Spice][id_Weight] +
                                   pottery * before.item_shop[i_Pottery][id_Weight];
                if (cost > before.gold || weight > free_weight) {
                    continue;
                }
                const buy_list buy(
                    static_cast<uint8_t>(food),
                    static_cast<uint8_t>(spice),
                    static_cast<uint8_t>(pottery),
                    0,
                    0,
                    0
                );
                Player player = before;
                Game::event_buy_items(player, buy);
                rows.push_back(ExplicitBuy{std::move(player), buy});
            }
        }
    }
    return rows;
}

std::vector<std::pair<int, int>> resource_frontier(const std::vector<ExplicitBuy>& rows) {
    std::vector<std::pair<int, int>> by_food;
    by_food.reserve(rows.size());
    for (const ExplicitBuy& row : rows) {
        by_food.emplace_back(row.player.food, row.player.gold);
    }
    std::sort(by_food.begin(), by_food.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.first != rhs.first) {
            return lhs.first < rhs.first;
        }
        return lhs.second > rhs.second;
    });
    std::vector<std::pair<int, int>> frontier;
    for (const auto& [food, gold] : by_food) {
        if (frontier.empty() || frontier.back().second != food) {
            frontier.emplace_back(gold, food);
        }
    }
    std::sort(frontier.begin(), frontier.end());
    frontier.erase(std::unique(frontier.begin(), frontier.end()), frontier.end());
    return frontier;
}

std::vector<std::pair<int, int>> resource_frontier(
    const std::vector<DeferredPurchaseResolution>& rows
) {
    std::vector<std::pair<int, int>> frontier;
    for (const DeferredPurchaseResolution& row : rows) {
        frontier.emplace_back(row.player.gold, row.player.food);
    }
    std::sort(frontier.begin(), frontier.end());
    frontier.erase(std::unique(frontier.begin(), frontier.end()), frontier.end());
    return frontier;
}

void assert_same_food_gold_dominance(
    const std::vector<DeferredPurchaseResolution>& stronger,
    const std::vector<DeferredPurchaseResolution>& weaker
) {
    for (const DeferredPurchaseResolution& weak : weaker) {
        const bool covered = std::any_of(
            stronger.begin(),
            stronger.end(),
            [&weak](const DeferredPurchaseResolution& strong) {
                return strong.player.food == weak.player.food &&
                       strong.player.gold >= weak.player.gold;
            }
        );
        assert(covered);
    }
}

void test_food_only_boundary_matches_explicit_low_interest() {
    const Player before = small_market_player();
    DeferredPurchaseModel model = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 17}}
    );
    const auto started = model.after_start_of_day();
    assert(started.has_value());
    const auto interested = started->after_interest();
    assert(interested.has_value());
    const auto symbolic = interested->resolve_food_only_for_next_buy();

    std::vector<ExplicitBuy> explicit_rows;
    for (ExplicitBuy row : explicit_buys(before)) {
        if (row.buy.spice != 0 || row.buy.pottery != 0) {
            continue;
        }
        Game::start_of_day(row.player);
        if (row.player.food < row.player.food_consumption) {
            continue;
        }
        Game::event_interest(row.player);
        explicit_rows.push_back(std::move(row));
    }

    assert(resource_frontier(symbolic) == resource_frontier(explicit_rows));
    assert(symbolic.size() > 1);
    for (const DeferredPurchaseResolution& row : symbolic) {
        assert(row.parent_trace == 17);
        assert(row.actions.size() == 2);
        assert(row.actions[0].action == _a_buy);
        assert(row.actions[1].action == _a_interest);
    }
}

void test_sell_resolution_matches_explicit_enumeration() {
    const Player before = small_market_player();
    DeferredPurchaseModel model = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 23}}
    );
    const auto day_two = model.after_start_of_day();
    assert(day_two.has_value());
    const auto interested = day_two->after_interest();
    assert(interested.has_value());
    const auto day_three = interested->after_start_of_day();
    assert(day_three.has_value());
    BestPurchaseHandler handler;
    const auto symbolic = day_three->resolve_sell(handler);

    std::vector<ExplicitBuy> explicit_rows;
    for (ExplicitBuy row : explicit_buys(before)) {
        if (row.buy.spice == 0 && row.buy.pottery == 0) {
            continue;
        }
        Game::start_of_day(row.player);
        if (row.player.food < row.player.food_consumption) {
            continue;
        }
        Game::event_interest(row.player);
        Game::start_of_day(row.player);
        if (row.player.food < row.player.food_consumption) {
            continue;
        }
        Game::event_sell_items(row.player);
        explicit_rows.push_back(std::move(row));
    }

    assert(resource_frontier(symbolic) == resource_frontier(explicit_rows));
    assert(!symbolic.empty());
    for (const DeferredPurchaseResolution& row : symbolic) {
        assert(row.parent_trace == 23);
        assert(row.actions.size() == 3);
        assert(row.actions[0].action == _a_buy);
        assert(row.actions[1].action == _a_interest);
        assert(row.actions[2].action == _a_sell);
    }
}

void test_final_day_sell_discards_extra_food_frontier() {
    Player before = small_market_player();
    before.day = 14;
    before.max_day = 15;
    before.food = 1;
    before.item_shop[i_Food][id_Weight] = 0;
    DeferredPurchaseModel model = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 24}}
    );
    const auto final_day = model.after_start_of_day();
    assert(final_day.has_value());
    BestPurchaseHandler handler;
    const auto symbolic = final_day->resolve_sell(handler);

    int explicit_best = std::numeric_limits<int>::min();
    for (ExplicitBuy row : explicit_buys(before)) {
        if (row.buy.spice == 0 && row.buy.pottery == 0) {
            continue;
        }
        Game::start_of_day(row.player);
        if (row.player.food < 0) {
            continue;
        }
        Game::event_sell_items(row.player);
        explicit_best = std::max(explicit_best, Game::check_points(row.player, "Final"));
    }

    assert(symbolic.size() == 1);
    assert(Game::check_points(symbolic.front().player, "Final") == explicit_best);
}

void test_hire_filters_gold_and_does_not_expand_old_buy_capacity() {
    Player before = small_market_player();
    before.npc_shop[n_Camel][nd_Cost] = 4;
    before.npc_shop[n_Camel][nd_Eat] = 1;
    before.npc_shop[n_Camel][nd_Effect] = 20;
    DeferredPurchaseModel model = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 31}}
    );
    const auto started = model.after_start_of_day();
    assert(started.has_value());
    const auto hired = started->after_hire(n_Camel);
    assert(hired.has_value());
    const auto symbolic = hired->resolve_food_only_for_next_buy();

    std::vector<ExplicitBuy> explicit_rows;
    for (ExplicitBuy row : explicit_buys(before)) {
        if (row.buy.spice != 0 || row.buy.pottery != 0) {
            continue;
        }
        Game::start_of_day(row.player);
        if (row.player.gold < row.player.npc_shop[n_Camel][nd_Cost] ||
            row.player.food < row.player.food_consumption + row.player.npc_shop[n_Camel][nd_Eat]) {
            continue;
        }
        Game::event_buy_npc(row.player, n_Camel);
        explicit_rows.push_back(std::move(row));
    }

    assert(resource_frontier(symbolic) == resource_frontier(explicit_rows));
    for (const DeferredPurchaseResolution& row : symbolic) {
        assert(row.concrete_buy.food <= 14);
        assert(row.actions.size() == 2);
        assert(row.actions[1].action == _a_npc);
        assert(row.actions[1].option == static_cast<uint8_t>(n_Camel));
    }
}

void test_merchant_sell_value_uses_old_buy_market_and_new_sell_market() {
    const Player before = small_market_player();
    DeferredPurchaseModel model = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 41}}
    );
    const auto started = model.after_start_of_day();
    assert(started.has_value());
    const auto merchant = started->after_merchant(m_Route_to_Mahaji);
    assert(merchant.has_value());
    const DeferredPurchaseModel resumed = merchant->after_resume_same_day();
    BestPurchaseHandler handler;
    const auto symbolic = resumed.resolve_sell(handler);

    std::vector<ExplicitBuy> explicit_rows;
    for (ExplicitBuy row : explicit_buys(before)) {
        if (row.buy.spice == 0 && row.buy.pottery == 0) {
            continue;
        }
        Game::start_of_day(row.player);
        if (!Game::check__afford_merchant(row.player, m_Route_to_Mahaji) ||
            !Game::event_merchant(row.player, m_Route_to_Mahaji)) {
            continue;
        }
        row.player.day_start_skip = false;
        if (row.player.food < row.player.food_consumption) {
            continue;
        }
        Game::event_sell_items(row.player);
        explicit_rows.push_back(std::move(row));
    }

    assert(resource_frontier(symbolic) == resource_frontier(explicit_rows));
    assert(!symbolic.empty());
    for (const DeferredPurchaseResolution& row : symbolic) {
        assert(row.actions.size() == 3);
        assert(row.actions[1].action == _a_merchant);
        assert(row.actions[1].option == static_cast<uint8_t>(m_Route_to_Mahaji));
        assert(row.actions[2].action == _a_sell);
    }
}

void test_repeated_buy_origin_frontier_reaches_the_same_sell_states() {
    const Player before = small_market_player();
    DeferredPurchaseModel first = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 51}}
    );
    const auto day_two = first.after_start_of_day();
    assert(day_two.has_value());
    const auto interested = day_two->after_interest();
    assert(interested.has_value());
    const auto day_three = interested->after_start_of_day();
    assert(day_three.has_value());
    const auto first_origins = day_three->resolve_food_only_for_next_buy();
    assert(first_origins.size() > 1);

    std::vector<DeferredPurchaseSeed> second_seeds;
    second_seeds.reserve(first_origins.size());
    for (size_t index = 0; index < first_origins.size(); ++index) {
        second_seeds.push_back(DeferredPurchaseSeed{
            first_origins[index].player.gold,
            first_origins[index].player.food,
            static_cast<uint32_t>(100 + index),
        });
    }
    DeferredPurchaseModel second = DeferredPurchaseModel::open(
        first_origins.front().player,
        second_seeds
    );
    const auto day_four = second.after_start_of_day();
    assert(day_four.has_value());
    BestPurchaseHandler handler;
    const auto symbolic = day_four->resolve_sell(handler);

    std::vector<ExplicitBuy> second_explicit;
    for (ExplicitBuy first_buy : explicit_buys(before)) {
        if (first_buy.buy.spice != 0 || first_buy.buy.pottery != 0) {
            continue;
        }
        Game::start_of_day(first_buy.player);
        if (first_buy.player.food < first_buy.player.food_consumption) {
            continue;
        }
        Game::event_interest(first_buy.player);
        Game::start_of_day(first_buy.player);
        if (first_buy.player.food < 0) {
            continue;
        }
        for (ExplicitBuy second_buy : explicit_buys(first_buy.player)) {
            second_explicit.push_back(std::move(second_buy));
        }
    }

    std::vector<ExplicitBuy> explicit_sold;
    for (ExplicitBuy row : second_explicit) {
        if (row.buy.spice == 0 && row.buy.pottery == 0) {
            continue;
        }
        Game::start_of_day(row.player);
        if (row.player.food < row.player.food_consumption) {
            continue;
        }
        Game::event_sell_items(row.player);
        explicit_sold.push_back(std::move(row));
    }

    assert(resource_frontier(symbolic) == resource_frontier(explicit_sold));
    assert(!symbolic.empty());
    for (const DeferredPurchaseResolution& row : symbolic) {
        assert(row.actions.size() == 2);
        assert(row.actions[0].action == _a_buy);
        assert(row.actions[1].action == _a_sell);
        assert(row.parent_trace >= 100);
    }
}

void test_sold_ledger_matches_eager_sell_through_later_actions() {
    const Player before = small_market_player();
    DeferredPurchaseModel purchase = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 75}}
    );
    const auto day_two = purchase.after_start_of_day();
    assert(day_two.has_value());
    const auto before_sell = day_two->after_interest();
    assert(before_sell.has_value());

    BestPurchaseHandler handler;
    const auto eager_sell = before_sell->resolve_sell(handler);
    const auto sold = DeferredSoldModel::from_purchase(*before_sell, handler);
    assert(sold.has_value());
    assert(resource_frontier(sold->materialize()) == resource_frontier(eager_sell));

    const auto started = sold->after_start_of_day();
    assert(started.has_value());
    const auto interested = started->after_interest();
    assert(interested.has_value());

    std::vector<DeferredPurchaseResolution> explicit_later;
    for (DeferredPurchaseResolution row : eager_sell) {
        Game::start_of_day(row.player);
        if (row.player.food < 0 ||
            (row.player.day != row.player.max_day &&
             row.player.food < row.player.food_consumption)) {
            continue;
        }
        Game::event_interest(row.player);
        row.actions.push_back(SolverAction{_a_interest, buy_list(), 0});
        explicit_later.push_back(std::move(row));
    }
    const auto materialized_later = interested->materialize();
    assert(resource_frontier(materialized_later) == resource_frontier(explicit_later));

    std::vector<int> expected_scores;
    for (const DeferredPurchaseResolution& row : materialized_later) {
        expected_scores.push_back(Game::check_points(row.player, "Final"));
    }
    std::sort(expected_scores.begin(), expected_scores.end(), std::greater<int>());
    expected_scores.resize(std::min<size_t>(3, expected_scores.size()));

    std::vector<int> finalist_scores;
    for (const DeferredPurchaseResolution& row : interested->resolve_finish(3)) {
        finalist_scores.push_back(Game::check_points(row.player, "Final"));
    }
    assert(finalist_scores == expected_scores);
}

void test_sold_envelope_transforms_without_requerying_sale_market() {
    const Player before = small_market_player();
    DeferredPurchaseModel purchase = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 76}}
    );
    const auto day_two = purchase.after_start_of_day();
    assert(day_two.has_value());
    const auto before_sell = day_two->after_interest();
    assert(before_sell.has_value());

    BestPurchaseHandler handler;
    const auto sold = DeferredSoldModel::from_purchase(*before_sell, handler);
    assert(sold.has_value());
    const auto started = sold->after_start_of_day();
    assert(started.has_value());

    const auto first = started->after_interest();
    assert(first.has_value());
    const uint64_t calls_after_first = handler.getProfilingSummary().exact_sale_calls;
    const auto second = started->after_interest();
    assert(second.has_value());
    const uint64_t calls_after_second = handler.getProfilingSummary().exact_sale_calls;

    assert(calls_after_second == calls_after_first);
    assert(second->representative().gold == first->representative().gold);
    assert(second->representative().food == first->representative().food);
}

void test_weightless_sold_envelope_is_a_canonical_resource_frontier() {
    Player before = small_market_player();
    before.item_shop[i_Food][id_Weight] = 0;
    DeferredPurchaseModel purchase = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 78}}
    );
    const auto started = purchase.after_start_of_day();
    assert(started.has_value());
    BestPurchaseHandler handler;
    const auto sold = DeferredSoldModel::from_purchase(*started, handler);
    assert(sold.has_value());
    const auto rows = sold->materialize();
    assert(!rows.empty());

    for (size_t index = 0; index < rows.size(); ++index) {
        for (size_t other = 0; other < rows.size(); ++other) {
            if (index == other) {
                continue;
            }
            assert(!(rows[other].player.food >= rows[index].player.food &&
                     rows[other].player.gold >= rows[index].player.gold));
        }
    }
}

void test_deferred_search_state_identity_ignores_trace_provenance() {
    const Player before = small_market_player();
    DeferredSearchState first_purchase(DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 80}}
    ));
    DeferredSearchState second_purchase(DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 900}}
    ));
    assert(first_purchase.has_open_purchase());
    assert(first_purchase.semantically_equal(second_purchase));
    assert(first_purchase.semantic_hash() == second_purchase.semantic_hash());

    BestPurchaseHandler handler;
    const auto first_sold_model = DeferredSoldModel::from_purchase(
        DeferredPurchaseModel::open(before, {DeferredPurchaseSeed{before.gold, before.food, 81}}),
        handler
    );
    const auto second_sold_model = DeferredSoldModel::from_purchase(
        DeferredPurchaseModel::open(before, {DeferredPurchaseSeed{before.gold, before.food, 901}}),
        handler
    );
    assert(first_sold_model.has_value());
    assert(second_sold_model.has_value());
    DeferredSearchState first_sold(*first_sold_model);
    DeferredSearchState second_sold(*second_sold_model);
    assert(first_sold.has_sold_ledger());
    assert(first_sold.semantically_equal(second_sold));
    assert(first_sold.semantic_hash() == second_sold.semantic_hash());
    assert(!first_purchase.semantically_equal(first_sold));
    assert(first_purchase.memory_bytes() > 0);

    const std::unordered_map<uint32_t, uint32_t> remap{{80, 8}};
    const DeferredSearchState remapped = first_purchase.remap_provenances(remap);
    assert(remapped.provenances().size() == 1);
    assert(remapped.provenances().front() == 8);
    assert(remapped.semantically_equal(first_purchase));
}

void test_unresolved_purchase_finishes_with_the_best_concrete_basket() {
    Player before = small_market_player();
    before.max_day = 2;
    DeferredPurchaseModel model = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 61}}
    );
    const auto final_day = model.after_start_of_day();
    assert(final_day.has_value());
    const auto interested = final_day->after_interest();
    assert(interested.has_value());
    BestPurchaseHandler handler;
    const auto symbolic = interested->resolve_finish(handler);

    int explicit_best = std::numeric_limits<int>::min();
    for (ExplicitBuy row : explicit_buys(before)) {
        Game::start_of_day(row.player);
        if (row.player.food < 0) {
            continue;
        }
        Game::event_interest(row.player);
        explicit_best = std::max(explicit_best, Game::check_points(row.player, "Final"));
    }
    int symbolic_best = std::numeric_limits<int>::min();
    for (const DeferredPurchaseResolution& row : symbolic) {
        symbolic_best = std::max(symbolic_best, Game::check_points(row.player, "Final"));
        assert(row.actions.size() == 2);
        assert(row.actions[0].action == _a_buy);
        assert(row.actions[1].action == _a_interest);
    }
    assert(symbolic_best == explicit_best);
}

void test_final_inventory_uses_current_buy_value_after_witch() {
    Player before = small_market_player();
    before.max_day = 2;
    before.gold = 20;
    before.food = 2;
    before.backpack = 10;
    before.item_shop[i_Jewelry][id_Buy] = 4;
    before.item_shop[i_Jewelry][id_Weight] = 1;

    DeferredPurchaseModel model = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 62}}
    );
    const auto final_day = model.after_start_of_day();
    assert(final_day.has_value());
    const DeferredPurchaseModel witch = final_day->after_witch(w_Vertue_of_Patience);
    BestPurchaseHandler handler;
    const auto symbolic = witch.resolve_finish(handler);

    int explicit_best = std::numeric_limits<int>::min();
    for (int food = 0; food <= 10; ++food) {
        for (int jewelry = 0; jewelry <= 10; ++jewelry) {
            const int cost = food * before.item_shop[i_Food][id_Buy] +
                             jewelry * before.item_shop[i_Jewelry][id_Buy];
            const int weight = food * before.item_shop[i_Food][id_Weight] +
                               jewelry * before.item_shop[i_Jewelry][id_Weight];
            if (cost > before.gold || weight > Game::check_weight_left(before)) {
                continue;
            }
            Player player = before;
            Game::event_buy_items(player, buy_list(
                static_cast<uint8_t>(food), 0, 0, 0, 0, static_cast<uint8_t>(jewelry)
            ));
            Game::start_of_day(player);
            if (player.food < 0) {
                continue;
            }
            Game::event_witch(player, w_Vertue_of_Patience);
            explicit_best = std::max(explicit_best, Game::check_points(player, "Final"));
        }
    }

    assert(!symbolic.empty());
    const auto best = std::max_element(symbolic.begin(), symbolic.end(), [](const auto& lhs, const auto& rhs) {
        return Game::check_points(lhs.player, "Final") < Game::check_points(rhs.player, "Final");
    });
    assert(best->concrete_buy.jewelry > 0);
    assert(Game::check_points(best->player, "Final") == explicit_best);
}

void test_open_purchase_discards_an_origin_dominated_for_every_food_total() {
    Player before = small_market_player();
    DeferredPurchaseModel model = DeferredPurchaseModel::open(
        before,
        {
            DeferredPurchaseSeed{20, 10, 1},
            DeferredPurchaseSeed{50, 0, 2},
        }
    );

    assert(model.origin_count() == 1);
    assert(model.provenances() == std::vector<uint32_t>{2});
    assert(model.representative().gold == 52);
    assert(model.representative().food == 0);
}

void test_open_purchase_keeps_origins_that_win_different_food_ranges() {
    Player before = small_market_player();
    DeferredPurchaseModel model = DeferredPurchaseModel::open(
        before,
        {
            DeferredPurchaseSeed{10, 0, 10},
            DeferredPurchaseSeed{5, 5, 20},
        }
    );

    assert(model.origin_count() == 2);
    assert(model.provenances() == (std::vector<uint32_t>{10, 20}));

    BestPurchaseHandler handler;
    const auto low_food = model.resolve_sell_exact_remaining_food(handler, 0);
    const auto high_food = model.resolve_sell_exact_remaining_food(handler, 6);
    assert(low_food.has_value());
    assert(high_food.has_value());
    assert(low_food->parent_trace == 10);
    assert(high_food->parent_trace == 20);
}

void test_exact_buy_coverage_distinguishes_exact_replaced_and_missing() {
    Player before = small_market_player();
    DeferredPurchaseModel purchase = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 301}}
    );
    assert(purchase.classify_exact_buy(buy_list(2, 1, 0, 0, 0, 0)) ==
           DeferredBuyCoverage::Exact);
    assert(purchase.classify_exact_buy(buy_list(0, 0, 0, 0, 0, 1)) ==
           DeferredBuyCoverage::Missing);

    before.max_day = before.day;
    before.merchant_happened = true;
    before.witch_happened = true;
    purchase = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 302}}
    );
    assert(purchase.classify_exact_buy(buy_list(1, 0, 0, 0, 0, 0)) ==
           DeferredBuyCoverage::DominatingReplacement);

    before = small_market_player();
    purchase = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 303}}
    );
    const auto started = purchase.after_start_of_day();
    assert(started.has_value());
    BestPurchaseHandler handler;
    const auto sold = DeferredSoldModel::from_purchase(*started, handler);
    assert(sold.has_value());
    const auto rows = sold->materialize();
    assert(!rows.empty());
    assert(sold->classify_exact_buy(rows.front().concrete_buy) ==
           DeferredBuyCoverage::Exact);
    assert(sold->classify_exact_buy(buy_list(1, 0, 0, 0, 0, 0)) ==
           DeferredBuyCoverage::DominatingReplacement);
}

void test_continuation_views_are_independent_and_do_not_mutate_owner() {
    Player before = small_market_player();
    DeferredSearchState state(DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{before.gold, before.food, 77}}
    ));
    const Player owner_before = state.representative();
    BestPurchaseHandler handler;
    const auto views = state.continuation_views(handler);
    assert(state.continuation_views(handler, false).empty());

    std::vector<DeferredContinuationKind> kinds;
    for (const DeferredContinuationView& view : views) {
        kinds.push_back(view.kind);
    }
    assert(std::find(kinds.begin(), kinds.end(), DeferredContinuationKind::ImmediateSell) != kinds.end());
    assert(std::find(kinds.begin(), kinds.end(), DeferredContinuationKind::Trader) != kinds.end());
    assert(std::find(kinds.begin(), kinds.end(), DeferredContinuationKind::Camel) != kinds.end());
    assert(std::find(kinds.begin(), kinds.end(), DeferredContinuationKind::Interest) != kinds.end());
    assert(std::find(kinds.begin(), kinds.end(), DeferredContinuationKind::FoodOnlyBuy) == kinds.end());
    assert(state.has_open_purchase());
    assert(state.representative().day == owner_before.day);
    assert(state.representative().gold == owner_before.gold);
    assert(state.representative().food == owner_before.food);

    auto after_start = state.purchase().after_start_of_day();
    assert(after_start.has_value());
    auto after_trader = after_start->after_hire(n_Trader);
    assert(after_trader.has_value());
    DeferredSearchState after_intervening_action(std::move(*after_trader));
    const auto later_views = after_intervening_action.continuation_views(handler);
    assert(std::find_if(later_views.begin(), later_views.end(), [](const DeferredContinuationView& view) {
        return view.kind == DeferredContinuationKind::FoodOnlyBuy;
    }) != later_views.end());
}

void test_deferred_dominance_preserves_materialized_continuations() {
    const Player before = small_market_player();
    DeferredPurchaseModel stronger = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{60, 1, 101}}
    );
    DeferredPurchaseModel weaker = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{50, 1, 102}}
    );
    assert(stronger.dominates(weaker));
    assert(!weaker.dominates(stronger));

    const auto stronger_day_two = stronger.after_start_of_day();
    const auto weaker_day_two = weaker.after_start_of_day();
    assert(stronger_day_two.has_value());
    assert(weaker_day_two.has_value());
    assert(stronger_day_two->dominates(*weaker_day_two));

    const auto stronger_interest = stronger_day_two->after_interest();
    const auto weaker_interest = weaker_day_two->after_interest();
    assert(stronger_interest.has_value());
    assert(weaker_interest.has_value());
    assert(stronger_interest->dominates(*weaker_interest));
    assert_same_food_gold_dominance(
        stronger_interest->resolve_food_only_for_next_buy(),
        weaker_interest->resolve_food_only_for_next_buy()
    );

    BestPurchaseHandler handler;
    assert_same_food_gold_dominance(
        stronger_interest->resolve_sell(handler),
        weaker_interest->resolve_sell(handler)
    );

    const auto stronger_camel = stronger_day_two->after_hire(n_Camel);
    const auto weaker_camel = weaker_day_two->after_hire(n_Camel);
    assert(stronger_camel.has_value());
    assert(weaker_camel.has_value());
    assert(stronger_camel->dominates(*weaker_camel));
    assert_same_food_gold_dominance(
        stronger_camel->resolve_sell(handler),
        weaker_camel->resolve_sell(handler)
    );

    const auto stronger_sold = DeferredSoldModel::from_purchase(*stronger_day_two, handler);
    const auto weaker_sold = DeferredSoldModel::from_purchase(*weaker_day_two, handler);
    assert(stronger_sold.has_value());
    assert(weaker_sold.has_value());
    assert(stronger_sold->dominates(*weaker_sold));
    assert_same_food_gold_dominance(
        stronger_sold->materialize(),
        weaker_sold->materialize()
    );
}

void test_deferred_dominance_requires_identical_allocation_shape_and_suffix() {
    const Player before = small_market_player();
    const DeferredPurchaseModel stronger = DeferredPurchaseModel::open(
        before,
        {
            DeferredPurchaseSeed{15, 0, 201},
            DeferredPurchaseSeed{10, 5, 202},
        }
    );
    const DeferredPurchaseModel weaker = DeferredPurchaseModel::open(
        before,
        {
            DeferredPurchaseSeed{10, 0, 203},
            DeferredPurchaseSeed{5, 5, 204},
        }
    );
    assert(stronger.origin_count() == 2);
    assert(weaker.origin_count() == 2);
    assert(stronger.dominance_partition_hash() == weaker.dominance_partition_hash());
    assert(stronger.dominates(weaker));

    Player changed_market = before;
    ++changed_market.item_shop[i_Spice][id_Buy];
    const DeferredPurchaseModel other_market = DeferredPurchaseModel::open(
        changed_market,
        {DeferredPurchaseSeed{10, 0, 205}}
    );
    assert(!stronger.dominates(other_market));

    BestPurchaseHandler handler;
    const auto stronger_sold = DeferredSoldModel::from_purchase(stronger, handler);
    const auto weaker_sold = DeferredSoldModel::from_purchase(weaker, handler);
    assert(stronger_sold.has_value());
    assert(weaker_sold.has_value());
    const auto weaker_after_merchant = weaker_sold->after_merchant(m_No_Merchant);
    assert(weaker_after_merchant.has_value());
    assert(!stronger_sold->dominates(*weaker_after_merchant));
    assert(stronger_sold->dominance_partition_hash() !=
           weaker_after_merchant->dominance_partition_hash());
}

void test_cross_origin_shape_dominance_preserves_every_materialized_continuation() {
    const Player before = small_market_player();
    const DeferredPurchaseModel stronger = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{60, 0, 401}}
    );
    const DeferredPurchaseModel weaker = DeferredPurchaseModel::open(
        before,
        {DeferredPurchaseSeed{50, 1, 402}}
    );
    uint64_t gold_comparisons = 0;
    uint64_t spend_evaluations = 0;
    uint64_t coverage_checks = 0;
    uint64_t cross_shape_successes = 0;
    assert(stronger.dominates(
        weaker,
        &gold_comparisons,
        &spend_evaluations,
        &coverage_checks,
        &cross_shape_successes
    ));
    assert(!weaker.dominates(stronger));
    assert(gold_comparisons > 0);
    assert(spend_evaluations > 0);
    assert(coverage_checks > 0);
    assert(cross_shape_successes == 1);

    assert_same_food_gold_dominance(
        stronger.resolve_food_only_for_next_buy(),
        weaker.resolve_food_only_for_next_buy()
    );
    BestPurchaseHandler handler;
    assert_same_food_gold_dominance(
        stronger.resolve_sell(handler),
        weaker.resolve_sell(handler)
    );
}
}

void run_deferred_purchase_model_tests() {
    test_food_only_boundary_matches_explicit_low_interest();
    test_sell_resolution_matches_explicit_enumeration();
    test_final_day_sell_discards_extra_food_frontier();
    test_hire_filters_gold_and_does_not_expand_old_buy_capacity();
    test_merchant_sell_value_uses_old_buy_market_and_new_sell_market();
    test_repeated_buy_origin_frontier_reaches_the_same_sell_states();
    test_sold_ledger_matches_eager_sell_through_later_actions();
    test_sold_envelope_transforms_without_requerying_sale_market();
    test_weightless_sold_envelope_is_a_canonical_resource_frontier();
    test_deferred_search_state_identity_ignores_trace_provenance();
    test_unresolved_purchase_finishes_with_the_best_concrete_basket();
    test_final_inventory_uses_current_buy_value_after_witch();
    test_open_purchase_discards_an_origin_dominated_for_every_food_total();
    test_open_purchase_keeps_origins_that_win_different_food_ranges();
    test_exact_buy_coverage_distinguishes_exact_replaced_and_missing();
    test_continuation_views_are_independent_and_do_not_mutate_owner();
    test_deferred_dominance_preserves_materialized_continuations();
    test_deferred_dominance_requires_identical_allocation_shape_and_suffix();
    test_cross_origin_shape_dominance_preserves_every_materialized_continuation();
}
