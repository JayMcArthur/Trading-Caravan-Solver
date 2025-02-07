#include <vector>
#include <iostream>
#include "consts/enums.h"
#include "purchaseHandler/BestPurchaseHandler.h"
#include "player/player.h"
#include "solvers/Gamesolver.h"
#include "solvers/TCSC.cpp"


int main() {

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
    int array_limit = 1000000;
    int print_counter = 100000000;
    //que.clear();

    GameSolver solver(town, merch_on, witch_on, array_limit, print_counter, que);

    //solver.find_solve_breath();
    solver.find_solve_depth();
    //solver.find_solve_threads(8);
    //Path(f'./hashes/{town_options_string[town]}_{merch}_{witch}').mkdir(parents=True, exist_ok=True)
    //solver.find_solve(f'./hashes/{town_options_string[town]}_{merch}_{witch}')

    return 0;
}