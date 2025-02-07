//
// Created by Jay on 6/14/2023.
//

#ifndef TRADING_CARAVAN_SOLVER_GAMESOLVER_H
#define TRADING_CARAVAN_SOLVER_GAMESOLVER_H

#include <set>
#include <deque>
#include <mutex>
#include <thread>
#include "../game/Game.h"
#include "../purchaseHandler/BestPurchaseHandler.h"
#include "../consts/enums.h"


class GameSolver {
public:
    explicit GameSolver (const town_options town = t_Normalia, const bool _merch_on = true, const bool _witch_on = true, const int _array_limit = 1000000, const int _print_counter = 1000, const std::vector<ga_data> &que={}) {
        array_limit = _array_limit;
        print_counter = _print_counter;
        Game::apply_town(start_player, town);
        if (!que.empty()) {
            Game::auto_game_loop(start_player, que);
        }
        to_process.emplace_back(start_player);
        // hash Dict
        // hash Path
        processed = 0;
        merch_on = _merch_on;
        witch_on = _witch_on;
    };
    void find_solve_breath(bool continuous = true);
    void find_solve_threads(int amount);
    void find_solve_depth(bool main_thread = true);

private:
    Player start_player;
    BestPurchaseHandler handler;
    std::deque<Player> to_process = {};
    std::set<Player> finished;
    std::mutex p_mutex;
    std::mutex f_mutex;
    int array_limit;
    int print_counter;
    int processed;
    bool merch_on;
    const int merch_days[3] = {5, 6, 7};
    bool witch_on;
    const int witch_days[3] = {9, 10, 11};
    std::vector<std::thread> threads = {};

    void progress_state(Player current);
    void print_solves(const int &amount);
    // TODO - HASH
    //  add_hash
    //  finish_hash
    //  cleanup_hash
    //  get_hash
    //  save_hash
};


#endif //TRADING_CARAVAN_SOLVER_GAMESOLVER_H
