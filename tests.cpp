#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include "benchmarks/NormaliaBenchmark.h"
#include "benchmarks/RecordRoutes.h"
#include "game/Game.h"
#include "purchaseHandler/FenwickTree2D.h"
#include "purchaseHandler/BestPurchaseHandler.h"
#include "solvers/GameSolver.h"
#include "solvers/AdmissibleUpperBound.h"
#include "solvers/ContinuationMatrixSelector.h"
#include "solvers/DeferredGoldFunction.h"
#include "solvers/DeferredPurchaseModel.h"
#include "solvers/DeferredSearchState.h"
#include "solvers/DeferredSoldModel.h"
#include "solvers/DeferredTraceRecipe.h"
#include "solvers/SolverStateCodec.h"
#include "solvers/PersistentSearchGraph.h"
#include "solvers/EncounterTiming.h"
#include "solvers/FrontierAccumulator.h"
#include "solvers/FrontierReducer.h"
#include "solvers/RoutePolisher.h"
#include "solvers/RouteSurvivalAudit.h"
#include "solvers/SkeletonWorkQueue.h"
#include "solvers/ValidatedRouteReplay.h"
#include "solvers/helpers.h"
#include "runtime/ProcessSafety.h"
#include "tests/DeferredPurchaseOracle.h"
#include "tests/DeferredPurchaseModelTests.h"

namespace {
const int TEST_MERCH_DAYS[3] = {5, 6, 7};
const int TEST_WITCH_DAYS[3] = {9, 10, 11};

std::vector<buy_option> build_small_options(const Player& player, const int gold_limit, const int weight_limit) {
    std::vector<buy_option> options;
    uint32_t best = 0;
    for (int spice = 0; spice <= gold_limit / player.item_shop[i_Spice][id_Buy]; ++spice) {
        for (int pottery = 0; pottery <= gold_limit / player.item_shop[i_Pottery][id_Buy]; ++pottery) {
            for (int marble = 0; marble <= gold_limit / player.item_shop[i_Marble][id_Buy]; ++marble) {
                for (int silk = 0; silk <= gold_limit / player.item_shop[i_Silk][id_Buy]; ++silk) {
                    for (int jewelry = 0; jewelry <= gold_limit / player.item_shop[i_Jewelry][id_Buy]; ++jewelry) {
                        const int cost =
                            spice * player.item_shop[i_Spice][id_Buy] +
                            pottery * player.item_shop[i_Pottery][id_Buy] +
                            marble * player.item_shop[i_Marble][id_Buy] +
                            silk * player.item_shop[i_Silk][id_Buy] +
                            jewelry * player.item_shop[i_Jewelry][id_Buy];
                        const int weight =
                            spice * player.item_shop[i_Spice][id_Weight] +
                            pottery * player.item_shop[i_Pottery][id_Weight] +
                            marble * player.item_shop[i_Marble][id_Weight] +
                            silk * player.item_shop[i_Silk][id_Weight] +
                            jewelry * player.item_shop[i_Jewelry][id_Weight];

                        if (cost > gold_limit || weight > weight_limit) {
                            continue;
                        }

                        const uint32_t profit =
                            spice * std::max(player.item_shop[i_Spice][id_Sell] - player.item_shop[i_Spice][id_Buy], 0) +
                            pottery * std::max(player.item_shop[i_Pottery][id_Sell] - player.item_shop[i_Pottery][id_Buy], 0) +
                            marble * std::max(player.item_shop[i_Marble][id_Sell] - player.item_shop[i_Marble][id_Buy], 0) +
                            silk * std::max(player.item_shop[i_Silk][id_Sell] - player.item_shop[i_Silk][id_Buy], 0) +
                            jewelry * std::max(player.item_shop[i_Jewelry][id_Sell] - player.item_shop[i_Jewelry][id_Buy], 0);
                        best = std::max(best, profit);
                        options.emplace_back(
                            static_cast<uint16_t>(cost),
                            static_cast<uint16_t>(weight),
                            profit,
                            buy_list(0, static_cast<uint8_t>(spice), static_cast<uint8_t>(pottery), static_cast<uint8_t>(marble), static_cast<uint8_t>(silk), static_cast<uint8_t>(jewelry))
                        );
                    }
                }
            }
        }
    }
    (void)best;
    return options;
}

uint32_t brute_force_profit(const Player& player, const int gold_limit, const int weight_limit) {
    uint32_t best = 0;
    for (const auto& option : build_small_options(player, gold_limit, weight_limit)) {
        if (option.cost <= gold_limit && option.weight <= weight_limit) {
            best = std::max(best, option.profit);
        }
    }
    return best;
}

void test_sturdy_saddle() {
    Player player;
    player.gold = 100;
    const int original_capacity = player.npc_shop[n_Camel][nd_Effect];
    const bool purchased = Game::event_merchant(player, m_Sturdy_Saddle);
    assert(purchased);
    assert(player.npc_shop[n_Camel][nd_Effect] == original_capacity + 20);
}

void test_persistent_codec_preserves_sold_allocation_order() {
    Player player;
    player.gold = 180;
    player.food = 12;
    player.backpack = 20;
    DeferredPurchaseModel purchase = DeferredPurchaseModel::open(
        player, {DeferredPurchaseSeed{player.gold, player.food, 17}}
    );
    DeferredPurchaseModel alternate_provenance = DeferredPurchaseModel::open(
        player, {DeferredPurchaseSeed{player.gold, player.food, 99}}
    );
    SolverState purchase_state = SolverState::from_player(purchase.representative(), 17);
    purchase_state.deferred = std::make_shared<const DeferredSearchState>(purchase);
    SolverState alternate_state = SolverState::from_player(
        alternate_provenance.representative(), 99
    );
    alternate_state.deferred = std::make_shared<const DeferredSearchState>(
        std::move(alternate_provenance)
    );
    assert(purchase_state == alternate_state);
    assert(SolverStateCodec::semantic_key(purchase_state) ==
           SolverStateCodec::semantic_key(alternate_state));
    assert(SolverStateCodec::snapshot(purchase_state) !=
           SolverStateCodec::snapshot(alternate_state));
    SolverState compact_purchase = SolverStateCodec::restore(
        SolverStateCodec::semantic_key(purchase_state),
        SolverStateCodec::provenance(purchase_state)
    );
    assert(purchase_state == compact_purchase);
    assert(compact_purchase.trace_id == purchase_state.trace_id);
    assert(compact_purchase.deferred->provenances() ==
           purchase_state.deferred->provenances());
    SolverState different_resources = purchase_state;
    ++different_resources.gold;
    assert(!(purchase_state == different_resources));
    assert(SolverStateCodec::semantic_key(purchase_state) !=
           SolverStateCodec::semantic_key(different_resources));

    BestPurchaseHandler handler;
    std::optional<DeferredSoldModel> sold = DeferredSoldModel::from_purchase(
        purchase, handler
    );
    assert(sold.has_value());
    SolverState original = SolverState::from_player(sold->representative(), 23);
    original.deferred = std::make_shared<const DeferredSearchState>(std::move(*sold));
    SolverState restored = SolverStateCodec::restore(
        SolverStateCodec::snapshot(original)
    );
    SolverState compact_restored = SolverStateCodec::restore(
        SolverStateCodec::semantic_key(original),
        SolverStateCodec::provenance(original)
    );
    assert(original == restored);
    assert(original == compact_restored);
    assert(compact_restored.trace_id == original.trace_id);
    assert(compact_restored.deferred->provenances() ==
           original.deferred->provenances());

    const auto before = original.deferred->sold().materialize();
    const auto after = restored.deferred->sold().materialize();
    assert(before.size() == after.size());
    bool saw_asymmetric_buy = false;
    for (size_t index = 0; index < before.size(); ++index) {
        const buy_list& lhs = before[index].concrete_buy;
        const buy_list& rhs = after[index].concrete_buy;
        saw_asymmetric_buy = saw_asymmetric_buy ||
            lhs.food != lhs.jewelry || lhs.spice != lhs.silk ||
            lhs.pottery != lhs.marble;
        assert(lhs.food == rhs.food);
        assert(lhs.spice == rhs.spice);
        assert(lhs.pottery == rhs.pottery);
        assert(lhs.marble == rhs.marble);
        assert(lhs.silk == rhs.silk);
        assert(lhs.jewelry == rhs.jewelry);
        assert(before[index].parent_trace == after[index].parent_trace);
        const auto compact = compact_restored.deferred->sold().materialize();
        assert(lhs.food == compact[index].concrete_buy.food);
        assert(lhs.spice == compact[index].concrete_buy.spice);
        assert(lhs.pottery == compact[index].concrete_buy.pottery);
        assert(lhs.marble == compact[index].concrete_buy.marble);
        assert(lhs.silk == compact[index].concrete_buy.silk);
        assert(lhs.jewelry == compact[index].concrete_buy.jewelry);
        assert(before[index].parent_trace == compact[index].parent_trace);
    }
    assert(saw_asymmetric_buy);
}

void test_persistent_graph_resumes_only_unexplored_descendants() {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path database =
        std::filesystem::temp_directory_path() /
        ("caravan-persistent-graph-test-" + std::to_string(nonce) + ".sqlite3");
    const std::vector<uint8_t> scenario_key{1, 4, 9, 16};
    SolverState root = SolverState::from_player(Player{});
    SolverState first = root;
    first.gold += 1;
    first.day = 3;
    first.trace_id = 1;
    SolverState second = root;
    second.gold += 2;
    second.day = 1;
    second.trace_id = 1;
    std::vector<TraceNode> traces;

    {
        PersistentSearchGraph graph(database, scenario_key, "test", root);
        auto open_root = graph.next_open();
        assert(open_root.has_value());
        graph.localize_traces(*open_root, traces);
        traces.push_back(TraceNode{
            0,
            SolverAction{_a_interest, buy_list(), 0},
            traces[0].early_skeleton.extended(
                SolverAction{_a_interest, buy_list(), 0}
            ),
        });
        // This trace is not referenced by a retained successor or top-three
        // terminal and therefore must remain out of cold storage.
        traces.push_back(TraceNode{
            0,
            SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Camel)},
            traces[0].early_skeleton.extended(
                SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Camel)}
            ),
        });
        graph.commit_expansion(
            open_root->id, {first, second}, {1.0, 100.0}, {}, traces
        );
        const PersistentSearchStats partial = graph.stats();
        assert(partial.nodes == 3);
        assert(partial.open_nodes == 2);
        assert(partial.partial_nodes == 1);
        assert(partial.solved_nodes == 0);
        assert(partial.trace_links == 2);
        assert(!partial.root_solved);

        const PersistentSearchNodeSummary root_summary = graph.root();
        assert(root_summary.status == PersistentNodeStatus::Partial);
        const auto unresolved = graph.unresolved_children(root_summary.id);
        assert(unresolved.size() == 2);
        auto shallow = graph.load_open(unresolved.front().id);
        auto deep = graph.load_open(unresolved.back().id);
        assert(shallow.has_value());
        assert(deep.has_value());
        assert(shallow->state.gold == second.gold);
        assert(deep->state.gold == first.gold);

        auto open_first = graph.next_open();
        assert(open_first.has_value());
        assert(open_first->state.gold == first.gold);
        graph.localize_traces(*open_first, traces);
        graph.commit_expansion(
            open_first->id, {}, {},
            {FinishedSolve{123, {}, 1, FinishedSolve::Origin::Discovered}},
            traces
        );
        const PersistentSearchStats one_branch_done = graph.stats();
        assert(one_branch_done.open_nodes == 1);
        assert(one_branch_done.partial_nodes == 1);
        assert(one_branch_done.solved_nodes == 1);
        assert(!one_branch_done.root_solved);
        assert(one_branch_done.root_best_score == 123);
    }

    {
        PersistentSearchGraph resumed(database, scenario_key, "test", root);
        auto only_unexplored = resumed.next_open();
        assert(only_unexplored.has_value());
        assert(only_unexplored->state.gold == second.gold);
        resumed.localize_traces(*only_unexplored, traces);
        resumed.commit_expansion(only_unexplored->id, {}, {}, {}, traces);
        const PersistentSearchStats complete = resumed.stats();
        assert(complete.open_nodes == 0);
        assert(complete.partial_nodes == 0);
        assert(complete.solved_nodes == 3);
        assert(complete.root_solved);
        assert(complete.root_best_score == 123);
        const auto routes = resumed.best_routes(3);
        assert(routes.size() == 1);
        assert(routes.front().points == 123);
        assert(routes.front().actions.size() == 1);
        assert(routes.front().actions.front().action == _a_interest);
    }

    std::error_code ignored;
    std::filesystem::remove(database, ignored);
    std::filesystem::remove(database.string() + "-wal", ignored);
    std::filesystem::remove(database.string() + "-shm", ignored);
}

void test_persistent_graph_allocates_trace_ids_across_writers() {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path database =
        std::filesystem::temp_directory_path() /
        ("caravan-persistent-multiwriter-test-" + std::to_string(nonce) + ".sqlite3");
    const std::vector<uint8_t> scenario_key{2, 7, 1, 8};
    SolverState root = SolverState::from_player(Player{});
    SolverState first = root;
    first.gold += 1;
    first.day = 2;
    first.trace_id = 1;
    SolverState second = root;
    second.gold += 2;
    second.day = 1;
    second.trace_id = 1;
    std::vector<TraceNode> seed_traces;

    {
        PersistentSearchGraph seed(database, scenario_key, "multiwriter", root);
        auto open_root = seed.next_open();
        assert(open_root.has_value());
        seed.localize_traces(*open_root, seed_traces);
        seed_traces.push_back(TraceNode{
            0,
            SolverAction{_a_interest, buy_list(), 0},
            seed_traces[0].early_skeleton.extended(
                SolverAction{_a_interest, buy_list(), 0}
            ),
        });
        seed.commit_expansion(
            open_root->id, {first, second}, {2.0, 1.0}, {}, seed_traces
        );
    }

    {
        // Both writers open before either commits, so a process-local MAX+1
        // allocator gives them the same next ID.
        PersistentSearchGraph writer_a(database, scenario_key, "multiwriter", root);
        PersistentSearchGraph writer_b(database, scenario_key, "multiwriter", root);
        const auto children = writer_a.unresolved_children(writer_a.root().id);
        assert(children.size() == 2);

        auto open_a = writer_a.load_open(children[0].id);
        auto open_b = writer_b.load_open(children[1].id);
        assert(open_a.has_value());
        assert(open_b.has_value());
        std::vector<TraceNode> traces_a;
        std::vector<TraceNode> traces_b;
        writer_a.localize_traces(*open_a, traces_a);
        writer_b.localize_traces(*open_b, traces_b);

        const SolverAction camel{_a_npc, buy_list(), static_cast<uint8_t>(n_Camel)};
        traces_a.push_back(TraceNode{
            open_a->state.trace_id,
            camel,
            traces_a[open_a->state.trace_id].early_skeleton.extended(camel),
        });
        traces_b.push_back(TraceNode{
            open_b->state.trace_id,
            camel,
            traces_b[open_b->state.trace_id].early_skeleton.extended(camel),
        });
        SolverState next_a = open_a->state;
        next_a.gold += 10;
        next_a.trace_id = static_cast<uint32_t>(traces_a.size() - 1);
        SolverState next_b = open_b->state;
        next_b.gold += 20;
        next_b.trace_id = static_cast<uint32_t>(traces_b.size() - 1);

        writer_a.commit_expansion(open_a->id, {next_a}, {1.0}, {}, traces_a);
        writer_b.commit_expansion(open_b->id, {next_b}, {1.0}, {}, traces_b);
        assert(writer_a.stats().trace_links == 4);
    }

    std::error_code ignored;
    std::filesystem::remove(database, ignored);
    std::filesystem::remove(database.string() + "-wal", ignored);
    std::filesystem::remove(database.string() + "-shm", ignored);
}

