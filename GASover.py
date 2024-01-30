import copy

from Enums import TownOptions, PlayerAction, all_merch_options, all_witch_options, Items, ItemData, NPCs, NPCData
from Game import apply_town, auto_game_loop, start_of_day, event_merchant, event_witch, check__weight_left, \
    find_buy, check__has_items, event_sell_items, event_buy_npc, event_interest
from player import Player
from dataclasses import replace as dc_replace

import os
import neat


# INPUT
# -- Player State
# OUTPUT
# -- Do What Action
# -- We do every possible Trader and Witch
# ACTIVATION FUNCTION
# -- TanH?
# POPULATION SIZE
# -- 10,000+
# FITNESS FUNCTION
# -- Return Points
# Max Generations
# 1,000,000+

# Merchant Event
# -- Which to buy, what day
# Witch Event
# -- Which to Buy, what day
# Buy, Sell, Trader, Camel, Interest Event
# -- When to do action

class GeneticAlgorithmSolver:
    __slots__ = ["config", "population", "start_player", "merch_on", "merch_days",
                 "witch_on", "witch_days", "to_process", "finished"]

    def __init__(self, town: TownOptions = 0, merch_on: bool = False, witch_on: bool = False,
                 que: list[PlayerAction] = None):
        # Create Default Player
        self.start_player: Player = apply_town(Player(), town)
        if len(que) > 0:
            self.start_player = auto_game_loop(self.start_player, que)
        self.to_process: list[dict] = []
        self.finished: list[Player] = []

        # Merch / Witch Stuff
        self.merch_on = merch_on
        self.merch_days = [5, 6, 7]
        self.witch_on = witch_on
        self.witch_days = [9, 10, 11]

        # Solver Setup
        local_dir = os.path.dirname(__file__)
        config_path = os.path.join(local_dir, "neat.config")
        self.config = neat.config.Config(neat.DefaultGenome, neat.DefaultReproduction, neat.DefaultSpeciesSet, neat.DefaultStagnation, config_path)

        self.population = neat.Population(self.config)

        self.population.add_reporter(neat.StdOutReporter(True))
        self.population.add_reporter(neat.StatisticsReporter())

        winner = self.population.run(self.evaluate_genomes, 1000000)
        print('\nBest genome:\n{!s}'.format(winner))

    def evaluate_genomes(self, genomes, config):
        # Setup Genomes
        for genome_id, genome in genomes:
            genome.fitness = 0
            self.to_process.append(
                {
                    "net": neat.nn.FeedForwardNetwork.create(genome, config),
                    "players": [copy.deepcopy(self.start_player)],
                    "g": genome
                }
            )

        # Run Genomes
        for group in self.to_process:
            while len(group["players"]) > 0:
                current: Player = group["players"].pop()
                if not current.day_start_skip:
                    next_day: Player = start_of_day(current, False)

                    if next_day.food < 0:
                        self.finished.append(next_day)
                        continue

                    # Merchant Event
                    if self.merch_on and next_day.merchant_happened is False and (next_day.day in self.merch_days or next_day.call_merchant):
                        for event in all_merch_options:
                            merch_event = event_merchant(next_day, event)
                            group["players"].append(merch_event)

                    # Witch Event
                    if self.witch_on and next_day.witch_happened is False and next_day.day in self.witch_days:
                        for event in all_witch_options:
                            witch_event = event_witch(next_day, event)
                            self.to_process.append(witch_event)

                else:
                    next_day: Player = dc_replace(current, day_start_skip=False)

                current_food_cost = next_day.food_consumption

                actions = group["net"].activate((
                    next_day.gold,                 # Gold
                    next_day.food,                 # Food
                    check__has_items(next_day),    # Has Items to sell
                    check__weight_left(next_day),  # Weight Left for buying
                    next_day.trader,
                    next_day.camel,
                    # Skip Shops for now
                    current_food_cost,
                    # Backpack is in Weight Left
                    next_day.daily_income,
                    next_day.interest_rate,
                    next_day.set_interest,
                    next_day.quick_interest,
                    next_day.statue,
                    next_day.hand_of_midas,
                    # next_day.merch_discount,  # This is a town option
                    next_day.trader_interest,
                    next_day.day,
                    next_day.max_day,
                    # next_day.merchant_happened,  # This is for tracking
                    # next_day.call_merchant,  # This is a town option
                    # next_day.witch_happened,  # This is for tracking
                    # next_day.day_start_skip,  # This is for tracking
                    # next_day.actions,  # This is for tracking
                    # next_day.points,  # This is for tracking
                    next_day.bought_last
                ))
                if next_day.bought_last:
                    actions[0] = -101
                choice = actions.index(max(actions))
                match choice:
                    case 0:  # Buy Stuff
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
                            group["players"].append(buy_everything)
                            if i > 0 and next_day.gold - (next_day.item_shop[Items.Food][ItemData.buy] * i) >= \
                                    next_day.item_shop[Items.Spice][ItemData.buy]:
                                only_food = find_buy(next_day, i, True)
                                group["players"].append(only_food)

                    case 1:  # Sell Items
                        sell_event = event_sell_items(next_day)
                        if next_day.day < next_day.max_day and check__has_items(next_day):
                            group["players"].append(sell_event)
                        else:
                            self.finished.append(sell_event)

                    case 2:  # Trader Event
                        trader_event = event_buy_npc(next_day, NPCs.Trader)
                        if next_day.day < next_day.max_day and next_day.gold >= next_day.npc_shop[NPCs.Trader][NPCData.cost]:
                            group["players"].append(trader_event)
                        else:
                            self.finished.append(trader_event)

                    case 3:  # Camel Event
                        camel_event = event_buy_npc(next_day, NPCs.Camel)
                        if next_day.day < next_day.max_day and next_day.gold >= next_day.npc_shop[NPCs.Camel][NPCData.cost]:
                            group["players"].append(camel_event)
                        else:
                            self.finished.append(camel_event)

                    case 4:  # Interest Event
                        interest_event = event_interest(next_day)
                        if next_day.day < next_day.max_day:
                            group["players"].append(interest_event)
                        else:
                            self.finished.append(interest_event)

            if len(self.finished):
                self.finished.sort(key=lambda x: x.points, reverse=True)
                group["g"].fitness = self.finished[0].points
                # print(f'{self.finished[0].points} points')
                # print(self.finished[0].actions)
                self.finished = []






