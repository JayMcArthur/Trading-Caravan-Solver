from Enums import TownOptions, PlayerAction, all_merch_options, all_witch_options, Items, ItemData, NPCData, NPCs
from Game import apply_town, auto_game_loop, start_of_day, event_merchant, event_witch, check__weight_left, \
    find_buy, check__has_items, event_sell_items, event_buy_npc, event_interest
from player import Player
from dataclasses import replace as dc_replace
import pickle


class BruteForceSolver:
    __slots__ = ["limit", "game", "start_player", "to_process", "finished", "hash_table", "merch_on", "merch_days",
                 "witch_on", "witch_days"]

    def __init__(self, limit: int = 10, town: TownOptions = 0, merch_on: bool = False, witch_on: bool = False,
                 que: list[PlayerAction] = None):
        self.limit: int = limit
        self.start_player: Player = apply_town(Player(), town)
        if len(que) > 0:
            self.start_player = auto_game_loop(self.start_player, que)
        self.to_process: list[Player] = [self.start_player]
        self.finished: list[Player] = []
        self.hash_table = {}
        self.merch_on = merch_on
        self.merch_days = [5, 6, 7]
        self.witch_on = witch_on
        self.witch_days = [9, 10, 11]

    def print_solves(self, amount: int):
        if amount > 1:
            print("Printing Solves")
        self.finished.sort(key=lambda x: x.points, reverse=True)
        if len(self.finished) > 10:
            del self.finished[10:]
        for i, solve in enumerate(self.finished):
            if i == amount:
                break
            print(f'Place {i + 1}: {solve.points} points')
            # print(solve)
            print(solve.actions)
            print("")

    def find_solve(self, path_to_hashes: str = None, use_threads: bool = False):
        self.hash_table = {}
        hash_on = False
        if path_to_hashes is not None:
            hash_on = True
            try:
                with open(path_to_hashes, 'rb') as pick:
                    self.hash_table.update(pickle.load(pick))
            except:
                self.hash_table = {}

        round_num = 0
        # TODO Add thread stuff here
        # TODO only add option if it isn't already processed
        while len(self.to_process) > 0:
            # Copy and delete what we are working with
            current: Player = self.to_process.pop()
            if hash_on:
                current_hash = hash(current)
                if current_hash not in self.hash_table.keys():
                    self.hash_table[current_hash] = {
                        "from": [],
                        "makes": {
                            "working": [],
                            "finished": []
                        },
                        "best": 0,
                        "best_obj": None
                    }
                else:
                    if self.hash_table[current_hash]["best"] > 0 and len(
                            self.hash_table[current_hash]["makes"]["working"]) == 0:
                        self.finished.append(self.hash_table[current_hash]["best_obj"])
                        continue

            if not current.day_start_skip:
                next_day: Player = start_of_day(current)

                if next_day.food < 0:
                    if hash_on:
                        self.hash_table[current_hash]["makes"]["working"] = []
                        self.hash_table[current_hash]["best"] = 1
                        self.cleanup_hash(current_hash)
                    continue

                # Merchant Event
                if self.merch_on and next_day.merchant_happened is False and (
                        next_day.day in self.merch_days or next_day.call_merchant):
                    for event in all_merch_options:
                        merch_event = event_merchant(next_day, event)
                        if hash_on:
                            self.handle_hash(current_hash, hash(merch_event))
                        self.to_process.append(merch_event)

                # Witch Event
                if self.witch_on and next_day.witch_happened is False and next_day.day in self.witch_days:
                    for event in all_witch_options:
                        witch_event = event_witch(next_day, event)
                        if hash_on:
                            self.handle_hash(current_hash, hash(witch_event))
                        self.to_process.append(witch_event)

            else:
                next_day: Player = dc_replace(current, day_start_skip=False)

            current_food_cost = next_day.food_consumption

            # Buy Event
            # Check you didn't just buy
            if next_day.day < next_day.max_day and next_day.bought_last is False:
                days_left = next_day.max_day - next_day.day
                food_multi = 2 if days_left > 1 else 1
                max_food = min(next_day.gold // max(next_day.item_shop[Items.Food][ItemData.buy], 1),
                               # Max you can Afford
                               check__weight_left(next_day) // max(next_day.item_shop[Items.Food][ItemData.weight], 1),
                               # Max you can carry
                               current_food_cost * days_left + 2 * (days_left - 1))  # Max you could need? TODO REWORK
                needed_food = max(0, food_multi * current_food_cost - next_day.food)

                for i in range(needed_food, max_food):
                    buy_everything = find_buy(next_day, i)
                    if hash_on:
                        self.handle_hash(current_hash, hash(buy_everything))
                    self.to_process.append(buy_everything)
                    if i > 0 and next_day.gold - (next_day.item_shop[Items.Food][ItemData.buy] * i) >= \
                            next_day.item_shop[Items.Spice][ItemData.buy]:
                        only_food = find_buy(next_day, i, True)
                        if hash_on:
                            self.handle_hash(current_hash, hash(only_food))
                        self.to_process.append(only_food)

                # THIS IS AN ALT THAT I DON"T THINK IS THAT MUCH BETTER
                # options = set()
                # day_needs = [current_cost*2, current_cost*2+1]
                # next_day_needs = []
                # for i in range(days_left):
                #     if len(day_needs) == 0:
                #         break
                #     for need in day_needs:
                #         if max_food >= need > current.food:
                #            options.add(need - current.food)
                #             for l in range(i):
                #                 next_day_needs.append(need+current_cost+l)

            if next_day.day == next_day.max_day or next_day.food >= current_food_cost:
                # Sell Event
                # Check you have stuff to sell
                if check__has_items(next_day):
                    sell_event = event_sell_items(next_day)
                    if next_day.day < next_day.max_day:
                        if hash_on:
                            self.handle_hash(current_hash, hash(sell_event))
                        self.to_process.append(sell_event)
                    else:
                        if hash_on:
                            self.handle_hash(current_hash, hash(sell_event), finished=True, finished_obj=sell_event)
                        self.finished.append(sell_event)

                # Trader Event
                # Check cost and that you have new food cost
                if next_day.gold >= next_day.npc_shop[NPCs.Trader][
                    NPCData.cost] and next_day.day < next_day.max_day and (
                        next_day.food >= current_food_cost + next_day.npc_shop[NPCs.Trader][NPCData.eat]):
                    trader_event = event_buy_npc(next_day, NPCs.Trader)
                    if next_day.day < next_day.max_day:
                        if hash_on:
                            self.handle_hash(current_hash, hash(trader_event))
                        self.to_process.append(trader_event)
                    else:
                        if hash_on:
                            self.handle_hash(current_hash, hash(trader_event), finished=True, finished_obj=trader_event)
                        self.finished.append(trader_event)

                # Camel Event
                # Check cost and that you have new food cost
                if next_day.gold >= next_day.npc_shop[NPCs.Camel][
                    NPCData.cost] and next_day.day < next_day.max_day and (
                        next_day.food >= current_food_cost + next_day.npc_shop[NPCs.Camel][NPCData.eat]):
                    camel_event = event_buy_npc(next_day, NPCs.Camel)
                    if next_day.day < next_day.max_day:
                        if hash_on:
                            self.handle_hash(current_hash, hash(camel_event))
                        self.to_process.append(camel_event)
                    else:
                        if hash_on:
                            self.handle_hash(current_hash, hash(camel_event), finished=True, finished_obj=camel_event)
                        self.finished.append(camel_event)

                # Interest Event
                # Lowest buy sell is *1.65 for 2 days or *1.284 per day
                if next_day.interest_rate > 1.2 or next_day.day == next_day.max_day or next_day.quick_interest:
                    interest_event = event_interest(next_day)
                    if next_day.day < next_day.max_day:
                        if hash_on:
                            self.handle_hash(current_hash, hash(interest_event))
                        self.to_process.append(interest_event)
                    else:
                        if hash_on:
                            self.handle_hash(current_hash, hash(interest_event), finished=True,
                                             finished_obj=interest_event)
                        self.finished.append(interest_event)

            if hash_on and next_day.day == next_day.max_day:
                self.cleanup_hash(current_hash)

            if len(self.finished) >= self.limit:
                if hash_on:
                    with open(path_to_hashes, 'wb') as pick:
                        pickle.dump(self.hash_table, pick)
                round_num += 1
                print(f"Processing Round {round_num}")
                self.print_solves(1)

        if hash_on:
            with open(path_to_hashes, 'wb') as pick:
                pickle.dump(self.hash_table, pick)
        self.print_solves(10)

    def handle_hash(self, father_hash, child_hash, finished: bool = False, finished_obj: Player = None):
        if finished:
            self.hash_table[father_hash]["makes"]["finished"].append({"id": child_hash, "player": finished_obj})
            if finished_obj.points > self.hash_table[father_hash]["best"]:
                self.hash_table[father_hash]["best"] = finished_obj.points
                self.hash_table[father_hash]["best_obj"] = finished_obj
        else:
            if child_hash not in self.hash_table[father_hash]["makes"]["working"]:
                self.hash_table[father_hash]["makes"]["working"].append(child_hash)
            if child_hash not in self.hash_table.keys():
                self.hash_table[child_hash] = {
                    "from": [father_hash],
                    "makes": {
                        "working": [],
                        "finished": []
                    },
                    "best": 0,
                    "best_obj": None
                }
            elif father_hash not in self.hash_table[child_hash]["from"]:
                self.hash_table[child_hash]["from"].append(father_hash)

    def cleanup_hash(self, hash_to_check, best=0, best_obj=None, child_hash=None):
        if best != 0:
            if self.hash_table[hash_to_check]["best"] < best:
                self.hash_table[hash_to_check]["best"] = best
                self.hash_table[hash_to_check]["best_obj"] = best_obj
            self.hash_table[hash_to_check]["makes"]["working"].remove(child_hash)
            self.hash_table[hash_to_check]["makes"]["finished"].append(child_hash)
        if len(self.hash_table[hash_to_check]["makes"]["working"]) == 0:
            for father_hash in self.hash_table[hash_to_check]["from"]:
                self.cleanup_hash(father_hash, self.hash_table[hash_to_check]["best"],
                                  self.hash_table[hash_to_check]["best_obj"], hash_to_check)
            # self.hash_table.pop(hash_to_check)  # Remove completed Hash can't do this as then lower searchs just research