void test_unfixed_encounters_can_defer_until_the_last_eligible_day() {
    Player classical;
    classical.day = 5;
    assert(EncounterTiming::merchant(classical).due);
    assert(EncounterTiming::merchant(classical).may_defer);
    classical.day = 6;
    assert(EncounterTiming::merchant(classical).due);
    assert(EncounterTiming::merchant(classical).may_defer);
    classical.day = 7;
    assert(EncounterTiming::merchant(classical).due);
    assert(!EncounterTiming::merchant(classical).may_defer);

    classical.day = 9;
    assert(EncounterTiming::witch(classical).due);
    assert(EncounterTiming::witch(classical).may_defer);
    classical.day = 10;
    assert(EncounterTiming::witch(classical).due);
    assert(EncounterTiming::witch(classical).may_defer);
    classical.day = 11;
    assert(EncounterTiming::witch(classical).due);
    assert(!EncounterTiming::witch(classical).may_defer);

    Player fixed;
    fixed.merchant_day = 7;
    fixed.witch_day = 10;
    fixed.day = 6;
    assert(!EncounterTiming::merchant(fixed).due);
    assert(!EncounterTiming::witch(fixed).due);
    fixed.day = 7;
    assert(EncounterTiming::merchant(fixed).due);
    assert(!EncounterTiming::merchant(fixed).may_defer);
    fixed.day = 10;
    assert(EncounterTiming::witch(fixed).due);
    assert(!EncounterTiming::witch(fixed).may_defer);

    Player traveler;
    traveler.traveler_mode = true;
    traveler.day = 7;
    assert(EncounterTiming::merchant(traveler).may_defer);
    traveler.day = 9;
    assert(!EncounterTiming::merchant(traveler).may_defer);
    traveler.day = 11;
    assert(EncounterTiming::witch(traveler).may_defer);
    traveler.day = 13;
    assert(!EncounterTiming::witch(traveler).may_defer);
}

void test_animal_lover() {
    Player player;
    const int original_camels = player.camel;
    const int original_food = player.food_consumption;
    Game::event_witch(player, w_Animal_Lover);
    assert(player.camel == original_camels + 2);
    assert(player.food_consumption == original_food + 2);
    assert(player.max_day == 14);
}

void test_exact_merchant_mutations() {
    Player cornucopia;
    cornucopia.gold = 100;
    assert(Game::event_merchant(cornucopia, m_Cornucopia));
    assert(cornucopia.gold == 75);
    assert(cornucopia.bonus_food_consumption == -2);
    assert(cornucopia.food_consumption == 0);

    Player low_consumption;
    low_consumption.gold = 100;
    low_consumption.trader = 0;
    Game::recalculate_food_consumption(low_consumption);
    assert(low_consumption.food_consumption == 1);
    assert(Game::event_merchant(low_consumption, m_Cornucopia));
    assert(low_consumption.bonus_food_consumption == -1);
    assert(low_consumption.food_consumption == 0);

    Player leader;
    leader.gold = 100;
    leader.trader_interest = true;
    const double original_rate = leader.interest_rate;
    assert(Game::event_merchant(leader, m_Leaders_Necklace));
    assert(leader.trader == 2);
    assert(leader.food_consumption == 3);
    assert(leader.interest_rate == original_rate);
}

void test_exact_witch_mutations() {
    Player present;
    present.interest_rate = 0.1;
    Game::event_witch(present, w_Presents_Gift);
    assert(present.backpack == 20);
    assert(present.interest_rate == 0.0);

    Player camelization;
    camelization.camel = 3;
    Game::recalculate_food_consumption(camelization);
    Game::event_witch(camelization, w_Camelization);
    assert(camelization.npc_shop[n_Camel][nd_Effect] == 40);
    assert(camelization.npc_shop[n_Camel][nd_Eat] == 2);
    assert(camelization.food_consumption == 7);

    Player oasis;
    oasis.bonus_food_consumption = -2;
    Game::recalculate_food_consumption(oasis);
    Game::event_witch(oasis, w_Oasis_of_Sanctifan);
    assert(oasis.npc_shop[n_Trader][nd_Eat] == 0);
    assert(oasis.npc_shop[n_Camel][nd_Eat] == 0);
    assert(oasis.bonus_food_consumption == 0);
    assert(oasis.food_consumption == 0);
}

void test_all_disaster_mutations() {
    Player steal_even_up;
    steal_even_up.gold = 2;
    Game::event_disaster(steal_even_up, d_Steal);
    assert(steal_even_up.gold == 2); // 1.5 rounds to even 2.

    Player steal_even_down;
    steal_even_down.gold = 6;
    Game::event_disaster(steal_even_down, d_Steal);
    assert(steal_even_down.gold == 4); // 4.5 rounds to even 4.

    Player brokeleg;
    brokeleg.backpack = 20;
    Game::event_disaster(brokeleg, d_Brokeleg);
    assert(brokeleg.backpack == 10);

    Player tradersleft;
    tradersleft.trader = 3;
    tradersleft.camel = 2;
    tradersleft.bonus_food_consumption = -2;
    Game::recalculate_food_consumption(tradersleft);
    Game::event_disaster(tradersleft, d_Tradersleft);
    assert(tradersleft.trader == 0);
    assert(tradersleft.food_consumption == 0);

    Player marble;
    Game::event_disaster(marble, d_Marblecrisis);
    assert(marble.item_shop[i_Marble][id_Sell] == 0);

    Player saving;
    saving.interest_rate = 0.2;
    saving.set_interest = true;
    Game::event_disaster(saving, d_Savingcrisis);
    assert(std::abs(saving.interest_rate - (-0.1)) < 1e-9);

    Player silk_penury;
    Game::event_disaster(silk_penury, d_Silkpenury);
    assert(silk_penury.item_shop[i_Silk][id_Buy] == 730);

    Player seer;
    seer.camel = 0;
    Game::recalculate_food_consumption(seer);
    Game::event_disaster(seer, d_Seer);
    assert(seer.max_day == 16);
    assert(seer.camel == -1);
    assert(seer.food_consumption == 0);

    Player heavy_food;
    Game::event_disaster(heavy_food, d_Heavyfood);
    assert(heavy_food.item_shop[i_Food][id_Weight] == 4);

    Player jewelry;
    Game::event_disaster(jewelry, d_Jewelrycrisis);
    assert(jewelry.item_shop[i_Jewelry][id_Sell] == 1800);

    Player silk_crisis;
    Game::event_disaster(silk_crisis, d_Silkcrisis);
    assert(silk_crisis.item_shop[i_Silk][id_Sell] == 800);

    Player food_crisis;
    Game::event_disaster(food_crisis, d_Foodcrisis);
    assert(food_crisis.item_shop[i_Food][id_Buy] == 15);

    Player camel_sick;
    Game::event_disaster(camel_sick, d_Camelsick);
    assert(camel_sick.npc_shop[n_Camel][nd_Eat] == 2);
    assert(camel_sick.food_consumption == 3);
    assert(camel_sick.disaster_happened);
    assert(camel_sick.player_world.disaster == d_Camelsick);
}

void test_merchant_witch_disaster_interactions() {
    Player capacity;
    capacity.gold = 200;
    assert(Game::event_merchant(capacity, m_Canvas_Bag));
    Game::event_witch(capacity, w_Presents_Gift);
    Game::event_disaster(capacity, d_Brokeleg);
    assert(capacity.backpack == 30); // +20 +20 -10

    Player rate;
    rate.gold = 200;
    assert(Game::event_merchant(rate, m_Blue_Treasure));
    Game::event_disaster(rate, d_Savingcrisis);
    assert(std::abs(rate.interest_rate - 1.4) < 1e-9);

    Player crew;
    crew.gold = 200;
    crew.trader_interest = true;
    assert(Game::event_merchant(crew, m_Leaders_Necklace));
    Game::event_witch(crew, w_Midas_was_a_Trader);
    Game::event_disaster(crew, d_Tradersleft);
    assert(crew.trader == 0);
    assert(crew.npc_shop[n_Trader][nd_Effect] == 170);
    Game::event_buy_npc(crew, n_Trader);
    assert(crew.trader == 1);
    assert(crew.interest_rate == 1.6); // A real later purchase still triggers Eduming.

    Player camel_food;
    Game::event_witch(camel_food, w_Camelization);
    Game::event_disaster(camel_food, d_Camelsick);
    assert(camel_food.npc_shop[n_Camel][nd_Eat] == 4);
    assert(camel_food.food_consumption == 5);

    Player oasis;
    Game::event_witch(oasis, w_Oasis_of_Sanctifan);
    Game::event_disaster(oasis, d_Camelsick);
    assert(oasis.npc_shop[n_Camel][nd_Eat] == 0);
    assert(oasis.food_consumption == 0);

    Player animal_seer;
    Game::event_witch(animal_seer, w_Animal_Lover);
    Game::event_disaster(animal_seer, d_Seer);
    assert(animal_seer.camel == 2);
    assert(animal_seer.max_day == 15);
}

void test_seer_is_a_run_state_delta() {
    Player player;
    player.camel = 0;
    player.gold = 100;
    Game::recalculate_food_consumption(player);
    Game::event_disaster(player, d_Seer);
    assert(player.camel == -1);
    Game::event_buy_npc(player, n_Camel);
    assert(player.camel == 0);

    Player new_run;
    assert(new_run.camel == 1);
    assert(new_run.max_day == 15);
}

void test_solver_state_preserves_disaster_values() {
    Player player;
    player.camel = -1;
    player.bonus_food_consumption = -2;
    player.food_consumption = -1;
    player.interest_rate = -0.1;
    player.disaster_happened = true;
    player.disaster_day = 16;
    player.scheduled_disaster = d_Seer;
    player.merchant_day = 8;
    player.witch_day = 12;
    player.cargo_buy_locked = true;
    player.player_world.disaster = d_Seer;

    const Player restored = SolverState::from_player(player).to_player();
    assert(restored.camel == -1);
    assert(restored.bonus_food_consumption == -2);
    assert(restored.food_consumption == -1);
    assert(std::abs(restored.interest_rate - (-0.1)) < 1e-9);
    assert(restored.disaster_happened);
    assert(restored.disaster_day == 16);
    assert(restored.scheduled_disaster == d_Seer);
    assert(restored.merchant_day == 8);
    assert(restored.witch_day == 12);
    assert(restored.cargo_buy_locked);
    assert(restored.player_world.disaster == d_Seer);
}

void test_traveler_rate_precision_and_preparation() {
    Player precision;
    precision.interest_rate = 1.55;
    const Player restored = SolverState::from_player(precision).to_player();
    assert(std::abs(restored.interest_rate - 1.55) < 1e-9);

    Player base;
    for (const traveler_upgrades upgrade : ALL_TRAVELER_UPGRADES) {
        Player upgraded = base;
        Game::apply_traveler_upgrade(upgraded, upgrade);
        switch (upgrade) {
            case tu_Weight: assert(upgraded.backpack == 10); break;
            case tu_Gold: assert(upgraded.gold == 40); break;
            case tu_Pottery: assert(upgraded.item_shop[i_Pottery][id_Buy] == 15); break;
            case tu_Marble: assert(upgraded.item_shop[i_Marble][id_Weight] == 15); break;
            case tu_Jewelry: assert(upgraded.item_shop[i_Jewelry][id_Sell] == 2800); break;
            case tu_Trader: assert(upgraded.npc_shop[n_Trader][nd_Cost] == 15); break;
            case tu_Camel: assert(upgraded.npc_shop[n_Camel][nd_Cost] == 0); break;
            case tu_Traders_Income: assert(upgraded.npc_shop[n_Trader][nd_Effect] == 25); break;
            case tu_Load_Capacity: assert(upgraded.npc_shop[n_Camel][nd_Effect] == 35); break;
            case tu_Saving: assert(std::abs(upgraded.interest_rate - 1.55) < 1e-9); break;
        }
    }

    const Player traveler = Game::create_traveler_player(
        t_Skjollird,
        {tu_Camel, tu_Gold, tu_Saving},
        7,
        11,
        16,
        d_Seer
    );
    assert(traveler.traveler_mode);
    assert(traveler.gold == 20);
    assert(traveler.trader == 0);
    assert(traveler.camel == 0);
    assert(traveler.food_consumption == 0);
    assert(traveler.backpack == 20);
    assert(traveler.max_day == 20);
    assert(std::abs(traveler.interest_rate - 1.55) < 1e-9);
    assert(traveler.npc_shop[n_Camel][nd_Cost] == 5); // Prepare sets 0, then Skjollird += 5.
    assert(traveler.npc_shop[n_Camel][nd_Effect] == 40);
    assert(traveler.disaster_day == 16);
    assert(traveler.scheduled_disaster == d_Seer);
    assert(traveler.merchant_day == 7);
    assert(traveler.witch_day == 11);

    bool rejected_duplicate = false;
    try {
        (void)Game::create_traveler_player(t_Normalia, {tu_Gold, tu_Gold, tu_Saving});
    } catch (const std::invalid_argument&) {
        rejected_duplicate = true;
    }
    assert(rejected_duplicate);
}

