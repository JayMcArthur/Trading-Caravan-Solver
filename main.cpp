#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "consts/enums.h"
#include "purchaseHandler/BestPurchaseHandler.h"
#include "player/player.h"
#include "solvers/GameSolver.h"


int main(int argc, char** argv) {
    int array_limit = 1000000;
    int print_counter = 500000;
    uint64_t memory_limit_mb = 0;
    bool warm_trees = true;

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
        if (command == "--no-warm-trees") {
            warm_trees = false;
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
                << "  --no-warm-trees\n";
            return 0;
        }
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

    std::vector<ga_data> que;
    // This is best we have found for Normal
    que.emplace_back(_a_npc, n_Trader);
    que.emplace_back(_a_buy, 10, 0, 0, 0, 0, 0);
    que.emplace_back(_a_npc, n_Trader);
    que.emplace_back(_a_npc, n_Trader);
    que.emplace_back(_a_merchant, m_Magic_Cleppsydra);
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

    town_options town = t_Normalia;
    bool merch_on = true;
    bool witch_on = true;
    //que.clear();

    GameSolver solver(town, merch_on, witch_on, array_limit, print_counter, que, "", memory_limit_mb);
    if (warm_trees) {
        solver.warm_purchase_trees();
    }

    //solver.find_solve_breath();
    solver.find_solve_depth();
    //solver.find_solve_threads(8);
    //Path(f'./hashes/{town_options_string[town]}_{merch}_{witch}').mkdir(parents=True, exist_ok=True)
    //solver.find_solve(f'./hashes/{town_options_string[town]}_{merch}_{witch}')

    return 0;
}
