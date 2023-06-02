from os import system
from enum import IntEnum
from concurrent.futures import ProcessPoolExecutor
from more_itertools import divide as m_iter_divide
from dataclasses import dataclass, field, replace as dcReplace
clear: () = lambda: system('cls')


class Store(IntEnum):
    BUY = 0
    SELL = 1
    WEIGHT = 2


class NPC(IntEnum):
    BUY = 0
    EFFECT = 1
    EAT = 2


@dataclass(slots=True, frozen=True)
class Player:
    #  Inventory
    gold: int = 30
    food: int = 5
    spice: int = 0
    pottery: int = 0
    marble: int = 0
    silk: int = 0
    jewelry: int = 0
    # Crew
    trader: int = 1
    camel: int = 1
    backpack: int = 0
    #  Effects
    interest_rate: float = 1.50
    set_interest: bool = False
    one_free_interest: bool = False
    cornucopia: bool = False
    route_to_mahaji: bool = False
    statue: bool = False
    hand_of_midas: bool = False
    sturdy_saddle: bool = False
    vertue_of: bool = False
    midas_was_a_trader: bool = False
    camelization: bool = False
    time_is_money: bool = False
    oasis_of_sanctifan: bool = False
    the_stonecutter: bool = False
    #  Day Info
    day: int = field(default=0, compare=False, hash=False, repr=False)
    max_day: int = 15
    merchant_happened: bool = field(default=False, repr=False)
    witch_happened: bool = field(default=False, repr=False)
    bought_last: bool = field(default=False, compare=False, hash=False, repr=False)
    actions: list[str] = field(default_factory=list, compare=False, hash=False, repr=False)