void test_solver_uses_traveler_encounter_days() {
    Player merchant;
    merchant.traveler_mode = true;
    merchant.merchant_day = 8;
    merchant.day = 7;
    merchant.max_day = 8;
    merchant.gold = 100;
    merchant.food = 10;
    merchant.trader = 0;
    merchant.camel = 0;
    Game::recalculate_food_consumption(merchant);
    GameSolver merchant_solver(merchant, true, false, 500000, 1000000, "", 0);
    merchant_solver.find_solve_depth(false);
    assert(merchant_solver.profiling_stats().generated_merchant_states > 0);

    GameSolver forced_merchant_solver(merchant, true, false, 500000, 1000000, "", 0);
    forced_merchant_solver.set_forced_merchant_option(m_Blue_Treasure);
    forced_merchant_solver.find_solve_depth(false);
    assert(forced_merchant_solver.profiling_stats().generated_merchant_states == 1);

    Player later_merchant = merchant;
    later_merchant.merchant_day = 9;
    GameSolver later_merchant_solver(later_merchant, true, false, 500000, 1000000, "", 0);
    later_merchant_solver.find_solve_depth(false);
    assert(later_merchant_solver.profiling_stats().generated_merchant_states == 0);

    Player leave;
    assert(Game::event_merchant(leave, m_No_Merchant));
    assert(leave.merchant_happened);
    assert(leave.player_world.merch == m_No_Merchant);

    Player witch = merchant;
    witch.day = 11;
    witch.max_day = 12;
    witch.witch_day = 12;
    witch.merchant_happened = true;
    GameSolver witch_solver(witch, false, true, 500000, 1000000, "", 0);
    witch_solver.find_solve_depth(false);
    assert(witch_solver.profiling_stats().generated_witch_states > 0);

    GameSolver forced_witch_solver(witch, false, true, 500000, 1000000, "", 0);
    forced_witch_solver.set_forced_witch_option(w_Midas_was_a_Trader);
    forced_witch_solver.find_solve_depth(false);
    assert(forced_witch_solver.profiling_stats().generated_witch_states == 1);
}

void test_classical_encounter_day_validation() {
    bool rejected_merchant = false;
    try {
        (void)GameSolver(
            t_Normalia, true, true, 10, 1000000, {}, "", 0,
            PurchaseProfile::Strategic, 25, false, 0, 4, 9
        );
    } catch (const std::invalid_argument&) {
        rejected_merchant = true;
    }
    assert(rejected_merchant);

    bool rejected_witch = false;
    try {
        (void)GameSolver(
            t_Normalia, true, true, 10, 1000000, {}, "", 0,
            PurchaseProfile::Strategic, 25, false, 0, 5, 12
        );
    } catch (const std::invalid_argument&) {
        rejected_witch = true;
    }
    assert(rejected_witch);

    (void)GameSolver(
        t_Normalia, true, true, 10, 1000000, {}, "", 0,
        PurchaseProfile::Strategic, 25, false, 0, 7, 11
    );
}

void test_solver_can_defer_callable_merchant() {
    Player state;
    state.gold = 100;
    state.food = 10;
    state.trader = 0;
    state.camel = 0;
    Game::recalculate_food_consumption(state);
    state.max_day = 2;
    state.call_merchant = true;

    GameSolver solver(state, true, false, 500000, 1000000, "", 0);
    solver.find_solve_depth(false);

    assert(solver.profiling_stats().generated_merchant_states > 0);
    assert(solver.profiling_stats().merchant_deferral_continuations > 0);
    assert(!solver.best_solves().empty());
}

void test_solver_applies_a_fixed_disaster_scenario() {
    Player state;
    state.day = 14;
    state.max_day = 15;
    state.gold = 6;
    state.food = 10;
    state.trader = 0;
    state.camel = 0;
    Game::recalculate_food_consumption(state);
    state.disaster_day = 15;
    state.scheduled_disaster = d_Steal;

    GameSolver solver(state, false, false, 500000, 1000000, "", 0);
    solver.find_solve_depth(false);

    assert(solver.profiling_stats().generated_disaster_states > 0);
    assert(!solver.best_solves().empty());
    for (const FinishedSolve& solve : solver.best_solves()) {
        assert(std::any_of(solve.actions.begin(), solve.actions.end(), [](const SolverAction& action) {
            return action.action == _a_disaster && action.option == static_cast<uint8_t>(d_Steal);
        }));
    }
}

void test_purchase_cache_separates_price_disasters() {
    world base;
    const std::string base_key = BestPurchaseHandler::cacheKeyForWorld(base);

    world silk_penury = base;
    silk_penury.disaster = d_Silkpenury;
    const std::string disaster_key = BestPurchaseHandler::cacheKeyForWorld(silk_penury);
    assert(disaster_key != base_key);

    world food_crisis = base;
    food_crisis.disaster = d_Foodcrisis;
    assert(BestPurchaseHandler::cacheKeyForWorld(food_crisis) == base_key);
}

void test_interest_rounding() {
    Player player;
    player.gold = 1;
    player.trader = 0;
    player.npc_shop[n_Trader][nd_Effect] = 0;
    player.interest_rate = 2.8;
    Game::event_interest(player);
    assert(player.gold == 3);
}

void test_fenwick_matches_bruteforce() {
    Player player;
    const int gold_limit = 40;
    const int weight_limit = 20;
    FenwickTree2D tree(build_small_options(player, gold_limit, weight_limit));
    const auto [profit, buy_data] = tree.query(gold_limit, weight_limit);
    (void)buy_data;
    const uint32_t brute_profit = brute_force_profit(player, gold_limit, weight_limit);
    assert(profit == brute_profit);
}

void test_fenwick_exposes_every_cost_breakpoint() {
    Player player;
    FenwickTree2D tree(build_small_options(player, 80, 40));
    const auto frontier = tree.queryFrontier(80, 40);
    assert(!frontier.empty());
    assert(frontier.front().exact_cost == 0);
    assert(frontier.front().value == 0);
    for (size_t index = 1; index < frontier.size(); ++index) {
        assert(frontier[index - 1].exact_cost < frontier[index].exact_cost);
        assert(frontier[index - 1].value < frontier[index].value);
        const auto best = tree.query(frontier[index].exact_cost, 40);
        assert(best.first == frontier[index].value);
    }
}

void test_exact_sale_frontier_separates_buy_and_sell_markets() {
    Player buy_market;
    Player sell_market = buy_market;
    for (int item = i_Spice; item <= i_Jewelry; ++item) {
        buy_market.item_shop[item][id_Buy] = 1000;
        buy_market.item_shop[item][id_Weight] = 1000;
        sell_market.item_shop[item][id_Sell] = 0;
    }
    buy_market.item_shop[i_Spice][id_Buy] = 3;
    buy_market.item_shop[i_Spice][id_Weight] = 2;
    sell_market.item_shop[i_Spice][id_Sell] = 7;
    buy_market.item_shop[i_Pottery][id_Buy] = 5;
    buy_market.item_shop[i_Pottery][id_Weight] = 3;
    sell_market.item_shop[i_Pottery][id_Sell] = 11;

    constexpr int max_cost = 30;
    constexpr int max_weight = 15;
    struct Expected {
        int cost;
        int sale;
    };
    std::vector<Expected> brute;
    for (int spice = 0; spice <= 10; ++spice) {
        for (int pottery = 0; pottery <= 6; ++pottery) {
            if (spice + pottery == 0) {
                continue;
            }
            const int cost = spice * 3 + pottery * 5;
            const int weight = spice * 2 + pottery * 3;
            if (cost <= max_cost && weight <= max_weight) {
                brute.push_back(Expected{cost, spice * 7 + pottery * 11});
            }
        }
    }
    std::vector<Expected> nondominated;
    for (size_t candidate_index = 0; candidate_index < brute.size(); ++candidate_index) {
        bool dominated = false;
        for (size_t other_index = 0; other_index < brute.size(); ++other_index) {
            if (candidate_index == other_index) {
                continue;
            }
            const Expected& candidate = brute[candidate_index];
            const Expected& other = brute[other_index];
            if (other.cost <= candidate.cost && other.sale >= candidate.sale &&
                (other.cost < candidate.cost || other.sale > candidate.sale)) {
                dominated = true;
                break;
            }
        }
        if (!dominated) {
            nondominated.push_back(brute[candidate_index]);
        }
    }
    brute = std::move(nondominated);
    std::sort(brute.begin(), brute.end(), [](const Expected& lhs, const Expected& rhs) {
        return std::tie(lhs.cost, lhs.sale) < std::tie(rhs.cost, rhs.sale);
    });
    brute.erase(std::unique(brute.begin(), brute.end(), [](const Expected& lhs, const Expected& rhs) {
        return lhs.cost == rhs.cost && lhs.sale == rhs.sale;
    }), brute.end());

    BestPurchaseHandler handler;
    const auto actual = handler.getExactSaleFrontier(max_cost, max_weight, buy_market, sell_market);
    const auto cached = handler.getExactSaleFrontier(max_cost, max_weight, buy_market, sell_market);
    const auto prepared_market = handler.prepareExactSaleMarket(buy_market, sell_market);
    const auto prepared = prepared_market.frontier(max_cost, max_weight);
    assert(cached->size() == actual->size());
    assert(prepared == actual);
    const auto cache_profile = handler.getProfilingSummary();
    assert(cache_profile.exact_sale_calls == 3);
    assert(cache_profile.exact_sale_cache_hits == 2);
    assert(cache_profile.exact_sale_hot_cache_hits == 2);
    assert(cache_profile.exact_sale_cache_entries == 1);
    const size_t held_frontier_size = actual->size();
    BestPurchaseHandler eviction_handler(2);
    const auto held_across_eviction = eviction_handler.getExactSaleFrontier(
        max_cost,
        max_weight,
        buy_market,
        sell_market
    );
    (void)eviction_handler.getExactSaleFrontier(
        max_cost - 1,
        max_weight,
        buy_market,
        sell_market
    );
    (void)eviction_handler.getExactSaleFrontier(
        max_cost - 2,
        max_weight,
        buy_market,
        sell_market
    );
    assert(held_across_eviction->size() == held_frontier_size);
    std::vector<Expected> actual_values;
    for (const ExactPurchaseChoice& choice : *actual) {
        const int realized_cost =
            choice.buys.spice * buy_market.item_shop[i_Spice][id_Buy] +
            choice.buys.pottery * buy_market.item_shop[i_Pottery][id_Buy];
        const int realized_weight =
            choice.buys.spice * buy_market.item_shop[i_Spice][id_Weight] +
            choice.buys.pottery * buy_market.item_shop[i_Pottery][id_Weight];
        const int realized_sale =
            choice.buys.spice * sell_market.item_shop[i_Spice][id_Sell] +
            choice.buys.pottery * sell_market.item_shop[i_Pottery][id_Sell];
        assert(choice.actual_cost == realized_cost);
        assert(choice.actual_weight == realized_weight);
        assert(choice.sale_value == static_cast<uint32_t>(realized_sale));
        assert(realized_cost <= max_cost);
        assert(realized_weight <= max_weight);
        actual_values.push_back(Expected{realized_cost, realized_sale});
    }
    std::sort(actual_values.begin(), actual_values.end(), [](const Expected& lhs, const Expected& rhs) {
        return std::tie(lhs.cost, lhs.sale) < std::tie(rhs.cost, rhs.sale);
    });
    actual_values.erase(std::unique(actual_values.begin(), actual_values.end(), [](const Expected& lhs, const Expected& rhs) {
        return lhs.cost == rhs.cost && lhs.sale == rhs.sale;
    }), actual_values.end());
    assert(actual_values.size() == brute.size());
    for (size_t index = 0; index < brute.size(); ++index) {
        assert(actual_values[index].cost == brute[index].cost);
        assert(actual_values[index].sale == brute[index].sale);
    }
}

void test_fenwick_serialize_round_trip() {
    Player player;
    const int gold_limit = 40;
    const int weight_limit = 20;
    const std::string key = "unit_test_tree.bin";
    FenwickTree2D original(build_small_options(player, gold_limit, weight_limit));
    original.serialize(key);

    FenwickTree2D loaded;
    loaded.deserialize(key);
    const auto original_result = original.query(gold_limit, weight_limit);
    const auto loaded_result = loaded.query(gold_limit, weight_limit);
    assert(original_result.first == loaded_result.first);
    assert(original_result.second.food == loaded_result.second.food);
    assert(original_result.second.spice == loaded_result.second.spice);
    assert(original_result.second.pottery == loaded_result.second.pottery);
    assert(original_result.second.marble == loaded_result.second.marble);
    assert(original_result.second.silk == loaded_result.second.silk);
    assert(original_result.second.jewelry == loaded_result.second.jewelry);
}

void test_run_buy_event_with_zero_food_consumption() {
    Player player;
    Game::event_witch(player, w_Oasis_of_Sanctifan);
    player.day_start_skip = false;
    player.gold = 100;

    BestPurchaseHandler handler;
    std::deque<Player> generated;
    const BuyEventStats stats = Helpers::run_buy_event(player, generated, handler, false, false);

    assert(player.food_consumption == 0);
    assert(stats.best_buy_queries >= 1);
    assert(!generated.empty());
}

void test_run_buy_event_keeps_gold_for_next_turn_costs() {
    Player player;
    player.gold = 80;
    player.food = 6;
    player.trader = 1;
    player.camel = 1;
    player.food_consumption = 2;
    player.daily_income = 0;
    player.npc_shop[n_Trader][nd_Effect] = 20;
    player.npc_shop[n_Trader][nd_Cost] = 25;
    player.npc_shop[n_Camel][nd_Cost] = 30;

    BestPurchaseHandler handler;
    std::deque<Player> generated;
    Helpers::run_buy_event(player, generated, handler, false, false);

    bool found_reserved_state = false;
    while (!generated.empty()) {
        const Player candidate = generated.front();
        generated.pop_front();
        if (candidate.gold >= player.npc_shop[n_Trader][nd_Cost]) {
            found_reserved_state = true;
            break;
        }
    }
    assert(found_reserved_state);
}

void test_wait_matches_an_empty_buy_and_is_labeled_separately() {
    Player waited;
    waited.day = 4;
    waited.gold = 100;
    waited.trader = 2;

    Player empty_buy = waited;
    Game::event_wait(waited);
    Game::event_buy_items(empty_buy, buy_list());

    assert(waited.gold == empty_buy.gold);
    assert(waited.food == empty_buy.food);
    assert(waited.day == empty_buy.day);
    assert(waited.bought_last == empty_buy.bought_last);
    assert(waited.points == empty_buy.points);
    assert(waited.last_action == "4: Wait (empty Buy)");
    assert(empty_buy.last_action == "4: Buy [0, 0, 0, 0, 0, 0]");
}

