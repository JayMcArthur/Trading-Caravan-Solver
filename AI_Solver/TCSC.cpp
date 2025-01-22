//
// Created by Jay on 1/13/2025.
//

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <deque>
#include <set>
#include <vector>
#include <string>
#include <bits/stdc++.h>
#include "Game.h"
#include "enums.h"

namespace py = pybind11;
using namespace pybind11::literals;

class AISolver {
private:
    Game *game;
    Player start_player;
    Player working_state;
    deque<Player> state_queue;
    set<Player> final_states;
    bool merch_on;
    const int merch_days[3] = {5, 6, 7};
    bool witch_on;
    const int witch_days[3] = {9, 10, 11};
    static bool day_in_array(const int (&list)[3], const int &value) {
        if (value == list[0] || value == list[1] || value == list[2]) {
            return true;
        }
        return false;
    };

public:
    explicit AISolver (const town_options _town=_Normalia, const bool _merch_on = true, const bool _witch_on = true, const vector<ga_data> &que={}) {
        merch_on = _merch_on;
        witch_on = _witch_on;
        game = new Game();
        Game::apply_town(start_player, _town);
        if (!que.empty()) {
            Game::auto_game_loop(start_player, que);
        }
        state_queue.emplace_back(start_player);
        get_next_save();
    }
    ~AISolver() {
        delete game;
    }
    void reset() {
        while (!state_queue.empty()) state_queue.pop_back();
        final_states.clear();
        state_queue.emplace_back(start_player);
        get_next_save();
    }

    py::dict get_current_state() {
        return py::dict(
                "gold"_a=working_state.gold,
                "food"_a=working_state.food,
                "items"_a=Game::check_has_items(working_state),
                "traders"_a=working_state.trader,
                "camels"_a=working_state.camel,
                "food_consumption"_a=working_state.food_consumption,
                "backpack"_a=working_state.backpack,
                "daily_income"_a=working_state.daily_income,
                "interest_rate"_a=working_state.interest_rate,
                "set_interest"_a=working_state.set_interest,
                "quick_interest"_a=working_state.quick_interest,
                "statue"_a=working_state.statue,
                "hand_of_midas"_a=working_state.hand_of_midas,
                "trader_interest"_a=working_state.trader_interest,
                "day"_a=working_state.day,
                "max_day"_a=working_state.max_day,
                "available_actions"_a=working_state.available_actions,
                "points"_a=working_state.points
        );
    };

    bool get_next_save(){
        working_state = state_queue.back();
        state_queue.pop_back();
        do_start_of_day();
        working_state.available_actions.clear();
        // Buy Event
        // Will never buy do last day or two days in a row
        if (working_state.day < working_state.max_day && !working_state.bought_last) {
            working_state.available_actions.emplace_back(1);
        }
        if (working_state.day == working_state.max_day || working_state.food >=  working_state.food_consumption) {
            // Sell Event
            // Check you have stuff to sell
            if (Game::check_has_items(working_state)) {
                working_state.available_actions.emplace_back(2);
            }
            // Trader Event
            // Check cost and new food cost
            // Will never do on last day
            if (working_state.gold >= working_state.npc_shop[_Trader][_cost] and working_state.day < working_state.max_day and working_state.food >= working_state.food_consumption + working_state.npc_shop[_Trader][_eat]) {
                working_state.available_actions.emplace_back(3);
            }
            // Camel Event
            // Check cost and new food cost
            // Will never do on last day
            if (working_state.gold >= working_state.npc_shop[_Camel][_cost] and working_state.day < working_state.max_day and working_state.food >= working_state.food_consumption + working_state.npc_shop[_Camel][_eat]) {
                working_state.available_actions.emplace_back(4);
            }
            // Interest Event
            // Lowest buy sell combo is *1.65 for 2 days or *1.284 per day
            // ALl the rest are at least *2 for 2 days or *1.41 per day
            // This means interest at 1.5 and 1.4 is better aka at least 2 in a run
            if (working_state.interest_rate > 1.2 || working_state.day == working_state.max_day) {
                working_state.available_actions.emplace_back(5);
            }
        }
        if (working_state.available_actions.size() == 0) {
            if (not state_queue.empty()) {
                return get_next_save();
            } else {
                return false;
            }
        }
        return true;
    }

