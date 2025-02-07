//
// Created by Jay on 1/13/2025.
//

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <deque>
#include <set>
#include <vector>
#include "../game/Game.h"
#include "../purchaseHandler/BestPurchaseHandler.h"
#include "helpers.h"

namespace py = pybind11;
using namespace pybind11::literals;

class AISolver {
private:
    Player start_player;
    Player working_state;
    BestPurchaseHandler handler;
    std::deque<Player> state_queue = {};
    std::set<Player> final_states;
    bool merch_on;
    const int merch_days[3] = {5, 6, 7};
    bool witch_on;
    const int witch_days[3] = {9, 10, 11};


public:
    explicit AISolver (const town_options _town=t_Normalia, const bool _merch_on = true, const bool _witch_on = true, const std::vector<ga_data> &que={}) {
        merch_on = _merch_on;
        witch_on = _witch_on;
        Game::apply_town(start_player, _town);
        if (!que.empty()) {
            Game::auto_game_loop(start_player, que);
        }
        state_queue.emplace_back(start_player);
        get_next_save();
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
            if (working_state.gold >= working_state.npc_shop[n_Trader][nd_Cost] and working_state.day < working_state.max_day and working_state.food >= working_state.food_consumption + working_state.npc_shop[n_Trader][nd_Eat]) {
                working_state.available_actions.emplace_back(3);
            }
            // Camel Event
            // Check cost and new food cost
            // Will never do on last day
            if (working_state.gold >= working_state.npc_shop[n_Camel][nd_Cost] and working_state.day < working_state.max_day and working_state.food >= working_state.food_consumption + working_state.npc_shop[n_Camel][nd_Eat]) {
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
        if (working_state.available_actions.empty()) {
            if (not state_queue.empty()) {
                return get_next_save();
            } else {
                return false;
            }
        }
        return true;
    };

    void do_start_of_day() {
        if (not working_state.day_start_skip) {
            Game::start_of_day(working_state);
            if (working_state.food < 0) {
                return; // Starved to death
            }
            // Merchant Event
            if (merch_on and !working_state.merchant_happened and (working_state.call_merchant || Helpers::day_in_array(merch_days, working_state.day))) {
                Helpers::run_merchant_event(working_state, state_queue);
            }

            // Witch Event
            if (witch_on && !working_state.witch_happened and (Helpers::day_in_array(witch_days, working_state.day))) {
                Helpers::run_witch_event(working_state, state_queue);
            }
        } else {
            working_state.day_start_skip = false;
        }
    };

    bool step(int action) {
        switch (action) {
            case 1: {
                Helpers::run_buy_event(working_state, state_queue, handler);
                break;
            }
            case 2: {
                Helpers::run_sell_event(working_state, state_queue, final_states);
                break;
            }
            case 3: {
                Helpers::run_trader_event(working_state, state_queue);
                break;
            }
            case 4: {
                Helpers::run_camel_event(working_state, state_queue);
                break;
            }
            case 5: {
                Helpers::run_interest_event(working_state, state_queue, final_states);
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

#ifdef BUILD_PYTHON_MODULE
PYBIND11_MODULE(TCSC, m) {
    py::class_<AISolver>(m, "AISolver")
            .def(py::init<>())
            .def("get_current_state", &AISolver::get_current_state)
            .def("step", &AISolver::step)
            .def("reset", &AISolver::reset)
            .def("calculate_final_score", &AISolver::calculate_final_score);
}
#endif