void test_cargo_buy_lock_is_cleared_only_by_sell() {
    Player player;
    assert(Game::check_can_buy_items(player));

    Game::event_buy_items(player, buy_list(5, 0, 0, 0, 0, 0));
    assert(Game::check_can_buy_items(player));

    player.bought_last = false;
    Game::event_buy_items(player, buy_list(0, 1, 0, 0, 0, 0));
    assert(!Game::check_can_buy_items(player));

    Game::event_wait(player);
    assert(!Game::check_can_buy_items(player));

    Game::event_sell_items(player);
    assert(Game::check_can_buy_items(player));
}

void test_solver_does_not_generate_wait_while_cargo_buy_is_locked() {
    Player state;
    state.day = 11;
    state.max_day = 13;
    state.food = 20;
    state.spice = 1;
    state.cargo_buy_locked = true;
    state.bought_last = false;

    GameSolver solver(state, false, false, 500000, 1000000, "", 0);
    solver.find_solve_depth(false);

    assert(solver.profiling_stats().generated_wait_states == 0);
    assert(solver.profiling_stats().generated_buy_states == 0);
}

void test_purchase_portfolio_preserves_reserve_rich_near_optima() {
    Player player;
    BestPurchaseHandler handler;
    const auto maximum = handler.getBestBuy(500, 360, player);
    const auto choices = handler.getBestBuyPortfolio(500, 360, player);

    assert(!choices.empty());
    assert(choices.front().profit == maximum.first);
    bool found_lower_budget = false;
    for (const PurchaseChoice& choice : choices) {
        assert(choice.profit * 4 >= maximum.first * 3);
        const auto exact = handler.getBestBuy(choice.minimum_budget, 360, player);
        assert(exact.first == choice.profit);
        assert(exact.second.spice == choice.buys.spice);
        assert(exact.second.pottery == choice.buys.pottery);
        assert(exact.second.marble == choice.buys.marble);
        assert(exact.second.silk == choice.buys.silk);
        assert(exact.second.jewelry == choice.buys.jewelry);
        if (choice.minimum_budget < choices.front().minimum_budget) {
            found_lower_budget = true;
        }
    }
    assert(found_lower_budget);
}

void test_run_buy_event_considers_future_route_to_mahaji_value() {
    Player player;
    player.day = 4;
    player.gold = 500;
    player.food = 0;
    player.food_consumption = 0;
    player.backpack = 330;

    BestPurchaseHandler handler;
    std::deque<Player> generated;
    Helpers::run_buy_event(player, generated, handler, true, false);

    bool found_future_spice_basket = false;
    for (const Player& candidate : generated) {
        if (candidate.spice >= 80) {
            found_future_spice_basket = true;
            break;
        }
    }
    Player projected = player;
    assert(Game::event_merchant(projected, m_Route_to_Mahaji));
    const auto projected_buy = handler.getBestBuy(500, 360, projected).second;
    assert(projected_buy.spice >= 80);
    assert(found_future_spice_basket);
}

void test_run_buy_event_considers_future_midas_value() {
    Player player;
    player.day = 8;
    player.gold = 500;
    player.food = 0;
    player.food_consumption = 0;
    player.backpack = 330;

    BestPurchaseHandler handler;
    std::deque<Player> generated;
    Helpers::run_buy_event(player, generated, handler, false, true);

    bool found_midas_safe_basket = false;
    for (const Player& candidate : generated) {
        if (candidate.spice >= 80 && candidate.marble == 0) {
            found_midas_safe_basket = true;
            break;
        }
    }
    assert(found_midas_safe_basket);
}

void test_purchase_profiles_expand_candidates_deliberately() {
    Player player;
    player.day = 4;
    player.gold = 500;
    player.food_consumption = 0;
    player.backpack = 330;

    BestPurchaseHandler handler;
    std::deque<Player> greedy_generated;
    const BuyEventStats greedy = Helpers::run_buy_event(
        player, greedy_generated, handler, true, true, PurchaseProfile::Greedy
    );
    std::deque<Player> scenario_generated;
    const BuyEventStats scenario = Helpers::run_buy_event(
        player, scenario_generated, handler, true, true, PurchaseProfile::Scenario
    );
    std::deque<Player> wide_generated;
    const BuyEventStats wide = Helpers::run_buy_event(
        player, wide_generated, handler, true, true, PurchaseProfile::Wide
    );

    assert(std::string(purchase_profile_name(PurchaseProfile::Strategic)) == "strategic");
    assert(greedy.valuation_worlds == 1);
    assert(scenario.valuation_worlds > greedy.valuation_worlds);
    assert(wide.portfolio_choices > scenario.portfolio_choices);
    assert(wide.unique_buys >= scenario.unique_buys);
}

void test_purchase_tables_stay_compact_in_memory() {
    Player player;
    BestPurchaseHandler handler;
    (void)handler.getBestBuy(500, 60, player);
    assert(handler.loadedTreeMemoryBytes() < 16ULL * 1024ULL * 1024ULL);
}

int brute_force_best(Player current, const bool merch_on, const bool witch_on, BestPurchaseHandler& handler) {
    if (!current.day_start_skip) {
        Game::start_of_day(current);
        if (current.food < 0) {
            return std::numeric_limits<int>::min();
        }

        if (merch_on && !current.merchant_happened &&
            (current.call_merchant || Helpers::day_in_array(TEST_MERCH_DAYS, current.day))) {
            int best = std::numeric_limits<int>::min();
            for (const auto option : ALL_MERCH_OPTIONS) {
                if (!Game::check__afford_merchant(current, option)) {
                    continue;
                }
                Player merch = current;
                if (Game::event_merchant(merch, option)) {
                    best = std::max(best, brute_force_best(merch, merch_on, witch_on, handler));
                }
            }
            if (best != std::numeric_limits<int>::min()) {
                return best;
            }
        }

        if (witch_on && !current.witch_happened && Helpers::day_in_array(TEST_WITCH_DAYS, current.day)) {
            int best = std::numeric_limits<int>::min();
            for (const auto option : ALL_WITCH_OPTIONS) {
                Player witch = current;
                Game::event_witch(witch, option);
                best = std::max(best, brute_force_best(witch, merch_on, witch_on, handler));
            }
            if (best != std::numeric_limits<int>::min()) {
                return best;
            }
        }
    } else {
        current.day_start_skip = false;
    }

    int best = std::numeric_limits<int>::min();
    const int current_food_cost = current.food_consumption;

    if (current.day < current.max_day && !current.bought_last && Game::check_can_buy_items(current)) {
        std::deque<Player> generated;
        Helpers::run_buy_event(current, generated, handler, merch_on, witch_on);
        while (!generated.empty()) {
            best = std::max(best, brute_force_best(generated.front(), merch_on, witch_on, handler));
            generated.pop_front();
        }
    }

    if (current.day == current.max_day || current.food >= current_food_cost) {
        if (Game::check_has_items(current)) {
            Player sell = current;
            Game::event_sell_items(sell);
            if (sell.day == sell.max_day) {
                best = std::max(best, Game::check_points(sell, "Final"));
            } else {
                best = std::max(best, brute_force_best(sell, merch_on, witch_on, handler));
            }
        }

        if (current.gold >= current.npc_shop[n_Trader][nd_Cost] &&
            current.day < current.max_day &&
            current.food >= current_food_cost + current.npc_shop[n_Trader][nd_Eat]) {
            Player trader = current;
            Game::event_buy_npc(trader, n_Trader);
            best = std::max(best, brute_force_best(trader, merch_on, witch_on, handler));
        }

        if (current.gold >= current.npc_shop[n_Camel][nd_Cost] &&
            current.day < current.max_day &&
            current.food >= current_food_cost + current.npc_shop[n_Camel][nd_Eat]) {
            Player camel = current;
            Game::event_buy_npc(camel, n_Camel);
            best = std::max(best, brute_force_best(camel, merch_on, witch_on, handler));
        }

        Player interest = current;
        Game::event_interest(interest);
        if (interest.day == interest.max_day && !interest.day_start_skip) {
            best = std::max(best, Game::check_points(interest, "Final"));
        } else {
            best = std::max(best, brute_force_best(interest, merch_on, witch_on, handler));
        }
    }

    return best;
}

void test_solver_small_horizon_matches_bruteforce() {
    Player state;
    state.day = 12;
    state.max_day = 13;
    state.gold = 200;
    state.food = 10;
    state.trader = 2;
    state.camel = 1;
    state.food_consumption = 3;
    BestPurchaseHandler handler;
    const int brute_force = brute_force_best(state, false, false, handler);

    GameSolver solver(state, false, false, 500000, 1000000, "", 0);
    solver.find_solve_depth(false);

    assert(!solver.best_solves().empty());
    assert(solver.best_points() == brute_force);
    assert(solver.best_solves().size() <= 3);
}

void test_proof_search_matches_small_horizon_exhaustive_oracle() {
    Player state;
    state.day = 12;
    state.max_day = 13;
    state.gold = 90;
    state.food = 5;
    state.trader = 1;
    state.camel = 1;
    state.food_consumption = 2;
    BestPurchaseHandler handler;
    const int brute_force = brute_force_best(state, false, false, handler);

    const SolverState start = SolverState::from_player(state);
    assert(AdmissibleUpperBound::score_ceiling(start) >= brute_force);

    // A beam of one would prune this horizon in normal mode. Proof mode must
    // ignore that lossy setting entirely.
    GameSolver solver(state, false, false, 1, 1000000, "", 64);
    solver.find_solve_proof_depth(4096, false);

    const SolverProfiling& stats = solver.profiling_stats();
    assert(stats.proof_search_completed);
    assert(stats.beam_prunes == 0);
    assert(stats.proof_peak_transposition_states <= 4096);
    assert(solver.best_points() == brute_force);
}

void test_mapped_proof_search_matches_small_horizon_exhaustive_oracle() {
    Player state;
    state.day = 12;
    state.max_day = 13;
    state.gold = 90;
    state.food = 5;
    state.trader = 1;
    state.camel = 1;
    state.food_consumption = 2;
    BestPurchaseHandler handler;
    const int brute_force = brute_force_best(state, false, false, handler);
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path database = std::filesystem::temp_directory_path() /
        ("caravan-mapped-proof-test-" + std::to_string(nonce) + ".sqlite3");

    {
        GameSolver solver(state, false, false, 1, 1000000, "", 64);
        solver.find_solve_mapped(database, 0, PersistentSearchOrder::Depth, false);

        const SolverProfiling& stats = solver.profiling_stats();
        assert(stats.proof_search_completed);
        assert(stats.mapped_nodes > 0);
        assert(stats.mapped_open_nodes == 0);
        assert(solver.best_points() == brute_force);
    }

    std::error_code ignored;
    std::filesystem::remove(database, ignored);
    std::filesystem::remove(database.string() + "-wal", ignored);
    std::filesystem::remove(database.string() + "-shm", ignored);
}

void test_admissible_upper_bound_covers_exhaustive_variants() {
    const std::array<Player, 3> states = [] {
        std::array<Player, 3> cases{};

        cases[0].day = 12;
        cases[0].max_day = 13;
        cases[0].gold = 90;
        cases[0].food = 5;

        cases[1].day = 12;
        cases[1].max_day = 13;
        cases[1].gold = 120;
        cases[1].food = 8;
        cases[1].trader = 2;
        cases[1].camel = 2;
        cases[1].food_consumption = 4;
        cases[1].spice = 2;
        cases[1].pottery = 1;
        cases[1].backpack = 15;
        cases[1].interest_rate = 1.7;

        cases[2].day = 12;
        cases[2].max_day = 13;
        cases[2].gold = 180;
        cases[2].food = 10;
        cases[2].trader = 3;
        cases[2].camel = 1;
        cases[2].food_consumption = 4;
        cases[2].marble = 1;
        cases[2].silk = 1;
        cases[2].trader_interest = true;

        return cases;
    }();

    for (const Player& state : states) {
        BestPurchaseHandler handler;
        const int brute_force = brute_force_best(state, false, false, handler);
        assert(AdmissibleUpperBound::score_ceiling(SolverState::from_player(state)) >=
               brute_force);
    }
}

void test_skeleton_queue_detects_unsafe_heuristic_ceiling_without_pruning() {
    Player state;
    state.day = 11;
    state.max_day = 13;
    state.gold = 60;
    state.food = 9;
    state.trader = 1;
    state.camel = 1;
    state.food_consumption = 2;
    state.backpack = 10;
    BestPurchaseHandler handler;
    const int brute_force = brute_force_best(state, false, false, handler);

    GameSolver solver(state, false, false, 500000, 1000000, "", 0);
    solver.set_skeleton_queue_day(12);
    solver.find_solve_depth(false);

    const SolverProfiling& stats = solver.profiling_stats();
    assert(solver.best_points() == brute_force);
    assert(stats.beam_prunes == 0);
    assert(stats.skeleton_queue_groups > 1);
    assert(stats.skeleton_queue_completed_groups == stats.skeleton_queue_groups);
    assert(stats.skeleton_queue_ceiling_checked_groups > 0);
    assert(stats.skeleton_queue_ceiling_violations > 0);
    assert(stats.skeleton_queue_ceiling_max_overrun > 0);
}

void test_solver_witch_day_matches_bruteforce() {
    Player state;
    state.day = 8;
    state.max_day = 10;
    state.gold = 220;
    state.food = 8;
    state.trader = 1;
    state.camel = 1;
    state.food_consumption = 3;
    state.backpack = 25;
    state.spice = 1;
    state.pottery = 2;
    BestPurchaseHandler handler;
    const int brute_force = brute_force_best(state, false, true, handler);

    GameSolver solver(state, false, true, 500000, 1000000, "", 0);
    solver.find_solve_depth(false);

    assert(!solver.best_solves().empty());
    assert(solver.best_points() == brute_force);
}

void test_solver_respects_array_limit_during_generation() {
    Player state;
    state.max_day = 8;

    constexpr int candidate_limit = 32;
    GameSolver solver(state, false, false, candidate_limit, 1000000, "", 256);
    solver.find_solve_depth(false);

    assert(!solver.best_solves().empty());
    assert(solver.profiling_stats().peak_candidate_states <= static_cast<uint64_t>(candidate_limit * 2));
}