class Game:
    def __init__(self, with_merch, with_witch):
        self.daily_income = 0
        self.merch_on = with_merch
        self.merch_days = [5, 6, 7]  # TODO Find out which random days it can it (5-7)?
        self.merch_discount = False
        self.witch_on = with_witch
        self.witch_days = [8, 9, 10]  # TODO Find out which random days it can it (8-10)?
        self.item_shop = {
            # Name: Buy, Sell, Weight,  Sell_Value, Profit_per_weight
            'Food': [3, 2, 2],  # .........*0.6667,     -0.5
            'Spice': [5, 10, 4],  # .......*2.0000,      1.25
            'Pottery': [20, 33, 5],  # ....*1.6500,      2.6
            'Marble': [110, 300, 20],  # ..*2.7273,      9.5
            'Silk': [530, 1150, 8],  # ....*2.1698,     77.5
            'Jewelry': [900, 2500, 12],  # *2.7778,    133.3
        }
        self.npc = {
            # Name: Buy, Earn/Bag, Eat
            'Camel': [30, 30, 1],
            'Trader': [25, 20, 1]
        }
        self.trader_interest = False

    def event_buy_items(self, player, food, spice, pottery, marble, silk, jewelry):
        new_gold = player.gold - (
                food * self.item_shop['Food'][Store.BUY] + spice * self.item_shop['Spice'][Store.BUY] +
                pottery * self.item_shop['Pottery'][Store.BUY] + marble * (self.item_shop['Marble'][Store.BUY] + player.the_stonecutter * 60) +
                silk * (self.item_shop['Silk'][Store.BUY] + player.oasis_of_sanctifan * 200) + jewelry * (self.item_shop['Jewelry'][Store.BUY] + player.vertue_of * 100))
        if new_gold < 0:
            raise Exception(f'Negative Gold')

        new_food = player.food + food
        new_spice = player.spice + spice
        new_pottery = player.pottery + pottery
        new_marble = player.marble + marble
        new_silk = player.silk + silk
        new_jewelry = player.jewelry + jewelry

        return dcReplace(player,  bought_last=True, gold=new_gold, food=new_food, spice=new_spice, pottery=new_pottery, marble=new_marble, silk=new_silk, jewelry=new_jewelry)

    def event_sell_items(self, player):
        new_gold = player.gold + (
                player.spice * (self.item_shop['Spice'][Store.SELL] + player.route_to_mahaji * 20) + player.pottery * self.item_shop['Pottery'][Store.SELL] +
                player.marble * (self.item_shop['Marble'][Store.SELL] - player.midas_was_a_trader * 300) + player.silk * self.item_shop['Silk'][Store.SELL] +
                player.jewelry * (self.item_shop['Jewelry'][Store.SELL] - player.time_is_money * 500) + player.hand_of_midas * 100)

        return dcReplace(player, bought_last=False, gold=new_gold, spice=0, pottery=0, marble=0, silk=0, jewelry=0)

    def event_buy_npc(self, player, selection):
        if selection == 'Trader' and player.gold >= self.npc['Trader'][NPC.BUY]:
            new_gold = player.gold - self.npc['Trader'][NPC.BUY]
            new_trader = player.trader + 1
            new_interest = player.interest_rate + self.trader_interest * 0.1
            return dcReplace(player, bought_last=False, gold=new_gold, trader=new_trader, interest_rate=new_interest)

        elif selection == 'Camel' and player.gold >= self.npc['Camel'][NPC.BUY]:
            new_gold = player.gold - self.npc['Camel'][NPC.BUY]
            new_camel = player.camel + 1
            return dcReplace(player, bought_last=False, gold=new_gold, camel=new_camel)

    def event_interest(self, player):
        new_gold = round(player.interest_rate * player.gold)
        new_interest = player.interest_rate
        if not player.set_interest and not player.one_free_interest:
            new_interest -= 0.1
        return dcReplace(player, bought_last=False, gold=new_gold, interest_rate=new_interest, one_free_interest=False)

    def event_merchant(self, player: Player, selection):
        if player.gold >= 25 - (5 * self.merch_discount):
            match selection:
                case 'Cornucopia':  # +2 food every turn
                    new_gold = player.gold - (25 - (5 * self.merch_discount))
                    return dcReplace(player, merchant_happened=True, gold=new_gold, cornucopia=True, actions=player.actions + [f'Merch Event {selection}'])
                case 'Route to Mahaji':  # Spice is worth $20 more
                    new_gold = player.gold - (25 - (5 * self.merch_discount))
                    return dcReplace(player, merchant_happened=True, gold=new_gold, route_to_mahaji=True, actions=player.actions + [f'Merch Event {selection}'])
                case 'Wooden Statuette':  # +1 Statue (aka +500 per Trader and Camel)
                    new_gold = player.gold - (25 - (5 * self.merch_discount))
                    return dcReplace(player, merchant_happened=True, gold=new_gold, statue=True, actions=player.actions + [f'Merch Event {selection}'])

        if player.gold >= 50 - (10 * self.merch_discount):
            match selection:
                case 'Canvas Bag':  # +20 storage
                    new_gold = player.gold - (50 - (10 * self.merch_discount))
                    new_backpack = player.backpack + 20
                    return dcReplace(player, merchant_happened=True, gold=new_gold, backpack=new_backpack, actions=player.actions + [f'Merch Event {selection}'])
                case 'Leaders Necklace':  # +1 Trader
                    new_gold = player.gold - (50 - (10 * self.merch_discount))
                    new_trader = player.trader + 1
                    new_interest = player.interest_rate + self.trader_interest * 0.1
                    return dcReplace(player, merchant_happened=True, gold=new_gold, trader=new_trader, interest_rate=new_interest, actions=player.actions + [f'Merch Event {selection}'])
                case 'Hand of Midas':  # 100g when you sell everything
                    new_gold = player.gold - (50 - (10 * self.merch_discount))
                    return dcReplace(player, merchant_happened=True, gold=new_gold, hand_of_midas=True, actions=player.actions + [f'Merch Event {selection}'])

        if player.gold >= 100 - (20 * self.merch_discount):
            match selection:
                case 'Sturdy Saddle':  # Camels carry an extra 20
                    new_gold = player.gold - (100 - (20 * self.merch_discount))
                    return dcReplace(player, merchant_happened=True, gold=new_gold, sturdy_saddle=True, actions=player.actions + [f'Merch Event {selection}'])
                case 'Magic Cleppsydra':  # 1 more day to trade
                    new_gold = player.gold - (100 - (20 * self.merch_discount))
                    new_max_day = player.max_day + 1
                    return dcReplace(player, merchant_happened=True, gold=new_gold, max_day=new_max_day, actions=player.actions + [f'Merch Event {selection}'])
                case 'Blue Treasure':  # An extra 20% from saving
                    new_gold = player.gold - (100 - (20 * self.merch_discount))
                    new_interest = player.interest_rate + 0.2
                    return dcReplace(player, merchant_happened=True, gold=new_gold, interest_rate=new_interest, actions=player.actions + [f'Merch Event {selection}'])

    def event_witch(self, player: Player, selection):
        match selection:
            case "Present's Gift":  # +20 weight, -20% Saving
                new_backpack = player.backpack + 20
                new_interest = player.interest_rate - 0.2
                return dcReplace(player, witch_happened=True, backpack=new_backpack, interest_rate=new_interest, actions=player.actions + [f'Witch Event {selection}'])

            case "Vertue of":  # Saving doesn't decrease, +100 Jewelry cost
                return dcReplace(player, witch_happened=True, set_interest=True, vertue_of=True, actions=player.actions + [f'Witch Event {selection}'])

            case "Midas was a trader":  # +130 Trader earn, -300 Marble sell
                return dcReplace(player, witch_happened=True, midas_was_a_trader=True, actions=player.actions + [f'Witch Event {selection}'])

            case "Camelization":  # +10 Camel weight, +1 Camel food cost
                return dcReplace(player, witch_happened=True, camelization=True, actions=player.actions + [f'Witch Event {selection}'])

            case "Time is Money":  # +1 Day, - 500 Jewelry sell
                new_max_day = player.max_day + 1
                return dcReplace(player, witch_happened=True, time_is_money=True, max_day=new_max_day, actions=player.actions + [f'Witch Event {selection}'])

            case "Animal Lover":  # +2 Camels, -1 Day
                new_camel = player.camel + 2
                new_max_day = player.max_day - 1
                return dcReplace(player, witch_happened=True, max_day=new_max_day, camel=new_camel, actions=player.actions + [f'Witch Event {selection}'])

            case "Oasis of Sanctifan":  # Crew don't eat, +200 Silk buy
                return dcReplace(player, witch_happened=True, oasis_of_sanctifan=True, actions=player.actions + [f'Witch Event {selection}'])

            case "The Stonecutter":  # -15 Marble weight, +60 Marble cost
                return dcReplace(player, witch_happened=True, the_stonecutter=True, actions=player.actions + [f'Witch Event {selection}'])

    def start_of_day(self, player):
        new_food = player.food
        if player.day != 0:
            new_food -= self.check__food_cost(player)
            if new_food < 0 and False:
                raise Exception(f'Zero Food')
        new_day = player.day + 1
        return dcReplace(player, day=new_day, food=new_food)

    def end_of_day(self, player, action):
        new_gold = player.gold + self.daily_income + (player.trader * (self.npc['Trader'][NPC.EFFECT] + player.midas_was_a_trader * 130))
        return dcReplace(player, gold=new_gold, actions=player.actions + action)

    def check__food_cost(self, player):
        return 0 if player.day == player.max_day else (not player.oasis_of_sanctifan) * max(0, player.trader * self.npc['Trader'][NPC.EAT] + player.camel * (self.npc['Camel'][NPC.EAT] + player.camelization) - player.cornucopia * 2)

    @classmethod
    def check__has_items(cls, player):
        return player.spice + player.pottery + player.marble + player.silk + player.jewelry

    def check__points(self, player):
        value = (player.spice * self.item_shop['Spice'][Store.BUY] +
                 player.pottery * self.item_shop['Pottery'][Store.BUY] +
                 player.marble * (self.item_shop['Marble'][Store.BUY] + player.the_stonecutter * 60) +
                 player.silk * (self.item_shop['Silk'][Store.BUY] + player.oasis_of_sanctifan * 200) +
                 player.jewelry * (self.item_shop['Jewelry'][Store.BUY] + player.vertue_of * 100) +
                 player.trader * 25 +
                 player.camel * 30 +
                 player.statue * 500 * (player.trader + player.camel) +
                 player.gold)
        return value

    def check__hash_id(self, player):
        has_items = 0
        value = (player.spice * self.item_shop['Spice'][Store.BUY] +
                 player.pottery * self.item_shop['Pottery'][Store.BUY] +
                 player.marble * self.item_shop['Marble'][Store.BUY] +
                 player.silk * self.item_shop['Silk'][Store.BUY] +
                 player.jewelry * self.item_shop['Jewelry'][Store.BUY] +
                 player.gold)
        if value > player.gold:
            has_items = 1
        if player.statue:
            value += 500 * (player.trader + player.camel)
        value *= 100
        value += player.trader
        value *= 100
        value += player.camel
        value *= 10
        value += has_items
        return value

    def check__weight_left(self, player):
        return (player.food * self.item_shop['Food'][Store.WEIGHT] +
                player.spice * self.item_shop['Spice'][Store.WEIGHT] +
                player.pottery * self.item_shop['Pottery'][Store.WEIGHT] +
                player.marble * (self.item_shop['Marble'][Store.WEIGHT] - player.the_stonecutter * 15) +
                player.silk * self.item_shop['Silk'][Store.WEIGHT] +
                player.jewelry * self.item_shop['Jewelry'][Store.WEIGHT] -
                player.camel * (self.npc['Camel'][NPC.EFFECT] + player.camelization * 10) -
                player.sturdy_saddle * player.camel * 20 -
                player.backpack) * -1

    def check__worth(self, player, item_to_check):
        weight = self.item_shop[item_to_check][Store.WEIGHT]
        sell = self.item_shop[item_to_check][Store.SELL]
        buy = self.item_shop[item_to_check][Store.BUY]
        if item_to_check == 'Silk' and player.route_to_mahaji:
            sell += 20
        if item_to_check == 'Silk' and player.oasis_of_sanctifan:
            buy += 200
        if item_to_check == 'Jewelry' and player.vertue_of:
            buy += 100
        if item_to_check == 'Jewelry' and player.time_is_money:
            sell -= 500
        if item_to_check == 'Marble' and player.midas_was_a_trader:
            sell -= 300
        if item_to_check == 'Marble' and player.the_stonecutter:
            buy += 60
            weight -= 15
        return weight / (sell - buy)

    def check__find_buy(self, player, food_needed, only_food=False):
        # Food, Spice, Pottery, Marble, Silk, Jewelry
        what_to_buy = [food_needed, 0, 0, 0, 0, 0]
        gold = player.gold - self.item_shop['Food'][Store.BUY] * food_needed
        weight_left = self.check__weight_left(player) - self.item_shop['Food'][Store.WEIGHT] * food_needed

        if not only_food:
            items = [['Jewelry', 5], ['Silk', 4], ['Marble', 3], ['Pottery', 2], ['Spice', 1]]
            items.sort(key=lambda x: self.check__worth(player, x[0]), reverse=False)
            for i, pos in items:
                while weight_left >= self.item_shop[i][Store.WEIGHT] and gold >= self.item_shop[i][Store.BUY]:
                    weight_left -= self.item_shop[i][Store.WEIGHT]
                    gold -= self.item_shop[i][Store.BUY]
                    what_to_buy[pos] += 1

        return what_to_buy

    def game_loop(self, player, event_list):
        for event in event_list:
            player = self.start_of_day(player)
            match event[0]:
                case 'Buy':
                    player = self.event_buy_items(player, *event[1])
                    player = self.end_of_day(player, [f'{event[0]} {event[1]}'])
                case 'Sell':
                    player = self.event_sell_items(player)
                    player = self.end_of_day(player, [f'{event[0]}'])
                case 'Trader' | 'Camel':
                    player = self.event_buy_npc(player, event[0])
                    player = self.end_of_day(player, [f'{event[0]}'])
                case 'Interest':
                    player = self.event_interest(player)
                    player = self.end_of_day(player, [f'{event[0]}'])
                case 'Merchant':
                    player = self.event_merchant(player, event[1])
                    player = self.end_of_day(player, [f'{event[1]}'])
                case 'Witch':
                    player = self.event_witch(player, event[1])
                    player = self.end_of_day(player, [f'{event[1]}'])
        return player




