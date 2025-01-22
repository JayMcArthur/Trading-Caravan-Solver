from Game import apply_town, event_merchant, event_witch
from Enums import TownOptions, MerchOptions, WitchOptions, Items, ItemData
from player import Player
from dataclasses import replace as dc_replace
from math import ceil
from bisect import bisect_right
import pickle

valid_towns = [
    TownOptions.Normalia,  # Default
    TownOptions.Mahaji,  # Spice +5 Sell
    TownOptions.Huangoz,  # Marble -20 Buy
    TownOptions.Cornecopea,  # Food is free
    TownOptions.Gayawaku,  # Food is weightless
    TownOptions.Ipartus  # Silk -100 Buy
]

valid_merchant = [
    TownOptions.Normalia, # Merch has not happened or can not afford
    MerchOptions.Route_to_Mahaji+1  # Spice +20 Sell
]

valid_witch = [
    TownOptions.Normalia, # Witch has not happened
    WitchOptions.Vertue_of_Patience+1,  # Jewelry +100 Buy
    WitchOptions.Midas_was_a_Trader+1,  # Marble -300 Sell
    WitchOptions.Time_is_Money+1,  # Jewelry -500 Sell
    WitchOptions.Oasis_of_Sanctifan+1,  # Silk +200 Buy
    WitchOptions.The_Stonecutter+1  # Marble -15 Weight +60 Buy
]

MAX_WEIGHT = 360 # This is from internal tests but could be a little higher
MAX_GOLD = 50000 # this

class Item:
    def __init__(self, name: str, buy: int, sell: int, weight: int):
        self.name = name
        self.buy = buy
        self.sell = sell
        self.weight = weight
        self.buy_fix = buy == 0
        self.weight_fix = weight == 0
        self.profit = self.sell - self.buy
        self.roi = self.profit / (self.buy+self.buy_fix)
        self.efficiency = self.roi / (self.weight+self.weight_fix)
        self.p_e = self.profit / (self.weight+self.weight_fix)

    def __str__(self):
        return f'{self.name:>7}: [{self.buy:03}, {self.sell:04}, {self.weight:02}] {self.profit:>4}, {self.roi*100:>3.0f}, {self.p_e:>6.2f}, {self.efficiency*100:>6.2f}'


class FenwickTree2D:
    def __init__(self, elements: list[list[int | list[int]]]):
        self.costs: list[int] = sorted(set(e[0] for e in elements))
        self.weights: list[int] = sorted(set(e[1] for e in elements))
        self.x_to_idx: dict[int, int] = {x: i + 1 for i, x in enumerate(self.costs)}
        self.y_to_idx: dict[int, int] = {y: i + 1 for i, y in enumerate(self.weights)}
        self.max_x: int = len(self.costs)
        self.max_y: int = len(self.weights)
        self.tree: list[list[tuple[int|float, None|list[int]]]] = [[(float('-inf'), None)] * (self.max_y + 1) for _ in range(self.max_x + 1)]

        # Populate the Fenwick Tree
        for cost, weight, profit, buys in elements:
            self.update(cost, weight, profit, buys)

    def update(self, cost: int, weight: int, profit: int, buys: list[int]):
        xi = self.x_to_idx[cost]
        while xi <= self.max_x:
            yi = self.y_to_idx[weight]
            while yi <= self.max_y:
                current_profit, _ = self.tree[xi][yi]
                if profit > current_profit:
                    self.tree[xi][yi] = (profit, buys)
                yi += yi & -yi
            xi += xi & -xi

    def query(self, gold: int, weight: int) -> tuple[int, list[int]]:
        # norm_gold = max([self.x_to_idx[x] for x in self.costs if x <= gold], default=0)
        # norm_weight = max([self.y_to_idx[y] for y in self.weights if y <= weight], default=0)
        norm_q_x  = bisect_right(self.costs, gold)
        norm_q_y  = bisect_right(self.weights, weight)
        max_value = float('-inf')
        best_buy = None
        xi = norm_q_x
        while xi > 0:
            yi = norm_q_y
            while yi > 0:
                current_profit, buy = self.tree[xi][yi]
                if current_profit > max_value:
                    max_value = current_profit
                    best_buy = buy
                yi -= yi & -yi
            xi -= xi & -xi
        return max_value, best_buy


def get_file_name(world: str) -> str:
    town, merch, witch = [int(x) for x in world.split('_')]
    if town not in valid_towns:
        town = 0
    if merch not in valid_merchant:
        merch = 0
    if witch not in valid_witch:
        witch = 0
    return f'{town}_{merch}_{witch}.pkl'