void test_solver_drains_a_forced_hybrid_frontier_in_batches() {
    Player state;
    state.day = 10;
    state.max_day = 12;
    state.gold = 250;
    state.food = 12;
    state.trader = 2;
    state.camel = 1;
    state.food_consumption = 3;

    constexpr size_t batch_size = 1;
    GameSolver solver(state, false, false, 128, 1000000, "", 256);
    solver.set_hybrid_batch_size(batch_size);
    solver.set_hybrid_suffix_limit(4);
    solver.set_hybrid_split_day(11);
    solver.set_hybrid_skeleton_order(HybridSkeletonOrder::StructureInterleave);
    solver.find_solve_depth(false);

    const SolverProfiling& stats = solver.profiling_stats();
    assert(!solver.best_solves().empty());
    assert(stats.hybrid_batches > 1);
    assert(stats.hybrid_frontier_states > batch_size);
    assert(stats.hybrid_split_day == 12);
    assert(stats.hybrid_peak_waiting_states == stats.hybrid_frontier_states);
    assert(stats.hybrid_split_skeleton_groups > 1);

    std::ostringstream json;
    solver.print_benchmark_json(json);
    assert(json.str().find("\"hybrid_batch_size\":1") != std::string::npos);
    assert(json.str().find("\"hybrid_suffix_limit\":4") != std::string::npos);
    assert(json.str().find("\"hybrid_batches\":") != std::string::npos);
    assert(json.str().find("\"hybrid_split_day\":12") != std::string::npos);
    assert(json.str().find("\"hybrid_skeleton_order\":\"structure-interleave\"") != std::string::npos);
}

void test_early_action_skeleton_is_quantity_free_and_event_free() {
    EarlyActionSkeleton base;
    const EarlyActionSkeleton trader_buy = base
        .extended(SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Trader)})
        .extended(SolverAction{_a_merchant, buy_list(), static_cast<uint8_t>(m_Sturdy_Saddle)})
        .extended(SolverAction{_a_buy, buy_list(1, 2, 0, 0, 0, 0), 0});
    const EarlyActionSkeleton same_shape = base
        .extended(SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Trader)})
        .extended(SolverAction{_a_buy, buy_list(40, 0, 0, 0, 0, 0), 0});
    const EarlyActionSkeleton camel_buy = base
        .extended(SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Camel)})
        .extended(SolverAction{_a_buy, buy_list(40, 0, 0, 0, 0, 0), 0});

    assert(trader_buy.key() == same_shape.key());
    assert(trader_buy.key() != camel_buy.key());
    assert(trader_buy.prefix_mismatches(same_shape) == 0);
    assert(trader_buy.prefix_mismatches(camel_buy) == 1);
    assert(trader_buy.action_count() == 2);

    const EarlyActionSkeleton longer = trader_buy
        .extended(SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Camel)});
    assert(longer.key(2) == trader_buy.key());
    assert(longer.key(3) != trader_buy.key());

    Player player;
    const DeferredPurchaseModel purchase = DeferredPurchaseModel::open(
        player,
        {DeferredPurchaseSeed{player.gold, player.food, 0}}
    );
    assert(purchase.extend_skeleton(base).key() ==
           base.extended(SolverAction{_a_buy, buy_list(), 0}).key());
}

void test_continuation_matrix_reserves_independent_action_lanes() {
    std::vector<ContinuationMatrixRow> rows(6);
    for (size_t index = 0; index < rows.size(); ++index) {
        rows[index].baseline_realistic = 1000.0L - static_cast<long double>(index);
        rows[index].baseline_optimistic = rows[index].baseline_realistic;
        rows[index].stable_id = static_cast<uint32_t>(index);
    }
    rows[4].lanes[static_cast<size_t>(ContinuationLane::Trader)] =
        ContinuationCell{true, 20.0L, 20.0L};
    rows[5].lanes[static_cast<size_t>(ContinuationLane::Camel)] =
        ContinuationCell{true, 10.0L, 10.0L};

    const ContinuationMatrixSelection selected =
        ContinuationMatrixSelector::select(rows, 4, 50, 0);
    assert(selected.owner_indices.size() == 4);
    assert(std::find(selected.owner_indices.begin(), selected.owner_indices.end(), 4) !=
           selected.owner_indices.end());
    assert(std::find(selected.owner_indices.begin(), selected.owner_indices.end(), 5) !=
           selected.owner_indices.end());
    assert(selected.lane_owner_counts[static_cast<size_t>(ContinuationLane::Trader)] == 1);
    assert(selected.lane_owner_counts[static_cast<size_t>(ContinuationLane::Camel)] == 1);
}

void test_continuation_matrix_stores_each_owner_once() {
    std::vector<ContinuationMatrixRow> rows(4);
    for (size_t index = 0; index < rows.size(); ++index) {
        rows[index].baseline_realistic = static_cast<long double>(index);
        rows[index].baseline_optimistic = static_cast<long double>(index);
        rows[index].stable_id = static_cast<uint32_t>(index);
    }
    rows[0].lanes[static_cast<size_t>(ContinuationLane::ImmediateSell)] =
        ContinuationCell{true, 500.0L, 500.0L};
    rows[0].lanes[static_cast<size_t>(ContinuationLane::Trader)] =
        ContinuationCell{true, 500.0L, 500.0L};
    rows[1].lanes[static_cast<size_t>(ContinuationLane::ImmediateSell)] =
        ContinuationCell{true, 400.0L, 400.0L};
    rows[2].lanes[static_cast<size_t>(ContinuationLane::Trader)] =
        ContinuationCell{true, 300.0L, 300.0L};

    const ContinuationMatrixSelection selected =
        ContinuationMatrixSelector::select(rows, 3, 100, 0);
    assert(selected.owner_indices.size() == 3);
    std::vector<size_t> unique = selected.owner_indices;
    std::sort(unique.begin(), unique.end());
    assert(std::unique(unique.begin(), unique.end()) == unique.end());
    assert(selected.duplicate_ticket_skips > 0);
}

void test_frontier_reducer_canonicalizes_before_lossy_selection() {
    Player player;
    SolverState first = SolverState::from_player(player, 8);
    first.route_audit_progress = 1;
    SolverState duplicate = first;
    duplicate.trace_id = 3;
    duplicate.route_audit_progress = 4;
    SolverState distinct = first;
    distinct.daily_income += 1;
    distinct.trace_id = 5;
    distinct.route_audit_progress = RouteSurvivalAudit::NO_MATCH;

    ExactFrontierReduction reduced = FrontierReducer::reduce_exact(
        {first, duplicate, distinct}
    );
    assert(reduced.duplicate_prunes == 1);
    assert(reduced.dominance_prunes == 0);
    assert(reduced.states.size() == 2);

    const auto canonical = std::find_if(
        reduced.states.begin(),
        reduced.states.end(),
        [](const SolverState& state) {
            return state.daily_income == 0;
        }
    );
    assert(canonical != reduced.states.end());
    assert(canonical->trace_id == 3);
    assert(canonical->route_audit_progress == 4);
}

void test_frontier_accumulator_is_insertion_order_invariant() {
    Player player;
    player.gold = 100;
    player.food = 5;
    SolverState strongest = SolverState::from_player(player, 9);
    SolverState duplicate = strongest;
    duplicate.trace_id = 3;
    player.gold = 90;
    SolverState dominated = SolverState::from_player(player, 5);
    SolverState distinct = strongest;
    distinct.daily_income = 1;
    distinct.trace_id = 7;
    const std::vector<SolverState> inputs{
        strongest,
        duplicate,
        dominated,
        distinct,
    };

    std::vector<std::tuple<int, int, int, uint32_t>> expected;
    std::vector<size_t> order{0, 1, 2, 3};
    do {
        FrontierAccumulator accumulator;
        for (const size_t index : order) {
            accumulator.insert(inputs[index]);
        }
        ExactFrontierReduction result = accumulator.finish();
        assert(result.duplicate_prunes == 1);
        assert(result.dominance_prunes == 1);
        std::vector<std::tuple<int, int, int, uint32_t>> actual;
        for (const SolverState& state : result.states) {
            actual.emplace_back(
                state.daily_income,
                state.gold,
                state.food,
                state.trace_id
            );
        }
        std::sort(actual.begin(), actual.end());
        if (expected.empty()) {
            expected = actual;
        } else {
            assert(actual == expected);
        }
    } while (std::next_permutation(order.begin(), order.end()));
}

void test_frontier_reducer_transfers_audit_witness_through_safe_dominance() {
    Player player;
    player.gold = 200;
    player.food = 20;
    SolverState stronger = SolverState::from_player(player, 1);
    player.gold = 150;
    SolverState weaker = SolverState::from_player(player, 2);
    weaker.route_audit_progress = 3;
    weaker.route_audit_quantity = RouteAuditQuantityStatus::Exact;

    ExactFrontierReduction reduced = FrontierReducer::reduce_exact(
        {stronger, weaker}
    );
    assert(reduced.duplicate_prunes == 0);
    assert(reduced.dominance_prunes == 1);
    assert(reduced.states.size() == 1);
    assert(reduced.states.front().gold == 200);
    assert(reduced.states.front().route_audit_progress == 3);
    assert(reduced.states.front().route_audit_quantity ==
           RouteAuditQuantityStatus::DominatingReplacement);
}

void test_frontier_reducer_respects_weighted_inventory_dominance() {
    Player player;
    player.gold = 200;
    player.food = 11;
    SolverState more_weighted_food = SolverState::from_player(player, 1);
    player.gold = 150;
    player.food = 10;
    SolverState less_weighted_food = SolverState::from_player(player, 2);

    ExactFrontierReduction weighted = FrontierReducer::reduce_exact(
        {more_weighted_food, less_weighted_food}
    );
    assert(weighted.dominance_prunes == 0);
    assert(weighted.states.size() == 2);

    more_weighted_food.item_shop[i_Food * 3 + id_Weight] = 0;
    less_weighted_food.item_shop[i_Food * 3 + id_Weight] = 0;
    ExactFrontierReduction weightless = FrontierReducer::reduce_exact(
        {more_weighted_food, less_weighted_food}
    );
    assert(weightless.dominance_prunes == 1);
    assert(weightless.states.size() == 1);
    assert(weightless.states.front().food == 11);

    player.gold = 200;
    player.food = 5;
    player.spice = 1;
    SolverState extra_weighted_cargo = SolverState::from_player(player, 3);
    player.gold = 150;
    player.spice = 0;
    SolverState free_capacity = SolverState::from_player(player, 4);
    ExactFrontierReduction cargo = FrontierReducer::reduce_exact(
        {extra_weighted_cargo, free_capacity}
    );
    assert(cargo.dominance_prunes == 0);
    assert(cargo.states.size() == 2);
}

void test_frontier_reducer_dominates_exact_deferred_purchase_envelopes() {
    Player before_buy;
    before_buy.gold = 200;
    before_buy.food = 5;
    const DeferredPurchaseModel stronger_model = DeferredPurchaseModel::open(
        before_buy,
        {DeferredPurchaseSeed{200, 5, 1}}
    );
    const DeferredPurchaseModel weaker_model = DeferredPurchaseModel::open(
        before_buy,
        {DeferredPurchaseSeed{150, 5, 2}}
    );

    SolverState stronger = SolverState::from_player(stronger_model.representative(), 1);
    stronger.deferred = std::make_shared<DeferredSearchState>(stronger_model);
    SolverState weaker = SolverState::from_player(weaker_model.representative(), 2);
    weaker.deferred = std::make_shared<DeferredSearchState>(weaker_model);
    weaker.route_audit_progress = 5;

    assert(!(stronger == weaker));
    ExactFrontierReduction reduced = FrontierReducer::reduce_exact(
        {stronger, weaker}
    );
    assert(reduced.duplicate_prunes == 0);
    assert(reduced.dominance_prunes == 1);
    assert(reduced.deferred_purchase_dominance_prunes == 1);
    assert(reduced.deferred_sold_dominance_prunes == 0);
    assert(reduced.deferred_dominance_checks > 0);
    assert(reduced.deferred_gold_function_comparisons > 0);
    assert(reduced.states.size() == 1);
    assert(reduced.states.front().route_audit_progress == 5);
}

void test_frontier_reducer_keeps_incomparable_deferred_origin_shapes() {
    Player before_buy;
    before_buy.gold = 200;
    before_buy.food = 5;
    const DeferredPurchaseModel more_food = DeferredPurchaseModel::open(
        before_buy,
        {DeferredPurchaseSeed{200, 5, 1}}
    );
    const DeferredPurchaseModel less_food = DeferredPurchaseModel::open(
        before_buy,
        {DeferredPurchaseSeed{150, 4, 2}}
    );
    SolverState first = SolverState::from_player(more_food.representative(), 1);
    first.deferred = std::make_shared<DeferredSearchState>(more_food);
    SolverState second = SolverState::from_player(less_food.representative(), 2);
    second.deferred = std::make_shared<DeferredSearchState>(less_food);

    ExactFrontierReduction reduced = FrontierReducer::reduce_exact({first, second});
    assert(reduced.dominance_prunes == 0);
    assert(reduced.states.size() == 2);
}

void test_frontier_reducer_dominates_exact_deferred_sold_ledgers() {
    Player before_buy;
    before_buy.gold = 200;
    before_buy.food = 5;
    const DeferredPurchaseModel stronger_purchase = DeferredPurchaseModel::open(
        before_buy,
        {DeferredPurchaseSeed{200, 5, 1}}
    );
    const DeferredPurchaseModel weaker_purchase = DeferredPurchaseModel::open(
        before_buy,
        {DeferredPurchaseSeed{150, 5, 2}}
    );
    BestPurchaseHandler handler;
    const auto stronger_sold = DeferredSoldModel::from_purchase(stronger_purchase, handler);
    const auto weaker_sold = DeferredSoldModel::from_purchase(weaker_purchase, handler);
    assert(stronger_sold.has_value());
    assert(weaker_sold.has_value());

    SolverState stronger = SolverState::from_player(stronger_sold->representative(), 1);
    stronger.deferred = std::make_shared<DeferredSearchState>(*stronger_sold);
    SolverState weaker = SolverState::from_player(weaker_sold->representative(), 2);
    weaker.deferred = std::make_shared<DeferredSearchState>(*weaker_sold);
    ExactFrontierReduction reduced = FrontierReducer::reduce_exact({weaker, stronger});
    assert(reduced.dominance_prunes == 1);
    assert(reduced.deferred_purchase_dominance_prunes == 0);
    assert(reduced.deferred_sold_dominance_prunes == 1);
    assert(reduced.states.size() == 1);
}

void test_deferred_gold_dominance_checks_is_pointwise_exact() {
    DeferredGoldFunction stronger = DeferredGoldFunction::open(100, 0, 100);
    DeferredGoldFunction weaker = DeferredGoldFunction::open(90, 0, 90);
    assert(stronger.dominates(weaker));
    assert(!weaker.dominates(stronger));

    DeferredGoldFunction rounded = DeferredGoldFunction::open(100, 0, 100);
    rounded.apply_interest(50);
    DeferredGoldFunction flat = DeferredGoldFunction::open(80, 0, 80);
    uint64_t evaluations = 0;
    assert(!rounded.dominates(flat, &evaluations));
    assert(evaluations > 0);
}

