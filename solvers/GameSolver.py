from Enums import TownOptions, PlayerAction, all_merch_options, all_witch_options, Items, ItemData, NPCData, NPCs
from Game import apply_town, auto_game_loop, start_of_day, event_merchant, event_witch, check__weight_left, \
    find_buy, check__has_items, event_sell_items, event_buy_npc, event_interest
from player import Player
from dataclasses import replace as dc_replace
from pathlib import Path
import pickle


class BruteForceSolver:
    __slots__ = ["limit", "game", "start_player", "to_process", "finished", "hash_dict", "hash_path", "merch_on", "merch_days",
                 "witch_on", "witch_days"]

    def __init__(self, limit: int = 10, town: TownOptions = 0, merch_on: bool = False, witch_on: bool = False,
                 que: list[PlayerAction] = None):
        self.limit: int = limit
        self.start_player: Player = apply_town(Player(), town)
        if len(que) > 0:
            self.start_player = auto_game_loop(self.start_player, que)
        self.to_process: list[Player] = [self.start_player]
        self.finished: list[Player] = []
        self.hash_dict = {}
        self.hash_path = ""
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
        hash_on = False
        if path_to_hashes is not None:
            hash_on = True
            self.hash_path = path_to_hashes
            try:
                with open(path_to_hashes, 'rb') as pick:
                    self.hash_dict.update(pickle.load(pick))
            except:
                self.hash_dict = {}
                self.add_hash(None, hash(self.to_process[0]))

        round_num = 0
        # TODO Add thread stuff here
        # TODO only add option if it isn't already processed
        while len(self.to_process) > 0:
            # Copy and delete what we are working with
            current: Player = self.to_process.pop()
            current_hash = hash(current)
            if hash_on:
                self.get_hash(current_hash)
                if self.hash_dict["completed"]:
                    self.finished.append(self.hash_dict['best_obj'])
                    continue

            if not current.day_start_skip:
                next_day: Player = start_of_day(current)

                if next_day.food < 0:
                    if hash_on:
                        self.finish_hash(current_hash, next_day, True)
                    continue

                # Merchant Event
                if self.merch_on and next_day.merchant_happened is False and (
                    next_day.day in self.merch_days or next_day.call_merchant):
                    for event in all_merch_options:
                        merch_event = event_merchant(next_day, event)
                        # Remove events where merchant didn't happen
                        if merch_event is None:
                            continue
                        save = True
                        if hash_on:
                            save = self.add_hash(current_hash, hash(merch_event))
                        if save:
                            self.to_process.append(merch_event)

                # Witch Event
                if self.witch_on and next_day.witch_happened is False and next_day.day in self.witch_days:
                    for event in all_witch_options:
                        witch_event = event_witch(next_day, event)
                        save = True
                        if hash_on:
                            save = self.add_hash(current_hash, hash(witch_event))
                        if save:
                            self.to_process.append(witch_event)

            else:
                next_day: Player = dc_replace(current, day_start_skip=False)

            current_food_cost = next_day.food_consumption

            # Buy Event
            # Check you didn't just buy
            if next_day.day < next_day.max_day and next_day.bought_last is False:
                days_left = next_day.max_day - next_day.day
                food_multi = 2 if days_left > 1 else 1

                max_weight = 100 if next_day.item_shop[Items.Food][ItemData.weight] == 0 else  check__weight_left(next_day) // next_day.item_shop[Items.Food][ItemData.weight]
                max_afford = 100 if next_day.item_shop[Items.Food][ItemData.buy] == 0 else next_day.gold // next_day.item_shop[Items.Food][ItemData.buy]
                max_eat = max(next_day.npc_shop[NPCs.Trader][NPCData.eat], next_day.npc_shop[NPCs.Camel][NPCData.eat])
                future_days = max(days_left - 1, 0)
                max_want = (current_food_cost * days_left) + (max_eat * (future_days * (future_days + 1))/2)

                max_food = min(max_weight, max_afford, max_want)
                needed_food = max(0, food_multi * current_food_cost - next_day.food)

                for i in range(needed_food, max_food):
                    buy_everything = find_buy(next_day, i)
                    save = True
                    if hash_on:
                        save = self.add_hash(current_hash, hash(buy_everything))
                    if save:
                        self.to_process.append(buy_everything)

                    if i > 0 and next_day.gold - (next_day.item_shop[Items.Food][ItemData.buy] * i) >= \
                        next_day.item_shop[Items.Spice][ItemData.buy]:
                        only_food = find_buy(next_day, i, True)
                        save = True
                        if hash_on:
                            save = self.add_hash(current_hash, hash(only_food))
                        if save:
                            self.to_process.append(only_food)

            if next_day.day == next_day.max_day or next_day.food >= current_food_cost:
                # Sell Event
                # Check you have stuff to sell
                if check__has_items(next_day):
                    sell_event = event_sell_items(next_day)
                    save = True
                    if next_day.day < next_day.max_day:
                        if hash_on:
                            save = self.add_hash(current_hash, hash(sell_event))
                        if save:
                            self.to_process.append(sell_event)
                    else:
                        if hash_on:
                            save = self.add_hash(current_hash, hash(sell_event))
                            if save:
                                self.finish_hash(hash(sell_event), sell_event)
                        if save:
                            self.finished.append(sell_event)

                # Trader Event
                # Check cost and that you have new food cost
                if (next_day.gold >= next_day.npc_shop[NPCs.Trader][NPCData.cost] and
                    next_day.day < next_day.max_day and (next_day.food >= current_food_cost + next_day.npc_shop[NPCs.Trader][NPCData.eat])):
                    trader_event = event_buy_npc(next_day, NPCs.Trader)
                    save = True
                    if next_day.day < next_day.max_day:
                        if hash_on:
                            save = self.add_hash(current_hash, hash(trader_event))
                        if save:
                            self.to_process.append(trader_event)
                    else:
                        if hash_on:
                            save = self.add_hash(current_hash, hash(trader_event))
                            if save:
                                self.finish_hash(hash(trader_event), trader_event)
                        if save:
                            self.finished.append(trader_event)

                # Camel Event
                # Check cost and that you have new food cost
                if (next_day.gold >= next_day.npc_shop[NPCs.Camel][NPCData.cost] and
                    next_day.day < next_day.max_day and (next_day.food >= current_food_cost + next_day.npc_shop[NPCs.Camel][NPCData.eat])):
                    camel_event = event_buy_npc(next_day, NPCs.Camel)
                    save = True
                    if next_day.day < next_day.max_day:
                        if hash_on:
                            save = self.add_hash(current_hash, hash(camel_event))
                        if save:
                            self.to_process.append(camel_event)
                    else:
                        if hash_on:
                            save = self.add_hash(current_hash, hash(camel_event))
                            if save:
                                self.finish_hash(hash(camel_event), camel_event)
                        if save:
                            self.finished.append(camel_event)

                # Interest Event
                # Lowest buy sell is *1.65 for 2 days or *1.284 per day
                # ALl the rest are at least *2 for 2 days or *1.41 per day
                # We can figure 2 max interest before last day
                # I'll use >= 1.3 due to above
                if next_day.interest_rate >= 1.3 or next_day.day == next_day.max_day or next_day.quick_interest:
                    interest_event = event_interest(next_day)
                    save = True
                    if next_day.day < next_day.max_day:
                        if hash_on:
                            save = self.add_hash(current_hash, hash(interest_event))
                        if save:
                            self.to_process.append(interest_event)
                    else:
                        if hash_on:
                            save = self.add_hash(current_hash, hash(interest_event))
                            if save:
                                self.finish_hash(hash(interest_event), interest_event)
                        if save:
                            self.finished.append(interest_event)

            if hash_on and next_day.day == next_day.max_day:
                # This might not be needed but uk
                self.cleanup_hash(current_hash)

            if len(self.finished) >= self.limit:
                round_num += 1
                print(f"Processing Round {round_num}")
                self.print_solves(1)

        self.print_solves(10)

    def add_hash(self, father_hash, child_hash) -> bool:
        # This is the starting state:
        if father_hash is None:
            self.hash_dict = {
                "from": [],
                "makes": {
                    "working": [],
                    "finished": []
                },
                "completed": False,
                "best": 0,
                "best_obj": None
            }
            self.save_hash(child_hash)
            return True

        self.get_hash(father_hash)
        # If hash has already been finished
        if child_hash in self.hash_dict["makes"]["finished"]:
            return False
        # If hash isn't marked as child
        if child_hash not in self.hash_dict["makes"]["working"]:
            self.hash_dict["makes"]["working"].append(child_hash)
            self.save_hash(father_hash)

        # Create Child hash
        if not self.get_hash(child_hash):
            self.hash_dict = {
                "from": [father_hash],
                "makes": {
                    "working": [],
                    "finished": []
                },
                "completed": False,
                "best": 0,
                "best_obj": None
            }
            self.save_hash(child_hash)
        # Child hash already made, update it
        elif father_hash not in self.hash_dict["from"]:
            self.hash_dict["from"].append(father_hash)
            self.save_hash(child_hash)
            if self.hash_dict["completed"]:
                self.cleanup_hash(child_hash)
                return False
        return True

    def finish_hash(self, hash_to_check, finished_obj: Player, out_of_food: bool = False) -> None:
        self.get_hash(hash_to_check)
        self.hash_dict["completed"] = True
        if out_of_food:
            self.hash_dict["best"] = 1
        else:
            self.hash_dict["best"] = finished_obj.points
        self.hash_dict["best_obj"] = finished_obj
        self.save_hash(hash_to_check)
        self.cleanup_hash(hash_to_check)

    def cleanup_hash(self, hash_to_check, best=0, best_obj=None, child_hash=None) -> None:
        self.get_hash(hash_to_check)
        if best != 0:
            if self.hash_dict["best"] < best:
                self.hash_dict["best"] = best
                self.hash_dict["best_obj"] = best_obj
                self.save_hash(hash_to_check)
            if child_hash in self.hash_dict["makes"]["working"]:
                self.hash_dict["makes"]["working"].remove(child_hash)
                self.hash_dict["makes"]["finished"].append(child_hash)
                self.save_hash(hash_to_check)
        if len(self.hash_dict["makes"]["working"]) == 0:
            self.hash_dict["completed"] = True
            self.save_hash(hash_to_check)
            for father_hash in self.hash_dict["from"]:
                self.cleanup_hash(father_hash, self.hash_dict["best"],
                                  self.hash_dict["best_obj"], hash_to_check)

    def get_hash(self, hash_name):
        check_file = Path(self.hash_path + f'/{hash_name}.pkl')
        if not check_file.exists():
            self.hash_dict = {}
            return False
        with open(self.hash_path + f'/{hash_name}.pkl', 'rb') as pick:
            self.hash_dict.update(pickle.load(pick))
        return True

    def save_hash(self, hash_name):
        with open(self.hash_path + f'/{hash_name}.pkl', 'wb') as pick:
            # noinspection PyTypeChecker
            pickle.dump(self.hash_dict, pick, protocol=pickle.HIGHEST_PROTOCOL)