merchant_event_list = [
    ['Cornucopia', 1],        # +2 food every turn
    ['Route to Mahaji', 1],   # Spice is worth $20 more
    ['Wooden Statuette', 1],  # +1 Statue (aka +500 per Trader and Camel)
    ['Canvas Bag', 2],        # +20 storage
    ['Leaders Necklace', 2],  # +1 Trader
    ['Hand of Midas', 2],     # 100g when you sell everything
    ['Sturdy Saddle', 4],     # Camels carry an extra 20
    ['Magic Cleppsydra', 4],  # 1 more day to trade
    ['Blue Treasure', 4]      # An extra 20% from saving
]
witch_event_list = [
    "Present's Gift",      # +20 weight, -20% Saving
    'Vertue of',           # Saving doesn't decrease, +100 Jewelry cost
    'Midas was a Trader',  # +130 Trader earn, -300 Marble sell
    'Camelization',        # +10 Camel weight, +1 Camel food cost
    'Time is Money',       # +1 Day, - 500 Jewelry sell
    'Animal Lover',        # +2 Camels, -1 Day
    'Oasis of Sanctifan',  # Crew don't eat, +200 Silk buy
    'The Stonecutter'      # -15 Marble weight, +60 Marble cost
]
town_list = [
    'Normalia',     # 00 - No Effect
    'Fang_Mo',      # 01 - Begin with 1 Pottery
    'L_Exquise',    # 02 - Traders don't need Food
    'Mahaji',       # 03 - Spice sell +5
    'Tocaccialli',  # 04 - Max Day +1
    'Bajaar',       # 05 - x2 Trader income
    'Doba_Lao',     # 06 - Saving Rate +20%
    'Huangoz',      # 07 - Marble Cost -20
    'Port_Willam',  # 08 - Carry Weight +15
    'Moonin',       # 09 - Call Merchant at anytime
    'Cornecopea',   # 10 - Food is free
    'Eduming',      # 11 - Buying a trader > +10% Saving
    'Octoyashi',    # 12 - Begin with +3 Food
    'Kifuai',       # 13 - Begin with +10 Gold
    'Skjollird',    # 14 - Camel cost +10 and weight +10
    'Petrinov',     # 15 - Saving rate doesn't decrease
    'Jilliqo',      # 16 - Daily Gold Income +10
    'Gayawaku',     # 17 - Food is weightless
    'Vilparino',    # 18 - Strange Merchant 20% Sale!
    'Sanctifan',    # 19 - One Free Collect Interest Action
    'Mehaz',        # 20 - Camels don't need food
    'Ipartus'       # 21 - Silk Cost -100
]