void test_route_survival_audit_is_observational_and_quantity_deferred() {
    const std::vector<SolverAction> route{
        SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Trader)},
        SolverAction{_a_buy, buy_list(12, 0, 0, 0, 0, 0), 0},
        SolverAction{_a_merchant, buy_list(), static_cast<uint8_t>(m_Magic_Cleppsydra)},
        SolverAction{_a_interest, buy_list(), 0},
    };
    RouteSurvivalAudit audit(route);
    int16_t progress = 0;
    progress = audit.advance(
        progress,
        SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Trader)}
    );
    assert(progress == 1);
    progress = audit.advance(progress, SolverAction{_a_buy, buy_list(), 0});
    assert(progress == 2);
    assert(audit.advance_exact(
        1,
        SolverAction{_a_buy, buy_list(11, 0, 0, 0, 0, 0), 0}
    ) == RouteSurvivalAudit::NO_MATCH);
    assert(audit.expected_open_buy(progress) != nullptr);
    assert(audit.expected_open_buy(progress)->food == 12);
    assert(audit.advance(
        progress,
        SolverAction{_a_merchant, buy_list(), static_cast<uint8_t>(m_Blue_Treasure)}
    ) == RouteSurvivalAudit::NO_MATCH);
    progress = audit.advance(
        progress,
        SolverAction{_a_merchant, buy_list(), static_cast<uint8_t>(m_Magic_Cleppsydra)}
    );
    progress = audit.advance(progress, SolverAction{_a_interest, buy_list(), 0});
    assert(progress == 4);

    audit.note_generated(progress, RouteAuditQuantityStatus::Exact);
    audit.note_reduction(
        RouteAuditReductionStage::BeforeExact,
        progress,
        RouteAuditQuantityStatus::Exact
    );
    audit.note_reduction(
        RouteAuditReductionStage::AfterExact,
        progress,
        RouteAuditQuantityStatus::Exact
    );
    audit.note_reduction(
        RouteAuditReductionStage::AfterDominance,
        progress,
        RouteAuditQuantityStatus::DominatingReplacement
    );
    audit.note_reduction(
        RouteAuditReductionStage::AfterBeam,
        progress,
        RouteAuditQuantityStatus::DominatingReplacement
    );
    audit.note_materialized({
        SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Trader)},
        SolverAction{_a_buy, buy_list(12, 0, 0, 0, 0, 0), 0},
        SolverAction{_a_merchant, buy_list(), static_cast<uint8_t>(m_Magic_Cleppsydra)},
        SolverAction{_a_interest, buy_list(), 0},
    });
    audit.note_finished(progress);
    audit.note_ranks(progress, 7, 11, {1, 2, 3, 4, 5});
    assert(audit.highest_generated_prefix() == 4);
    assert(audit.highest_retained_prefix() == 4);
    assert(audit.prefix_stats()[4].best_realistic_rank == 7);
    assert(audit.prefix_stats()[4].quantity_generated.exact == 1);
    assert(audit.prefix_stats()[4].quantity_after_dominance.dominating_replacement == 1);
    assert(audit.prefix_stats()[4].quantity_materialized == 1);

    std::ostringstream json;
    audit.write_json(json);
    assert(json.str().find("\"mode\":\"symbolic-exact-buy\"") != std::string::npos);
    assert(json.str().find("\"quantity_after_beam\"") != std::string::npos);
}

void test_solver_observes_known_route_without_protecting_it() {
    Player state;
    state.day = 11;
    state.max_day = 13;
    state.gold = 200;
    state.food = 20;
    state.food_consumption = 1;
    GameSolver control(state, false, false, 500, 1000000, "", 0);
    control.find_solve_depth(false);

    GameSolver solver(state, false, false, 500, 1000000, "", 0);
    solver.set_route_survival_audit({
        SolverAction{_a_interest, buy_list(), 0},
        SolverAction{_a_interest, buy_list(), 0},
    });
    solver.find_solve_depth(false);

    const RouteSurvivalAudit* audit = solver.route_survival_audit();
    assert(audit != nullptr);
    assert(audit->highest_generated_prefix() == 2);
    assert(audit->highest_retained_prefix() == 2);
    assert(audit->prefix_stats()[2].finished > 0);
    assert(solver.profiling_stats().processed_states ==
           control.profiling_stats().processed_states);
    assert(solver.profiling_stats().generated_states ==
           control.profiling_stats().generated_states);
}

void test_skeleton_queue_groups_and_orders_independent_families() {
    const auto row = [](
        const long double realistic,
        const long double optimistic,
        const uint32_t stable_id
    ) {
        ContinuationMatrixRow value;
        value.baseline_realistic = realistic;
        value.baseline_optimistic = optimistic;
        value.gold = 100;
        value.stable_id = stable_id;
        return value;
    };
    ContinuationMatrixRow realistic_member = row(30.0L, 35.0L, 2);
    ContinuationMatrixRow trader_member = row(15.0L, 16.0L, 3);
    trader_member.lanes[static_cast<size_t>(ContinuationLane::Trader)] =
        ContinuationCell{true, 500.0L, 550.0L};
    ContinuationMatrixRow camel_member = row(14.0L, 15.0L, 4);
    camel_member.lanes[static_cast<size_t>(ContinuationLane::Camel)] =
        ContinuationCell{true, 400.0L, 450.0L};

    const std::vector<SkeletonQueueCandidate> candidates{
        {11, row(10.0L, 20.0L, 0), 40},
        {22, row(20.0L, 1000.0L, 1), 30},
        {11, realistic_member, 20},
        {33, trader_member, 10},
        {44, camel_member, 50},
    };
    const SkeletonQueueSchedule schedule =
        SkeletonWorkQueue::schedule(candidates, 5, 25);

    assert(schedule.groups.size() == 4);
    assert(schedule.groups[0].skeleton_key == 33);
    assert(schedule.groups[0].first_ticket == SkeletonQueueTicket::Realistic);
    assert(schedule.groups[1].skeleton_key == 22);
    assert(schedule.groups[1].first_ticket == SkeletonQueueTicket::Optimistic);
    assert(schedule.groups[2].skeleton_key == 44);
    assert(schedule.groups[2].first_ticket == SkeletonQueueTicket::Camel);
    assert(schedule.groups[3].skeleton_key == 11);
    assert((schedule.groups[3].source_indices == std::vector<size_t>{20, 40}));
    assert(schedule.duplicate_ticket_skips > 0);
}

void test_skeleton_queue_aggregates_each_owner_once_and_is_deterministic() {
    ContinuationMatrixRow first;
    first.baseline_realistic = 100.0L;
    first.baseline_optimistic = 200.0L;
    first.stable_id = 1;
    first.lanes[static_cast<size_t>(ContinuationLane::Trader)] =
        ContinuationCell{true, 300.0L, 350.0L};
    ContinuationMatrixRow second = first;
    second.baseline_realistic = 110.0L;
    second.stable_id = 2;
    second.lanes[static_cast<size_t>(ContinuationLane::Trader)] =
        ContinuationCell{true, 250.0L, 400.0L};
    ContinuationMatrixRow tied = first;
    tied.stable_id = 3;

    const std::vector<SkeletonQueueCandidate> candidates{
        {9, second, 8},
        {7, tied, 6},
        {9, first, 4},
    };
    const SkeletonQueueSchedule schedule =
        SkeletonWorkQueue::schedule(candidates, 5, 25);

    assert(schedule.groups.size() == 2);
    const auto family = std::find_if(
        schedule.groups.begin(),
        schedule.groups.end(),
        [](const SkeletonQueueGroup& group) { return group.skeleton_key == 9; }
    );
    assert(family != schedule.groups.end());
    assert(family->source_indices.size() == 2);
    assert(family->continuation.baseline_realistic == 110.0L);
    const ContinuationCell& trader =
        family->continuation.lanes[static_cast<size_t>(ContinuationLane::Trader)];
    assert(trader.valid);
    assert(trader.realistic == 300.0L);
    assert(trader.optimistic == 400.0L);

    std::vector<SkeletonQueueCandidate> reversed = candidates;
    std::reverse(reversed.begin(), reversed.end());
    const SkeletonQueueSchedule reversed_schedule =
        SkeletonWorkQueue::schedule(reversed, 5, 25);
    assert(reversed_schedule.groups.size() == schedule.groups.size());
    for (size_t index = 0; index < schedule.groups.size(); ++index) {
        assert(reversed_schedule.groups[index].skeleton_key ==
               schedule.groups[index].skeleton_key);
    }

    const SkeletonQueueSchedule realistic_only =
        SkeletonWorkQueue::schedule(candidates, 0, 0);
    assert(realistic_only.head_ticket_counts[
        static_cast<size_t>(SkeletonQueueTicket::Realistic)] == 1);
    for (size_t ticket = static_cast<size_t>(SkeletonQueueTicket::Optimistic);
         ticket < SKELETON_QUEUE_TICKET_COUNT; ++ticket) {
        assert(realistic_only.head_ticket_counts[ticket] == 0);
    }
}

void test_solver_completes_each_skeleton_queue_family() {
    Player state;
    state.day = 5;
    state.max_day = 8;
    state.gold = 250;
    state.food = 20;
    state.trader = 2;
    state.camel = 1;
    state.food_consumption = 3;

    GameSolver solver(state, false, false, 64, 1000000, "", 256);
    solver.set_skeleton_queue_day(6);
    solver.find_solve_depth(false);

    const SolverProfiling& stats = solver.profiling_stats();
    assert(!solver.best_solves().empty());
    assert(stats.skeleton_queue_groups > 1);
    assert(stats.skeleton_queue_states >= stats.skeleton_queue_groups);
    assert(stats.skeleton_queue_completed_groups == stats.skeleton_queue_groups);
    assert(stats.skeleton_queue_split_day >= 6);
}

void test_solver_reserves_early_action_skeletons() {
    Player state;
    state.max_day = 8;

    GameSolver solver(state, false, false, 32, 1000000, "", 256);
    solver.set_skeleton_share_percent(50);
    solver.set_skeleton_depth(6);
    solver.find_solve_depth(false);

    const SolverProfiling& stats = solver.profiling_stats();
    assert(!solver.best_solves().empty());
    assert(stats.skeleton_group_candidates > 0);
    assert(stats.skeleton_reserved_states > 0);
    assert(stats.skeleton_max_groups > 1);

    std::ostringstream json;
    solver.print_benchmark_json(json);
    assert(json.str().find("\"skeleton_share_percent\":50") != std::string::npos);
    assert(json.str().find("\"skeleton_depth\":6") != std::string::npos);
    assert(json.str().find("\"skeleton_reserved_states\":") != std::string::npos);

    GameSolver refined(state, false, false, 32, 1000000, "", 256);
    refined.set_refinement_template({
        SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Trader)},
        SolverAction{_a_buy, buy_list(), 0},
        SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Camel)},
    });
    refined.find_solve_depth(false);
    const SolverProfiling& refined_stats = refined.profiling_stats();
    assert(!refined.best_solves().empty());
    assert(refined_stats.refinement_group_candidates > 0);
    assert(refined_stats.refinement_reserved_states > 0);
    assert(refined_stats.refinement_max_groups > 1);
}

void print_bajaar_point_breakdown() {
    Player player;
    Game::apply_town(player, t_Bajaar);
    for (const ga_data& action : best_bajaar_route()) {
        Game::full_day(player, action);
        std::cout << "  " << player.last_action
                  << " gold=" << player.gold
                  << " food=" << player.food
                  << " traders=" << player.trader
                  << " camels=" << player.camel
                  << " rate=" << std::lround(player.interest_rate * 100.0)
                  << " points=" << player.points
                  << '\n';
    }
    const int cargo_points =
        player.spice * player.item_shop[i_Spice][id_Buy] +
        player.pottery * player.item_shop[i_Pottery][id_Buy] +
        player.marble * player.item_shop[i_Marble][id_Buy] +
        player.silk * player.item_shop[i_Silk][id_Buy] +
        player.jewelry * player.item_shop[i_Jewelry][id_Buy];
    const int trader_points = player.trader * player.npc_shop[n_Trader][nd_Cost];
    const int camel_points = player.camel * player.npc_shop[n_Camel][nd_Cost];
    const int statue_points = player.statue ? (player.trader + player.camel) * 500 : 0;
    std::cout << "Bajaar breakdown: total=" << Game::check_points(player, "Final")
              << " gold=" << player.gold
              << " cargo=" << cargo_points
              << " trader_points=" << trader_points
              << " camel_points=" << camel_points
              << " statue_points=" << statue_points
              << " traders=" << player.trader
              << " camels=" << player.camel
              << " food=" << player.food
              << " day=" << player.day
              << " max_day=" << player.max_day
              << " interest_hundredths=" << std::lround(player.interest_rate * 100.0)
              << '\n';
}

void test_documented_normalia_route_score() {
    const std::vector<ga_data> route = documented_normalia_route();
    Player player;
    Game::apply_town(player, t_Normalia);
    Game::auto_game_loop(player, route);
    assert(Game::check_points(player, "Final") == 34848);
}

void test_cornecopea_deferred_sell_stays_within_memory_budget() {
    const std::vector<ga_data> prefix{ga_data(_a_npc, n_Trader)};
    GameSolver solver(
        t_Cornecopea,
        true,
        true,
        250,
        1000000,
        prefix,
        "",
        512,
        PurchaseProfile::Strategic,
        25,
        false
    );
    solver.find_solve_depth(false);
    assert(!solver.best_solves().empty());
    assert(solver.profiling_stats().peak_candidate_states <= 500);
}

void test_memory_budget_cadence_avoids_near_limit_scan_stall() {
    constexpr uint64_t working_limit = 691ULL * 1024ULL * 1024ULL;
    const MemoryCheckCadence ordinary = memory_check_cadence(
        300ULL * 1024ULL * 1024ULL,
        working_limit
    );
    assert(ordinary.resident_sample_interval == 64);
    assert(ordinary.exact_scan_interval == 4096);

    const MemoryCheckCadence pressured = memory_check_cadence(
        570ULL * 1024ULL * 1024ULL,
        working_limit
    );
    assert(pressured.resident_sample_interval == 16);
    assert(pressured.exact_scan_interval == 4096);

    const MemoryCheckCadence critical = memory_check_cadence(
        630ULL * 1024ULL * 1024ULL,
        working_limit
    );
    assert(critical.resident_sample_interval == 1);
    assert(critical.exact_scan_interval == 4096);

    const MemoryCheckCadence exceeded = memory_check_cadence(
        working_limit + 1,
        working_limit
    );
    assert(exceeded.resident_sample_interval == 1);
    assert(exceeded.exact_scan_interval == 1);
}