# TODO - This needs to be double checked
def find_buy(player: Player, food_needed: int, only_food: bool = False):
    # Food, Spice, Pottery, Marble, Silk, Jewelry
    what_to_buy = [food_needed, 0, 0, 0, 0, 0]
    gold_left = player.gold - player.item_shop[Items.Food][ItemData.buy] * food_needed
    weight_left = check__weight_left(player) - player.item_shop[Items.Food][ItemData.weight] * food_needed

    if not only_food:
        items = [
            [0.0, Items.Food],
            [check__item_worth(player, Items.Spice), Items.Spice],
            [check__item_worth(player, Items.Pottery), Items.Pottery],
            [check__item_worth(player, Items.Marble), Items.Marble],
            [check__item_worth(player, Items.Silk), Items.Silk],
            [check__item_worth(player, Items.Jewelry), Items.Jewelry]
        ]
        items.sort(key=lambda x: x[0], reverse=True)
        for [value, item] in items:
            if item == Items.Food:
                continue
            while weight_left >= player.item_shop[item][ItemData.weight] and gold_left >= player.item_shop[item][
                ItemData.buy]:
                weight_left -= player.item_shop[item][ItemData.weight]
                gold_left -= player.item_shop[item][ItemData.buy]
                what_to_buy[item] += 1

    return event_buy_items(player, *what_to_buy)
