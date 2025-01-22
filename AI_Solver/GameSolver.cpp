//
// Created by Jay on 6/14/2023.
//

#include <algorithm>
#include <iostream>
#include <bits/stdc++.h>
#include "GameSolver.h"

explicit GameSolver(const town_options town, const bool _merch_on, const bool _witch_on, const int _array_limit, const int _print_counter, const vector<ga_data> &que) {
    array_limit = _array_limit;
    print_counter = _print_counter;
    game = new Game();
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
}

void GameSolver::print_solves(const int &amount) {
    if (amount != 1) [[unlikely]] {
        std::cout << "Printing Solves\n";
    }
    int place = 1;
    for (auto p = finished.rbegin(); p != finished.rend(); p++) {
        cout << "Place " << place << ": " << p->points << "\n";
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

void GameSolver::add_finished(const Player &save_file) {
    f_mutex.lock();
    finished.insert(save_file);
    if (finished.size() > 10) [[likely]] {
        ++processed;
        finished.erase(finished.begin());
    }
    f_mutex.unlock();
}

void GameSolver::progress_state(Player current) {
    if (!current.day_start_skip) {
        Game::start_of_day(current);

        if (current.food < 0) {
            return; // Starved to death
        }

        // Merchant Event
        if (merch_on and !current.merchant_happened and (current.call_merchant || day_in_array(merch_days, current.day))) {
            for (auto event : ALL_MERCH_OPTIONS) {
                Player merch = current;
                const int worked = Game::event_merchant(merch, event);
                if (worked) {
                    p_mutex.lock();
                    to_process.emplace_back(merch);
                    p_mutex.unlock();
                }
            }
        }

        // Witch Event
        if (witch_on && !current.witch_happened and (day_in_array(witch_days, current.day))) {
            for (auto event : ALL_WITCH_OPTIONS) {
                Player witch = current;
                Game::event_witch(witch, event);
                p_mutex.lock();
                to_process.emplace_back(witch);
                p_mutex.unlock();

            }
        }
    } else {
        current.day_start_skip = false;
    }

    const int current_food_cost = current.food_consumption;

    // Buy Event
    // Will never buy do last day or two days in a row
    if (current.day < current.max_day && !current.bought_last) {
        const int days_left = current.max_day - current.day;
        const int food_multi = days_left > 1 ? 2 : 1; // Would buy for today and tomorrow or no point
        const int max_weight = current.item_shop[_Food][_weight] == 0 ? 100 : Game::check_weight_left(current) / current.item_shop[_Food][_weight];
        const int max_afford = current.item_shop[_Food][_buy] == 0 ? 100 : current.gold / current.item_shop[_Food][_buy];
        const int max_eat = max(current.npc_shop[_Trader][_eat], current.npc_shop[_Camel][_eat]);
        const int future_days = days_left - 1;
        const int max_want = (current_food_cost * days_left) + (max_eat * (future_days * (future_days + 1))/2); // Max could need (Today + tomorrow+2 + Day3+4 + etc)

        const int max_food = min({max_weight, max_afford, max_want});
        int needed_food = max(0, (food_multi * current_food_cost) - current.food);
        const int only_food_requirement = min((current_food_cost*2)+max_eat-current.food, 0);

        for (; needed_food <= max_food; ++needed_food) {
            Player to_buy_everything = current;
            Game::find_buy(to_buy_everything, needed_food);
            p_mutex.lock();
            to_process.emplace_back(to_buy_everything);
            p_mutex.unlock();
            if (needed_food >= only_food_requirement) {
                Player to_buy_food = current;
                Game::find_buy(to_buy_food, needed_food, true);
                p_mutex.lock();
                to_process.emplace_back(to_buy_food);
                p_mutex.unlock();
            }
        }
    }

    if (current.day == current.max_day || current.food >= current_food_cost) {
        // Sell Event
        // Check you have stuff to sell
        if (Game::check_has_items(current)) {
            Player sell = current;
            Game::event_sell_items(sell);
            if (sell.day != sell.max_day) {
                p_mutex.lock();
                to_process.emplace_back(sell);
                p_mutex.unlock();
            } else {
                add_finished(sell);
            }
        }

        // Trader Event
        // Check cost and new food cost
        // Will never do on last day
        if (current.gold >= current.npc_shop[_Trader][_cost] and current.day < current.max_day and current.food >= current_food_cost + current.npc_shop[_Trader][_eat]) {
            Player trader = current;
            Game::event_buy_npc(trader, _Trader);
            p_mutex.lock();
            to_process.emplace_back(trader);
            p_mutex.unlock();

        }

        // Camel Event
        // Check cost and new food cost
        // Will never do on last day
        if (current.gold >= current.npc_shop[_Camel][_cost] and current.day < current.max_day and current.food >= current_food_cost + current.npc_shop[_Camel][_eat]) {
            Player camel = current;
            Game::event_buy_npc(camel, _Camel);
            p_mutex.lock();
            to_process.emplace_back(camel);
            p_mutex.unlock();

        }

        // Interest Event
        // Lowest buy sell combo is *1.65 for 2 days or *1.284 per day
        // ALl the rest are at least *2 for 2 days or *1.41 per day
        // This means interest at 1.5 and 1.4 is better aka at least 2 in a run
        // Will never do if before day 4 for search space limiting -- Might take this off
        if (current.day > 3 && (current.interest_rate > 1.2 || current.day == current.max_day)) {
            Player interest = current;
            Game::event_interest(interest);
            if (interest.day != interest.max_day) {
                p_mutex.lock();
                to_process.emplace_back(interest);
                p_mutex.unlock();
            } else {
                add_finished(interest);
            }
        }
    }

}

bool GameSolver::day_in_array(const int (&list)[3], const int &value) {
    if (value == list[0]) {
        return true;
    }
    if (value == list[1]) {
        return true;
    }
    if (value == list[2]) {
        return true;
    }
    return false;
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
            cout << "Processing Round " << round_num << "\n";
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
        cout << "Processing Day " << day << " - #" << to_process.size() << "\n";
    }

    while(!to_process.empty()) {
        Player current = to_process.front();
        to_process.pop_front();

        progress_state(current);

        // Limit array per day
        ++count;
        if (!continuous and count >= goal) {
            if (to_process.size() > array_limit) {
                sort(to_process.begin(), to_process.end(), greater <>());
                to_process.resize(array_limit);
            }
            goal = to_process.size();
            count = 0;
            day += 1;
            cout << "Processing Day " << day << " - #" << to_process.size() << "\n";
        }

        // Print temp results
        if (processed >= print_counter) {
            round_num += 1;
            cout << "Processing Round " << round_num << "\n";
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