void test_solver_reports_seeded_and_discovered_scores_separately() {
    Player state;
    state.day = 12;
    state.max_day = 13;
    state.gold = 200;
    state.food = 10;
    state.trader = 2;
    state.camel = 1;
    state.food_consumption = 3;
    BestPurchaseHandler handler;
    const int discovered_score = brute_force_best(state, false, false, handler);
    const int seeded_score = discovered_score + 1000;

    GameSolver solver(state, false, false, 500000, 1000000, "", 0);
    solver.seed_incumbent(seeded_score, {});
    solver.find_solve_depth(false);

    assert(solver.best_points() == seeded_score);
    assert(solver.incumbent_points() == seeded_score);
    assert(solver.best_discovered_points() == discovered_score);

    std::ostringstream json;
    solver.print_benchmark_json(json);
    assert(json.str().find("\"best_score\":" + std::to_string(seeded_score)) != std::string::npos);
    assert(json.str().find("\"best_source\":\"incumbent\"") != std::string::npos);
    assert(json.str().find("\"search_complete\":true") != std::string::npos);
    assert(json.str().find("\"stop_reason\":null") != std::string::npos);
    assert(json.str().find("\"discovered_score\":" + std::to_string(discovered_score)) != std::string::npos);
    assert(json.str().find("\"incumbent_score\":" + std::to_string(seeded_score)) != std::string::npos);
    assert(json.str().find("\"generated_wait_states\":0") != std::string::npos);
    assert(json.str().find("\"states_per_second\":") != std::string::npos);
    assert(json.str().find("\"memory_budget_checks\":") != std::string::npos);
    assert(json.str().find("\"memory_budget_exact_scans\":") != std::string::npos);
    assert(json.str().find("\"memory_budget_resident_samples\":") != std::string::npos);
    assert(json.str().find("\"memory_budget_scan_ms\":") != std::string::npos);
    assert(json.str().find("\"purchase_food_range_total\":") != std::string::npos);
    assert(json.str().find("\"purchase_total_ms\":") != std::string::npos);
}

void test_improved_normalia_route_score() {
    const std::vector<ga_data> route = improved_normalia_route();
    Player player;
    Game::apply_town(player, t_Normalia);
    Game::auto_game_loop(player, route);
    const int improved_score = Game::check_points(player, "Final");
    assert(improved_score == 35054);
    assert(improved_score > 34848);
}

void test_record_normalia_route_score() {
    const std::vector<ga_data> route = record_normalia_route();
    Player player;
    Game::apply_town(player, t_Normalia);
    Game::auto_game_loop(player, route);
    const int score = Game::check_points(player, "Final");
    assert(score == 37527);
    assert(score > 37478);
}

void test_validated_route_replay_rejects_impossible_actions() {
    const RouteReplayResult record = ValidatedRouteReplay::run(
        t_Normalia,
        record_normalia_route()
    );
    assert(record.legal);
    assert(record.score == 37527);

    std::vector<ga_data> overweight = record_normalia_route();
    overweight[1] = ga_data(_a_buy, 255, 0, 0, 0, 0, 0);
    const RouteReplayResult invalid = ValidatedRouteReplay::run(t_Normalia, overweight);
    assert(!invalid.legal);
    assert(invalid.failed_action == 1);
}

void test_route_polisher_recovers_a_nearby_normalia_route() {
    std::vector<ga_data> weakened = record_normalia_route();
    weakened[15] = ga_data(_a_buy, 20, 0, 0, 0, 0, 8);
    const RouteReplayResult starting = ValidatedRouteReplay::run(t_Normalia, weakened);
    assert(starting.legal);
    assert(starting.score < 37527);

    const RoutePolishResult polished = RoutePolisher::polish(
        t_Normalia,
        {weakened},
        RoutePolishSettings{16, 2, 20000}
    );
    assert(polished.replay.legal);
    assert(polished.replay.score >= 37527);
    assert(polished.evaluated_routes > 1);
}

void test_sanctifan_blue_polish_seed_score() {
    const RouteReplayResult replay = ValidatedRouteReplay::run(
        t_Sanctifan,
        candidate_sanctifan_blue_route()
    );
    assert(replay.legal);
    assert(replay.score == 55445);
}

void test_route_polisher_restores_a_profitable_sale() {
    std::vector<ga_data> weakened = record_normalia_route();
    weakened[15] = ga_data(_a_buy, 20, 0, 0, 0, 0, 0);
    weakened[16] = ga_data(_a_interest);
    const RouteReplayResult starting = ValidatedRouteReplay::run(t_Normalia, weakened);
    assert(starting.legal);
    assert(starting.score < 37527);

    const RoutePolishResult polished = RoutePolisher::polish(
        t_Normalia, {weakened}, RoutePolishSettings{1, 1, 20000}
    );
    assert(polished.replay.legal);
    assert(polished.replay.score >= 37527);
}

void test_route_polisher_reprices_cargo_before_a_late_merchant() {
    std::vector<ga_data> weakened = record_moonin_route();
    weakened[12] = ga_data(_a_buy, 2, 1, 0, 0, 0, 0);
    weakened[15] = ga_data(_a_buy, 20, 0, 0, 0, 0, 1);
    const RouteReplayResult starting = ValidatedRouteReplay::run(t_Moonin, weakened);
    assert(starting.legal);
    assert(starting.score < 37584);

    const RoutePolishResult polished = RoutePolisher::polish(
        t_Moonin, {weakened}, RoutePolishSettings{1, 1, 20000}
    );
    assert(polished.replay.legal);
    assert(polished.replay.score >= 37584);
}

void test_route_polisher_reprices_cargo_across_a_hire() {
    std::vector<ga_data> weakened = record_mehaz_route();
    for (ga_data& action : weakened) {
        if (action.action == _a_buy) {
            action.buy_data = buy_list(action.buy_data.food, 1, 0, 0, 0, 0);
        }
    }
    const RouteReplayResult starting = ValidatedRouteReplay::run(t_Mehaz, weakened);
    assert(starting.legal);
    assert(starting.score < 48738);

    const RoutePolishResult polished = RoutePolisher::polish(
        t_Mehaz, {weakened}, RoutePolishSettings{1, 1, 20000}
    );
    assert(polished.replay.legal);
    assert(polished.replay.score >= 48738);
}

void test_route_polisher_retimes_food_with_a_cargo_change() {
    std::vector<ga_data> weakened = record_normalia_route();
    weakened[15] = ga_data(_a_buy, 21, 0, 0, 0, 0, 8);
    const RouteReplayResult starting = ValidatedRouteReplay::run(t_Normalia, weakened);
    assert(starting.legal);
    assert(starting.score < 37527);

    // Restoring the Jewelry alone still pays for surplus food. A single round
    // must adjust both the cargo and the food schedule to regain the record.
    const RoutePolishResult polished = RoutePolisher::polish(
        t_Normalia, {weakened}, RoutePolishSettings{1, 1, 20000}
    );
    assert(polished.replay.legal);
    assert(polished.replay.score >= 37527);
}

void test_route_polisher_adjusts_days_when_changing_merchant() {
    std::vector<ga_data> weakened = improved_petrinov_route();
    weakened[6] = ga_data(_a_merchant, m_Magic_Cleppsydra);
    weakened.emplace_back(_a_interest);
    const RouteReplayResult starting = ValidatedRouteReplay::run(t_Petrinov, weakened);
    assert(starting.legal);
    assert(starting.score < 45578);

    const RoutePolishResult polished = RoutePolisher::polish(
        t_Petrinov, {weakened}, RoutePolishSettings{1, 1, 20000}
    );
    assert(polished.replay.legal);
    assert(polished.replay.score >= 45578);
}

void test_polished_sanctifan_route_has_two_actions_on_final_day() {
    const RouteReplayResult replay = ValidatedRouteReplay::run(
        t_Sanctifan,
        best_sanctifan_route()
    );
    assert(replay.legal);
    assert(replay.score == 55728);
    assert(replay.action_log.size() >= 2);
    assert(replay.action_log[replay.action_log.size() - 2] == "16: Quick Interest");
    assert(replay.action_log.back() == "16: Interest");
}

void test_record_mahaji_route_score() {
    const std::vector<ga_data> route = record_mahaji_route();
    Player player;
    Game::apply_town(player, t_Mahaji);
    Game::auto_game_loop(player, route);
    const int record_score = Game::check_points(player, "Final");
    assert(record_score == 37589);
    assert(record_score > 35211);
}

void test_fang_mo_starts_with_weighted_pottery_inventory() {
    Player player;
    const int initial_weight_left = Game::check_weight_left(player);
    const int pottery_weight = player.item_shop[i_Pottery][id_Weight];
    const int pottery_sell = player.item_shop[i_Pottery][id_Sell];
    const int end_of_day_income = player.trader * player.npc_shop[n_Trader][nd_Effect];
    const int initial_gold = player.gold;

    Game::apply_town(player, t_Fang_Mo);

    assert(player.pottery == 1);
    assert(Game::check_weight_left(player) == initial_weight_left - pottery_weight);
    assert(Game::check_has_items(player));

    Game::event_sell_items(player);
    assert(player.gold == initial_gold + pottery_sell + end_of_day_income);
    assert(player.pottery == 0);
    assert(!Game::check_has_items(player));
}

void test_record_tocaccialli_route_score() {
    Player player;
    Game::apply_town(player, t_Tocaccialli);
    Game::auto_game_loop(player, record_tocaccialli_route());
    assert(Game::check_points(player, "Final") == 49634);
}

void test_record_moonin_route_score() {
    const std::vector<ga_data> route = record_moonin_route();
    const RouteReplayResult replay = ValidatedRouteReplay::run(t_Moonin, route);
    assert(replay.legal);
    assert(replay.score == 37584);
    assert(replay.action_log[12] == "12: Buy [2, 0, 15, 0, 1, 2]");
    assert(replay.action_log[13] == "Merch - Magic Cleppsydra");

    Player player;
    Game::apply_town(player, t_Moonin);
    Game::auto_game_loop(player, route);
    assert(player.day == 16);
    assert(player.gold == 37309);
    assert(Game::check_points(player, "Final") == 37584);
}

void test_record_vilparino_route_score() {
    Player player;
    Game::apply_town(player, t_Vilparino);
    Game::auto_game_loop(player, record_vilparino_route());
    assert(Game::check_points(player, "Final") == 37569);
}

void test_record_mehaz_route_score() {
    Player player;
    Game::apply_town(player, t_Mehaz);
    Game::auto_game_loop(player, record_mehaz_route());
    assert(Game::check_points(player, "Final") == 48738);
}

void test_record_ipartus_route_score() {
    const std::vector<ga_data> route = record_ipartus_route();
    Player player;
    Game::apply_town(player, t_Ipartus);
    Game::auto_game_loop(player, route);
    const int record_score = Game::check_points(player, "Final");
    assert(record_score == 37734);
    assert(record_score > 36295);
}

void test_record_doba_lao_route_score() {
    const std::vector<ga_data> route = record_doba_lao_route();
    Player player;
    Game::apply_town(player, t_Doba_Lao);
    Game::auto_game_loop(player, route);
    const int record_score = Game::check_points(player, "Final");
    assert(record_score == 45543);
    assert(record_score > 44631);
}

void test_record_cornecopea_route_score() {
    const std::vector<ga_data> route = record_cornecopea_route();
    Player player;
    Game::apply_town(player, t_Cornecopea);
    Game::auto_game_loop(player, route);
    const int record_score = Game::check_points(player, "Final");
    assert(record_score == 43881);
    assert(record_score > 40281);
}

void test_huangoz_kifuai_and_skjollird_record_routes() {
    const auto verify = [](const town_options town, const std::vector<ga_data>& route, const int expected) {
        Player player;
        Game::apply_town(player, town);
        Game::auto_game_loop(player, route);
        assert(Game::check_points(player, "Final") == expected);
    };

    const RouteReplayResult fang_mo = ValidatedRouteReplay::run(t_Fang_Mo, record_fang_mo_route());
    assert(fang_mo.legal);
    assert(fang_mo.score == 37569);
    verify(t_Huangoz, record_huangoz_route(), 37527);
    verify(t_Kifuai, record_kifuai_route(), 37547);
    verify(t_Skjollird, record_skjollird_route(), 47065);
}

void test_improved_petrinov_route_score() {
    const std::vector<ga_data> route = improved_petrinov_route();
    Player player;
    Game::apply_town(player, t_Petrinov);
    Game::auto_game_loop(player, route);
    const int score = Game::check_points(player, "Final");
    assert(score == 45578);
    assert(score > 44447);
}

void test_open_town_bot_floor_routes() {
    const auto verify = [](const town_options town, const std::vector<ga_data>& route, const int expected) {
        Player player;
        Game::apply_town(player, town);
        Game::auto_game_loop(player, route);
        assert(Game::check_points(player, "Final") == expected);
    };

    verify(t_Bajaar, best_bajaar_route(), 56147);
    verify(t_Port_Willam, best_port_willam_route(), 42296);
    verify(t_Gayawaku, best_gayawaku_route(), 50105);
    verify(t_Sanctifan, best_sanctifan_route(), 55728);
}

void test_sell_aware_record_route_scores() {
    const auto verify = [](const town_options town, const std::vector<ga_data>& route, const int expected, const int previous) {
        Player player;
        Game::apply_town(player, town);
        Game::auto_game_loop(player, route);
        const int score = Game::check_points(player, "Final");
        assert(score == expected);
        assert(score > previous);
    };

    verify(t_L_Exquise, record_l_exquise_route(), 56817, 56140);
    verify(t_Eduming, record_eduming_route(), 81321, 80043);
    verify(t_Octoyashi, record_octoyashi_route(), 40056, 37983);
    verify(t_Jilliqo, record_jilliqo_route(), 37819, 37291);
}

int explicit_interest_gold(const int gold, const int interest_hundredths) {
    Player player;
    player.gold = gold;
    player.trader = 0;
    player.daily_income = 0;
    player.interest_rate = static_cast<double>(interest_hundredths) / 100.0;
    Game::event_interest(player);
    return player.gold;
}

void test_deferred_gold_function_matches_game_rounding() {
    for (int interest_hundredths = 0; interest_hundredths <= 300; interest_hundredths += 5) {
        for (int gold = 0; gold <= 300; ++gold) {
            DeferredGoldFunction function = DeferredGoldFunction::open(gold, 0, 0);
            function.apply_interest(interest_hundredths);
            assert(function.evaluate(0) == explicit_interest_gold(gold, interest_hundredths));
        }
    }
}

