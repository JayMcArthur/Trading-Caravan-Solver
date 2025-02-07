//
// Created by Jay on 6/14/2023.
//

#include <algorithm>
#include <iostream>
#include <bits/stdc++.h>
#include "GameSolver.h"
#include "helpers.h"

void GameSolver::print_solves(const int &amount) {
    if (amount != 1) [[unlikely]] {
        std::cout << "Printing Solves\n";
    }
    int place = 1;
    for (auto p = finished.rbegin(); p != finished.rend(); p++) {
        std::cout << "Place " << place << ": " << p->points << "\n";
        for (const auto & action : p->actions) {
            if (&action != &p->actions.front()) [[likely]] {
                std::cout << ", "<<  action ;
            } else [[unlikely]]{
                std::cout <<  action;
            }
        }
        std::cout << "\n";
        ++place;
        if (place > amount) {
            std::cout << "\n";
            break;
        }
    }
}

void GameSolver::progress_state(Player current) {
    if (!current.day_start_skip) {
        Game::start_of_day(current);

        if (current.food < 0) {
            return; // Starved to death
        }

        // Merchant Event
        if (merch_on and !current.merchant_happened and (current.call_merchant || Helpers::day_in_array(merch_days, current.day))) {
            p_mutex.lock();
            Helpers::run_merchant_event(current, to_process);
            p_mutex.unlock();
        }

        // Witch Event
        if (witch_on && !current.witch_happened and (Helpers::day_in_array(witch_days, current.day))) {
            p_mutex.lock();
            Helpers::run_witch_event(current, to_process);
            p_mutex.unlock();
        }
    } else {
        current.day_start_skip = false;
    }

    const int current_food_cost = current.food_consumption;

    // Buy Event
    // Will never buy do last day or two days in a row
    if (current.day < current.max_day && !current.bought_last) {
        p_mutex.lock();
        Helpers::run_buy_event(current, to_process, handler);
        p_mutex.unlock();
    }

    if (current.day == current.max_day || current.food >= current_food_cost) {
        // Sell Event
        // Check you have stuff to sell
        if (Game::check_has_items(current)) {
            f_mutex.lock();
            p_mutex.lock();
            Helpers::run_sell_event(current, to_process, finished);
            p_mutex.unlock();
            f_mutex.unlock();

        }

        // Trader Event
        // Check cost and new food cost
        // Will never do on last day
        if (current.gold >= current.npc_shop[n_Trader][nd_Cost] and current.day < current.max_day and current.food >= current_food_cost + current.npc_shop[n_Trader][nd_Eat]) {
            p_mutex.lock();
            Helpers::run_trader_event(current, to_process);
            p_mutex.unlock();

        }

        // Camel Event
        // Check cost and new food cost
        // Will never do on last day
        if (current.gold >= current.npc_shop[n_Camel][nd_Cost] and current.day < current.max_day and current.food >= current_food_cost + current.npc_shop[n_Camel][nd_Eat]) {
            p_mutex.lock();
            Helpers::run_camel_event(current, to_process);
            p_mutex.unlock();

        }

        // Interest Event
        // Lowest buy sell combo is *1.65 for 2 days or *1.284 per day
        // ALl the rest are at least *2 for 2 days or *1.41 per day
        // This means interest at 1.5 and 1.4 is better aka at least 2 in a run
        // Will never do if before day 4 for search space limiting -- Might take this off
        if (current.day > 3 && (current.interest_rate > 1.2 || current.day == current.max_day)) {
            f_mutex.lock();
            p_mutex.lock();
            Helpers::run_interest_event(current, to_process, finished);
            p_mutex.unlock();
            f_mutex.unlock();
        }
    }

}

void GameSolver::find_solve_threads(const int amount) {
    // TODO -- This is old code
    bool first = true;
    for (int i = 0; i < amount; ++i) {
        std::thread th([this, first] {this->find_solve_depth(first);});
        threads.push_back(std::move(th));
        first = false;
    }

    for (int i = 0; i < amount; ++i) {
        threads[i].join();
    }

}

void GameSolver::find_solve_depth(const bool main_thread) {
    int round_num = 0;
    bool check = true;
    while (check) {
        p_mutex.lock();
        Player current = to_process.back();
        to_process.pop_back();
        p_mutex.unlock();

        progress_state(current);

        if (main_thread and processed >= print_counter) {
            round_num += 1;
            std::cout << "Processing Round " << round_num << "\n";
            print_solves(1);
            processed = 0;
        }
        p_mutex.lock();
        check = !to_process.empty();
        p_mutex.unlock();
    }
    if (main_thread) {
        print_solves(10);
    }
}

void GameSolver::find_solve_breath(const bool continuous) {
    // TODO -- This is old code
    int round_num = 0;
    unsigned long long count = 0;
    unsigned long long goal = 0;
    int day = to_process[0].day + 1;
    if (!continuous) {
        goal = to_process.size();
        std::cout << "Processing Day " << day << " - #" << to_process.size() << "\n";
    }

    while(!to_process.empty()) {
        Player current = to_process.front();
        to_process.pop_front();

        progress_state(current);

        // Limit array per day
        ++count;
        if (!continuous and count >= goal) {
            if (to_process.size() > array_limit) {
                sort(to_process.begin(), to_process.end(), std::greater <>());
                to_process.resize(array_limit);
            }
            goal = to_process.size();
            count = 0;
            day += 1;
            std::cout << "Processing Day " << day << " - #" << to_process.size() << "\n";
        }

        // Print temp results
        if (processed >= print_counter) {
            round_num += 1;
            std::cout << "Processing Round " << round_num << "\n";
            print_solves(1);
            processed = 0;
        }
    }
    print_solves(10);
}

/*
 * def add_hash(self, father_hash, child_hash) -> bool:
 * -- This is the starting state:
 * -- If hash has already been finished
 * -- If hash isn't marked as child
 * -- Create Child hash
 * -- Child hash already made, update it
 *
 * def finish_hash(self, hash_to_check, finished_obj: Player, out_of_food: bool = False) -> None:
 * -- Mark Completed
 * -- Save then cleanup hash
 *
 * def cleanup_hash(self, hash_to_check, best=0, best_obj=None, child_hash=None) -> None:
 * -- Marks complete and sets new best / working & finished
 * -- goes to all farther hashes and cleans them up
 *
 * def get_hash(self, hash_name):
 * -- Gets hash from files
 *
 * def save_hash(self, hash_name):
 * -- Saves hash to file
 */