class AI:
    def __init__(self, limit=10000, town='Normalia', with_merch=False, with_witch=False, que=None):
        self.game = Game(with_merch, with_witch)
        self.start_player = Player()
        self.game_array = []  # Wrap the game in what town you want to use
        self.finished_array = []
        self.town_effect(town)
        self.limit = limit
        if que is not None:
            self.start_player = self.game.game_loop(self.start_player, que)
        self.game_array.append(self.start_player)

    def find_solve(self):
        day = self.game_array[0].day
        seen = set()
        while True:
            if len(self.game_array) == 0:
                break

            self.game_array.sort(key=lambda x: self.game.check__points(x), reverse=True)
            self.game_array = self.game_array[0:self.limit]

            day += 1
            print(f'Processing Day {day} - #{len(self.game_array)}')

            current_states = self.game_array
            self.game_array = []

            with ProcessPoolExecutor(6) as executor:
                result = list(executor.map(self.execute_events, [list(c) for c in m_iter_divide(6, current_states)]))
            for [reprocess, solved] in result:
                for player in reprocess:
                    if hash(player) not in seen:
                        self.game_array.append(player)
                        seen.add(hash(player))
                for item in solved:
                    self.finished_array.append(item)

        print(f'Printing Solves')
        self.finished_array.sort(key=lambda x: self.game.check__points(x), reverse=True)
        for i in range(10):
            print(f'Place {i + 1}: {self.game.check__points(self.finished_array[i])} points')
            print(self.finished_array[i])
            print(self.finished_array[i].actions)
            print("")

    def execute_events(self, to_process):
        hold = [[], []]
        reprocess = 0
        solved = 1
        while len(to_process) > 0:
            # Copy and delete what we are working with
            current: Player = to_process.pop()

            if self.game.merch_on and current.merchant_happened is False and (current.day + 1 in self.game.merch_days or self.game.merch_days == -1):
                for [action, cost] in merchant_event_list:
                    if current.gold >= cost * (25 - (5 * self.game.merch_discount)):
                        merch_event = self.game.event_merchant(current, action)
                        to_process.append(merch_event)

            if self.game.witch_on and current.witch_happened is False and (current.day + 1 in self.game.witch_days or self.game.witch_days == -1):
                for action in witch_event_list:
                    witch_event = self.game.event_witch(current, action)
                    to_process.append(witch_event)

            current = self.game.start_of_day(current)
            if current.gold < 0:
                # print('Starved to death: ' + path[-1])
                continue

            # Make all options
            # TODO - Logic for witch and merchant will just append
            if current.day < current.max_day and current.bought_last is False:
                days_left = current.max_day - current.day - 1
                food_multi = 2 if days_left > 1 else 0
                current_cost = self.game.check__food_cost(current)
                max_food = min(current.gold // self.game.item_shop['Food'][Store.BUY], self.game.check__weight_left(current) // self.game.item_shop['Food'][Store.WEIGHT], days_left * current_cost - current.food)

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

                needed_food = max(0, food_multi * self.game.check__food_cost(current) - current.food)
                # Buy Items -- Everything
                for i in range(needed_food, max_food+1):
                    to_buy = self.game.check__find_buy(current, i)
                    new_action = [f'Buy - {to_buy}']
                    buy_everything = self.game.event_buy_items(current, *to_buy)
                    buy_everything = self.game.end_of_day(buy_everything, new_action)
                    hold[reprocess].append(buy_everything)

                # Buy Items -- only Food
                needed_food = max(1, food_multi*self.game.check__food_cost(current) - current.food)
                for i in range(needed_food, max_food+1):
                    to_buy = self.game.check__find_buy(current, i, True)
                    new_action = [f'Buy - {to_buy}']
                    buy_only_food = self.game.event_buy_items(current, *to_buy)
                    buy_only_food = self.game.end_of_day(buy_only_food, new_action)
                    hold[reprocess].append(buy_only_food)

            if current.day == current.max_day or current.food >= self.game.check__food_cost(current):
                # Sell Items
                if self.game.check__has_items(current):
                    new_action = ['Sell']
                    sell = self.game.event_sell_items(current)
                    sell = self.game.end_of_day(sell, new_action)
                    if sell.day < sell.max_day:
                        hold[reprocess].append(sell)
                    else:
                        hold[solved].append(sell)

                # Buy Trader
                if current.gold >= 25 and (current.day == current.max_day or current.food >= self.game.check__food_cost(current) + (not current.oasis_of_sanctifan) * self.game.npc['Trader'][NPC.EAT]):
                    new_action = ['Trader']
                    buy_trader = self.game.event_buy_npc(current, 'Trader')
                    buy_trader = self.game.end_of_day(buy_trader, new_action)
                    if buy_trader.day < buy_trader.max_day:
                        hold[reprocess].append(buy_trader)
                    else:
                        hold[solved].append(buy_trader)

                # Buy Camel
                if current.gold >= 30 and (current.day == current.max_day or current.food >= self.game.check__food_cost(current) + (not current.oasis_of_sanctifan) * (self.game.npc['Camel'][NPC.EAT] + current.camelization)):
                    new_action = ['Camel']
                    buy_camel = self.game.event_buy_npc(current, 'Camel')
                    buy_camel = self.game.end_of_day(buy_camel, new_action)
                    if buy_camel.day < buy_camel.max_day:
                        hold[reprocess].append(buy_camel)
                    else:
                        hold[solved].append(buy_camel)

                # Interest
                if current.interest_rate > 1.2 or current.day == current.max_day:
                    new_action = ['Interest']
                    interest = self.game.event_interest(current)
                    interest = self.game.end_of_day(interest, new_action)
                    if interest.day < interest.max_day:
                        hold[reprocess].append(interest)
                    else:
                        hold[solved].append(interest)




            if len(hold[solved]) >= 1000:
                hold[solved].sort(key=lambda x: self.game.check__points(x), reverse=True)
                hold[solved] = hold[1][0:10]
        return hold

    def town_effect(self, town):
        match town:
            case 'Normalia':
                pass
            case 'Fang_Mo':
                self.start_player = dcReplace(self.start_player, pottery=self.start_player.pottery + 1)
            case 'L_Exquise':
                self.game.npc['Trader'][NPC.EAT] = 0
            case 'Mahaji':
                self.game.item_shop['Spice'][Store.SELL] += 5
            case 'Tocaccialli':
                self.start_player = dcReplace(self.start_player, max_day=self.start_player.max_day + 1)
            case 'Bajaar':
                self.game.npc['Trader'][NPC.EFFECT] += 20
            case 'Doba_Lao':
                self.start_player = dcReplace(self.start_player, interest_rate=self.start_player.interest_rate + 0.2)
            case 'Huangoz':
                self.game.item_shop['Marble'][Store.BUY] -= 20
            case 'Port_Willam':
                self.start_player = dcReplace(self.start_player, backpack=self.start_player.backpack + 15)
            case 'Moonin':
                self.game.merch_days = -1
            case 'Cornecopea':
                self.game.item_shop['Food'][Store.BUY] = 0
            case 'Eduming':
                self.game.trader_interest = True
            case 'Octoyashi':
                self.start_player = dcReplace(self.start_player, food=self.start_player.food + 3)
            case 'Kifuai':
                self.start_player = dcReplace(self.start_player, gold=self.start_player.gold + 10)
            case 'Skjollird':
                self.game.npc['Camel'][NPC.BUY] += 10
                self.game.npc['Camel'][NPC.EFFECT] += 10
            case 'Petrinov':
                self.start_player = dcReplace(self.start_player, set_interest=True)
            case 'Jilliqo':
                self.game.daily_income += 10
            case 'Gayawaku':
                self.game.item_shop['Food'][Store.WEIGHT] = 0
            case 'Vilparino':
                self.game.merch_discount = True
            case 'Sanctifan':
                self.start_player = dcReplace(self.start_player, one_free_interest=True)
            case 'Mehaz':
                self.game.npc['Camel'][NPC.EAT] = 0
            case 'Ipartus':
                self.game.item_shop['Silk'][Store.BUY] -= 100


if __name__ == '__main__':
    solver = AI(town=town_list[2], limit=1000000, with_merch=True, with_witch=False, que=[
        ['Trader'],
        ['Trader'],
        ['Trader'],
        ['Camel']
    ])
    solver.find_solve()