    void do_start_of_day() {
        if (not working_state.day_start_skip) {
            Game::start_of_day(working_state);
            if (working_state.food < 0) {
                return; // Starved to death
            }
            // Merchant Event
            if (merch_on and !working_state.merchant_happened and (working_state.call_merchant || day_in_array(merch_days, working_state.day))) {
                for (auto event : ALL_MERCH_OPTIONS) {
                    Player merch = working_state;
                    const int worked = Game::event_merchant(merch, event);
                    if (worked) {
                        state_queue.emplace_back(merch);
                    }
                }
            }

            // Witch Event
            if (witch_on && !working_state.witch_happened and (day_in_array(witch_days, working_state.day))) {
                for (auto event : ALL_WITCH_OPTIONS) {
                    Player witch = working_state;
                    Game::event_witch(witch, event);
                    state_queue.emplace_back(witch);

                }
            }
        } else {
            working_state.day_start_skip = false;
        }
    }

    bool step(int action) {
        switch (action) {
            case 1: {
                const int current_food_cost = working_state.food_consumption;
                const int days_left = working_state.max_day - working_state.day;
                const int food_multi = days_left > 1 ? 2 : 1; // Would buy for today and tomorrow or no point
                const int max_weight = working_state.item_shop[_Food][_weight] == 0 ? 100 : Game::check_weight_left(working_state) / working_state.item_shop[_Food][_weight];
                const int max_afford = working_state.item_shop[_Food][_buy] == 0 ? 100 : working_state.gold / working_state.item_shop[_Food][_buy];
                const int max_eat = max(working_state.npc_shop[_Trader][_eat], working_state.npc_shop[_Camel][_eat]);
                const int future_days = days_left - 1;
                const int max_want = (current_food_cost * days_left) + (max_eat * (future_days * (future_days + 1)) / 2); // Max could need (Today + tomorrow+2 + Day3+4 + etc)

                const int max_food = min({max_weight, max_afford, max_want});
                int needed_food = max(0, (food_multi * current_food_cost) - working_state.food);
                const int only_food_requirement = min((current_food_cost * 2) + max_eat - working_state.food, 0);

                // TODO -- Find someway to cut this search space

                for (; needed_food <= max_food; ++needed_food) {
                    Player to_buy_everything = working_state;
                    Game::find_buy(to_buy_everything, needed_food);
                    state_queue.emplace_back(to_buy_everything);
                    if (needed_food >= only_food_requirement) {
                        Player to_buy_food = working_state;
                        Game::find_buy(to_buy_food, needed_food, true);
                        state_queue.emplace_back(to_buy_food);
                    }

                }
                break;
            }
            case 2: {
                Player sell = working_state;
                Game::event_sell_items(sell);
                if (sell.day != sell.max_day) {
                    state_queue.emplace_back(sell);
                } else {
                    final_states.emplace(sell);
                }
                break;
            }
            case 3: {
                Player trader = working_state;
                Game::event_buy_npc(trader, _Trader);
                state_queue.emplace_back(trader);
                break;
            }
            case 4: {
                Player camel = working_state;
                Game::event_buy_npc(camel, _Camel);
                state_queue.emplace_back(camel);
                break;
            }
            case 5: {
                Player interest = working_state;
                Game::event_interest(interest);
                if (interest.day != interest.max_day) {
                    state_queue.emplace_back(interest);
                } else {
                    final_states.emplace(interest);
                }
                break;
            }
            default: {
            }
        }
        if (not state_queue.empty()) {
            return get_next_save();
        }
        return false;
    };

    int calculate_final_score() {
        return final_states.empty() ? 0 : final_states.rbegin()->points;
    };

};


PYBIND11_MODULE(TCSC, m) {
    py::class_<AISolver>(m, "AISolver")
            .def(py::init<>())
            .def("get_current_state", &AISolver::get_current_state)
            .def("step", &AISolver::step)
            .def("reset", &AISolver::reset)
            .def("calculate_final_score", &AISolver::calculate_final_score);
}