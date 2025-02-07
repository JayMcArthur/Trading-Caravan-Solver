from ..consts.Enums import TownOptions, MerchOptions, WitchOptions, Items, ItemData
from ..player.player import Player
from fenwick_tree_2d import FenwickTree2D
from math import ceil
from pathlib import Path

MAX_WEIGHT = 360 # This is from internal tests but could be a little higher
MAX_GOLD = 50000 #

valid_towns = [
    TownOptions.Normalia,    # Default
    TownOptions.Mahaji,      # Spice +5 Sell
    TownOptions.Huangoz,     # Marble -20 Buy
    TownOptions.Cornecopea,  # Food is free
    TownOptions.Gayawaku,    # Food is weightless
    TownOptions.Ipartus      # Silk -100 Buy
]

valid_merchant = [
    MerchOptions.No_Merchant,     # Merch has not happened or can not afford
    MerchOptions.Route_to_Mahaji  # Spice +20 Sell
]

valid_witch = [
    WitchOptions.No_Witch,            # Witch has not happened
    WitchOptions.Vertue_of_Patience,  # Jewelry +100 Buy
    WitchOptions.Midas_was_a_Trader,  # Marble -300 Sell
    WitchOptions.Time_is_Money,       # Jewelry -500 Sell
    WitchOptions.Oasis_of_Sanctifan,  # Silk +200 Buy
    WitchOptions.The_Stonecutter      # Marble Weight -15 and Buy +60
]


def convert_world_to_key(world: str) -> str:
    town, merch, witch = [int(x) for x in world.split('_')]
    if town not in valid_towns:
        town = TownOptions.Normalia
    if merch not in valid_merchant:
        merch = MerchOptions.No_Merchant
    if witch not in valid_witch:
        witch = WitchOptions.No_Witch
    return f'tree_{town}_{merch}_{witch}.pkl'


def generate_elements_for_world(player: Player) -> list[list[int | list[int]]]:
    max_spice = ceil(MAX_WEIGHT / player.item_shop[Items.Spice][ItemData.weight])
    max_pottery = ceil(MAX_WEIGHT / player.item_shop[Items.Pottery][ItemData.weight])
    max_marble = ceil(MAX_WEIGHT / player.item_shop[Items.Marble][ItemData.weight])
    max_silk = ceil(MAX_WEIGHT / player.item_shop[Items.Silk][ItemData.weight])
    max_jewelry = ceil(MAX_WEIGHT / player.item_shop[Items.Jewelry][ItemData.weight])
    found = {}
    found_list = []
    for s in range(max_spice):
        for p in range(max_pottery):
            for m in range(max_marble):
                for i in range(max_silk):
                    for j in range(max_jewelry):
                        profit = ((player.item_shop[Items.Spice][ItemData.sell] - player.item_shop[Items.Spice][ItemData.buy]) * s +
                                 (player.item_shop[Items.Pottery][ItemData.sell] - player.item_shop[Items.Pottery][ItemData.buy]) * p +
                                 (player.item_shop[Items.Marble][ItemData.sell] - player.item_shop[Items.Marble][ItemData.buy]) * m +
                                 (player.item_shop[Items.Silk][ItemData.sell] - player.item_shop[Items.Silk][ItemData.buy]) * i +
                                 (player.item_shop[Items.Jewelry][ItemData.sell] - player.item_shop[Items.Jewelry][ItemData.buy]) * j)
                        weight = (player.item_shop[Items.Spice][ItemData.weight] * s +
                                 player.item_shop[Items.Pottery][ItemData.weight] * p +
                                 player.item_shop[Items.Marble][ItemData.weight] * m +
                                 player.item_shop[Items.Silk][ItemData.weight] * i +
                                 player.item_shop[Items.Jewelry][ItemData.weight] * j)
                        cost = (player.item_shop[Items.Spice][ItemData.buy] * s +
                               player.item_shop[Items.Pottery][ItemData.buy] * p +
                               player.item_shop[Items.Marble][ItemData.buy] * m +
                               player.item_shop[Items.Silk][ItemData.buy] * i +
                               player.item_shop[Items.Jewelry][ItemData.buy] * j)
                        index = cost + weight * 1j
                        if (MAX_GOLD < cost and MAX_WEIGHT < weight) or profit == 0:
                            continue
                        if index not in found:
                            found[index] = len(found_list)
                            found_list.append([cost, weight, profit, [0, s, p, m, i, j]])
                        elif found_list[found[index]][2] < profit:
                            found_list[found[index]][2] = profit
                            found_list[found[index]][3] = [0, s, p, m, i, j]
    return found_list


class BestPurchaseHandler:
    def __init__(self):
        self.prebuilt_keys = {}
        self.loaded_trees = {}

        saved_trees_path = Path.cwd() / "Saved_Trees"
        if saved_trees_path.exists():
            for entry in saved_trees_path.iterdir():
                if entry.is_file() and entry.suffix == ".pkl":
                    filename = entry.name
                    self.prebuilt_keys[filename] = True

    def get_best_buy(self, cost: int, weight: int, player: Player) -> tuple[int, list[int]]:
        key = convert_world_to_key(player.world)

        if key not in self.loaded_trees:
            if key in self.prebuilt_keys:
                self.load_tree(key)
            else:
                self.build_tree(player, key)
                self.save_tree(key)

        # Query the tree
        return self.loaded_trees[key].query(cost, weight)

    def load_tree(self, key: str) -> None:
        self.loaded_trees[key] = FenwickTree2D([])
        try:
            self.loaded_trees[key].deserialize(key)
        except Exception as e:
            raise RuntimeError(f"Deserialization failed for tree: {key}, Error: {str(e)}")

    def save_tree(self, key: str) -> None:
        self.loaded_trees[key].serialize(key)

    def build_tree(self, player: Player, key: str) -> None:
        self.loaded_trees[key] = FenwickTree2D(generate_elements_for_world(player))