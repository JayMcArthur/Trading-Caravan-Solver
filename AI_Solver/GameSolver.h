//
// Created by Jay on 6/14/2023.
//

#ifndef TRADING_CARAVAN_SOLVER_GAMESOLVER_H
#define TRADING_CARAVAN_SOLVER_GAMESOLVER_H

#include <set>
#include <deque>
#include <mutex>
#include <thread>
#include "Game.h"
#include "enums.h"


class GameSolver {
public:
    explicit GameSolver (const town_options town=_Normalia, const bool _merch_on = true, const bool _witch_on = true, const int _array_limit = 1000000, const int _print_counter = 1000, const vector<ga_data> &que={});
    ~GameSolver() {delete game};
    void find_solve_breath(bool continuous = true);
    void find_solve_threads(int amount);
    void find_solve_depth(bool main_thread = true);

private:
    Game *game;
    Player start_player;
    deque<Player> to_process = {};
    std::mutex p_mutex;
    set<Player> finished;
    std::mutex f_mutex;
    int array_limit;
    int print_counter;
    int processed;
    bool merch_on;
    const int merch_days[3] = {5, 6, 7};
    bool witch_on;
    const int witch_days[3] = {9, 10, 11};
    vector<std::thread> threads = {};

    void progress_state(Player current);
    static bool day_in_array(const int (&list)[3], const int &value);
    void print_solves(const int &amount);
    void add_finished(const Player &save_file);
    // add_hash
    // finish_hash
    // cleanup_hash
    // get_hash
    // save_hash
};


#endif //TRADING_CARAVAN_SOLVER_GAMESOLVER_H