void test_deferred_gold_function_affordability_is_exact() {
    const int rates[] = {0, 55, 100, 135, 150, 280};
    for (int gold_before_buy = 0; gold_before_buy <= 80; ++gold_before_buy) {
        for (const int buy_income : {0, 7}) {
            for (const int first_rate : rates) {
                for (const int cost : {0, 1, 11, 29, 61}) {
                    const int maximum_spend = gold_before_buy;
                    DeferredGoldFunction function = DeferredGoldFunction::open(
                        gold_before_buy,
                        buy_income,
                        maximum_spend
                    );
                    function.add_gold(3);
                    function.apply_interest(first_rate);
                    function.add_gold(5);

                    std::vector<int> explicit_gold(static_cast<size_t>(maximum_spend) + 1);
                    int expected_maximum = -1;
                    for (int spend = 0; spend <= maximum_spend; ++spend) {
                        int current = gold_before_buy - spend + buy_income + 3;
                        current = explicit_interest_gold(current, first_rate);
                        current += 5;
                        explicit_gold[spend] = current;
                        if (current >= cost) {
                            expected_maximum = spend;
                        }
                    }

                    const bool affordable = function.spend_if_affordable(cost);
                    assert(affordable == (expected_maximum >= 0));
                    if (!affordable) {
                        continue;
                    }
                    assert(function.maximum_original_spend() == expected_maximum);
                    for (int spend = 0; spend <= expected_maximum; ++spend) {
                        assert(function.evaluate(spend) == explicit_gold[spend] - cost);
                    }

                    function.apply_interest(55);
                    function.add_gold(9);
                    for (int spend = 0; spend <= expected_maximum; ++spend) {
                        const int expected = explicit_interest_gold(explicit_gold[spend] - cost, 55) + 9;
                        assert(function.evaluate(spend) == expected);
                    }
                }
            }
        }
    }
}

void test_deferred_gold_function_compacts_additions() {
    DeferredGoldFunction function = DeferredGoldFunction::open(20, 5, 20);
    function.add_gold(4);
    function.add_gold(-1);
    assert(function.operation_count() == 1);
    function.add_gold(-3);
    assert(function.operation_count() == 0);
    function.apply_interest(100);
    assert(function.operation_count() == 0);
    assert(function.evaluate(7) == 18);
}

void test_deferred_trace_recipe_patches_buy_before_later_actions() {
    DeferredTraceRecipe recipe;
    recipe.append(SolverAction{_a_interest, buy_list(), 0});
    recipe.append(SolverAction{_a_npc, buy_list(), static_cast<uint8_t>(n_Camel)});
    const buy_list concrete_buy(4, 2, 0, 0, 0, 1);
    const SolverAction sell{_a_sell, buy_list(), 0};
    const std::vector<SolverAction> resolved = recipe.resolve(concrete_buy, &sell);
    assert(resolved.size() == 4);
    assert(resolved[0].action == _a_buy);
    assert(resolved[0].buy.food == 4);
    assert(resolved[0].buy.spice == 2);
    assert(resolved[0].buy.jewelry == 1);
    assert(resolved[1].action == _a_interest);
    assert(resolved[2].action == _a_npc);
    assert(resolved[2].option == static_cast<uint8_t>(n_Camel));
    assert(resolved[3].action == _a_sell);
    assert(recipe.size() == 2);
}
}

int main(const int argc, char** argv) {
    if (!install_process_safety_limits(768, 120)) {
        std::cerr << "[!] Could not install native test safety limits; refusing to run\n";
        return 2;
    }
    assert(process_memory_limit_bytes() > 0);
    assert(process_memory_limit_bytes() <= 768ULL * 1024ULL * 1024ULL);
    if (argc == 2 && std::string(argv[1]) == "encounter-deferral") {
        test_unfixed_encounters_can_defer_until_the_last_eligible_day();
        std::cout << "Encounter-deferral timing regression passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "future-value") {
        test_run_buy_event_considers_future_route_to_mahaji_value();
        std::cout << "Future-value purchase regression passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "array-limit") {
        test_solver_respects_array_limit_during_generation();
        std::cout << "Array-limit regression passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "proof-bound") {
        test_proof_search_matches_small_horizon_exhaustive_oracle();
        test_mapped_proof_search_matches_small_horizon_exhaustive_oracle();
        test_admissible_upper_bound_covers_exhaustive_variants();
        std::cout << "Proof-search exhaustive regressions passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "hybrid-search") {
        test_solver_drains_a_forced_hybrid_frontier_in_batches();
        std::cout << "Hybrid-search regression passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "skeleton-search") {
        test_early_action_skeleton_is_quantity_free_and_event_free();
        test_continuation_matrix_reserves_independent_action_lanes();
        test_continuation_matrix_stores_each_owner_once();
        test_skeleton_queue_groups_and_orders_independent_families();
        test_skeleton_queue_aggregates_each_owner_once_and_is_deterministic();
        test_solver_completes_each_skeleton_queue_family();
        test_skeleton_queue_detects_unsafe_heuristic_ceiling_without_pruning();
        test_solver_reserves_early_action_skeletons();
        std::cout << "Skeleton-search regressions passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "frontier-reduction") {
        test_frontier_reducer_canonicalizes_before_lossy_selection();
        test_frontier_accumulator_is_insertion_order_invariant();
        test_frontier_reducer_transfers_audit_witness_through_safe_dominance();
        test_frontier_reducer_respects_weighted_inventory_dominance();
        test_frontier_reducer_dominates_exact_deferred_purchase_envelopes();
        test_frontier_reducer_keeps_incomparable_deferred_origin_shapes();
        test_frontier_reducer_dominates_exact_deferred_sold_ledgers();
        test_deferred_gold_dominance_checks_is_pointwise_exact();
        test_route_survival_audit_is_observational_and_quantity_deferred();
        test_solver_observes_known_route_without_protecting_it();
        std::cout << "Frontier reduction and route-survival audit regressions passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "bajaar-breakdown") {
        print_bajaar_point_breakdown();
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "known-route") {
        test_documented_normalia_route_score();
        std::cout << "Known-route benchmark passed: 34848\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "cornecopea-memory") {
        test_cornecopea_deferred_sell_stays_within_memory_budget();
        std::cout << "Cornecopea deferred Sell memory regression passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "memory-cadence") {
        test_memory_budget_cadence_avoids_near_limit_scan_stall();
        std::cout << "Memory budget cadence regression passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "best-route") {
        test_record_normalia_route_score();
        std::cout << "Best-route benchmark passed: 37527\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "route-polisher") {
        test_validated_route_replay_rejects_impossible_actions();
        test_route_polisher_recovers_a_nearby_normalia_route();
        test_route_polisher_restores_a_profitable_sale();
        test_route_polisher_reprices_cargo_before_a_late_merchant();
        test_route_polisher_reprices_cargo_across_a_hire();
        test_route_polisher_retimes_food_with_a_cargo_change();
        test_route_polisher_adjusts_days_when_changing_merchant();
        test_sanctifan_blue_polish_seed_score();
        test_polished_sanctifan_route_has_two_actions_on_final_day();
        std::cout << "Validated route replay and polisher regressions passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "mahaji-record") {
        test_record_mahaji_route_score();
        std::cout << "Mahaji record benchmark passed: 37589\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "fang-mo-inventory") {
        test_fang_mo_starts_with_weighted_pottery_inventory();
        std::cout << "Fang-Mo weighted starting pottery regression passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "tocaccialli-record") {
        test_record_tocaccialli_route_score();
        std::cout << "Tocaccialli record benchmark passed: 49634\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "moonin-record") {
        test_record_moonin_route_score();
        std::cout << "Moonin record benchmark passed: 37584\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "vilparino-record") {
        test_record_vilparino_route_score();
        std::cout << "Vilparino record benchmark passed: 37569\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "mehaz-record") {
        test_record_mehaz_route_score();
        std::cout << "Mehaz record benchmark passed: 48738\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "ipartus-record") {
        test_record_ipartus_route_score();
        std::cout << "Ipartus record benchmark passed: 37734\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "doba-record") {
        test_record_doba_lao_route_score();
        std::cout << "Doba Lao record benchmark passed: 45543\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "cornecopea-record") {
        test_record_cornecopea_route_score();
        std::cout << "Cornecopea record benchmark passed: 43881\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "new-town-records") {
        test_huangoz_kifuai_and_skjollird_record_routes();
        std::cout << "Fang-Mo, Huangoz, Kifuai, and Skjollird record benchmarks passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "petrinov-improved") {
        test_improved_petrinov_route_score();
        std::cout << "Petrinov improved benchmark passed: 45578\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "open-town-bot-floors") {
        test_open_town_bot_floor_routes();
        std::cout << "Open-town bot-floor benchmarks passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "deferred-gold") {
        test_deferred_gold_function_matches_game_rounding();
        test_deferred_gold_function_affordability_is_exact();
        test_deferred_gold_function_compacts_additions();
        std::cout << "Deferred gold function regressions passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "deferred-purchase-oracle") {
        run_deferred_purchase_oracle_tests();
        std::cout << "Deferred purchase oracle regressions passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "exact-sale-frontier") {
        test_fenwick_exposes_every_cost_breakpoint();
        test_exact_sale_frontier_separates_buy_and_sell_markets();
        std::cout << "Exact sale frontier regressions passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "deferred-trace") {
        test_deferred_trace_recipe_patches_buy_before_later_actions();
        std::cout << "Deferred trace recipe regressions passed\n";
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "deferred-purchase-model") {
        run_deferred_purchase_model_tests();
        std::cout << "Deferred purchase model regressions passed\n";
        return 0;
    }

    test_sturdy_saddle();
    test_persistent_codec_preserves_sold_allocation_order();
    test_persistent_graph_resumes_only_unexplored_descendants();
    test_persistent_graph_allocates_trace_ids_across_writers();
    test_animal_lover();
    test_exact_merchant_mutations();
    test_exact_witch_mutations();
    test_all_disaster_mutations();
    test_merchant_witch_disaster_interactions();
    test_seer_is_a_run_state_delta();
    test_solver_state_preserves_disaster_values();
    test_traveler_rate_precision_and_preparation();
    test_solver_uses_traveler_encounter_days();
    test_classical_encounter_day_validation();
    test_solver_can_defer_callable_merchant();
    test_solver_applies_a_fixed_disaster_scenario();
    test_purchase_cache_separates_price_disasters();
    test_interest_rounding();
    test_fenwick_matches_bruteforce();
    test_fenwick_exposes_every_cost_breakpoint();
    test_exact_sale_frontier_separates_buy_and_sell_markets();
    test_fenwick_serialize_round_trip();
    test_purchase_portfolio_preserves_reserve_rich_near_optima();
    test_run_buy_event_with_zero_food_consumption();
    test_wait_matches_an_empty_buy_and_is_labeled_separately();
    test_cargo_buy_lock_is_cleared_only_by_sell();
    test_solver_does_not_generate_wait_while_cargo_buy_is_locked();
    test_run_buy_event_keeps_gold_for_next_turn_costs();
    test_run_buy_event_considers_future_route_to_mahaji_value();
    test_run_buy_event_considers_future_midas_value();
    test_purchase_profiles_expand_candidates_deliberately();
    test_purchase_tables_stay_compact_in_memory();
    test_solver_small_horizon_matches_bruteforce();
    test_proof_search_matches_small_horizon_exhaustive_oracle();
    test_mapped_proof_search_matches_small_horizon_exhaustive_oracle();
    test_admissible_upper_bound_covers_exhaustive_variants();
    test_solver_witch_day_matches_bruteforce();
    test_solver_reports_seeded_and_discovered_scores_separately();
    test_solver_respects_array_limit_during_generation();
    test_solver_drains_a_forced_hybrid_frontier_in_batches();
    test_early_action_skeleton_is_quantity_free_and_event_free();
    test_continuation_matrix_reserves_independent_action_lanes();
    test_continuation_matrix_stores_each_owner_once();
    test_frontier_reducer_canonicalizes_before_lossy_selection();
    test_frontier_accumulator_is_insertion_order_invariant();
    test_frontier_reducer_transfers_audit_witness_through_safe_dominance();
    test_frontier_reducer_respects_weighted_inventory_dominance();
    test_frontier_reducer_dominates_exact_deferred_purchase_envelopes();
    test_frontier_reducer_keeps_incomparable_deferred_origin_shapes();
    test_frontier_reducer_dominates_exact_deferred_sold_ledgers();
    test_deferred_gold_dominance_checks_is_pointwise_exact();
    test_route_survival_audit_is_observational_and_quantity_deferred();
    test_solver_observes_known_route_without_protecting_it();
    test_skeleton_queue_groups_and_orders_independent_families();
    test_skeleton_queue_aggregates_each_owner_once_and_is_deterministic();
    test_solver_completes_each_skeleton_queue_family();
    test_skeleton_queue_detects_unsafe_heuristic_ceiling_without_pruning();
    test_solver_reserves_early_action_skeletons();
    test_cornecopea_deferred_sell_stays_within_memory_budget();
    test_memory_budget_cadence_avoids_near_limit_scan_stall();
    test_documented_normalia_route_score();
    test_improved_normalia_route_score();
    test_record_normalia_route_score();
    test_validated_route_replay_rejects_impossible_actions();
    test_route_polisher_recovers_a_nearby_normalia_route();
    test_route_polisher_restores_a_profitable_sale();
    test_route_polisher_reprices_cargo_before_a_late_merchant();
    test_route_polisher_reprices_cargo_across_a_hire();
    test_route_polisher_retimes_food_with_a_cargo_change();
    test_route_polisher_adjusts_days_when_changing_merchant();
    test_sanctifan_blue_polish_seed_score();
    test_polished_sanctifan_route_has_two_actions_on_final_day();
    test_record_mahaji_route_score();
    test_fang_mo_starts_with_weighted_pottery_inventory();
    test_record_tocaccialli_route_score();
    test_record_moonin_route_score();
    test_record_vilparino_route_score();
    test_record_mehaz_route_score();
    test_record_ipartus_route_score();
    test_record_doba_lao_route_score();
    test_record_cornecopea_route_score();
    test_huangoz_kifuai_and_skjollird_record_routes();
    test_improved_petrinov_route_score();
    test_open_town_bot_floor_routes();
    test_sell_aware_record_route_scores();
    test_deferred_gold_function_matches_game_rounding();
    test_deferred_gold_function_affordability_is_exact();
    test_deferred_gold_function_compacts_additions();
    test_deferred_trace_recipe_patches_buy_before_later_actions();
    run_deferred_purchase_oracle_tests();
    run_deferred_purchase_model_tests();
    std::cout << "All tests passed\n";
    return 0;
}
