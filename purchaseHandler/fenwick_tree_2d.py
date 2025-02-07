from bisect import bisect_right
import pickle
from pathlib import Path


class FenwickTree2D:
    def __init__(self, elements: list[list[int | list[int]]]):
        if len(elements) == 0:
            return
        self.costs: list[int] = sorted(set(e[0] for e in elements))
        self.weights: list[int] = sorted(set(e[1] for e in elements))
        self.x_to_idx: dict[int, int] = {x: i + 1 for i, x in enumerate(self.costs)}
        self.y_to_idx: dict[int, int] = {y: i + 1 for i, y in enumerate(self.weights)}
        self.max_x: int = len(self.costs)
        self.max_y: int = len(self.weights)
        self.tree: list[list[tuple[int | float, None | list[int]]]] = [[(float('-inf'), None)] * (self.max_y + 1) for _ in range(self.max_x + 1)]

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
        norm_q_x = bisect_right(self.costs, gold)
        norm_q_y = bisect_right(self.weights, weight)
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

    def serialize(self, filename: str):
        filepath = Path.cwd() / "Saved_Trees" / filename
        print(f"Writing {filepath}")
        with open(filepath , 'wb') as file:
            # noinspection PyTypeChecker
            pickle.dump(self, file, pickle.HIGHEST_PROTOCOL)

    def deserialize(self, filename: str):
        filepath = Path.cwd() / "Saved_Trees" / filename
        print(f"Reading {filepath}")
        with open(filepath, 'rb') as file:
            loaded_object  = pickle.load(file)
            self.__dict__.update(loaded_object)