def create_items(town, merch, witch):
    player = apply_town(Player(), town)
    if merch != 0:
        player = dc_replace(player, gold=100)
        player = event_merchant(player, merch-1)
    if witch != 0:
        player = event_witch(player, witch-1)
    food = Item('Food', player.item_shop[Items.Food][ItemData.buy], player.item_shop[Items.Food][ItemData.sell], player.item_shop[Items.Food][ItemData.weight])
    spice = Item('Spice', player.item_shop[Items.Spice][ItemData.buy], player.item_shop[Items.Spice][ItemData.sell], player.item_shop[Items.Spice][ItemData.weight])
    pottery = Item('Pottery', player.item_shop[Items.Pottery][ItemData.buy], player.item_shop[Items.Pottery][ItemData.sell], player.item_shop[Items.Pottery][ItemData.weight])
    marble = Item('Marble', player.item_shop[Items.Marble][ItemData.buy], player.item_shop[Items.Marble][ItemData.sell], player.item_shop[Items.Marble][ItemData.weight])
    silk = Item('Silk', player.item_shop[Items.Silk][ItemData.buy], player.item_shop[Items.Silk][ItemData.sell], player.item_shop[Items.Silk][ItemData.weight])
    jewelry = Item('Jewelry', player.item_shop[Items.Jewelry][ItemData.buy], player.item_shop[Items.Jewelry][ItemData.sell], player.item_shop[Items.Jewelry][ItemData.weight])
    print(f"{player.world}: [BUY, SELL, WEIGHT] Profit, ROI, P/W, Efficiency")
    print(f"{food}\n{spice}\n{pottery}\n{marble}\n{silk}\n{jewelry}")

    max_spice = ceil(MAX_WEIGHT/spice.weight)
    max_pottery = ceil(MAX_WEIGHT/pottery.weight)
    max_marble = ceil(MAX_WEIGHT/marble.weight)
    max_silk = ceil(MAX_WEIGHT/silk.weight)
    max_jewelry = ceil(MAX_WEIGHT/jewelry.weight)
    found = {}
    found_list = []
    for s in range(max_spice):
        for p in range(max_pottery):
            for m in range(max_marble):
                for i in range(max_silk):
                    for j in range(max_jewelry):
                        profit = spice.profit * s + pottery.profit * p + marble.profit * m + silk.profit * i + jewelry.profit * j
                        weight = spice.weight * s + pottery.weight * p + marble.weight * m + silk.weight * i + jewelry.weight * j
                        cost = spice.buy * s + pottery.buy * p + marble.buy * m + silk.buy * i + jewelry.buy * j
                        index = cost + weight * 1j
                        if (MAX_GOLD < cost and MAX_WEIGHT < weight) or profit == 0:
                            continue
                        if index not in found:
                            found[index] = len(found_list)
                            found_list.append([cost, weight, profit, [0, s, p, m, i, j]])
                        elif found_list[found[index]][2] < profit:
                            found_list[found[index]][2] = profit
                            found_list[found[index]][3] = [0, s, p, m, i, j]
    del found
    print("Found:", len(found_list))
    fenwick = FenwickTree2D(found_list)
    del found_list

    with open(get_file_name(player.world), 'wb') as f:
        # noinspection PyTypeChecker
        pickle.dump(fenwick, f, pickle.HIGHEST_PROTOCOL)
    del fenwick

    # Answer queries
    # results = []
    queries = [
        (50, 50),
        (100, 75),
        (500, 100),
    ]
    #for q_x, q_y in queries:
        #results.append(fenwick.query(q_x, q_y))

    #print(results)
    #input("Press Enter to continue...")
    example = """
    # Input data
elements = [
    (1, 1, 5, [1]),
    (3, 3, 7, [2]),
    (4, 2, 8, [3]),
    (5, 1, 9, [4]),
]
queries = [
    (3, 5),  # Query for max profit where cost <= 3 and weight <= 5
    (5, 2),  # Query for max profit where cost <= 5 and weight <= 2
    (1, 1),  # Query for max profit where cost <= 1 and weight <= 1
]

# Initialize Fenwick Tree with elements
fenwick = FenwickTree2D(elements)

# Query Fenwick Tree
for gold, weight in queries:
    max_profit, best_buys = fenwick.query(gold, weight)
    print(f"Max profit: {max_profit}, Best buys: {best_buys}")
    """


def generation_options():
    for town in valid_towns:
        for merch in valid_merchant:
            for witch in valid_witch:
                create_items(town, merch, witch)

generation_options()