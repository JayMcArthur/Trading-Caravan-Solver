#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "benchmarks/NormaliaBenchmark.h"
#include "benchmarks/RecordRoutes.h"
#include "consts/enums.h"
#include "game/Game.h"
#include "purchaseHandler/BestPurchaseHandler.h"
#include "player/player.h"
#include "solvers/GameSolver.h"
#include "solvers/RoutePolisher.h"
#include "runtime/ProcessSafety.h"

namespace {
std::string normalized_name(std::string value) {
    value.erase(std::remove_if(value.begin(), value.end(), [](const unsigned char character) {
        return !std::isalnum(character);
    }), value.end());
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

std::optional<town_options> parse_town(const std::string& value) {
    if (!value.empty() && value.size() <= 2 && std::all_of(value.begin(), value.end(), [](const unsigned char character) {
        return std::isdigit(character);
    })) {
        const int town_id = std::stoi(value);
        if (town_id >= t_Normalia && town_id <= t_Ipartus) {
            return static_cast<town_options>(town_id);
        }
    }

    const std::string wanted = normalized_name(value);
    for (const auto& [town, name] : to_conversion) {
        if (normalized_name(name) == wanted) {
            return town;
        }
    }
    return std::nullopt;
}

std::optional<PurchaseProfile> parse_purchase_profile(const std::string& value) {
    const std::string wanted = normalized_name(value);
    if (wanted == "greedy") {
        return PurchaseProfile::Greedy;
    }
    if (wanted == "scenario") {
        return PurchaseProfile::Scenario;
    }
    if (wanted == "strategic") {
        return PurchaseProfile::Strategic;
    }
    if (wanted == "wide") {
        return PurchaseProfile::Wide;
    }
    return std::nullopt;
}

std::optional<merch_options> parse_merchant_option(const std::string& value) {
    const std::string wanted = normalized_name(value);
    for (const auto& [option, name] : mo_conversion) {
        if (normalized_name(name) == wanted) {
            return option;
        }
    }
    return std::nullopt;
}

std::optional<witch_options> parse_witch_option(const std::string& value) {
    const std::string wanted = normalized_name(value);
    for (const auto& [option, name] : wo_conversion) {
        if (normalized_name(name) == wanted) {
            return option;
        }
    }
    return std::nullopt;
}

std::vector<SolverAction> solver_actions_from_route(const std::vector<ga_data>& route, const size_t skip_actions) {
    std::vector<SolverAction> actions;
    actions.reserve(route.size() - std::min(skip_actions, route.size()));
    for (size_t index = skip_actions; index < route.size(); ++index) {
        const ga_data& event = route[index];
        SolverAction action;
        action.action = event.action;
        switch (event.action) {
            case _a_buy:
                action.buy = event.buy_data;
                break;
            case _a_npc:
                action.option = static_cast<uint8_t>(event.npc_data);
                break;
            case _a_merchant:
                action.option = static_cast<uint8_t>(event.merch_data);
                break;
            case _a_witch:
                action.option = static_cast<uint8_t>(event.witch_data);
                break;
            case _a_disaster:
                action.option = static_cast<uint8_t>(event.disaster_data);
                break;
            case _a_sell:
            case _a_interest:
            case _a_wait:
                break;
        }
        actions.emplace_back(action);
    }
    return actions;
}

bool route_respects_cargo_buy_rule(const std::vector<ga_data>& route) {
    bool cargo_buy_locked = false;
    for (const ga_data& event : route) {
        if (event.action == _a_sell) {
            cargo_buy_locked = false;
            continue;
        }
        if (event.action != _a_buy) {
            continue;
        }
        if (cargo_buy_locked) {
            return false;
        }
        const buy_list& buy = event.buy_data;
        if (buy.spice > 0 || buy.pottery > 0 || buy.marble > 0 || buy.silk > 0 || buy.jewelry > 0) {
            cargo_buy_locked = true;
        }
    }
    return true;
}

std::vector<ga_data> verified_incumbent_route(const town_options town) {
    switch (town) {
        case t_Normalia:
            return record_normalia_route();
        case t_Fang_Mo:
            return record_fang_mo_route();
        case t_Mahaji:
            return record_mahaji_route();
        case t_Tocaccialli:
            return record_tocaccialli_route();
        case t_Moonin:
            return record_moonin_route();
        case t_Vilparino:
            return record_vilparino_route();
        case t_Mehaz:
            return record_mehaz_route();
        case t_L_Exquise:
            return record_l_exquise_route();
        case t_Doba_Lao:
            return record_doba_lao_route();
        case t_Cornecopea:
            return record_cornecopea_route();
        case t_Eduming:
            return record_eduming_route();
        case t_Octoyashi:
            return record_octoyashi_route();
        case t_Petrinov:
            return improved_petrinov_route();
        case t_Jilliqo:
            return record_jilliqo_route();
        case t_Ipartus:
            return record_ipartus_route();
        case t_Huangoz:
            return record_huangoz_route();
        case t_Kifuai:
            return record_kifuai_route();
        case t_Skjollird:
            return record_skjollird_route();
        case t_Bajaar:
            return best_bajaar_route();
        case t_Port_Willam:
            return best_port_willam_route();
        case t_Gayawaku:
            return best_gayawaku_route();
        case t_Sanctifan:
            return best_sanctifan_route();
        default:
            return {};
    }
}

std::pair<int, int> incumbent_encounter_days(
    const town_options town,
    const std::vector<ga_data>& route
) {
    Player player;
    Game::apply_town(player, town);
    std::pair<int, int> days{0, 0};
    for (const ga_data& action : route) {
        Game::full_day(player, action);
        if (action.action == _a_merchant) {
            days.first = player.day;
        } else if (action.action == _a_witch) {
            days.second = player.day;
        }
    }
    return days;
}

std::vector<std::vector<ga_data>> polish_seed_routes(const town_options town) {
    std::vector<std::vector<ga_data>> seeds;
    const std::vector<ga_data> incumbent = verified_incumbent_route(town);
    if (!incumbent.empty()) {
        seeds.push_back(incumbent);
    }
    if (town == t_Normalia) {
        seeds.push_back(documented_normalia_route());
        seeds.push_back(improved_normalia_route());
    } else if (town == t_Sanctifan) {
        seeds.push_back(candidate_sanctifan_blue_route());
    }
    return seeds;
}
}


int main(int argc, char** argv) {
    if (!install_process_safety_limits(1024)) {
        std::cerr << "[!] Could not install native process safety limits; refusing to run\n";
        return 2;
    }
    int array_limit = 5000;
    int print_counter = 5000000;
    uint64_t memory_limit_mb = 0;
    uint64_t time_limit_seconds = 0;
    size_t hybrid_batch_size = 0;
    size_t hybrid_suffix_limit = 0;
    int hybrid_split_day = 0;
    HybridSkeletonOrder hybrid_skeleton_order = HybridSkeletonOrder::Score;
    int merchant_day = 0;
    int witch_day = 0;
    std::optional<merch_options> forced_merchant_option;
    std::optional<witch_options> forced_witch_option;
    bool warm_trees = false;
    bool benchmark_json = false;
    bool seed_incumbent = true;
    bool starting_trader = false;
    bool merch_on = true;
    bool witch_on = true;
    bool sell_action_aware_beam = false;
    bool auto_refine = true;
    bool audit_known_route = false;
    bool proof_search = false;
    size_t proof_transposition_limit = 250000;
    std::filesystem::path search_map;
    size_t search_node_budget = 0;
    PersistentSearchOrder search_order = PersistentSearchOrder::Depth;
    int incumbent_prefix_actions = 0;
    PurchaseProfile purchase_profile = PurchaseProfile::Strategic;
    uint8_t optimistic_share_percent = 25;
    uint8_t diversity_share_percent = 0;
    uint8_t skeleton_share_percent = 0;
    uint8_t skeleton_depth = 8;
    uint8_t continuation_lane_share_percent = 5;
    int skeleton_queue_day = 0;
    uint8_t refinement_share_percent = 50;
    uint8_t refinement_maximum_mismatches = 1;
    bool polish_record = false;
    size_t polish_beam = 64;
    size_t polish_rounds = 10;
    size_t polish_evaluations = 250000;
    town_options town = t_Normalia;

    for (int index = 1; index < argc; ++index) {
        const std::string command = argv[index];
        if (command == "--prebuild-trees") {
            BestPurchaseHandler handler;
            handler.prebuildSupportedTrees();
            std::cout << "[*] Purchase trees ready in Saved_Trees\n";
            return 0;
        }
        if (command == "--verify-trees") {
            uint16_t max_gold = 250;
            uint16_t max_weight = 60;
            if (index + 1 < argc) {
                max_gold = static_cast<uint16_t>(std::stoi(argv[index + 1]));
            }
            if (index + 2 < argc) {
                max_weight = static_cast<uint16_t>(std::stoi(argv[index + 2]));
            }

            BestPurchaseHandler handler;
            const bool ok = handler.verifySupportedTrees(max_gold, max_weight, std::cout);
            std::cout << (ok ? "[*] Verification passed\n" : "[!] Verification failed\n");
            return ok ? 0 : 1;
        }
        if (command == "--array-limit" && index + 1 < argc) {
            array_limit = std::stoi(argv[++index]);
            continue;
        }
        if (command == "--print-counter" && index + 1 < argc) {
            print_counter = std::stoi(argv[++index]);
            continue;
        }
        if (command == "--memory-limit-mb" && index + 1 < argc) {
            memory_limit_mb = static_cast<uint64_t>(std::stoull(argv[++index]));
            continue;
        }
        if (command == "--time-limit-seconds" && index + 1 < argc) {
            time_limit_seconds = static_cast<uint64_t>(std::stoull(argv[++index]));
            continue;
        }
        if (command == "--hybrid-batch-size" && index + 1 < argc) {
            hybrid_batch_size = static_cast<size_t>(std::stoull(argv[++index]));
            if (hybrid_batch_size == 0) {
                std::cerr << "[!] --hybrid-batch-size must be greater than zero\n";
                return 2;
            }
            continue;
        }
        if (command == "--hybrid-suffix-limit" && index + 1 < argc) {
            hybrid_suffix_limit = static_cast<size_t>(std::stoull(argv[++index]));
            if (hybrid_suffix_limit == 0) {
                std::cerr << "[!] --hybrid-suffix-limit must be greater than zero\n";
                return 2;
            }
            continue;
        }
        if (command == "--hybrid-split-day" && index + 1 < argc) {
            hybrid_split_day = std::stoi(argv[++index]);
            if (hybrid_split_day <= 0) {
                std::cerr << "[!] --hybrid-split-day must be greater than zero\n";
                return 2;
            }
            continue;
        }
        if (command == "--hybrid-skeleton-order" && index + 1 < argc) {
            const std::string value = argv[++index];
            if (value == "score") {
                hybrid_skeleton_order = HybridSkeletonOrder::Score;
            } else if (value == "cohort") {
                hybrid_skeleton_order = HybridSkeletonOrder::Cohort;
            } else if (value == "interleave") {
                hybrid_skeleton_order = HybridSkeletonOrder::Interleave;
            } else if (value == "structure-interleave") {
                hybrid_skeleton_order = HybridSkeletonOrder::StructureInterleave;
            } else {
                std::cerr << "[!] --hybrid-skeleton-order must be score, cohort, interleave, or structure-interleave\n";
                return 2;
            }
            continue;
        }
        if (command == "--no-warm-trees") {
            warm_trees = false;
            continue;
        }
        if (command == "--warm-trees") {
            warm_trees = true;
            continue;
        }
        if (command == "--optimistic-share" && index + 1 < argc) {
            const int value = std::stoi(argv[++index]);
            if (value < 0 || value > 100) {
                std::cerr << "[!] --optimistic-share must be between 0 and 100\n";
                return 2;
            }
            optimistic_share_percent = static_cast<uint8_t>(value);
            continue;
        }
        if (command == "--diversity-share" && index + 1 < argc) {
            const int value = std::stoi(argv[++index]);
            if (value < 0 || value > 100) {
                std::cerr << "[!] --diversity-share must be between 0 and 100\n";
                return 2;
            }
            diversity_share_percent = static_cast<uint8_t>(value);
            continue;
        }
        if (command == "--skeleton-share" && index + 1 < argc) {
            const int value = std::stoi(argv[++index]);
            if (value < 0 || value > 100) {
                std::cerr << "[!] --skeleton-share must be between 0 and 100\n";
                return 2;
            }
            skeleton_share_percent = static_cast<uint8_t>(value);
            continue;
        }
        if (command == "--skeleton-depth" && index + 1 < argc) {
            const int value = std::stoi(argv[++index]);
            if (value < 1 || value > 8) {
                std::cerr << "[!] --skeleton-depth must be between 1 and 8\n";
                return 2;
            }
            skeleton_depth = static_cast<uint8_t>(value);
            continue;
        }
        if (command == "--continuation-lane-share" && index + 1 < argc) {
            const int value = std::stoi(argv[++index]);
            if (value < 0 || value > 100) {
                std::cerr << "[!] --continuation-lane-share must be between 0 and 100\n";
                return 2;
            }
            continuation_lane_share_percent = static_cast<uint8_t>(value);
            continue;
        }
        if (command == "--skeleton-queue-day" && index + 1 < argc) {
            skeleton_queue_day = std::stoi(argv[++index]);
            if (skeleton_queue_day < 0 || skeleton_queue_day > 255) {
                std::cerr << "[!] --skeleton-queue-day must be between 0 and 255\n";
                return 2;
            }
            continue;
        }
        if (command == "--no-skeleton-queue") {
            skeleton_queue_day = -1;
            continue;
        }
        if (command == "--refinement-share" && index + 1 < argc) {
            const int value = std::stoi(argv[++index]);
            if (value < 1 || value > 100) {
                std::cerr << "[!] --refinement-share must be between 1 and 100\n";
                return 2;
            }
            refinement_share_percent = static_cast<uint8_t>(value);
            continue;
        }
        if (command == "--refinement-mismatches" && index + 1 < argc) {
            const int value = std::stoi(argv[++index]);
            if (value < 0 || value > 8) {
                std::cerr << "[!] --refinement-mismatches must be between 0 and 8\n";
                return 2;
            }
            refinement_maximum_mismatches = static_cast<uint8_t>(value);
            continue;
        }
        if (command == "--benchmark-json") {
            benchmark_json = true;
            continue;
        }
        if (command == "--polish-record") {
            polish_record = true;
            continue;
        }
        if (command == "--polish-beam" && index + 1 < argc) {
            polish_beam = static_cast<size_t>(std::stoull(argv[++index]));
            continue;
        }
        if (command == "--polish-rounds" && index + 1 < argc) {
            polish_rounds = static_cast<size_t>(std::stoull(argv[++index]));
            continue;
        }
        if (command == "--polish-evaluations" && index + 1 < argc) {
            polish_evaluations = static_cast<size_t>(std::stoull(argv[++index]));
            continue;
        }
        if (command == "--no-incumbent") {
            seed_incumbent = false;
            continue;
        }
        if (command == "--incumbent-prefix-actions" && index + 1 < argc) {
            incumbent_prefix_actions = std::stoi(argv[++index]);
            if (incumbent_prefix_actions <= 0) {
                std::cerr << "[!] --incumbent-prefix-actions must be greater than zero\n";
                return 2;
            }
            continue;
        }
        if (command == "--town" && index + 1 < argc) {
            const std::string town_value = argv[++index];
            const auto parsed = parse_town(town_value);
            if (!parsed.has_value()) {
                std::cerr << "[!] Unknown town: " << town_value << " (use --list-towns)\n";
                return 2;
            }
            town = *parsed;
            continue;
        }
        if (command == "--merchant-option" && index + 1 < argc) {
            const std::string option_value = argv[++index];
            const auto parsed = parse_merchant_option(option_value);
            if (!parsed.has_value()) {
                std::cerr << "[!] Unknown merchant option: " << option_value << '\n';
                return 2;
            }
            forced_merchant_option = *parsed;
            continue;
        }
        if (command == "--merchant-day" && index + 1 < argc) {
            merchant_day = std::stoi(argv[++index]);
            if (merchant_day < 5 || merchant_day > 7) {
                std::cerr << "[!] --merchant-day must be 5, 6, or 7\n";
                return 2;
            }
            continue;
        }
        if (command == "--witch-option" && index + 1 < argc) {
            const std::string option_value = argv[++index];
            const auto parsed = parse_witch_option(option_value);
            if (!parsed.has_value()) {
                std::cerr << "[!] Unknown witch option: " << option_value << '\n';
                return 2;
            }
            forced_witch_option = *parsed;
            continue;
        }
        if (command == "--witch-day" && index + 1 < argc) {
            witch_day = std::stoi(argv[++index]);
            if (witch_day < 9 || witch_day > 11) {
                std::cerr << "[!] --witch-day must be 9, 10, or 11\n";
                return 2;
            }
            continue;
        }
        if (command == "--purchase-profile" && index + 1 < argc) {
            const std::string profile_value = argv[++index];
            const auto parsed = parse_purchase_profile(profile_value);
            if (!parsed.has_value()) {
                std::cerr << "[!] Unknown purchase profile: " << profile_value
                          << " (expected greedy, scenario, strategic, or wide)\n";
                return 2;
            }
            purchase_profile = *parsed;
            continue;
        }
        if (command == "--list-towns") {
            for (int town_id = t_Normalia; town_id <= t_Ipartus; ++town_id) {
                const auto option = static_cast<town_options>(town_id);
                std::cout << town_id << ": " << to_conversion.at(option) << '\n';
            }
            return 0;
        }
        if (command == "--no-starting-trader") {
            starting_trader = false;
            continue;
        }
        if (command == "--starting-trader") {
            starting_trader = true;
            continue;
        }
        if (command == "--no-merchants") {
            merch_on = false;
            continue;
        }
        if (command == "--no-witches") {
            witch_on = false;
            continue;
        }
        if (command == "--experimental-sell-aware-beam") {
            sell_action_aware_beam = true;
            continue;
        }
        if (command == "--auto-refine") {
            auto_refine = true;
            continue;
        }
        if (command == "--no-auto-refine") {
            auto_refine = false;
            continue;
        }
        if (command == "--audit-known-route") {
            audit_known_route = true;
            continue;
        }
        if (command == "--proof-search") {
            proof_search = true;
            auto_refine = false;
            continue;
        }
        if (command == "--proof-transposition-limit" && index + 1 < argc) {
            proof_transposition_limit = static_cast<size_t>(std::stoull(argv[++index]));
            if (proof_transposition_limit == 0) {
                std::cerr << "[!] --proof-transposition-limit must be greater than zero\n";
                return 2;
            }
            continue;
        }
        if (command == "--search-map" && index + 1 < argc) {
            search_map = argv[++index];
            auto_refine = false;
            continue;
        }
        if (command == "--search-node-budget" && index + 1 < argc) {
            search_node_budget = static_cast<size_t>(std::stoull(argv[++index]));
            if (search_node_budget == 0) {
                std::cerr << "[!] --search-node-budget must be greater than zero\n";
                return 2;
            }
            continue;
        }
        if (command == "--search-map-order" && index + 1 < argc) {
            const std::string value = argv[++index];
            if (value == "depth") {
                search_order = PersistentSearchOrder::Depth;
            } else if (value == "beam") {
                search_order = PersistentSearchOrder::Beam;
            } else {
                std::cerr << "[!] --search-map-order must be depth or beam\n";
                return 2;
            }
            continue;
        }
        if (command == "--help") {
            std::cout
                << "Usage: Merchant_Trading_Caravan [options]\n"
                << "  --prebuild-trees\n"
                << "  --verify-trees [max_gold] [max_weight]\n"
                << "  --array-limit <count>\n"
                << "  --print-counter <count>\n"
                << "  --memory-limit-mb <mb>\n"
                << "  --time-limit-seconds <seconds>\n"
                << "  --hybrid-batch-size <count> (finish suffix batches when memory pressure rises)\n"
                << "  --hybrid-suffix-limit <count> (optional in-batch beam; defaults to batch size)\n"
                << "  --hybrid-split-day <day> (optional deterministic split for experiments)\n"
                << "  --hybrid-skeleton-order <score|cohort|interleave|structure-interleave> (default: score)\n"
                << "  --optimistic-share <0-100> (default: 25)\n"
                << "  --diversity-share <0-100> (default: 0; experimental)\n"
                << "  --skeleton-share <0-100> (reserve beam slots by first eight non-event actions)\n"
                << "  --skeleton-depth <1-8> (default: 8)\n"
                << "  --continuation-lane-share <0-100> (independent next-action lanes; default: 5)\n"
                << "  --skeleton-queue-day <0-255> (0: automatic memory trigger; positive: fixed day)\n"
                << "  --no-skeleton-queue (use legacy memory-pressure batching)\n"
                << "  --refinement-share <1-100> (learned-skeleton reservation; default: 50)\n"
                << "  --refinement-mismatches <0-8> (learned-skeleton radius; default: 1)\n"
                << "  --town <name-or-id> (default: Normalia)\n"
                << "  --merchant-option <name> (search only one Merchant choice)\n"
                << "  --merchant-day <5|6|7> (fixed classical encounter day)\n"
                << "  --witch-option <name> (search only one Witch choice)\n"
                << "  --witch-day <9|10|11> (fixed classical encounter day)\n"
                << "  --list-towns\n"
                << "  --starting-trader (lock the historical opening; default explores all starts)\n"
                << "  --no-starting-trader (compatibility alias for the default)\n"
                << "  --no-merchants\n"
                << "  --no-witches\n"
                << "  Cargo Buy rule: after buying cargo, Buy stays locked until Sell\n"
                << "  Wait action: replayable, but not generated by normal search\n"
                << "  --experimental-sell-aware-beam\n"
                << "  --auto-refine (default for open-timing searches, including pinned options)\n"
                << "  --no-auto-refine (single broad pass)\n"
                << "  --no-incumbent (measure only routes discovered by this run)\n"
                << "  --audit-known-route (observe the saved route without protecting it)\n"
                << "  --proof-search (exact depth-first search; combine with --search-map to spill open subtrees)\n"
                << "  --proof-transposition-limit <count> (default: 250000)\n"
                << "  --search-map <sqlite-file> (resume an exact persistent state graph; proof overflow store)\n"
                << "  --search-node-budget <count> (optional atomic expansions this session)\n"
                << "  --search-map-order <depth|beam> (default: depth)\n"
                << "  --incumbent-prefix-actions <count> (search from a verified route prefix)\n"
                << "  --purchase-profile <greedy|scenario|strategic|wide>\n"
                << "  --benchmark-json (quiet, machine-readable result)\n"
                << "  --polish-record (bounded exact-replay neighborhood search around known routes)\n"
                << "  --polish-beam <count> (default: 64)\n"
                << "  --polish-rounds <count> (default: 10)\n"
                << "  --polish-evaluations <count> (default: 250000)\n"
                << "  --warm-trees (optional; lazy loading is the safe default)\n"
                << "  --no-warm-trees\n";
            return 0;
        }
        std::cerr << "[!] Unknown option: " << command << '\n';
        return 2;
    }

    if (array_limit <= 0) {
        std::cerr << "[!] --array-limit must be greater than zero\n";
        return 2;
    }
    if (search_map.empty() && search_node_budget > 0) {
        std::cerr << "[!] --search-node-budget requires --search-map\n";
        return 2;
    }
    if (!search_map.empty() && proof_search && search_order != PersistentSearchOrder::Depth) {
        std::cerr << "[!] --proof-search with --search-map requires --search-map-order depth\n";
        return 2;
    }
    if ((hybrid_split_day > 0 || hybrid_suffix_limit > 0 ||
         hybrid_skeleton_order != HybridSkeletonOrder::Score) && hybrid_batch_size == 0) {
        std::cerr << "[!] Hybrid split/suffix options require --hybrid-batch-size\n";
        return 2;
    }
    if (static_cast<int>(skeleton_share_percent) + static_cast<int>(diversity_share_percent) > 100) {
        std::cerr << "[!] --skeleton-share and --diversity-share cannot total more than 100\n";
        return 2;
    }
    if (polish_record && (polish_beam == 0 || polish_rounds == 0 || polish_evaluations == 0)) {
        std::cerr << "[!] Route-polish limits must be greater than zero\n";
        return 2;
    }

    if (polish_record) {
        try {
            const auto seeds = polish_seed_routes(town);
            if (seeds.empty()) {
                std::cerr << "[!] No verified route is available to polish for "
                          << to_conversion.at(town) << '\n';
                return 2;
            }
            RoutePolishResult polished;
            for (size_t index = 0; index < seeds.size() &&
                 polished.evaluated_routes < polish_evaluations; ++index) {
                const size_t evaluations = std::max<size_t>(
                    1, (polish_evaluations - polished.evaluated_routes) / (seeds.size() - index)
                );
                RoutePolishResult candidate = RoutePolisher::polish(
                    town, {seeds[index]},
                    RoutePolishSettings{polish_beam, polish_rounds, evaluations}
                );
                polished.evaluated_routes += candidate.evaluated_routes;
                polished.legal_routes += candidate.legal_routes;
                polished.completed_rounds = std::max(polished.completed_rounds, candidate.completed_rounds);
                if (candidate.replay.legal && (!polished.replay.legal ||
                    candidate.replay.score > polished.replay.score)) {
                    polished.route = std::move(candidate.route);
                    polished.replay = std::move(candidate.replay);
                } else if (!polished.replay.legal && polished.replay.reason.empty()) {
                    polished.replay = std::move(candidate.replay);
                }
            }
            if (!polished.replay.legal) {
                std::cerr << "[!] No legal polish seed: " << polished.replay.reason
                          << " at action " << polished.replay.failed_action << '\n';
                return 2;
            }
            std::cout << "Polished " << to_conversion.at(town)
                      << ": score=" << polished.replay.score
                      << " evaluated=" << polished.evaluated_routes
                      << " legal=" << polished.legal_routes
                      << " rounds=" << polished.completed_rounds << '\n';
            for (const std::string& action : polished.replay.action_log) {
                std::cout << action << '\n';
            }
        } catch (const std::bad_alloc&) {
            std::cerr << "[!] Route polish reached the native memory ceiling; "
                      << "retry with fewer evaluations or a smaller beam\n";
            return 2;
        } catch (const std::exception& error) {
            std::cerr << "[!] Route polish failed: " << error.what() << '\n';
            return 2;
        }
        return 0;
    }

    // Code to Test Handler
    /*
    Player player;
    BestPurchaseHandler handler;
    std::pair<uint32_t, buy_list> profit;
    profit = handler.getBestBuy(250, 50, player);
    std::cout << "250g, 50w: +" << profit.first << "g " << profit.second << std::endl;
    profit = handler.getBestBuy(500, 50, player);
    std::cout << "500g, 50w: +" << profit.first << "g " << profit.second << std::endl;
    profit = handler.getBestBuy(250, 100, player);
    std::cout << "250g, 100w: +" << profit.first << "g " << profit.second << std::endl;

    handler = BestPurchaseHandler();
    profit = handler.getBestBuy(250, 50, player);
    std::cout << "250g, 50w: +" << profit.first << "g " << profit.second << std::endl;
    profit = handler.getBestBuy(500, 50, player);
    std::cout << "500g, 50w: +" << profit.first << "g " << profit.second << std::endl;
    profit = handler.getBestBuy(250, 100, player);
    std::cout << "250g, 100w: +" << profit.first << "g " << profit.second << std::endl;
    return 0;
    */


    /*
    // CODE to test TCSC
    AISolver test = AISolver();
    // test.get_current_state();
    std::cout << test.step(1);
    std::cout << test.calculate_final_score();
    return 0;
     */

    const std::vector<ga_data> incumbent_route = verified_incumbent_route(town);
    if (audit_known_route && incumbent_route.empty()) {
        std::cerr << "[!] No verified route is available to audit for "
                  << to_conversion.at(town) << '\n';
        return 2;
    }
    const auto [incumbent_merchant_day, incumbent_witch_day] = incumbent_encounter_days(town, incumbent_route);
    std::vector<ga_data> que;
    bool using_incumbent_prefix = false;
    if (incumbent_prefix_actions > 0) {
        if (incumbent_route.empty() ||
            static_cast<size_t>(incumbent_prefix_actions) >= incumbent_route.size()) {
            std::cerr << "[!] Requested incumbent prefix is unavailable or does not leave a suffix\n";
            return 2;
        }
        que.assign(incumbent_route.begin(), incumbent_route.begin() + incumbent_prefix_actions);
        using_incumbent_prefix = true;
    } else if (starting_trader) {
        que.emplace_back(_a_npc, n_Trader);
    }
    if (incumbent_prefix_actions > 0 && (merchant_day > 0 || witch_day > 0)) {
        std::cerr << "[!] Fixed encounter days cannot be combined with an incumbent prefix\n";
        return 2;
    }
    //que.emplace_back(_a_buy, 10, 0, 0, 0, 0, 0);
    //que.emplace_back(_a_npc, n_Trader);
    //que.emplace_back(_a_npc, n_Trader);
    //que.emplace_back(_a_merchant, m_Magic_Cleppsydra);
    //que.emplace_back(_a_buy, 11, 0, 0, 0, 0, 0);
    //que.emplace_back(_a_npc, n_Camel);
    //que.emplace_back(_a_buy, 30, 0, 0, 0, 0, 0);
    //que.emplace_back(_a_npc, n_Trader);
    //que.emplace_back(_a_witch, w_Midas_was_a_Trader);
    //que.emplace_back(_a_npc, n_Camel);
    //que.emplace_back(_a_npc, n_Camel);
    //que.emplace_back(_a_buy, 18, 1, 12, 0, 1, 1);
    //que.emplace_back(_a_sell);
    //que.emplace_back(_a_buy, 27, 0, 1, 0, 0, 5);
    //que.emplace_back(_a_sell);
    //que.emplace_back(_a_interest);
    //que.emplace_back(_a_interest);

    try {
        const auto campaign_started_at = std::chrono::steady_clock::now();
        const auto make_solver = [&]() {
            auto solver = std::make_unique<GameSolver>(
                town,
                merch_on,
                witch_on,
                array_limit,
                print_counter,
                que,
                "",
                memory_limit_mb,
                purchase_profile,
                optimistic_share_percent,
                sell_action_aware_beam,
                diversity_share_percent,
                merchant_day,
                witch_day
            );
            solver->set_time_limit_seconds(time_limit_seconds);
            solver->set_forced_merchant_option(forced_merchant_option);
            solver->set_forced_witch_option(forced_witch_option);
            solver->set_hybrid_batch_size(hybrid_batch_size);
            solver->set_hybrid_suffix_limit(hybrid_suffix_limit);
            solver->set_hybrid_split_day(hybrid_split_day);
            solver->set_hybrid_skeleton_order(hybrid_skeleton_order);
            solver->set_skeleton_share_percent(skeleton_share_percent);
            solver->set_skeleton_depth(skeleton_depth);
            solver->set_continuation_lane_share_percent(continuation_lane_share_percent);
            solver->set_skeleton_queue_day(skeleton_queue_day);
            if (audit_known_route) {
                solver->set_route_survival_audit(
                    solver_actions_from_route(incumbent_route, que.size())
                );
            }
            return solver;
        };

        std::unique_ptr<GameSolver> solver = make_solver();
        const bool starts_with_verified_prefix = using_incumbent_prefix || que.empty() ||
            (que.size() == 1 &&
             que.front().action == _a_npc &&
             que.front().npc_data == n_Trader);
        const bool incumbent_schedule_matches =
            (merchant_day == 0 || merchant_day == incumbent_merchant_day) &&
            (witch_day == 0 || witch_day == incumbent_witch_day);
        if (seed_incumbent && incumbent_schedule_matches &&
            !forced_merchant_option.has_value() && !forced_witch_option.has_value() &&
            starts_with_verified_prefix && !incumbent_route.empty() &&
            route_respects_cargo_buy_rule(incumbent_route)) {
            Player incumbent_player;
            Game::apply_town(incumbent_player, town);
            Game::auto_game_loop(incumbent_player, incumbent_route);
            solver->seed_incumbent(
                Game::check_points(incumbent_player, "Final"),
                solver_actions_from_route(incumbent_route, que.size())
            );
        }
        if (warm_trees) {
            solver->warm_purchase_trees();
        }

        if (!search_map.empty()) {
            solver->find_solve_mapped(
                search_map,
                search_node_budget,
                search_order,
                !benchmark_json
            );
        } else if (proof_search) {
            solver->find_solve_proof_depth(
                proof_transposition_limit,
                !benchmark_json
            );
        } else {
            solver->find_solve_depth(!benchmark_json);
        }
        std::unique_ptr<GameSolver> refined;
        GameSolver* selected = solver.get();
        const bool refinement_compatible_search =
            incumbent_prefix_actions == 0 &&
            !starting_trader &&
            merchant_day == 0 &&
            witch_day == 0;
        uint64_t remaining_seconds = time_limit_seconds;
        if (time_limit_seconds > 0) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - campaign_started_at
            ).count();
            remaining_seconds = elapsed >= static_cast<long long>(time_limit_seconds)
                ? 0
                : time_limit_seconds - static_cast<uint64_t>(elapsed);
        }
        const bool refinement_has_time =
            time_limit_seconds == 0 || remaining_seconds > 1;
        if (search_map.empty() && auto_refine && refinement_compatible_search && refinement_has_time) {
            const auto discovered = std::find_if(
                solver->best_solves().begin(),
                solver->best_solves().end(),
                [](const FinishedSolve& solve) {
                    return solve.origin == FinishedSolve::Origin::Discovered;
                }
            );
            if (discovered != solver->best_solves().end()) {
                refined = make_solver();
                if (time_limit_seconds > 0) {
                    refined->set_time_limit_seconds(remaining_seconds);
                }
                refined->set_refinement_template(
                    discovered->actions,
                    refinement_share_percent,
                    refinement_maximum_mismatches
                );
                if (warm_trees) {
                    refined->warm_purchase_trees();
                }
                refined->find_solve_depth(!benchmark_json);
                if (refined->best_points() > solver->best_points()) {
                    selected = refined.get();
                }
            }
        }
        if (benchmark_json) {
            const double campaign_elapsed_seconds = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - campaign_started_at
            ).count();
            selected->print_benchmark_json(std::cout, campaign_elapsed_seconds);
        }
    } catch (const std::bad_alloc&) {
        std::cerr << "[!] Allocation failed before the solver could shed memory. "
                  << "Retry with scripts/run_safe.sh or lower --array-limit.\n";
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "[!] Solver failed: " << error.what() << '\n';
        return 2;
    }
    //Path(f'./hashes/{town_options_string[town]}_{merch}_{witch}').mkdir(parents=True, exist_ok=True)
    //solver.find_solve(f'./hashes/{town_options_string[town]}_{merch}_{witch}')

    return 0;
}
