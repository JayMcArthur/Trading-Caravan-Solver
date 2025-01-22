#include <vector>
#include <cstdio>
#include "enums.h"
#include "GameSolver.h"


int main() {
    std::vector<ga_data> que;
    // This is best we have found for Normal
    que.emplace_back(_a_npc, _Trader);
    que.emplace_back(_a_buy, 10, 0, 0, 0, 0, 0);
    que.emplace_back(_a_npc, _Trader);
    que.emplace_back(_a_npc, _Trader);
    que.emplace_back(_a_merchant, _Magic_Cleppsydra);
    //que.emplace_back(_a_buy, 11, 0, 0, 0, 0, 0);
    //que.emplace_back(_a_npc, _Camel);
    //que.emplace_back(_a_buy, 30, 0, 0, 0, 0, 0);
    //que.emplace_back(_a_npc, _Trader);
    //que.emplace_back(_a_witch, _Midas_was_a_Trader);
    //que.emplace_back(_a_npc, _Camel);
    //que.emplace_back(_a_npc, _Camel);
    //que.emplace_back(_a_buy, 18, 1, 12, 0, 1, 1);
    //que.emplace_back(_a_sell);
    //que.emplace_back(_a_buy, 27, 0, 1, 0, 0, 5);
    //que.emplace_back(_a_sell);
    //que.emplace_back(_a_interest);


    town_options town = _Normalia;
    bool merch_on = true;
    bool witch_on = true;
    int array_limit = 1000000;
    int print_counter = 100000000;
    que.clear();

    GameSolver solver(town, merch_on, witch_on, array_limit, print_counter, que);

    //solver.find_solve_breath();
    solver.find_solve_depth();
    //solver.find_solve_threads(8);
    //Path(f'./hashes/{town_options_string[town]}_{merch}_{witch}').mkdir(parents=True, exist_ok=True)
    //solver.find_solve(f'./hashes/{town_options_string[town]}_{merch}_{witch}')

    return 0;
}